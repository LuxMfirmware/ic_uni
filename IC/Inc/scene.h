/**
 ******************************************************************************
 * @file    scene.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za modul koji upravlja sistemskim scenama.
 *
 * @note
 * Ovaj modul enkapsulira kompletnu logiku za memorisanje, aktiviranje i
 * upravljanje korisnički definisanim scenama. Scene omogućavaju korisniku da
 * jednim dodirom postavi stanja više različitih uređaja (svjetla, roletne,
 * kapije, termostati) u unaprijed definisane pozicije.
 * Modul također upravlja globalnim stanjem sistema (npr. "Away Mode").
 ******************************************************************************
 */

#ifndef __SCENE_CTRL_H__
#define __SCENE_CTRL_H__                             FW_BUILD // verzija

#include "main.h"
#include "display.h" // Potrebno zbog IconID i TextID
#include "lights.h"  // Potrebno zbog LIGHTS_MODBUS_SIZE
#include "curtain.h" // Potrebno zbog CURTAINS_SIZE

/*============================================================================*/
/* JAVNE DEFINICIJE I ENUMERATORI                                             */
/*============================================================================*/

/**
 * @brief Maksimalan broj scena koje sistem podržava.
 */
#define SCENE_MAX_COUNT 6

/**
 * @brief Definiše globalna stanja sistema, primarno za "Away" logiku.
 */
typedef enum {
    SYSTEM_STATE_HOME,          /**< Normalno stanje, ukućani su prisutni. */
    SYSTEM_STATE_AWAY_SETTLING, /**< Stanje nakon aktivacije "Leaving" scene, traje kratko vrijeme. */
    SYSTEM_STATE_AWAY_ACTIVE    /**< Sistem je u "Away" modu, aktivna je simulacija prisustva i čekaju se "Homecoming" okidači. */
} SystemState_e;

/**
 * @brief Struktura koja definiše vizuelni izgled jedne scene.
 * @note  Niz ovih struktura će biti definisan u `translations.h` kao
 * `scene_appearance_table[]` i služit će kao predefinisana
 * biblioteka izgleda koje korisnik može odabrati.
 */
typedef struct {
    IconID icon_id;             /**< ID ikonice iz `display.h` koja predstavlja scenu. */
    TextID text_id;             /**< ID teksta iz `display.h` koji služi kao naziv scene. */
} SceneAppearance_t;

/**
 * @brief Glavna struktura koja čuva kompletnu konfiguraciju i stanje jedne scene.
 * @note  Niz ovih struktura će se čuvati u EEPROM-u.
 */
#pragma pack(push, 1)
typedef struct
{
    /**
     * @brief Indeks u `scene_appearance_table[]` koji definiše ikonicu i naziv scene.
     */
    uint8_t appearance_id;

    /**
     * @brief Fleg koji označava da li je scena konfigurisana od strane korisnika.
     * @note  `false` za defaultnu, praznu scenu; `true` čim je korisnik prvi put snimi.
     */
    bool is_configured;

    /**
     * @brief Bitmaska koja definiše koja svjetla su uključena u scenu.
     * @note  Bit 0 odgovara svjetlu 0, Bit 1 svjetlu 1, itd. (do 6 svjetala).
     */
    uint8_t lights_mask;

    /**
     * @brief Bitmaska koja definiše koje roletne su uključene u scenu.
     * @note  Bit 0 odgovara roletni 0, Bit 1 roletni 1, itd. (do 16 roletni).
     */
    uint16_t curtains_mask;

    /**
     * @brief Bitmaska koja definiše koji termostati su uključeni u scenu.
     * @note  Koristit ćemo 8 bitova, što omogućava do 8 termostata u sistemu.
     */
    uint8_t thermostat_mask;

    // Ovdje se mogu dodati maske za kapije i druge buduće uređaje
    // uint8_t gate_mask;

    /** @brief Polje koje čuva memorisana stanja (ON/OFF) za svjetla. */
    uint8_t light_values[LIGHTS_MODBUS_SIZE];
    
    /** @brief Polje koje čuva memorisane vrijednosti svjetline (0-100) za dimere ili obojena svjetla. */
    uint8_t light_brightness[LIGHTS_MODBUS_SIZE];
    
    /** @brief NOVO: Polje koje čuva memorisane RGB boje (u 0x00RRGGBB formatu) za svako svjetlo. */
    uint32_t light_colors[LIGHTS_MODBUS_SIZE];

    /** @brief Polje koje čuva memorisana stanja (STOP, UP, DOWN) za roletne. */
    uint8_t curtain_states[CURTAINS_SIZE];

    /** @brief Vrijednost zadate temperature za termostate uključene u scenu. */
    uint8_t thermostat_setpoint;

    // Ovdje se mogu dodati polja za stanje kapija, itd.

} Scene_t;
#pragma pack(pop)


/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

// Ovdje će doći deklaracije svih javnih funkcija kao što su:
// void Scene_Init(void);
// void Scene_Save(void);
// void Scene_Activate(uint8_t scene_index);
// void Scene_Memorize(uint8_t scene_index);
// Scene_t* Scene_GetInstance(uint8_t scene_index);
// uint8_t Scene_GetCount(void);
// void Scene_SetSystemState(SystemState_e state);
// SystemState_e Scene_GetSystemState(void);

#endif // __SCENE_CTRL_H__