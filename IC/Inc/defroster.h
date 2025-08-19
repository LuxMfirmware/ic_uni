#ifndef __DEFROSTER_CTRL_H__
#define __DEFROSTER_CTRL_H__                             FW_BUILD // verzija

#include "main.h"


// Rucno definisane velicine na osnovu pravila "velicina * 2 zaokruženo na najbliži višekratnik 16"
#define EEPROM_DEFROSTER_CONFIG_SIZE        16  

/**
 * @brief Struktura koja sadrži iskljucivo podatke koji se cuvaju u EEPROM.
 * @note Kompaktna je zahvaljujuci #pragma pack.
 */
#pragma pack(push, 1)
typedef struct
{
    uint16_t magic_number;  // "Potpis" za validaciju podataka
    uint8_t cycleTime;      // Vreme ciklusa u minutama
    uint8_t activeTime;     // Aktivno vreme u minutama
    uint8_t pin;            // GPIO pin na koji je povezan odmrzivac
    uint16_t crc;           // CRC za provjeru integriteta
} Defroster_EepromConfig_t;
#pragma pack(pop)

/**
 * @brief Glavna struktura za cuvanje stanja i konfiguracije odmrzivaca.
 */
typedef struct
{
    // Konfiguracioni dio strukture koji se cuva u EEPROM
    Defroster_EepromConfig_t config;

    // Runtime varijable koje se ne cuvaju
    uint32_t cycleTime_TimerStart, activeTime_TimerStart;
} Defroster;

// Eksterna deklaracija globalne instance
extern Defroster defroster;

// Prototipovi funkcija
void Defroster_Init(void);
void Defroster_Save(void);
bool Defroster_isActive(void);
void Defroster_On(void);
void Defroster_Off(void);
void Defroster_Service(void);
void Defroster_SetCycleTime(uint8_t time);
void Defroster_SetActiveTime(uint8_t time);
void Defroster_SetDefault(void);

#endif // __DEFROSTER_CTRL_H__