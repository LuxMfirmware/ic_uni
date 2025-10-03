/**
 ******************************************************************************
 * @file    security.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Implementacija logike za alarmni sistem.
 *
 * @note    Ovaj modul je "mozak" alarmnog sistema. On upravlja postavkama,
 * šalje komande za aktivaciju/deaktivaciju i obrađuje povratne
 * informacije o stanju sistema. Komunikacija sa hardverskim
 * modulima (relejima, digitalnim ulazima) se vrši isključivo
 * preko `rs485` modula, dodavanjem komandi u odgovarajuće redove,
 * čime se poštuje postojeća ne-blokirajuća arhitektura sistema.
 ******************************************************************************
 */

#if (__SECURITY_H__ != FW_BUILD)
#error "security header version mismatch"
#endif

/*============================================================================*/
/* UKLJUČENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "security.h"
#include "stm32746g_eeprom.h"
#include "rs485.h"


/*============================================================================*/
/* PRIVATNE (STATIČKE) VARIJABLE                                              */
/*============================================================================*/

/**
 * @brief Definiše korisnički PIN kod za validaciju akcija.
 * @note  Prema Vašem zahtjevu, postavljen je na "7891,7892,7893".
 */
static const char* USER1_PIN = "7891";
static const char* USER2_PIN = "7892";
static const char* USER3_PIN = "7893";

/**
 * @brief Globalna, ali privatna instanca strukture sa postavkama alarma.
 * @note  Ova varijabla se inicijalizuje iz EEPROM-a prilikom pokretanja
 * sistema. Vidljiva je samo unutar ovog fajla (`security.c`).
 */
static Security_Settings_t g_security_settings;

/**
 * @brief Niz koji u RAM-u čuva posljednje poznato stanje svake particije.
 * @note  Ovo su `runtime` podaci. Vrijednost (`true`=naoružana, `false`=razoružana)
 * se ažurira na osnovu `DIN_EVENT` poruka sa RS485 bus-a.
 * GUI modul čita ove vrijednosti da bi prikazao ispravno stanje.
 */
static bool partition_is_armed[SECURITY_PARTITION_COUNT];

/**
 * @brief Fleg koji u RAM-u čuva posljednje poznato stanje alarma sistema.
 * @note  Vrijednost (`true`=u alarmu, `false`=nije u alarmu) se ažurira na
 * osnovu `DIN_EVENT` poruke sa odgovarajuće feedback adrese.
 */
static bool system_is_in_alarm = false;


/*============================================================================*/
/* PROTOTIPOVI PRIVATNIH POMOĆNIH FUNKCIJA                                    */
/*============================================================================*/
static void Execute_Command(uint8_t partition_index);
static void HandleSensorEvent(uint16_t sensor_addr, uint8_t state); 

/*============================================================================*/
/* IMPLEMENTACIJA JAVNOG API-JA                                               */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief       Inicijalizuje alarmni modul pri pokretanju sistema.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova funkcija se poziva jednom iz `main()` funkcije. Učitava
 * konfiguraciju iz EEPROM-a, provjerava njen integritet pomoću
 * magičnog broja i CRC-a. Ako podaci nisu ispravni, poziva
 * `Security_SetDefault()` da postavi sigurne fabričke vrijednosti
 * i odmah ih snima. Koristi ispravnu `EE_SECURITY_SETTINGS` definiciju.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Security_Init(void)
{
    // << ISPRAVKA: Koristi se ispravna EEPROM adresa `EE_SECURITY_SETTINGS` >>
    EE_ReadBuffer((uint8_t*)&g_security_settings, EE_SECURITY, sizeof(Security_Settings_t));

    if (g_security_settings.magic_number != EEPROM_MAGIC_NUMBER)
    {
        Security_SetDefault();
        Security_Save();
    }
    else
    {
        uint16_t received_crc = g_security_settings.crc;
        g_security_settings.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&g_security_settings, sizeof(Security_Settings_t));
        if (received_crc != calculated_crc)
        {
            Security_SetDefault();
            Security_Save();
        }
    }
    Security_RefreshState();
}

