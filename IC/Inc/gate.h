/**
 ******************************************************************************
 * @file    gate.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za modul koji upravlja kapijama i garažnim vratima.
 *
 * @note
 * Ovaj modul enkapsulira kompletnu logiku za upravljanje sa do 6 motorizovanih
 * kapija ili garažnih vrata. Koristi "Opaque Pointer" tehniku (`Gate_Handle`)
 * za potpunu enkapsulaciju runtime podataka.
 * Podržava rad sa i bez senzora za povratnu informaciju (feedback),
 * kao i hardverski (preko posebnog releja) i softverski (preko tajmera)
 * definisan "pedestrian" mod. Arhitektura je pripremljena za podršku
 * TinyFrame (apsolutno adresiranje) i Modbus (modul+pin) protokola.
 ******************************************************************************
 */

#ifndef __GATE_CTRL_H__
#define __GATE_CTRL_H__                              FW_BUILD // verzija

#include "main.h"

/*============================================================================*/
/* JAVNE DEFINICIJE I ENUMERATORI                                             */
/*============================================================================*/

/**
 * @brief Maksimalan broj kapija/garažnih vrata koje sistem podržava.
 */
#define GATE_MAX_COUNT 6

/**
 * @brief Definiše tip motorizovanog ulaza, što utiče na logiku i UI.
 */
typedef enum {
    GATE_TYPE_UNCONFIGURED,     /**< Tip nije podešen (default stanje). */
    GATE_TYPE_SWING,            /**< Standardna krilna kapija. */
    GATE_TYPE_SLIDING,          /**< Klizna kapija. */
    GATE_TYPE_GARAGE            /**< Garažna vrata. */
} GateType_e;

/**
 * @brief Definiše moguća stanja jedne kapije.
 */
typedef enum {
    GATE_STATE_UNDEFINED,       /**< Stanje nije definisano ili je nepoznato (npr. nakon restarta). */
    GATE_STATE_CLOSED,          /**< Kapija je potvrđeno zatvorena (feedback senzor aktivan). */
    GATE_STATE_OPEN,            /**< Kapija je potvrđeno otvorena (feedback senzor aktivan). */
    GATE_STATE_MOVING,          /**< Kapija je trenutno u pokretu. */
    GATE_STATE_PARTIALLY_OPEN,  /**< Kapija je zaustavljena u međupoziciji (npr. nakon pedestrian moda). */
    GATE_STATE_FAULT,           /**< Desila se greška (npr. timeout, senzor nije aktiviran na vrijeme). */
} GateState_e;


/*============================================================================*/
/* JAVNA STRUKTURA (ZA EEPROM)                                                */
/*============================================================================*/

#pragma pack(push, 1)
/**
 * @brief Struktura koja sadrži sve EEPROM postavke za jednu kapiju.
 * @note  Zahvaljujući `#pragma pack`, ova struktura je kompaktna i njen raspored u
 * memoriji je garantovano isti. Koristi `union` za fleksibilno čuvanje
 * adresa za TinyFrame i Modbus protokole.
 */
typedef struct
{
    uint16_t magic_number;          /**< "Magični broj" za validaciju podataka u EEPROM-u. */
    GateType_e gate_type;           /**< Tip motora (krilna, klizna, garažna) koji određuje logiku i UI. */

    // --- Definicije Adresa za Releje i Senzore ---
    union {
        uint16_t tf;                /**< Apsolutna adresa za TinyFrame protokol. */
        struct { uint16_t mod; uint8_t pin; } mb; /**< Par adresa (modul+pin) za Modbus protokol. */
    } relay_open;                   /**< Adresa releja za komandu OTVORI. */

    union {
        uint16_t tf;
        struct { uint16_t mod; uint8_t pin; } mb;
    } relay_close;                  /**< Adresa releja za komandu ZATVORI. */

    union {
        uint16_t tf;
        struct { uint16_t mod; uint8_t pin; } mb;
    } relay_pedestrian;             /**< Adresa releja za PEDESTRIAN komandu. */

    union {
        uint16_t tf;
        struct { uint16_t mod; uint8_t pin; } mb;
    } relay_stop;                   /**< Adresa releja za STOP komandu. */

    union {
        uint16_t tf;
        struct { uint16_t mod; uint8_t pin; } mb;
    } feedback_open;                /**< Adresa senzora za stanje "potpuno otvoreno". */

    union {
        uint16_t tf;
        struct { uint16_t mod; uint8_t pin; } mb;
    } feedback_close;               /**< Adresa senzora za stanje "potpuno zatvoreno". */

    // --- Tajmeri ---
    uint8_t  cycle_timer_s;         /**< Vrijeme (u sekundama) za puni ciklus. Koristi se za detekciju greške (timeout). */
    uint8_t  pedestrian_timer_s;    /**< Vrijeme (u sekundama) za softverski pedestrian mod. 0 = nije konfigurisano. */
    uint16_t pulse_timer_ms;        /**< Trajanje impulsa (u milisekundama) za OPEN/CLOSE komande. */

    uint16_t crc;                   /**< CRC za provjeru integriteta podataka. */
} Gate_EepromConfig_t;
#pragma pack(pop)


/*============================================================================*/
/* OPAQUE (SAKRIVENI) TIP PODATAKA                                            */
/*============================================================================*/

/**
 * @brief  "Opaque" (sakriveni) tip za jednu kapiju.
 * @note   Stvarna definicija ove strukture koja sadrži runtime podatke
 * (npr. trenutno stanje, tajmere) je sakrivena unutar `gate.c`.
 */
typedef struct Gate_s Gate_Handle;


/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

/** Ovdje će doći deklaracije svih javnih funkcija, npr.: */
// void Gate_Init(void);
// void Gate_Service(void);
// Gate_Handle* Gate_GetInstance(uint8_t index);
// void Gate_Save(void);
// void Gate_TriggerSmartStep(Gate_Handle* handle);
// void Gate_TriggerFullCycleOpen(Gate_Handle* handle);
// void Gate_TriggerFullCycleClose(Gate_Handle* handle);
// void Gate_TriggerPedestrian(Gate_Handle* handle);
// void Gate_TriggerStop(Gate_Handle* handle);
// void Gate_CheckEvent(uint16_t module_addr, uint8_t pin_num, uint8_t state);

#endif // __GATE_CTRL_H__