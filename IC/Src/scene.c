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
#include "gate.h"
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
 * @brief       Glavna servisna petlja za modul scena.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva periodično iz glavne `while(1)` petlje u `main.c`.
 * Njena uloga je da izvršava dugotrajne ili periodične zadatke
 * vezane za scene. Trenutno je placeholder za buduću logiku, kao
 * što je "simulacija prisustva" (npr. nasumično paljenje i gašenje
 * svjetala) kada je sistem u `SYSTEM_STATE_AWAY_ACTIVE` stanju.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Scene_Service(void)
{
    // Provjeri trenutno stanje sistema pozivom implementirane "getter" funkcije
    if (Scene_GetSystemState() == SYSTEM_STATE_AWAY_ACTIVE)
    {
        // TODO: Implementirati logiku za simulaciju prisustva.
        // Ovdje će se nalaziti tajmeri i logika koja će periodično pozivati
        // LIGHT_Flip() za nasumično odabrana svjetla kako bi se stvorio
        // utisak da je neko kod kuće.
    }
    else
    {
        // Ako sistem nije u "Away" modu, servisna funkcija trenutno nema šta da radi.
    }
}
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
    // KORISTIMO DIREKTNO DEFINISANI TIP IZ scene.h
    Scene_EepromBlock_t eeprom_block;

    // Učitaj cijeli blok iz EEPROM-a
    EE_ReadBuffer((uint8_t*)&eeprom_block, EE_SCENES, sizeof(Scene_EepromBlock_t));

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
        // Koristimo sizeof(Scene_EepromBlock_t) za tačan proračun
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&eeprom_block, sizeof(Scene_EepromBlock_t));

        if (received_crc != calculated_crc)
        {
            // Podaci su oštećeni (CRC se ne poklapa), postavi defaultne vrijednosti
            Scene_SetDefault();
            Scene_Save();
        }
        else
        {
            // Podaci su validni, prekopiraj ih u radnu memoriju (RAM)
            memcpy(scenes, eeprom_block.scenes, sizeof(scenes));
        }
    }
}

/**
 ******************************************************************************
 * @brief       Snima trenutno stanje svih scena iz RAM-a u EEPROM.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva nakon svake promjene u konfiguraciji
 * scena (npr. nakon poziva `Scene_Memorize`). Ona priprema kompletan
 * memorijski blok tako što u lokalnu `Scene_EepromBlock_t` strukturu
 * upiše magični broj, prekopira podatke iz `scenes` niza, izračuna
 * CRC32 nad cijelim blokom i na kraju ga atomarno upiše u EEPROM.
 * Ovaj proces osigurava integritet i validnost podataka.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Scene_Save(void)
{
    // Kreiraj lokalnu instancu strukture koja odgovara EEPROM bloku.
    Scene_EepromBlock_t block_to_save;

    // KORAK 1: Postavi "potpis" (magični broj) za validaciju.
    block_to_save.magic_number = EEPROM_MAGIC_NUMBER;

    // KORAK 2: Prekopiraj trenutno stanje svih scena iz RAM-a u strukturu za snimanje.
    memcpy(block_to_save.scenes, scenes, sizeof(scenes));

    // KORAK 3: Pripremi za izračunavanje CRC-a tako što se CRC polje postavi na 0.
    block_to_save.crc = 0;

    // KORAK 4: Izračunaj CRC nad cijelim blokom (magični broj + podaci o scenama).
    block_to_save.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&block_to_save, sizeof(Scene_EepromBlock_t));

    // KORAK 5: Snimi kompletan, pripremljen blok u EEPROM u jednoj operaciji.
    EE_WriteBuffer((uint8_t*)&block_to_save, EE_SCENES, sizeof(Scene_EepromBlock_t));
}

/**
 ******************************************************************************
 * @brief       Aktivira odabranu scenu primjenjujući memorisana stanja na uređaje.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je "player" za scene. Prvo provjerava da li je scena
 * uopšte konfigurisana. Ako jeste, prolazi kroz bitmaske za svaki
 * tip uređaja (svjetla, roletne, itd.). Za svaki uređaj koji je
 * označen u masci, funkcija poziva odgovarajuću 'setter' API funkciju
 * tog modula kako bi postavila memorisano stanje. Nakon primjene
 * stanja uređaja, provjerava `scene_type` i izvršava dodatnu
 * specijalizovanu logiku (npr. promjenu globalnog stanja sistema).
 * @param       scene_index Indeks scene (0-5) koju treba aktivirati.
 * @retval      None
 ******************************************************************************
 */
