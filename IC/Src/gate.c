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
    GateState_e     current_state;      /**< Trenutno stanje kapije (Otvorena, Zatvorena, Kreće se...). */
    GateTimerType_e active_timer_type;  /**< Tip tajmera koji je trenutno aktivan. */
    uint32_t        timer_start_tick;   /**< Vrijeme (HAL_GetTick()) kada je posljednji tajmer pokrenut. */
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
static void Gate_SendRelayCommand(Gate_Handle* handle, uint16_t relay_addr, uint8_t command);
static void Gate_StopAllRelays(Gate_Handle* const handle);
static void Gate_SendPulse(Gate_Handle* handle, uint16_t relay_addr);
static Gate_Handle* Gate_FindByFeedbackSensor(uint16_t sensor_addr);
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

/******************************************************************************
 * @brief       Glavna servisna petlja za modul kapija.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ova funkcija se poziva periodično iz `main()`. Prolazi kroz
 * sve konfigurisane kapije i upravlja njihovim stanjem na osnovu
 * aktivnih tajmera. Odgovorna je za završavanje pulseva,
 * izvršavanje pješačkog moda i detekciju grešaka (timeout).
 * @param       None
 * @retval      None
 *****************************************************************************/
void Gate_Service(void)
{
    // Petlja prolazi kroz sve podržane kapije
    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++)
    {
        Gate_Handle* handle = &gates[i];

        // Ako kapija nije konfigurisana ili nema aktivan tajmer, preskoči je
        if (handle->config.gate_type == GATE_TYPE_UNCONFIGURED || handle->active_timer_type == GATE_TIMER_NONE) {
            continue;
        }

        uint32_t now = HAL_GetTick();
        uint32_t elapsed_ms = now - handle->timer_start_tick;

        // Obrada logike na osnovu tipa aktivnog tajmera
        switch (handle->active_timer_type)
        {
            case GATE_TIMER_PULSE:
            {
                uint16_t pulse_duration_ms = handle->config.pulse_timer_ms;
                if (pulse_duration_ms == 0) pulse_duration_ms = 500; // Sigurnosna vrijednost ako nije konfigurisano

                if (elapsed_ms >= pulse_duration_ms) {
                    // Zabilježi da li je ovaj puls bio početak kretanja
                    bool was_starting_a_cycle = (handle->current_state == GATE_STATE_MOVING);
                    
                    Gate_StopAllRelays(handle); // Ugasi relej koji je bio aktivan
                    
                    if (was_starting_a_cycle) {
                        // Puls je završio, sada pokreni dugi tajmer za nadzor ciklusa
                        handle->active_timer_type = GATE_TIMER_CYCLE;
                        handle->timer_start_tick = now;
                    } else {
                        // Ako nije bio početak ciklusa (npr. samo STOP puls), završi sa tajmerima
                        handle->active_timer_type = GATE_TIMER_NONE;
                        handle->timer_start_tick = 0;
                    }
                }
                break;
            }

            case GATE_TIMER_PEDESTRIAN:
            {
                uint32_t ped_duration_ms = (uint32_t)handle->config.pedestrian_timer_s * 1000;
                if (elapsed_ms >= ped_duration_ms) {
                    // Vrijeme za pješački mod je isteklo, pošalji STOP puls
                    Gate_SendRelayCommand(handle, handle->config.relay_stop.tf, BINARY_ON);
                    
                    // Pokreni kratki puls tajmer da se STOP relej ugasi
                    handle->active_timer_type = GATE_TIMER_PULSE;
                    handle->timer_start_tick = now;

                    handle->current_state = GATE_STATE_PARTIALLY_OPEN;
                    shouldDrawScreen = 1; // Signaliziraj GUI-ju da se stanje promijenilo
                }
                break;
            }

            case GATE_TIMER_CYCLE:
            {
                uint32_t cycle_duration_ms = (uint32_t)handle->config.cycle_timer_s * 1000;
                if (elapsed_ms >= cycle_duration_ms) {
                    // Vrijeme za puni ciklus je isteklo - ovo je GREŠKA (TIMEOUT)
                    Gate_StopAllRelays(handle); // Odmah zaustavi motore
                    handle->current_state = GATE_STATE_FAULT;
                    handle->active_timer_type = GATE_TIMER_NONE;
                    handle->timer_start_tick = 0;
                    shouldDrawScreen = 1; // Signaliziraj GUI-ju da se desila greška
                }
                break;
            }
            
            case GATE_TIMER_NONE:
            default:
                // Ne radi ništa ako nema aktivnog tajmera
                break;
        }
    }
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
/******************************************************************************
 * @brief       Javni Getteri za čitanje konfiguracije kapije.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ove funkcije omogućavaju drugim modulima (npr. display) da
 * sigurno pročitaju vrijednosti iz konfiguracije kapije bez
 * direktnog pristupa privatnoj strukturi.
 *****************************************************************************/