/**
 ******************************************************************************
 * @brief       Snima trenutnu konfiguraciju alarmnog modula u EEPROM.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija priprema `g_security_settings` strukturu sa ispravnim
 * `magic_number` i CRC-om, te je snima na definisanu EEPROM lokaciju.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Security_Save(void)
{
    g_security_settings.magic_number = EEPROM_MAGIC_NUMBER;
    g_security_settings.crc = 0;
    g_security_settings.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&g_security_settings, sizeof(Security_Settings_t));
    // << ISPRAVKA: Koristi se ispravna EEPROM adresa `EE_SECURITY_SETTINGS` >>
    EE_WriteBuffer((uint8_t*)&g_security_settings, EE_SECURITY, sizeof(Security_Settings_t));
}

/**
 ******************************************************************************
 * @brief       Postavlja sve konfiguracione parametre na fabričke vrijednosti.
 * @author      Gemini & [Vaše Ime]
 * @note        Resetuje sve adrese na 0 i postavlja trajanje pulsa na 500ms
 * kao sigurnu početnu vrijednost.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Security_SetDefault(void)
{
    memset(&g_security_settings, 0, sizeof(Security_Settings_t));
    g_security_settings.pulse_duration_ms = 500;
}

// --- Implementacija Geter-a i Seter-a ---
uint16_t Security_GetPartitionRelayAddr(uint8_t p) { return (p < SECURITY_PARTITION_COUNT) ? g_security_settings.partition_relay_addr[p] : 0; }
void Security_SetPartitionRelayAddr(uint8_t p, uint16_t addr) { if (p < SECURITY_PARTITION_COUNT) g_security_settings.partition_relay_addr[p] = addr; }
uint16_t Security_GetPartitionFeedbackAddr(uint8_t p) { return (p < SECURITY_PARTITION_COUNT) ? g_security_settings.partition_feedback_addr[p] : 0; }
void Security_SetPartitionFeedbackAddr(uint8_t p, uint16_t addr) { if (p < SECURITY_PARTITION_COUNT) g_security_settings.partition_feedback_addr[p] = addr; }
uint16_t Security_GetSystemStatusFeedbackAddr(void) { return g_security_settings.system_status_feedback_addr; }
void Security_SetSystemStatusFeedbackAddr(uint16_t addr) { g_security_settings.system_status_feedback_addr = addr; }
uint16_t Security_GetSilentAlarmAddr(void) { return g_security_settings.silent_alarm_addr; }
void Security_SetSilentAlarmAddr(uint16_t addr) { g_security_settings.silent_alarm_addr = addr; }
uint16_t Security_GetPulseDuration(void) { return g_security_settings.pulse_duration_ms; }
void Security_SetPulseDuration(uint16_t duration) { g_security_settings.pulse_duration_ms = duration; }

/**
 ******************************************************************************
 * @brief       Vraća ukupan broj particija koje su konfigurisane.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija iterira kroz `partition_relay_addr` niz i broji sve
 * unose čija adresa nije 0. Koristi je `display.c` da dinamički
 * prilagodi interfejs na `SCREEN_SECURITY` ekranu (prikaz jednog
 * ili više dugmadi).
 * @param       None
 * @retval      uint8_t Broj konfigurisanih particija (0 do 3).
 ******************************************************************************
 */
uint8_t Security_GetConfiguredPartitionsCount(void)
{
    uint8_t count = 0;
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++)
    {
        // Uvećaj brojač samo ako je za particiju definisana adresa releja
        if (g_security_settings.partition_relay_addr[i] != 0)
        {
            count++;
        }
    }
    return count;
}

/**
 ******************************************************************************
 * @brief       Provjerava da li je ijedna od konfigurisanih particija naoružana.
 * @author      Gemini & [Vaše Ime]
 * @note        Funkcija prolazi kroz sve particije i vraća `true` čim pronađe
 * prvu koja je istovremeno konfigurisana (adresa releja > 0) i u
 * naoružanom stanju (`partition_is_armed` fleg). Koristi je
 * `display.c` za određivanje teksta na glavnom 'SYSTEM' dugmetu.
 * @param       None
 * @retval      bool    Vraća `true` ako je bar jedna particija naoružana,
 * inače `false`.
 ******************************************************************************
 */
bool Security_IsAnyPartitionArmed(void)
{
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++)
    {
        // Uslov je ispunjen ako je particija konfigurisana I ako je njen status "naoružan"
        if (g_security_settings.partition_relay_addr[i] != 0 && partition_is_armed[i])
        {
            return true; // Pronađena je bar jedna, nema potrebe dalje provjeravati
        }
    }
    return false; // Ako petlja završi, nijedna konfigurisana particija nije naoružana
}

/**
 ******************************************************************************
 * @brief       Šalje komandu za promjenu stanja za jednu particiju.
 * @author      Gemini & [Vaše Ime]
 * @param       partition_index Indeks particije (0, 1 ili 2).
 * @retval      None
 ******************************************************************************
 */
void Security_TogglePartition(uint8_t partition_index)
{
    // << ISPRAVKA: Poziva se Execute_Command sa indeksom, ne sa adresom >>
    if (partition_index < SECURITY_PARTITION_COUNT)
    {
        Execute_Command(partition_index);
    }
}

