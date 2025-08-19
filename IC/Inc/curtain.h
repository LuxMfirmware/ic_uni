/**
 ******************************************************************************
 * File Name          : curtain.h
 * Date               : 05.08.2025.
 * Description        : Motorized Curtain Control Module Header
 ******************************************************************************
 * @brief
 * Ovaj header fajl definiše strukture, konstante i funkcije za upravljanje
 * motorizovanim zavjesama. Modul omogucava kontrolu pojedinacnih zavjesa
 * ili svih zavjesa odjednom, sa automatskim zaustavljanjem nakon isteka
 * definisanog vremena.
 ******************************************************************************
 */

#ifndef __CURTAIN_CTRL_H__
#define __CURTAIN_CTRL_H__                             FW_BUILD // version


#include "main.h"
#include "rs485.h"
#include "display.h"
#include "thermostat.h"
#include "ventilator.h"
#include "curtain.h"
#include "lights.h"
#include "defroster.h"
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"
/*============================================================================*/
/* DEFINICIJE I KONSTANTE                                                     */
/*============================================================================*/
// Rucno definisane velicine na osnovu pravila "velicina * 2 zaokruženo na najbliži višekratnik 16"

#define CURTAINS_SIZE    16     // Maksimalan broj zavjesa koje sistem podržava.

// --- Definicije za smjer kretanja zavjese ---
#define CURTAIN_STOP     0
#define CURTAIN_UP       1
#define CURTAIN_DOWN     2

/*============================================================================*/
/* STRUKTURE PODATAKA                                                         */
/*============================================================================*/

#pragma pack(push, 1)
/**
 * @brief Struktura koja sadrži KONFIGURACIJU jedne zavjese za EEPROM.
 */
typedef struct {
    uint16_t relayUp;               // Modbus adresa releja za podizanje.
    uint16_t relayDown;             // Modbus adresa releja za spuštanje.
} Curtain_EepromConfig_t;

/**
 * @brief Struktura koja objedinjuje SVE podatke o zavjesama za EEPROM.
 */
typedef struct {
    uint16_t magic_number;          // "Potpis" za validaciju.
    uint8_t  upDownDurationSeconds; // Globalno vrijeme kretanja za sve zavjese.
    Curtain_EepromConfig_t curtains[CURTAINS_SIZE]; // Niz konfiguracija za sve zavjese.
    uint16_t crc;                   // CRC za provjeru integriteta cijelog bloka.
} Curtains_EepromData_t;
#pragma pack(pop)

/**
 * @brief Glavna "runtime" struktura za jednu zavjesu.
 */
typedef struct
{
    // Konfiguracioni dio (procitan iz glavnog EEPROM bloka)
    Curtain_EepromConfig_t config;

    // --- Stanje i Kontrola (runtime) ---
    uint8_t  upDown;                // Trenutno željeno stanje (STOP, UP, DOWN).
    uint8_t  upDown_old;            // Prethodno stanje (za detekciju promjene).
    
    // --- Interni Tajmeri i Flegovi (runtime) ---
    uint32_t upDownTimer;           // Vrijeme starta tajmera za kretanje.
    uint8_t  external_cmd;          // Fleg koji ukazuje da je komanda došla sa busa.
} Curtain;
/*============================================================================*/
/* EKSTERNE VARIJABLE                                                         */
/*============================================================================*/

extern Curtain curtains[CURTAINS_SIZE]; // Globalni niz koji sadrži stanje svih zavjesa.
extern uint8_t curtain_selected;        // Indeks trenutno odabrane zavjese.
/*============================================================================*/
/* PROTOTIPOVI FUNKCIJA                                                       */
/*============================================================================*/

// --- Inicijalizacija i Servis ---
void Curtains_Init(void);
void Curtain_Service(void);

// --- Konfiguracija i Cuvanje ---
void Curtains_Save(void);
void Curtains_SetDefault(void);
void Curtain_SetDefault(Curtain* const cur);
void Curtain_SetMoveTime(const uint8_t seconds);
uint8_t Curtain_GetMoveTime(void);
uint16_t Curtain_GetRelayUp(const Curtain* const cur);
void Curtain_SetRelayUp(Curtain* const cur, const uint16_t val);
uint16_t Curtain_GetRelayDown(const Curtain* const cur);
void Curtain_SetRelayDown(Curtain* const cur, const uint16_t val);

// --- Kontrola i Upravljanje ---
void Curtains_Stop(void);
void Curtain_Stop(Curtain* const cur);
void Curtains_Move(const uint8_t direction);
void Curtains_MoveSignal(const uint8_t direction);
void Curtain_Move(Curtain* const cur, const uint8_t direction);
void Curtain_MoveSignal(Curtain* const cur, const uint8_t direction);
void Curtain_MoveSignal_byIndex(uint8_t const index, const uint8_t direction);

// --- Selekcija i Brojaci (za GUI) ---
void Curtain_Select(const uint8_t curtain);
uint8_t Curtain_getSelected();
bool Curtain_areAllSelected();
void Curtain_ResetSelection();
uint8_t Curtains_getCount(void);

// --- Provjera Stanja ---
bool Curtain_hasRelays(const Curtain* const cur);
bool Curtain_isMoving(const Curtain* const cur);
bool Curtains_isAnyCurtainMoving(void);
bool Curtain_isMovingUp(const Curtain* const cur);
bool Curtains_isAnyCurtainMovingUp(void);
bool Curtain_isMovingDown(const Curtain* const cur);
bool Curtains_isAnyCurtainMovingDown(void);
uint8_t Curtain_getNewDirection(const Curtain* const cur);
bool Curtain_isNewDirectionStop(const Curtain* const cur);
bool Curtain_isNewDirectionUp(const Curtain* const cur);
bool Curtains_isNewDirectionUp(void);
bool Curtain_isNewDirectionDown(const Curtain* const cur);
bool Curtains_isNewDirectionDown(void);
bool Curtains_areAllMovinginSameDirection(const uint8_t direction);

// --- Interna Logika i Ažuriranje ---
void Curtain_Update_External(Curtain* cur, uint8_t val);
bool Curtain_hasDirectionChanged(const Curtain* const cur);
void Curtain_DirectionEqualize(Curtain* const cur);
void Curtain_RestartTimer(Curtain* const cur);
bool Curtain_hasMoveTimeExpired(const Curtain* const cur);
void Curtain_Reset(Curtain* const cur);
void Curtain_HandleTouchLogic(const uint8_t direction);

/**
 * @brief Pronalazi pointer na roletnu na osnovu njenog rednog broja.
 * @param logical_index Redni broj roletne (0 = prva, 1 = druga, itd.).
 * @return Pokazivac na strukturu Curtain* ili NULL ako indeks nije validan.
 */
Curtain* Curtain_GetByLogicalIndex(const uint8_t logical_index);

#endif
