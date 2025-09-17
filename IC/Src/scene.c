/**
 ******************************************************************************
 * @file    scene.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Implementacija modula za upravljanje sistemskim scenama.
 *
 * @note
 * Ovaj fajl sadrži kompletnu pozadinsku (backend) logiku za sistem scena.
 * Odgovoran je za čuvanje i čitanje konfiguracija scena iz EEPROM-a,
 * aktiviranje scena (slanje komandi drugim modulima), memorisanje trenutnog
 * stanja sistema u scenu, kao i za upravljanje globalnim stanjem sistema
 * (npr. "Away Mode").
 ******************************************************************************
 */

#if (__SCENE_CTRL_H__ != FW_BUILD)
#error "scene header version mismatch"
#endif

/*============================================================================*/
/* UKLJUČENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h"
#include "scene.h"
#include "display.h"
#include "lights.h"
#include "curtain.h"
#include "thermostat.h"
// #include "gate.h" // Bit će uključeno kada gate.h bude kreiran
#include "rs485.h"
#include "stm32746g_eeprom.h"


/*============================================================================*/
/* PRIVATNE DEFINICIJE I MAKROI (INTERNI)                                     */
/*============================================================================*/

/**
 * @brief Puna veličina bloka podataka za scene koji se čuva u EEPROM-u.
 * @note  Uključuje magični broj, niz scena i CRC.
 */
#define EE_SCENES_BLOCK_SIZE (sizeof(uint16_t) + sizeof(scenes) + sizeof(uint16_t))


/*============================================================================*/
/* PRIVATNE (STATIČKE) VARIJABLE                                              */
/*============================================================================*/

/**
 * @brief Statički niz koji u RAM-u čuva konfiguraciju i stanje za sve scene.
 * @note  Ovo je "jedinstveni izvor istine" za stanje scena tokom rada uređaja.
 * Inicijalizuje se iz EEPROM-a prilikom pokretanja sistema.
 */
static Scene_t scenes[SCENE_MAX_COUNT];

/**
 * @brief Statička varijabla koja čuva trenutno globalno stanje sistema.
 * @note  Koristi se za implementaciju "Away" i "Homecoming" logike.
 */
static SystemState_e system_state = SYSTEM_STATE_HOME;


/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH POMOCNIH FUNKCIJA                                    */
/*============================================================================*/
static void Scene_SetDefault(void);


/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief       Inicijalizuje modul za scene pri pokretanju sistema.
 * @author      Gemini & [Vaše Ime]
 * @note        Učitava kompletan blok podataka za sve scene iz EEPROM-a. Vrši
 * provjeru validnosti podataka pomoću magičnog broja i CRC-a. Ako
 * podaci nisu validni (npr. prvo pokretanje ili oštećeni podaci),
 * poziva `Scene_SetDefault()` da kreira jednu, defaultnu,
 * nekonfigurisanu scenu i odmah je snima u EEPROM.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Scene_Init(void)
{
    // Privremena struktura za čuvanje cijelog EEPROM bloka
    struct {
        uint16_t magic_number;
        Scene_t  scene_data[SCENE_MAX_COUNT];
        uint16_t crc;
    } eeprom_block;

    // Učitaj cijeli blok iz EEPROM-a
    EE_ReadBuffer((uint8_t*)&eeprom_block, EE_SCENES_START_ADDR, EE_SCENES_BLOCK_SIZE);

    // Provjeri validnost podataka
    if (eeprom_block.magic_number != EEPROM_MAGIC_NUMBER)
    {
        // Podaci nisu validni (npr. prvo pokretanje), postavi defaultne vrijednosti
        Scene_SetDefault();
        Scene_Save(); // Odmah snimi ispravne defaultne vrijednosti u EEPROM
    }
    else
    {
        uint16_t received_crc = eeprom_block.crc;
        eeprom_block.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&eeprom_block, EE_SCENES_BLOCK_SIZE);

        if (received_crc != calculated_crc)
        {
            // Podaci su oštećeni (CRC se ne poklapa), postavi defaultne vrijednosti
            Scene_SetDefault();
            Scene_Save();
        }
        else
        {
            // Podaci su validni, prekopiraj ih u radnu memoriju (RAM)
            memcpy(scenes, eeprom_block.scene_data, sizeof(scenes));
        }
    }
}

