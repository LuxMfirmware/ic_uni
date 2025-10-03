/**
 ******************************************************************************
 * @file    security.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni API za modul alarmnog sistema.
 *
 * @note    Ovaj modul enkapsulira logiku za interakciju sa eksternim alarmnim
 * sistemom (npr. Paradox) preko RS485 I/O modula. Upravlja slanjem
 * komandi za aktivaciju/deaktivaciju i prima povratne informacije o
 * stanju sistema i particija. Ne sadrži internu logiku alarma, već
 * služi kao most između GUI-ja i hardverskih modula na bus-u.
 ******************************************************************************
 */
#ifndef __SECURITY_H__
#define __SECURITY_H__

#include "main.h"

/**
 * @brief Definiše ukupan broj particija koje sistem podržava.
 * @note  Ova konstanta se koristi za dimenzioniranje nizova unutar
 * Security_Settings_t strukture.
 */
#define SECURITY_PARTITION_COUNT 3
#define SECURITY_USER_COUNT 3
#define SECURITY_PIN_LENGTH 5

#pragma pack(push, 1)
/**
 * @brief Struktura koja sadrži sve konfiguracione podatke za alarmni modul
 * koji se trajno čuvaju u EEPROM-u.
 * @note  Ova definicija je javna kako bi `stm32746g_eeprom.h` (EEPROM mapa)
 * mogao da koristi `sizeof()` za automatsko izračunavanje adresa[cite: 49, 50].
 * Zahvaljujući `#pragma pack`, raspored u memoriji je kompaktan i
 * pouzdan za EEPROM operacije.
 */
typedef struct
{
    uint16_t magic_number;              /**< "Magični broj" (0xABCD) koji služi kao potpis za validaciju ispravnosti podataka pročitanih iz EEPROM-a[cite: 52]. */
    uint16_t partition_relay_addr[SECURITY_PARTITION_COUNT]; /**< Niz adresa za releje koji kontrolišu pojedinačne particije (1, 2 i 3)[cite: 1]. */
    uint16_t partition_feedback_addr[SECURITY_PARTITION_COUNT]; /**< Niz adresa digitalnih ulaza sa kojih se čita status (naoružana/razoružana) za svaku particiju[cite: 1]. */
    uint16_t system_status_feedback_addr; /**< Adresa digitalnog ulaza koji javlja da li je kompletan sistem u stanju alarma (npr. aktivirana sirena)[cite: 1]. */
    uint16_t silent_alarm_addr;         /**< Adresa releja za tihi alarm, koji se aktivira putem SOS funkcije[cite: 2]. */
    uint16_t pulse_duration_ms;         /**< Trajanje impulsa u milisekundama za "Momentary (toggle)" tip kontrole. Vrijednost 0 označava "Maintained (latched)" tip kontrole[cite: 1]. */
    uint16_t crc;                       /**< CRC (Cyclic Redundancy Check) suma izračunata preko cijele strukture radi provjere integriteta podataka[cite: 60]. */
} Security_Settings_t;

/**
 * @brief Struktura koja sadrži korisničke PIN kodove za alarmni sistem.
 * @note  Ova struktura se čuva u posebnom bloku u EEPROM-u kako bi se
 * odvojila od hardverske konfiguracije sistema.
 */
typedef struct
{
    uint16_t magic_number;                  /**< "Magični broj" (0xABCD) za validaciju podataka. */
    char pins[SECURITY_USER_COUNT][SECURITY_PIN_LENGTH]; /**< Dvodimenzionalni niz koji čuva do 3 korisnička PIN koda, svaki dužine 4 karaktera. */
    uint16_t crc;                           /**< CRC suma za provjeru integriteta podataka o korisnicima. */
} Security_Users_t;
#pragma pack(pop)

/******************************************************************************
 * @brief       Inicijalizuje alarmni modul pri pokretanju sistema.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva jednom iz `main()` funkcije. Učitava
 * konfiguraciju iz EEPROM-a, provjerava njen integritet pomoću
 * magičnog broja i CRC-a, i u slučaju neispravnih podataka,
 * postavlja fabričke vrijednosti i snima ih.
 * @param       None
 * @retval      None
 ******************************************************************************/
void Security_Init(void);
void Security_Users_Init(void);

/******************************************************************************
 * @brief       Snima trenutnu konfiguraciju alarmnog modula u EEPROM.
 * @author      Gemini & [Vaše Ime]
 * @note        Prije snimanja, funkcija postavlja magični broj i izračunava
 * CRC sumu nad cijelom `Security_Settings_t` strukturom kako bi
 * osigurala integritet podataka.
 * @param       None
 * @retval      None
 ******************************************************************************/
void Security_Save(void);
void Security_Users_Save(void);

/******************************************************************************
 * @brief       Postavlja sve konfiguracione parametre na fabričke vrijednosti.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva interno iz `Security_Init` ako podaci
 * u EEPROM-u nisu validni.
 * @param       None
 * @retval      None
 ******************************************************************************/
void Security_SetDefault(void);

// --- Geteri i Seteri za konfiguracione parametre ---
// (Doxygen komentari su izostavljeni radi kratkoće, ali prate istu logiku kao struktura)