void Scene_Activate(uint8_t scene_index)
{
    // Sigurnosna provjera da se ne čita izvan granica niza
    if (scene_index >= SCENE_MAX_COUNT)
    {
        return;
    }

    // Dobijamo "handle" na scenu koju želimo aktivirati
    Scene_t* target_scene = &scenes[scene_index];

    // Ne radimo ništa ako scena nije prethodno konfigurisana
    if (!target_scene->is_configured)
    {
        // Opciono: Ovdje se može pokrenuti "čarobnjak" za kreiranje scene
        // Za sada, samo izlazimo iz funkcije.
        return;
    }

    // --- Dio 1: Postavljanje stanja uređaja na osnovu maski i sačuvanih vrijednosti ---

    // Postavljanje stanja za SVJETLA
    for (uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        // Provjeri da li je i-ti bit u maski postavljen
        if (target_scene->lights_mask & (1 << i))
        {
            LIGHT_Handle* light_handle = LIGHTS_GetInstance(i);
            if (light_handle) // Dodatna provjera da je handle validan
            {
                // Postavi sačuvane vrijednosti koristeći API svjetala
                LIGHT_SetState(light_handle, target_scene->light_values[i]);
                LIGHT_SetBrightness(light_handle, target_scene->light_brightness[i]);
                LIGHT_SetColor(light_handle, target_scene->light_colors[i]);
            }
        }
    }

    // Postavljanje stanja za ROLETNE
    for (uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        if (target_scene->curtains_mask & (1 << i))
        {
            Curtain_Handle* curtain_handle = Curtain_GetInstanceByIndex(i);
            if (curtain_handle)
            {
                // Pokreni roletnu u memorisanom smjeru
                Curtain_Move(curtain_handle, target_scene->curtain_states[i]);
            }
        }
    }

    // Postavljanje stanja za TERMOSTAT
    if (target_scene->thermostat_mask) // Provjera da li je termostat uključen u scenu
    {
        THERMOSTAT_TypeDef* thst_handle = Thermostat_GetInstance();
        if (thst_handle)
        {
            Thermostat_SP_Temp_Set(thst_handle, target_scene->thermostat_setpoint);
        }
    }

    // --- Dio 2: Izvršavanje specijalne logike na osnovu tipa scene ---
    switch (target_scene->scene_type)
    {
        case SCENE_TYPE_LEAVING:
            // Nakon postavljanja uređaja, promijeni globalno stanje sistema u "odsutan"
            Scene_SetSystemState(SYSTEM_STATE_AWAY_ACTIVE);
            break;

        case SCENE_TYPE_HOMECOMING:
            // Vrati globalno stanje sistema u "prisutan"
            Scene_SetSystemState(SYSTEM_STATE_HOME);
            break;

        case SCENE_TYPE_SLEEP:
            if (target_scene->wakeup_hour != -1)
            {
                // TODO: Pozvati buduću funkciju iz Timer modula za postavljanje alarma
                // Npr. Timer_SetWakeUp(target_scene->wakeup_hour, target_scene->wakeup_minute);
            }
            break;

        case SCENE_TYPE_SECURITY:
            // TODO: Pozvati buduću funkciju iz Security modula sa bitmaskom kao parametrom
            // Npr. Security_SetArmedPartitions(target_scene->security_partitions_to_arm);
            break;

        case SCENE_TYPE_STANDARD:
        default:
            // Za standardne scene, ne izvršava se nikakva dodatna logika.
            break;
    }
}

