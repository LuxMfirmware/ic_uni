#ifndef __VENTILATOR_CTRL_H__
#define __VENTILATOR_CTRL_H__                             FW_BUILD // verzija

#include "main.h"
#include "rs485.h"
#include "display.h"
#include "curtain.h"
#include "lights.h"
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "stm32746g.h"
#include "stm32746g_ts.h"
#include "stm32746g_qspi.h"
#include "stm32746g_sdram.h"
#include "stm32746g_eeprom.h"

// Makroi za aktivaciju/deaktivaciju/status ventilatora
#define VENTILATOR_ACTIVATE()       (ventilator.flags |= 1U)
#define VENTILATOR_DEACTIVATE()     (ventilator.flags &= (~1U))
#define VENTILATOR_IS_ACTIVE()      (ventilator.flags & 1U)

// Rucno definisane velicine na osnovu pravila "velicina * 2 zaokruženo na najbliži višekratnik 16"
#define EEPROM_VENTILATOR_CONFIG_SIZE           32
/**
 * @brief Struktura koja sadrži iskljucivo podatke koji se trajno cuvaju u EEPROM.
 */
#pragma pack(push, 1)
typedef struct {
    uint16_t magic_number;      // Magicni broj za validaciju
    uint16_t relay;             // Relejni indeks ventilatora
    uint8_t  delayOnTime;       // Vreme odlaganja ukljucivanja
    uint8_t  delayOffTime;      // Vreme odlaganja iskljucivanja
    uint8_t  trigger_source1;   // Indeks prvog svjetla okidaca
    uint8_t  trigger_source2;   // Indeks drugog svjetla okidaca
    uint8_t  local_pin;         // Lokalni GPIO pin
    uint16_t crc;               // CRC za provjeru integriteta
} Ventilator_EepromConfig_t;
#pragma pack(pop)

/**
 * @brief Glavna struktura za cuvanje stanja i konfiguracije ventilatora.
 */
typedef struct
{
    Ventilator_EepromConfig_t config; // Sadrži sve što se cuva u EEPROM

    // Runtime varijable (ne cuvaju se trajno)
    uint32_t delayOnTimerStart, DelayOffTimerStart;
    uint8_t flags;
} Ventilator;

// Eksterna deklaracija globalne instance ventilatora
extern Ventilator ventilator;

// Prototipovi funkcija za manipulaciju ventilatorom
void Ventilator_setRelay(Ventilator* const vent, const uint16_t val);
uint16_t Ventilator_getRelay(Ventilator* const vent);

void Ventilator_setDelayOnTime(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getDelayOnTime(Ventilator* const vent);

void Ventilator_setDelayOffTime(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getDelayOffTime(Ventilator* const vent);

void Ventilator_setTriggerSource1(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getTriggerSource1(Ventilator* const vent);

void Ventilator_setTriggerSource2(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getTriggerSource2(Ventilator* const vent);

void Ventilator_setLocalPin(Ventilator* const vent, const uint8_t val);
uint8_t Ventilator_getLocalPin(Ventilator* const vent);

void Ventilator_On(const bool useDelay);
void Ventilator_Off(void);

void Ventilator_Save(void);
void Ventilator_Init(void);

void Ventilator_Service(void);

#endif // __VENTILATOR_CTRL_H__