uint16_t Security_GetSystemRelayAddr(void);
void     Security_SetSystemRelayAddr(uint16_t addr);
uint16_t Security_GetPartitionRelayAddr(uint8_t partition);
void     Security_SetPartitionRelayAddr(uint8_t partition, uint16_t addr);
uint16_t Security_GetPartitionFeedbackAddr(uint8_t partition);
void     Security_SetPartitionFeedbackAddr(uint8_t partition, uint16_t addr);
uint16_t Security_GetSystemStatusFeedbackAddr(void);
void     Security_SetSystemStatusFeedbackAddr(uint16_t addr);
uint16_t Security_GetSilentAlarmAddr(void);
void     Security_SetSilentAlarmAddr(uint16_t addr);
uint16_t Security_GetPulseDuration(void);
void     Security_SetPulseDuration(uint16_t duration);

/******************************************************************************
 * @brief       Šalje komandu za promjenu stanja (ARM/DISARM) za jednu particiju.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je ne-blokirajuća funkcija. Ona formira `BINARY_SET` komandu
 * i dodaje je u `securityQueue` red. Stvarno slanje na bus vrši
 * `RS485_Service`.
 * @param       partition_index Indeks particije (0-2).
 * @retval      None
 ******************************************************************************/
void Security_TogglePartition(uint8_t partition_index);

/******************************************************************************
 * @brief       Šalje komandu za promjenu stanja (ARM/DISARM) za cijeli sistem.
 * @author      Gemini & [Vaše Ime]
 * @param       None
 * @retval      None
 ******************************************************************************/
void Security_ToggleSystem(void);

/******************************************************************************
 * @brief       Šalje komandu za aktivaciju tihog alarma (SOS).
 * @author      Gemini & [Vaše Ime]
 * @param       None
 * @retval      None
 ******************************************************************************/
void Security_TriggerSilentAlarm(void);

/******************************************************************************
 * @brief       Dohvata posljednje poznato stanje (naoružana/razoružana) particije.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija čita stanje iz interne `runtime` varijable koju ažurira
 * `Security_UpdateExternalState` kada stigne `DIN_EVENT` poruka.
 * @param       partition_index Indeks particije (0-2).
 * @retval      bool          Vraća `true` ako je particija naoružana, inače `false`.
 ******************************************************************************/
bool Security_GetPartitionState(uint8_t partition_index);

/******************************************************************************
 * @brief       Dohvata posljednje poznato stanje sistema (u alarmu/nije u alarmu).
 * @author      Gemini & [Vaše Ime]
 * @param       None
 * @retval      bool          Vraća `true` ako je sistem u alarmu, inače `false`.
 ******************************************************************************/
bool Security_GetSystemAlarmState(void);

/******************************************************************************
 * @brief       Procesira događaj primljen sa RS485 bus-a.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovo je jedina ulazna tačka za događaje sa bus-a. Poziva je
 * isključivo `DIN_EVENT_Listener` iz `rs485.c`. Funkcija brzo
 * provjerava da li je adresa iz događaja relevantna za ovaj modul
 * i ako jeste, prosljeđuje ga na internu obradu. Ako nije,
 * odmah se završava.
 * @param       address Adresa izvora događaja (npr. adresa digitalnog ulaza).
 * @param       command Tip komande (očekuje se DIN_EVENT).
 * @param       data    Pokazivač na podatke iz poruke.
 * @param       len     Dužina podataka.
 * @retval      None
 ******************************************************************************/
void SECURITY_BusEvent(uint16_t address, uint8_t command, uint8_t* data, uint8_t len);

/******************************************************************************
 * @brief       Eksplicitno osvježava sva interna stanja čitanjem sa bus-a.
 * @author      Gemini & [Vaše Ime]
 * @note        Ovu funkciju poziva `display.c` neposredno prije iscrtavanja
 * kontrolnog ekrana kako bi osigurao da GUI prikazuje najsvježije
 * podatke direktno sa hardvera.
 * @param       None
 * @retval      None
 ******************************************************************************/
void Security_RefreshState(void);

/******************************************************************************
 * @brief       Validira uneseni korisnički kod.
 * @author      Gemini & [Vaše Ime]
 * @note        Poredi proslijeđeni string sa globalnom `system_pin` varijablom
 * koja se učitava iz EEPROM-a u `main.c`.
 * @param       code Pokazivač na string koji sadrži uneseni kod.
 * @retval      bool Vraća `true` ako je kod ispravan, inače `false`.
 ******************************************************************************/
bool Security_ValidateUserCode(const char* code);

/******************************************************************************
 * @brief       Vraća ukupan broj particija koje su konfigurisane.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija iterira kroz `partition_relay_addr` niz i broji sve
 * unose čija adresa nije 0. Koristi je `display.c` da dinamički
 * prilagodi interfejs na `SCREEN_SECURITY` ekranu (prikaz jednog
 * ili više dugmadi).
 * @param       None
 * @retval      uint8_t Broj konfigurisanih particija (0 do 3).
 ******************************************************************************/
uint8_t Security_GetConfiguredPartitionsCount(void);

/******************************************************************************
 * @brief       Provjerava da li je ijedna od konfigurisanih particija naoružana.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija prolazi kroz sve particije i vraća `true` čim pronađe
 * prvu koja je istovremeno konfigurisana (adresa releja > 0) i u
 * naoružanom stanju (`partition_is_armed` fleg). Koristi je
 * `display.c` za određivanje teksta na glavnom 'SYSTEM' dugmetu.
 * @param       None
 * @retval      bool    Vraća `true` ako je bar jedna particija naoružana,
 * inače `false`.
 ******************************************************************************/
bool Security_IsAnyPartitionArmed(void);

#endif // __SECURITY_H__