GateType_e Gate_GetType(const Gate_Handle* handle) { return handle->config.gate_type; }
uint16_t Gate_GetRelayOpenAddr(const Gate_Handle* handle) { return handle->config.relay_open.tf; }
uint16_t Gate_GetRelayCloseAddr(const Gate_Handle* handle) { return handle->config.relay_close.tf; }
uint16_t Gate_GetRelayPedAddr(const Gate_Handle* handle) { return handle->config.relay_pedestrian.tf; }
uint16_t Gate_GetRelayStopAddr(const Gate_Handle* handle) { return handle->config.relay_stop.tf; }
uint16_t Gate_GetFeedbackOpenAddr(const Gate_Handle* handle) { return handle->config.feedback_open.tf; }
uint16_t Gate_GetFeedbackCloseAddr(const Gate_Handle* handle) { return handle->config.feedback_close.tf; }
uint8_t  Gate_GetCycleTimer(const Gate_Handle* handle) { return handle->config.cycle_timer_s; }
uint8_t  Gate_GetPedestrianTimer(const Gate_Handle* handle) { return handle->config.pedestrian_timer_s; }
uint16_t Gate_GetPulseTimer(const Gate_Handle* handle) { return handle->config.pulse_timer_ms; }

/******************************************************************************
 * @brief       Javni Setteri za pisanje konfiguracije kapije.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ove funkcije omogućavaju drugim modulima da sigurno izmijene
 * vrijednosti u konfiguraciji kapije. Svaka izmjena se dešava
 * u RAM-u; za trajno snimanje potrebno je pozvati Gate_Save().
 *****************************************************************************/
void Gate_SetType(Gate_Handle* handle, GateType_e type) { handle->config.gate_type = type; }
void Gate_SetRelayOpenAddr(Gate_Handle* handle, uint16_t addr) { handle->config.relay_open.tf = addr; }
void Gate_SetRelayCloseAddr(Gate_Handle* handle, uint16_t addr) { handle->config.relay_close.tf = addr; }
void Gate_SetRelayPedAddr(Gate_Handle* handle, uint16_t addr) { handle->config.relay_pedestrian.tf = addr; }
void Gate_SetRelayStopAddr(Gate_Handle* handle, uint16_t addr) { handle->config.relay_stop.tf = addr; }
void Gate_SetFeedbackOpenAddr(Gate_Handle* handle, uint16_t addr) { handle->config.feedback_open.tf = addr; }
void Gate_SetFeedbackCloseAddr(Gate_Handle* handle, uint16_t addr) { handle->config.feedback_close.tf = addr; }
void Gate_SetCycleTimer(Gate_Handle* handle, uint8_t seconds) { handle->config.cycle_timer_s = seconds; }
void Gate_SetPedestrianTimer(Gate_Handle* handle, uint8_t seconds) { handle->config.pedestrian_timer_s = seconds; }
void Gate_SetPulseTimer(Gate_Handle* handle, uint16_t ms) { handle->config.pulse_timer_ms = ms; }
/******************************************************************************
 * @brief       Obrađuje eksterni događaj sa senzora (npr. sa RS485).
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ovo je glavna ulazna tačka za povratne informacije. Funkcija
 * pronalazi odgovarajuću kapiju i ažurira njeno stanje. Ključna
 * funkcionalnost je zaustavljanje CYCLE tajmera, čime se
 * potvrđuje da je operacija otvaranja/zatvaranja uspješno
 * završena.
 * @param       sensor_addr Adresa senzora koji je aktiviran.
 * @param       state       Stanje senzora (1 = aktivan, 0 = neaktivan).
 * @retval      None
 *****************************************************************************/
