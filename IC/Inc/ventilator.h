/**
 ******************************************************************************
 * @file    ventilator.h
 * @author  Gemini & [Va�e Ime]
 * @brief   Javni API za potpuno enkapsulirani modul za kontrolu ventilatora.
 *
 * @note
 * Ovaj modul koristi "Opaque Pointer" tehniku (Ventilator_Handle) za potpunu
 * enkapsulaciju runtime podataka. Stvarna struktura i globalna instanca su
 * sakrivene unutar `ventilator.c`.
 *
 * Da biste radili sa ventilatorom, morate prvo dobiti "handle" (pointer)
 * na njega pozivanjem funkcije `Ventilator_GetInstance()`. Taj handle se
 * zatim prosljeduje svim ostalim API funkcijama (getterima i setterima).
 ******************************************************************************
 */

#ifndef __VENTILATOR_CTRL_H__
#define __VENTILATOR_CTRL_H__                             FW_BUILD // verzija

#include "main.h"

/*============================================================================*/
/* JAVNE DEFINICIJE I KONSTANTE                                               */
/*============================================================================*/

/**
 * @brief Struktura koja sadr�i iskljucivo podatke koji se trajno cuvaju u EEPROM.
 * @note  Ova definicija je javna kako bi `stm32746g_eeprom.h` (EEPROM mapa)
 * mogao da koristi `sizeof()` za automatsko izracunavanje adresa.
 * Zahvaljujuci `#pragma pack`, ova struktura je kompaktna i njen raspored
 * u memoriji je garantovano isti.
 */
#pragma pack(push, 1)
typedef struct {
    uint16_t magic_number;      /**< "Magicni broj" koji slu�i kao potpis firmvera za validaciju podataka. */
    uint16_t relay;             /**< Modbus adresa releja koji kontroli�e ovaj ventilator. */
    uint8_t  delayOnTime;       /**< Vrijeme odgode u sekundama (x10) prije paljenja ventilatora. */
    uint8_t  delayOffTime;      /**< Vrijeme u sekundama (x10) nakon kojeg ce se ventilator ugasiti. */
    uint8_t  trigger_source1;   /**< Indeks prvog svjetla (1-6) koje mo�e okinuti rad ventilatora. 0 = iskljuceno. */
    uint8_t  trigger_source2;   /**< Indeks drugog svjetla (1-6) koje mo�e okinuti rad ventilatora. 0 = iskljuceno. */
    uint8_t  local_pin;         /**< ID lokalnog GPIO pina na ovom uredaju koji kontroli�e ventilator. 0 = nije povezan. */
    uint16_t crc;               /**< CRC (Cyclic Redundancy Check) za provjeru integriteta podataka. */
} Ventilator_EepromConfig_t;
#pragma pack(pop)


/*============================================================================*/
/* OPAQUE (SAKRIVENI) TIP PODATAKA                                            */
/*============================================================================*/

/**
 * @brief  "Opaque" (sakriveni) tip za ventilator.
 * @note   Stvarna definicija ove strukture je sakrivena unutar `ventilator.c`
 * i nije dostupna ostatku programa. Pristup je moguc samo preko
 * pointera (handle-a) dobijenog od `Ventilator_GetInstance()`.
 */
typedef struct Ventilator_s Ventilator_Handle;


/*============================================================================*/
/* JAVNI API - PROTOTIPOVI FUNKCIJA                                           */
/*============================================================================*/

// --- Grupa 1: Upravljanje Instancom i Osnovne Funkcije ---

/**
 * @brief  Vraca pointer (handle) na jedinu (singleton) instancu ventilatora.
 * @note   Ovo je jedini ispravan nacin da se dobije "handle" za rad sa ventilatorom.
 * @retval Ventilator_Handle* Pointer (handle) na instancu ventilatora.
 */
Ventilator_Handle* Ventilator_GetInstance(void);

/**
 * @brief  Inicijalizuje modul ventilatora.
 * @note   Ucitava postavke iz EEPROM-a, vr�i validaciju i postavlja pocetno stanje.
 * Poziva se jednom pri startu sistema iz `main()`.
 * @param  handle Pointer na instancu ventilatora dobijen sa `Ventilator_GetInstance()`.
 */
void Ventilator_Init(Ventilator_Handle* const handle);

/**
 * @brief  Cuva trenutnu konfiguraciju ventilatora u EEPROM.
 * @note   Funkcija automatski racuna i upisuje CRC radi integriteta podataka.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_Save(Ventilator_Handle* const handle);

/**
 * @brief  Glavna servisna petlja za ventilator.
 * @note   Ova funkcija sadr�i kompletnu logiku za upravljanje tajmerima,
 * okidacima (triggerima) i slanjem komandi.
 * Treba je pozivati periodicno iz glavne `while(1)` petlje.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_Service(Ventilator_Handle* const handle);


// --- Grupa 2: Getteri i Setteri za Konfiguraciju ---

void Ventilator_setRelay(Ventilator_Handle* const handle, const uint16_t val);
uint16_t Ventilator_getRelay(const Ventilator_Handle* const handle);

void Ventilator_setDelayOnTime(Ventilator_Handle* const handle, const uint8_t val);
uint8_t Ventilator_getDelayOnTime(const Ventilator_Handle* const handle);

void Ventilator_setDelayOffTime(Ventilator_Handle* const handle, const uint8_t val);
uint8_t Ventilator_getDelayOffTime(const Ventilator_Handle* const handle);

void Ventilator_setTriggerSource1(Ventilator_Handle* const handle, const uint8_t val);
uint8_t Ventilator_getTriggerSource1(const Ventilator_Handle* const handle);

void Ventilator_setTriggerSource2(Ventilator_Handle* const handle, const uint8_t val);
uint8_t Ventilator_getTriggerSource2(const Ventilator_Handle* const handle);

void Ventilator_setLocalPin(Ventilator_Handle* const handle, const uint8_t val);
uint8_t Ventilator_getLocalPin(const Ventilator_Handle* const handle);


// --- Grupa 3: Kontrola Stanja ---

/**
 * @brief  Ukljucuje ventilator, sa opcijom odlo�enog starta.
 * @param  handle Pointer na instancu ventilatora.
 * @param  useDelay Ako je `true`, aktivirace se tajmer za odlo�eno paljenje
 * (ako je `delayOnTime` konfigurisan). Ako je `false`, pali se odmah.
 */
void Ventilator_On(Ventilator_Handle* const handle, const bool useDelay);

/**
 * @brief  Odmah iskljucuje ventilator.
 * @note   Ova komanda ima najvi�i prioritet i poni�tava sve aktivne tajmere.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_Off(Ventilator_Handle* const handle);

/**
 * @brief  Provjerava da li je ventilator trenutno aktivan (upaljen).
 * @param  handle Pointer na instancu ventilatora.
 * @retval bool `true` ako je ventilator upaljen, inace `false`.
 */
bool Ventilator_isActive(const Ventilator_Handle* const handle);
/**
 * @brief  Postavlja sve konfiguracione parametre ventilatora na fabricke vrijednosti.
 * @note   Ova funkcija se poziva eksterno (npr. iz menija u main.c) za resetovanje postavki.
 * @param  handle Pointer na instancu ventilatora.
 */
void Ventilator_SetDefault(Ventilator_Handle* const handle);

#endif // __VENTILATOR_CTRL_H__
