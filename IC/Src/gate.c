/**
 ******************************************************************************
 * @file    gate.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Implementacija modula za upravljanje kapijama i garažnim vratima.
 *
 * @note
 * Ovaj fajl sadrži kompletnu pozadinsku (backend) logiku za sistem kapija.
 * Odgovoran je za čuvanje i čitanje konfiguracija iz EEPROM-a,
 * upravljanje stanjima kapija, izvršavanje komandi i obradu signala
 * sa eksternih senzora.
 ******************************************************************************
 */

#if (__GATE_CTRL_H__ != FW_BUILD)
#error "gate header version mismatch"
#endif

/*============================================================================*/
/* UKLJUČENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h"
#include "gate.h"
#include "display.h"
#include "rs485.h"
#include "stm32746g_eeprom.h"


/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI (INTERNI)                                     */
/*============================================================================*/

/**
 * @brief Početna adresa u EEPROM-u gdje je smješten blok podataka za kapije.
 * @note  Ova adresa se mora ažurirati u `stm32746g_eeprom.h` fajlu.
 * Ovdje je samo kao privremena definicija.
 */
#define EE_GATES_START_ADDR (0x1000) // Privremena adresa, koristit će se ona iz eeprom.h


/*============================================================================*/
/* PRIVATNA DEFINICIJA "RUNTIME" STRUKTURE                                    */
/*============================================================================*/

/**
 * @brief  Puna definicija "runtime" strukture za jednu kapiju.
 * @note   Ovo je konkretna implementacija `Gate_Handle` opaque tipa. Sadrži
 * konfiguraciju koja se čuva u EEPROM-u (`config`) i dodatne
 * runtime varijable koje se koriste za praćenje stanja i tajmera.
 */
struct Gate_s
{
    Gate_EepromConfig_t config;     /**< Konfiguracioni podaci koji se čuvaju u EEPROM-u. */

    // --- Runtime podaci ---
    GateState_e current_state;      /**< Trenutno stanje kapije (Otvorena, Zatvorena, Kreće se...). */
    uint32_t    active_timer_start; /**< Vrijeme (HAL_GetTick()) kada je pokrenut aktivni tajmer (cycle, pedestrian, ili pulse). */
};


/*============================================================================*/
/* PRIVATNE (STATIČKE) VARIJABLE                                              */
/*============================================================================*/

/**
 * @brief Statički niz koji u RAM-u čuva kompletnu konfiguraciju i runtime stanje za sve kapije.
 * @note  Ovo je centralno mjesto za sve podatke vezane za kapije. Pristup izvana
 * je moguć isključivo preko API funkcija koje vraćaju `Gate_Handle`.
 */
static struct Gate_s gates[GATE_MAX_COUNT];


/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH POMOCNIH FUNKCIJA                                    */
/*============================================================================*/
static void Gate_SetDefault(Gate_Handle* const handle);
static void Gate_Init_Single(uint8_t index);
static void Gate_Save_Single(uint8_t index);


/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief       Inicijalizuje kompletan modul za kapije.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva jednom pri pokretanju sistema iz `main()`.
 * Iterira kroz sve podržane kapije i za svaku poziva internu
 * `Gate_Init_Single()` funkciju koja vrši učitavanje i
 * validaciju podataka iz EEPROM-a.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Gate_Init(void)
{
    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++)
    {
        Gate_Init_Single(i);
    }
}

/**
 ******************************************************************************
 * @brief       Snima konfiguraciju svih kapija u EEPROM.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je placeholder funkcija. U punoj implementaciji će
 * iterirati kroz sve kapije i pozivati `Gate_Save_Single()` za svaku.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Gate_Save(void)
{
    // TODO: Implementirati snimanje svih kapija
}

/**
 ******************************************************************************
 * @brief       Glavna servisna petlja za modul kapija.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je placeholder funkcija. U punoj implementaciji će se
 * pozivati periodično iz `main()`. Upravljat će tajmerima,
 * provjeravati stanje senzora i izvršavati logiku upravljanja.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Gate_Service(void)
{
    // TODO: Implementirati servisnu petlju
}

/**
 ******************************************************************************
 * @brief       Vraća "handle" (pokazivač) na instancu kapije.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je siguran način da drugi moduli dobiju pristup podacima
 * i funkcijama jedne kapije, poštujući princip enkapsulacije.
 * @param       index Indeks kapije (0-5).
 * @retval      Gate_Handle* Pokazivač na instancu kapije, ili NULL ako
 * je indeks neispravan.
 ******************************************************************************
 */