void Gate_CheckEvent(uint16_t sensor_addr, uint8_t state)
{
    // Pronađi kapiju koja je povezana sa ovim senzorom
    Gate_Handle* handle = Gate_FindByFeedbackSensor(sensor_addr);

    // Ako kapija nije pronađena ili senzor nije aktivan, ne radi ništa
    if (!handle || state == 0) return;

    // Provjeri da li je ovo senzor za OTVORENO stanje
    if (handle->config.feedback_open.tf == sensor_addr)
    {
        handle->current_state = GATE_STATE_OPEN;
        // Uspješno otvorena, zaustavi sve tajmere i motore
        handle->active_timer_type = GATE_TIMER_NONE;
        handle->timer_start_tick = 0;
        Gate_StopAllRelays(handle);
    }
    // Provjeri da li je ovo senzor za ZATVORENO stanje
    else if (handle->config.feedback_close.tf == sensor_addr)
    {
        handle->current_state = GATE_STATE_CLOSED;
        // Uspješno zatvorena, zaustavi sve tajmere i motore
        handle->active_timer_type = GATE_TIMER_NONE;
        handle->timer_start_tick = 0;
        Gate_StopAllRelays(handle);
    }
    
    // Signaliziraj GUI-ju da se stanje promijenilo i da treba osvježiti prikaz
    shouldDrawScreen = 1;
}
/******************************************************************************
 * @brief       Komanda za hitno zaustavljanje kapije.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        ISPRAVLJENA VERZIJA. Koristi postojeći `AddCommand` mehanizam.
 * Šalje BINARY_OFF komandu na sve relevantne releje.
 * @param       handle Pokazivač na instancu kapije.
 * @retval      None
 *****************************************************************************/
void Gate_TriggerStop(Gate_Handle* handle)
{
    if (!handle) return;
    
    uint8_t buffer[3];

    // Zaustavi OPEN relej
    if (handle->config.relay_open.tf > 0) {
        buffer[0] = (handle->config.relay_open.tf >> 8) & 0xFF;
        buffer[1] = handle->config.relay_open.tf & 0xFF;
        buffer[2] = BINARY_OFF;
        AddCommand(&binaryQueue, BINARY_SET, buffer, 3);
    }
    // Zaustavi CLOSE relej
    if (handle->config.relay_close.tf > 0) {
        buffer[0] = (handle->config.relay_close.tf >> 8) & 0xFF;
        buffer[1] = handle->config.relay_close.tf & 0xFF;
        buffer[2] = BINARY_OFF;
        AddCommand(&binaryQueue, BINARY_SET, buffer, 3);
    }
    // Zaustavi PEDESTRIAN relej
    if (handle->config.relay_pedestrian.tf > 0) {
        buffer[0] = (handle->config.relay_pedestrian.tf >> 8) & 0xFF;
        buffer[1] = handle->config.relay_pedestrian.tf & 0xFF;
        buffer[2] = BINARY_OFF;
        AddCommand(&binaryQueue, BINARY_SET, buffer, 3);
    }
    
    // Pošalji impuls na namjenski STOP relej, ako postoji
    if (handle->config.relay_stop.tf > 0) {
        buffer[0] = (handle->config.relay_stop.tf >> 8) & 0xFF;
        buffer[1] = handle->config.relay_stop.tf & 0xFF;
        buffer[2] = BINARY_ON; // Puls je ON, Gate_Service će ga ugasiti
        AddCommand(&binaryQueue, BINARY_SET, buffer, 3);

        handle->active_timer_type = GATE_TIMER_PULSE;
        handle->timer_start_tick = HAL_GetTick();
    }

    if (handle->current_state == GATE_STATE_MOVING) {
        handle->current_state = GATE_STATE_PARTIALLY_OPEN;
    }
    
    if (handle->active_timer_type == GATE_TIMER_CYCLE || handle->active_timer_type == GATE_TIMER_PEDESTRIAN) {
        handle->active_timer_type = GATE_TIMER_NONE;
    }
    
    shouldDrawScreen = 1;
}