/**
 ******************************************************************************
 * @brief       Šalje komandu za promjenu stanja za cijeli sistem.
 * @author      Gemini & [Vaše Ime]
 * @note        Logika određuje da li treba poslati ARM ili DISARM komandu svim
 * konfigurisanim particijama.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Security_ToggleSystem(void)
{
    bool arm_command = !Security_IsAnyPartitionArmed();
    
    for (int i = 0; i < SECURITY_PARTITION_COUNT; i++) {
        if (g_security_settings.partition_relay_addr[i] != 0) {
            if (partition_is_armed[i] != arm_command) {
                Execute_Command(i);
            }
        }
    }
}

/**
 ******************************************************************************
 * @brief       Šalje komandu za aktivaciju tihog alarma (SOS).
 * @author      Gemini & [Vaše Ime]
 * @note        Šalje pulsni `BINARY_ON` signal na definisanu adresu. Trajanje
 * pulsa od 5 sekundi je fiksno i konfigurisano na eksternom modulu.
 * @param       None
 * @retval      None
 ******************************************************************************
 */
void Security_TriggerSilentAlarm(void)
{
    uint16_t address = g_security_settings.silent_alarm_addr;
    if (address == 0) return;

    uint8_t data_buff[3];
    data_buff[0] = (address >> 8) & 0xFF;
    data_buff[1] = address & 0xFF;
    data_buff[2] = BINARY_ON;
    AddCommand(&binaryQueue, BINARY_SET, data_buff, 3);
}

bool Security_GetPartitionState(uint8_t partition_index)
{
    return (partition_index < SECURITY_PARTITION_COUNT) ? partition_is_armed[partition_index] : false;
}

bool Security_GetSystemAlarmState(void) { return system_is_in_alarm; }

void SECURITY_BusEvent(uint16_t address, uint8_t command, uint8_t* data, uint8_t len)
{
    if (command != DIN_EVENT || address == 0) return;

    bool is_relevant = false;
    if (address == g_security_settings.system_status_feedback_addr) is_relevant = true;
    else {
        for (uint8_t i = 0; i < SECURITY_PARTITION_COUNT; i++) {
            if (address == g_security_settings.partition_feedback_addr[i]) {
                is_relevant = true;
                break;
            }
        }
    }

    if (is_relevant) {
        uint8_t state = (len > 0) ? data[0] : 0;
        HandleSensorEvent(address, state);
    }
}

void Security_RefreshState(void)
{
    uint8_t response_data;
    for(int i = 0; i < SECURITY_PARTITION_COUNT; ++i) {
        if (g_security_settings.partition_feedback_addr[i] != 0) {
            if (GetState(DIN_GET, g_security_settings.partition_feedback_addr[i], &response_data)) {
                partition_is_armed[i] = (response_data == 1);
            }
        }
    }
    if (g_security_settings.system_status_feedback_addr != 0) {
        if (GetState(DIN_GET, g_security_settings.system_status_feedback_addr, &response_data)) {
            system_is_in_alarm = (response_data == 1);
        }
    }
}

bool Security_ValidateUserCode(const char* code)
{
    return (strcmp(code, USER1_PIN) == 0);
}


/*============================================================================*/
/* IMPLEMENTACIJA PRIVATNIH FUNKCIJA                                          */
/*============================================================================*/

/**
 ******************************************************************************
 * @brief       Interna funkcija koja obrađuje promjenu stanja senzora.
 * @param       sensor_addr Adresa digitalnog ulaza koji je javio promjenu.
 * @param       state       Novo stanje ulaza (1 za ON, 0 za OFF).
 ******************************************************************************
 */
static void HandleSensorEvent(uint16_t sensor_addr, uint8_t state)
{
    bool state_changed = false;
    for(int i = 0; i < SECURITY_PARTITION_COUNT; ++i) {
        if (g_security_settings.partition_feedback_addr[i] == sensor_addr) {
            if (partition_is_armed[i] != (bool)state) {
                partition_is_armed[i] = (bool)state;
                state_changed = true;
            }
            break;
        }
    }

    if (g_security_settings.system_status_feedback_addr == sensor_addr) {
        if (system_is_in_alarm != (bool)state) {
            system_is_in_alarm = (bool)state;
            state_changed = true;
        }
    }

    if (state_changed && (screen == SCREEN_SECURITY)) {
        shouldDrawScreen = 1;
    }
}

/**
 ******************************************************************************
 * @brief       Interna funkcija koja formira i šalje komandu na RS485 bus.
 * @param       partition_index Indeks particije (0-2) za koju se šalje komanda.
 ******************************************************************************
 */
static void Execute_Command(uint8_t partition_index)
{
    uint16_t address = g_security_settings.partition_relay_addr[partition_index];
    if (address == 0) return;

    uint8_t data_buff[3];
    data_buff[0] = (address >> 8) & 0xFF;
    data_buff[1] = address & 0xFF;

    if (g_security_settings.pulse_duration_ms > 0) {
        data_buff[2] = BINARY_ON;
        AddCommand(&binaryQueue, BINARY_SET, data_buff, 3);
    } else {
        data_buff[2] = partition_is_armed[partition_index] ? BINARY_OFF : BINARY_ON;
        AddCommand(&binaryQueue, BINARY_SET, data_buff, 3);
    }
}