/**
 ******************************************************************************
 * @brief       Memoriše trenutno stanje svih relevantnih uređaja u odabranu scenu.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija je srce "čarobnjaka" za kreiranje scena. Ona iterira
 * kroz sve konfigurisane uređaje (svjetla, roletne, termostat),
 * poziva njihove javne API funkcije (gettere) da bi prikupila
 * njihovo trenutno stanje, i te vrijednosti upisuje u strukturu
 * odabrane scene u RAM-u. Također postavlja 'is_configured' fleg
 * na 'true', signalizirajući da scena više nije prazna.
 * @param       scene_index Indeks scene (0-5) u koju treba memorisati stanje.
 * @retval      None
 ******************************************************************************
 */
void Scene_Memorize(uint8_t scene_index)
{
    // Sigurnosna provjera da se ne piše izvan granica niza
    if (scene_index >= SCENE_MAX_COUNT)
    {
        return;
    }

    // Dobijamo "handle" na scenu koju želimo modifikovati
    Scene_t* target_scene = &scenes[scene_index];

    // Resetujemo maske prije popunjavanja da osiguramo čisto stanje
    target_scene->lights_mask = 0;
    target_scene->curtains_mask = 0;
    target_scene->thermostat_mask = 0;

    // --- Memorisanje Stanja Svjetala ---
    for (uint8_t i = 0; i < LIGHTS_MODBUS_SIZE; i++)
    {
        LIGHT_Handle* light_handle = LIGHTS_GetInstance(i);
        if (light_handle && LIGHT_GetRelay(light_handle) != 0) // Provjera da li je svjetlo konfigurisano
        {
            // Postavi odgovarajući bit u maski
            target_scene->lights_mask |= (1 << i);
            
            // Sačuvaj trenutne vrijednosti koristeći API funkcije
            target_scene->light_values[i] = LIGHT_isActive(light_handle);
            target_scene->light_brightness[i] = LIGHT_GetBrightness(light_handle);
            target_scene->light_colors[i] = LIGHT_GetColor(light_handle);
        }
    }

    // --- Memorisanje Stanja Roletni ---
    for (uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        Curtain_Handle* curtain_handle = Curtain_GetInstanceByIndex(i);
        if (curtain_handle && Curtain_hasRelays(curtain_handle)) // Provjera da li je roletna konfigurisana
        {
            // Postavi odgovarajući bit u maski
            target_scene->curtains_mask |= (1 << i);
            
            // Sačuvaj trenutno stanje (STOP, UP, ili DOWN)
            target_scene->curtain_states[i] = Curtain_getNewDirection(curtain_handle);
        }
    }

    // --- Memorisanje Stanja Termostata ---
    THERMOSTAT_TypeDef* thst_handle = Thermostat_GetInstance();
    if (thst_handle)
    {
        // Za sada, pretpostavljamo da scena uvijek utiče na termostat ako je prisutan
        target_scene->thermostat_mask = 1; 
        target_scene->thermostat_setpoint = Thermostat_GetSetpoint(thst_handle);
    }
    
    // Ključni korak: Označi scenu kao konfigurisanu
    target_scene->is_configured = true;
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
 * @note        Ova funkcija prolazi kroz niz svih mogućih scena i broji koliko
 * njih ima postavljen `is_configured` fleg na `true`. Scene koje
 * korisnik još nije memorisao (prazni slotovi) se ne ubrajaju u
 * rezultat. Ključna je za dinamičko iscrtavanje GUI-ja.
 * @param       None
 * @retval      uint8_t Broj aktivnih, korisnički konfigurisanih scena (0 do 6).
 ******************************************************************************
 */
uint8_t Scene_GetCount(void)
{
    // Inicijalizuj brojač na nulu
    uint8_t configured_count = 0;

    // Prođi kroz sve slotove za scene
    for (uint8_t i = 0; i < SCENE_MAX_COUNT; i++)
    {
        // Ako je fleg 'is_configured' postavljen na true, uvećaj brojač
        if (scenes[i].is_configured)
        {
            configured_count++;
        }
    }

    // Vrati ukupan broj pronađenih konfigurisanih scena
    return configured_count;
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