/******************************************************************************
 * @brief       Pokreće puni ciklus otvaranja kapije.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        ISPRAVLJENA VERZIJA. Ispravno koristi `AddCommand`.
 * @param       handle Pokazivač na instancu kapije.
 * @retval      None
 *****************************************************************************/
void Gate_TriggerFullCycleOpen(Gate_Handle* handle)
{
    if (!handle || handle->current_state == GATE_STATE_OPEN || handle->config.relay_open.tf == 0) return;
    
    handle->current_state = GATE_STATE_MOVING;
    
    uint8_t buffer[3];
    buffer[0] = (handle->config.relay_open.tf >> 8) & 0xFF;
    buffer[1] = handle->config.relay_open.tf & 0xFF;
    buffer[2] = BINARY_ON;
    AddCommand(&binaryQueue, BINARY_SET, buffer, 3);
    
    handle->active_timer_type = GATE_TIMER_PULSE;
    handle->timer_start_tick = HAL_GetTick();
    
    // Nakon pulsa, pokrećemo i CYCLE tajmer kao watchdog
    // Ovdje se mora odlučiti da li CYCLE tajmer treba da krene odmah ili nakon PULSE-a.
    // Za sada, pretpostavimo da je bitniji PULSE. Logiku ćemo doraditi ako bude potrebno.
    
    shouldDrawScreen = 1;
}
/******************************************************************************
 * @brief       Pokreće puni ciklus zatvaranja kapije.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Koristi ispravan `AddCommand` mehanizam. Mijenja stanje u
 * MOVING i pokreće PULSE tajmer za aktivaciju releja.
 * `Gate_Service` će nakon toga automatski pokrenuti CYCLE tajmer.
 * @param       handle Pokazivač na instancu kapije.
 * @retval      None
 *****************************************************************************/
void Gate_TriggerFullCycleClose(Gate_Handle* handle)
{
    if (!handle || handle->current_state == GATE_STATE_CLOSED || handle->config.relay_close.tf == 0) return;
    
    handle->current_state = GATE_STATE_MOVING;
    
    uint8_t buffer[3];
    buffer[0] = (handle->config.relay_close.tf >> 8) & 0xFF;
    buffer[1] = handle->config.relay_close.tf & 0xFF;
    buffer[2] = BINARY_ON;
    AddCommand(&binaryQueue, BINARY_SET, buffer, 3);
    
    handle->active_timer_type = GATE_TIMER_PULSE;
    handle->timer_start_tick = HAL_GetTick();
    
    shouldDrawScreen = 1;
}

/******************************************************************************
 * @brief       Pokreće pješački mod.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        ISPRAVNA VERZIJA. Podržava hardverski i softverski mod.
 * @param       handle Pokazivač na instancu kapije.
 * @retval      None
 *****************************************************************************/
void Gate_TriggerPedestrian(Gate_Handle* handle)
{
    if (!handle) return;
    
    handle->current_state = GATE_STATE_MOVING;
    uint8_t buffer[3];

    if (handle->config.relay_pedestrian.tf > 0) {
        // Hardverski mod - pošalji puls na namjenski relej
        buffer[0] = (handle->config.relay_pedestrian.tf >> 8) & 0xFF;
        buffer[1] = handle->config.relay_pedestrian.tf & 0xFF;
        buffer[2] = BINARY_ON;
        AddCommand(&binaryQueue, BINARY_SET, buffer, 3);
        
        handle->active_timer_type = GATE_TIMER_PULSE; // `Service` će ovo prebaciti u CYCLE
        handle->timer_start_tick = HAL_GetTick();
    }
    else if (handle->config.pedestrian_timer_s > 0) {
        // Softverski mod - pošalji puls na OPEN relej
        buffer[0] = (handle->config.relay_open.tf >> 8) & 0xFF;
        buffer[1] = handle->config.relay_open.tf & 0xFF;
        buffer[2] = BINARY_ON;
        AddCommand(&binaryQueue, BINARY_SET, buffer, 3);

        // Pokreni PEDESTRIAN tajmer. `Service` će poslati STOP nakon isteka.
        handle->active_timer_type = GATE_TIMER_PEDESTRIAN;
        handle->timer_start_tick = HAL_GetTick();
    }
    
    shouldDrawScreen = 1;
}