/**
 ******************************************************************************
 * @brief       Snima trenutno stanje svih scena iz RAM-a u EEPROM.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva nakon svake promjene u konfiguraciji
 * scena. Kreira kompletan memorijski blok sa magičnim brojem,
 * podacima i CRC-om, te ga upisuje u EEPROM.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Scene_Save(void)
{
    // TODO: Implementirati snimanje 'scenes' niza u EEPROM
}

/**
 ******************************************************************************
 * @brief       Aktivira odabranu scenu.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je placeholder funkcija. U punoj implementaciji, ova
 * funkcija će čitati sačuvana stanja iz `scenes[scene_index]`
 * i pozivati `setter` funkcije drugih modula (lights, curtain...)
 * kako bi postavila uređaje u željeno stanje.
 * @param       scene_index Indeks scene (0-5) koju treba aktivirati.
 * @retval      None
 ******************************************************************************
 */
void Scene_Activate(uint8_t scene_index)
{
    // Placeholder da se izbjegne warning o neiskorištenom parametru
    (void)scene_index;
    // TODO: Implementirati logiku za aktiviranje scene
}

/**
 ******************************************************************************
 * @brief       Memoriše trenutno stanje svih uređaja u odabranu scenu.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je placeholder funkcija. U punoj implementaciji, ova
 * funkcija će pozivati `getter` funkcije svih ostalih modula
 * (lights, curtain, thermostat...) kako bi prikupila trenutno
 * stanje sistema i upisala ga u `scenes[scene_index]`.
 * @param       scene_index Indeks scene (0-5) u koju treba memorisati stanje.
 * @retval      None
 ******************************************************************************
 */
void Scene_Memorize(uint8_t scene_index)
{
    // Placeholder
    (void)scene_index;
    // TODO: Implementirati logiku za memorisanje stanja
}

/**
 ******************************************************************************
 * @brief       Vraća pokazivač na instancu odabrane scene u RAM-u.
 * @author      Gemini & [Vaše Ime]
 * @note        Omogućava sigurnan, "read-only" pristup podacima scene iz
 * drugih modula, npr. iz `display.c` za potrebe iscrtavanja.
 * @param       scene_index Indeks scene (0-5).
 * @retval      Scene_t* Pokazivač na traženu scenu, ili NULL ako je
 * indeks neispravan.
 ******************************************************************************
 */
Scene_t* Scene_GetInstance(uint8_t scene_index)
{
    if (scene_index < SCENE_MAX_COUNT)
    {
        return &scenes[scene_index];
    }
    return NULL;
}

/**
 ******************************************************************************
 * @brief       Vraća broj trenutno konfigurisanih scena.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija prolazi kroz niz scena i broji koliko njih ima
 * postavljen `is_configured` fleg na `true`. Defaultna,
 * nekonfigurisana scena se ne broji.
 * @param       None
 * @retval      uint8_t Broj aktivnih, korisnički konfigurisanih scena.
 ******************************************************************************
 */
uint8_t Scene_GetCount(void)
{
    // TODO: Implementirati brojanje konfigurisanih scena
    return 1; // Privremeno vraća 1 zbog defaultne scene
}

/**
 ******************************************************************************
 * @brief       Postavlja novo globalno stanje sistema.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se koristi za upravljanje "Away" modom i drugim
 * globalnim stanjima.
 * @param       state Novo stanje iz `SystemState_e` enumeracije.
 * @retval      None
 ******************************************************************************
 */
void Scene_SetSystemState(SystemState_e state)
{
    system_state = state;
    // TODO: Dodati logiku koja se izvršava pri promjeni stanja
}

/**
 ******************************************************************************
 * @brief       Vraća trenutno globalno stanje sistema.
 * @author      Gemini & [Vaše Ime]
 * @param       None
 * @retval      SystemState_e Trenutno stanje sistema.
 ******************************************************************************
 */
SystemState_e Scene_GetSystemState(void)
{
    return system_state;
}


/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH FUNKCIJA                                          */
/*============================================================================*/

/**
 * @brief  Postavlja scene na početno, fabričko stanje.
 * @note   Poziva se iz `Scene_Init()` kada podaci u EEPROM-u nisu validni.
 * Briše sve postojeće podatke i kreira jednu, defaultnu scenu koja
 * je označena kao nekonfigurisana (`is_configured = false`).
 * @param  None
 * @retval None
 */
static void Scene_SetDefault(void)
{
    // Prvo obriši kompletan niz scena u RAM-u
    memset(scenes, 0, sizeof(scenes));

    // Konfiguriši prvu scenu kao defaultnu
    scenes[0].appearance_id = 0; // Pretpostavka da je ID=0 za "Wizzard" ili "Add" ikonicu
    scenes[0].is_configured = false; // Ključni fleg za UI logiku
}