Gate_Handle* Gate_GetInstance(uint8_t index)
{
    if (index < GATE_MAX_COUNT)
    {
        return &gates[index];
    }
    return NULL;
}

// ... Ovdje dolaze prazne definicije za ostale API funkcije ...
// void Gate_TriggerSmartStep(Gate_Handle* handle) { (void)handle; }
// void Gate_TriggerFullCycleOpen(Gate_Handle* handle) { (void)handle; }
// itd.


/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH FUNKCIJA                                          */
/*============================================================================*/

/**
 * @brief  Postavlja konfiguraciju jedne kapije na fabričke vrijednosti.
 * @note   Ova funkcija se poziva interno iz `Gate_Init_Single` kada podaci
 * u EEPROM-u nisu validni. Resetuje sve parametre na sigurne, nulte
 * vrijednosti i postavlja tip na `GATE_TYPE_UNCONFIGURED`.
 * @param  handle Pokazivač na instancu kapije koju treba resetovati.
 * @retval None
 */
static void Gate_SetDefault(Gate_Handle* const handle)
{
    if (!handle) return;
    // Obriši kompletnu konfiguraciju
    memset(&handle->config, 0, sizeof(Gate_EepromConfig_t));
    // Postavi početni tip
    handle->config.gate_type = GATE_TYPE_UNCONFIGURED;
}


/**
 * @brief  Inicijalizuje jednu kapiju iz EEPROM-a.
 * @note   Čita konfiguracioni blok za dati indeks, provjerava magicni broj
 * i CRC. Ako je sve ispravno, podaci su učitani. Ako nije,
 * poziva `Gate_SetDefault` da postavi fabričke vrijednosti i
 * odmah ih snima nazad u EEPROM. Također inicijalizuje
 * runtime varijable.
 * @param  index Indeks kapije (0-5) koju treba inicijalizovati.
 * @retval None
 */
static void Gate_Init_Single(uint8_t index)
{
    if (index >= GATE_MAX_COUNT) return;

    Gate_Handle* handle = &gates[index];
    uint16_t address = EE_GATES_START_ADDR + (index * sizeof(Gate_EepromConfig_t));

    EE_ReadBuffer((uint8_t*)&handle->config, address, sizeof(Gate_EepromConfig_t));

    if (handle->config.magic_number != EEPROM_MAGIC_NUMBER)
    {
        Gate_SetDefault(handle);
        Gate_Save_Single(index);
    }
    else
    {
        uint16_t received_crc = handle->config.crc;
        handle->config.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&handle->config, sizeof(Gate_EepromConfig_t));
        if (received_crc != calculated_crc)
        {
            Gate_SetDefault(handle);
            Gate_Save_Single(index);
        }
    }
    // Inicijalizacija runtime podataka
    handle->current_state = GATE_STATE_UNKNOWN;
    handle->active_timer_start = 0;
}


/**
 * @brief  Snima konfiguraciju jedne kapije u EEPROM.
 * @note   Ovo je placeholder funkcija.
 * @param  index Indeks kapije (0-5) koju treba snimiti.
 * @retval None
 */
static void Gate_Save_Single(uint8_t index)
{
    // Placeholder
    (void)index;
    // TODO: Implementirati logiku za snimanje jedne kapije.
    // Primjer:
    // if (index >= GATE_MAX_COUNT) return;
    // Gate_Handle* handle = &gates[index];
    // handle->config.magic_number = EEPROM_MAGIC_NUMBER;
    // handle->config.crc = 0;
    // handle->config.crc = HAL_CRC_Calculate(...);
    // EE_WriteBuffer(...);
}