/******************************************************************************
 * @brief       "Pametna" step-by-step komanda.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        ISPRAVNA VERZIJA. Implementira OPEN-STOP-CLOSE-STOP logiku
 * pozivanjem drugih, već definisanih Trigger funkcija.
 * @param       handle Pokazivač na instancu kapije.
 * @retval      None
 *****************************************************************************/
void Gate_TriggerSmartStep(Gate_Handle* handle)
{
    if (!handle) return;

    switch (handle->current_state)
    {
        case GATE_STATE_CLOSED:
        case GATE_STATE_PARTIALLY_OPEN:
            Gate_TriggerFullCycleOpen(handle);
            break;

        case GATE_STATE_OPEN:
            Gate_TriggerFullCycleClose(handle);
            break;

        case GATE_STATE_MOVING:
            Gate_TriggerStop(handle);
            break;
        
        case GATE_STATE_FAULT:
        case GATE_STATE_UNDEFINED:
            Gate_TriggerFullCycleOpen(handle);
            break;
    }
}

/******************************************************************************
 * @brief       Pronađi kapiju na osnovu adrese njenog feedback senzora.
 * @author      Gemini
 * @note        Ova pomoćna funkcija prolazi kroz sve konfigurisane kapije
 * i traži onu koja koristi datu adresu za senzor otvorenog
 * ili zatvorenog stanja.
 * @param       sensor_addr Adresa senzora koji je promijenio stanje.
 * @retval      Gate_Handle* Pokazivač na pronađenu kapiju, ili NULL.
 *****************************************************************************/
static Gate_Handle* Gate_FindByFeedbackSensor(uint16_t sensor_addr)
{
    if (sensor_addr == 0) return NULL;

    for (uint8_t i = 0; i < GATE_MAX_COUNT; i++)
    {
        if (gates[i].config.gate_type != GATE_TYPE_UNCONFIGURED)
        {
            if (gates[i].config.feedback_open.tf == sensor_addr ||
                gates[i].config.feedback_close.tf == sensor_addr)
            {
                return &gates[i];
            }
        }
    }
    return NULL;
}


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
    handle->current_state = GATE_STATE_UNDEFINED;
    handle->timer_start_tick = 0; // <-- ISPRAVLJENO IME
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
/******************************************************************************
 * @brief       Šalje specifičnu komandu na datu adresu releja preko RS485.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ovo je privatna, pomoćna funkcija koja enkapsulira kreiranje
 * i slanje komande u red za RS485 komunikaciju, prateći
 * postojeću arhitekturu projekta.
 *****************************************************************************/
static void Gate_SendRelayCommand(Gate_Handle* handle, uint16_t relay_addr, uint8_t command)
{
    if (relay_addr == 0) return;

    uint8_t buffer[3];
    buffer[0] = (relay_addr >> 8) & 0xFF;
    buffer[1] = relay_addr & 0xFF;
    buffer[2] = command;

    AddCommand(&binaryQueue, BINARY_SET, buffer, 3);
}

/******************************************************************************
 * @brief       Zaustavlja sve motore slanjem OFF komande na sve releje kapije.
 * @author      Gemini (po specifikaciji korisnika)
 * @note        Ovo je sigurnosna funkcija koja osigurava da su svi izlazi
 * koji kontrolišu kretanje kapije deaktivirani.
 *****************************************************************************/
static void Gate_StopAllRelays(Gate_Handle* const handle)
{
    Gate_SendRelayCommand(handle, handle->config.relay_open.tf,      BINARY_OFF);
    Gate_SendRelayCommand(handle, handle->config.relay_close.tf,     BINARY_OFF);
    Gate_SendRelayCommand(handle, handle->config.relay_pedestrian.tf,BINARY_OFF);
    Gate_SendRelayCommand(handle, handle->config.relay_stop.tf,      BINARY_OFF);
}
