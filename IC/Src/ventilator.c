/**
 ******************************************************************************
 * Project          : KuceDzevadova
#include <stm32f7xx.h>
 ******************************************************************************
 */


#if (__VENTILATOR_CTRL_H__ != FW_BUILD)
#error "ventilator header version mismatch"
#endif

/*============================================================================*/
/* UKLJUCENI FAJLOVI (INCLUDES)                                               */
/*============================================================================*/
#include "main.h" // Uvijek prvi
#include "thermostat.h"
#include "ventilator.h"
#include "defroster.h"
#include "curtain.h"
#include "lights.h"
#include "display.h"
#include "stm32746g_eeprom.h" // Mapa ide nakon svih definicija
#include "rs485.h"

// Globalna instanca strukture ventilatora
Ventilator ventilator;

// --- NOVO: Konstanta za mnozenje tajmera ---
#define VENTILATOR_TIMER_FACTOR (10 * 1000) // Vreme se množi sa 10 sekundi u milisekunde

/* Private Function Prototypes -----------------------------------------------*/
static void HandleDelayOffTimer(void);
static void HandleDelayOnTimer(void);
static void HandleTriggerSources(void);
static void HandleVentilatorStatusChanges(void);
bool Ventilator_isConfigured(Ventilator* const vent);

/* Public Functions ----------------------------------------------------------*/
// Implementacija helper funkcije
bool Ventilator_isConfigured(Ventilator* const vent)
{
    return (vent->config.relay > 0) || (vent->config.local_pin > 0);
}

/**
 * @brief Postavlja sve parametre ventilatora na sigurne fabricke vrijednosti.
 * @note Ova funkcija resetuje i konfiguracione (EEPROM) i runtime clanove
 * globalne 'ventilator' instance. Koristi se prilikom prve inicijalizacije
 * ili kada CRC/magic number provjera u Ventilator_Init() ne uspije.
 */
void Ventilator_SetDefault(void)
{
    // KORAK 1: Sigurnosno nuliranje cijele strukture.
    // Ovo osigurava da svi clanovi, ukljucujuci i one koji bi se mogli dodati
    // u buducnosti, budu postavljeni na 0 i ne sadrže nasumicne vrijednosti iz memorije.
    memset(&ventilator, 0, sizeof(Ventilator));

    // KORAK 2: Eksplicitno postavljanje default vrijednosti za svaki clan radi
    //          citljivosti koda i jasnog definisanja pocetnog stanja.

    // --- Resetovanje Konfiguracionih (EEPROM) Parametara ---

    // 'magic_number' i 'crc' se namjerno ostavljaju na 0.
    // Njihove ispravne vrijednosti ce biti postavljene unutar
    // Ventilator_Save() funkcije neposredno prije upisa u EEPROM.
    ventilator.config.magic_number = 0;
    ventilator.config.crc = 0;

    // Vrijednost 0 oznacava da ventilator nije povezan na Modbus relej.
    ventilator.config.relay = 0;

    // Vrijednost 0 oznacava da je funkcija odloženog paljenja iskljucena.
    ventilator.config.delayOnTime = 0;

    // Vrijednost 0 oznacava da je funkcija odloženog gašenja iskljucena.
    ventilator.config.delayOffTime = 0;

    // Vrijednost 0 oznacava da nije definisan prvi izvor okidanja (svjetlo).
    ventilator.config.trigger_source1 = 0;

    // Vrijednost 0 oznacava da nije definisan drugi izvor okidanja (svjetlo).
    ventilator.config.trigger_source2 = 0;

    // Vrijednost 0 oznacava da ventilator nije povezan na lokalni GPIO pin.
    ventilator.config.local_pin = 0;


    // --- Resetovanje Runtime (privremenih) Parametara ---

    // Vrijednost 0 oznacava da tajmer za odloženo paljenje nije aktivan.
    ventilator.delayOnTimerStart = 0;

    // Vrijednost 0 oznacava da tajmer za odloženo gašenje nije aktivan.
    ventilator.DelayOffTimerStart = 0;

    // Vrijednost 0 resetuje sve flegove. Makro VENTILATOR_IS_ACTIVE() ce vracati false.
    ventilator.flags = 0;
}
/**
 * @brief Postavlja relejni indeks ventilatora.
 * @param vent Pokazivac na strukturu Ventilator.
 * @param val Vrednost relejnog indeksa.
 */
void Ventilator_setRelay(Ventilator* const vent, const uint16_t val)
{
    vent->config.relay = val;
}

/**
 * @brief Dobija relejni indeks ventilatora.
 * @param vent Pokazivac na strukturu Ventilator.
 * @return Vrednost relejnog indeksa.
 */
uint16_t Ventilator_getRelay(Ventilator* const vent)
{
    return vent->config.relay;
}

/**
 * @brief Postavlja vreme odlaganja ukljucivanja ventilatora.
 * @param vent Pokazivac na strukturu Ventilator.
 * @param val Vreme odlaganja (u jedinicama).
 */
void Ventilator_setDelayOnTime(Ventilator* const vent, uint8_t val)
{
    vent->config.delayOnTime = val;
}

/**
 * @brief Dobija vreme odlaganja ukljucivanja ventilatora.
 * @param vent Pokazivac na strukturu Ventilator.
 * @return Vreme odlaganja (u jedinicama).
 */
uint8_t Ventilator_getDelayOnTime(Ventilator* const vent)
{
    return vent->config.delayOnTime;
}

/**
 * @brief Postavlja vreme odlaganja iskljucivanja ventilatora.
 * @param vent Pokazivac na strukturu Ventilator.
 * @param val Vreme odlaganja (u jedinicama).
 */
void Ventilator_setDelayOffTime(Ventilator* const vent, uint8_t val)
{
    vent->config.delayOffTime = val;
}

/**
 * @brief Dobija vreme odlaganja iskljucivanja ventilatora.
 * @param vent Pokazivac na strukturu Ventilator.
 * @return Vreme odlaganja (u jedinicama).
 */
uint8_t Ventilator_getDelayOffTime(Ventilator* const vent)
{
    return vent->config.delayOffTime;
}
/**
 * @brief Cuva konfiguraciju ventilatora u EEPROM.
 */
void Ventilator_Save(void)
{
    // Postavi magicni broj prije snimanja
    ventilator.config.magic_number = EEPROM_MAGIC_NUMBER; // Koristimo globalni magic number
    // Nuliraj CRC polje radi ispravnog izracuna
    ventilator.config.crc = 0;
    // Izracunaj CRC nad cijelom 'config' strukturom
    ventilator.config.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&ventilator.config, sizeof(Ventilator_EepromConfig_t));
    // Snimi cijelu strukturu odjednom
    EE_WriteBuffer((uint8_t*)&(ventilator.config), EE_VENTILATOR, sizeof(Ventilator_EepromConfig_t));
}


/**
 * @brief Inicijalizuje konfiguraciju ventilatora iz EEPROM-a.
 */
void Ventilator_Init(void)
{
    // Procitaj cijeli blok podataka iz EEPROM-a
    EE_ReadBuffer((uint8_t*)&(ventilator.config), EE_VENTILATOR, sizeof(Ventilator_EepromConfig_t));

    // Provjeri magicni broj. Ako nije ispravan, podaci su nevažeci.
    if (ventilator.config.magic_number != EEPROM_MAGIC_NUMBER) {
        // Podaci nisu validni, postavi fabricke vrijednosti
        // (potrebno je kreirati Ventilator_SetDefault funkciju)
        Ventilator_SetDefault(); // Pretpostavka da ova funkcija postoji
        Ventilator_Save(); // I odmah ih snimi
    } else {
        // Magicni broj je OK, provjeri CRC
        uint16_t received_crc = ventilator.config.crc;
        ventilator.config.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&ventilator.config, sizeof(Ventilator_EepromConfig_t));

        if (received_crc != calculated_crc) {
            // Podaci su ošteceni, vrati na fabricke
            Ventilator_SetDefault();
            Ventilator_Save();
        }
    }

    // Inicijalizuj runtime varijable
    ventilator.flags = 0;
    ventilator.delayOnTimerStart = 0;
    ventilator.DelayOffTimerStart = 0;
}

/**
 * @brief Ukljucuje ventilator, sa opcijom odloženog ukljucivanja.
 * @param useDelay Ako je true, koristi odlaganje ukljucivanja ako je konfigurisano.
 */
void Ventilator_On(const bool useDelay)
{
    if(!Ventilator_isConfigured(&ventilator)) return;

    // PROMJENA: Poništavamo tajmer za gašenje ako je startovan
    ventilator.DelayOffTimerStart = 0;

    if(useDelay && (ventilator.config.delayOnTime > 0))
    {
        ventilator.delayOnTimerStart = HAL_GetTick();
        if(!ventilator.delayOnTimerStart) ventilator.delayOnTimerStart = 1;
    }
    else
    {
        ventilator.delayOnTimerStart = 0; // Poništava tajmer za ukljucenje
        VENTILATOR_ACTIVATE();

        // NOVO: Ako je delayOffTime podešeno, pokreni tajmer za gašenje
        if (ventilator.config.delayOffTime > 0)
        {
            ventilator.DelayOffTimerStart = HAL_GetTick();
            if(!ventilator.DelayOffTimerStart) ventilator.DelayOffTimerStart = 1;
        }
    }
}

/**
 * @brief Iskljucuje ventilator.
 */
void Ventilator_Off()
{
    if(!Ventilator_isConfigured(&ventilator)) return;

    // Rucna komanda za iskljucenje ima najviši prioritet.
    // Poništavamo sve tajmere i odmah gasimo ventilator.
    ventilator.delayOnTimerStart = 0;
    ventilator.DelayOffTimerStart = 0;

    VENTILATOR_DEACTIVATE();
}
// NOVO: Getteri i setteri za trigger_source
void Ventilator_setTriggerSource1(Ventilator* const vent, const uint8_t val)
{
    vent->config.trigger_source1 = val;
}
uint8_t Ventilator_getTriggerSource1(Ventilator* const vent)
{
    return vent->config.trigger_source1;
}
void Ventilator_setTriggerSource2(Ventilator* const vent, const uint8_t val)
{
    vent->config.trigger_source2 = val;
}
uint8_t Ventilator_getTriggerSource2(Ventilator* const vent)
{
    return vent->config.trigger_source2;
}
void Ventilator_setLocalPin(Ventilator* const vent, const uint8_t val)
{
    vent->config.local_pin = val;
}
uint8_t Ventilator_getLocalPin(Ventilator* const vent)
{
    return vent->config.local_pin;
}
/* Private Helper Functions for Ventilator_Service ---------------------------*/

/**
 * @brief Proverava i upravlja tajmerom za odloženo iskljucivanje ventilatora.
 */
static void HandleDelayOffTimer(void)
{
    if(ventilator.DelayOffTimerStart && ((HAL_GetTick() - ventilator.DelayOffTimerStart) >= (ventilator.config.delayOffTime * VENTILATOR_TIMER_FACTOR)))
    {
        VENTILATOR_DEACTIVATE();
        ventilator.DelayOffTimerStart = 0;
        DISP_SignalDynamicIconUpdate(); // NOVO: Obavijesti GUI da je doslo do promjene i da treba ažurirati ikonu
    }
}

/**
 * @brief Proverava i upravlja tajmerom za odloženo ukljucivanje ventilatora.
 */
static void HandleDelayOnTimer(void)
{
    if(ventilator.delayOnTimerStart && ((HAL_GetTick() - ventilator.delayOnTimerStart) >= (ventilator.config.delayOnTime * VENTILATOR_TIMER_FACTOR)))
    {
        VENTILATOR_ACTIVATE();
        ventilator.delayOnTimerStart = 0;
        DISP_SignalDynamicIconUpdate(); // NOVO: Obavijesti GUI da je doslo do promjene i da treba ažurirati ikonu
    }
}

/**
 * @brief Provjerava stanje okidackih svjetala i upravlja stanjem ventilatora.
 * @note Ova funkcija cita indekse svjetala okidaca iz konfiguracije ventilatora.
 * Zatim, koristeci novi `lights` API, provjerava da li je ijedno od tih
 * svjetala aktivno. Na osnovu promjene stanja okidaca, pali ili pokrece
 * tajmer za gašenje ventilatora.
 */
static void HandleTriggerSources(void)
{
    // Provjera da li je ventilator uopšte odabran u meniju (ova logika ostaje ista).
    if (g_display_settings.selected_control_mode != MODE_VENTILATOR) {
        return;
    }

    static bool old_trigger_state = false;
    bool current_trigger_state = false;

    // =======================================================================
    // === POCETAK REFAKTORISANJA ===
    
    // KORAK 1: Provjeravamo prvi izvor okidanja (trigger source 1).
    // Napomena: Pretpostavka je da se u meniju bira 1-6, a cuva 1-6.
    // API `GetInstance` ocekuje 0-5. Zato oduzimamo 1.
    uint8_t trigger_index_1 = Ventilator_getTriggerSource1(&ventilator);
    
    // Provjeravamo da li je triger uopšte konfigurisan (vrijednost > 0).
    if (trigger_index_1 > 0 && trigger_index_1 <= LIGHTS_MODBUS_SIZE) {
        
        // KORAK 2: Dobijamo "handle" na svjetlo koristeci novi API.
        LIGHT_Handle* handle1 = LIGHTS_GetInstance(trigger_index_1 - 1);
        
        // KORAK 3: Ako je handle validan i svjetlo je aktivno, postavljamo fleg.
        // `ventilator` modul više ne zanima da li svjetlo ima relej ili kako radi,
        // zanima ga samo da li je ono upaljeno.
        if (handle1 && LIGHT_isActive(handle1)) {
            current_trigger_state = true;
        }
    }
    
    // KORAK 4: Ponavljamo isti postupak za drugi izvor okidanja,
    // samo ako prvi vec nije aktivirao ventilator.
    uint8_t trigger_index_2 = Ventilator_getTriggerSource2(&ventilator);
    if (!current_trigger_state && trigger_index_2 > 0 && trigger_index_2 <= LIGHTS_MODBUS_SIZE) {
        
        LIGHT_Handle* handle2 = LIGHTS_GetInstance(trigger_index_2 - 1);
        
        if (handle2 && LIGHT_isActive(handle2)) {
            current_trigger_state = true;
        }
    }
    
    // === KRAJ REFAKTORISANJA ===
    // =======================================================================
    
    // Logika za ukljucivanje/iskljucivanje ventilatora na osnovu promjene stanja
    // ostaje nepromijenjena.
    if (current_trigger_state != old_trigger_state) {
        if (current_trigger_state) { // Stanje se promijenilo sa OFF na ON
            Ventilator_On(true); // Ukljuci sa odgodom ako je konfigurisana
        } else { // Stanje se promijenilo sa ON na OFF
            // Ne gasi se odmah, nego se pokrece tajmer za gašenje
            if (ventilator.config.delayOffTime > 0) {
                ventilator.DelayOffTimerStart = HAL_GetTick();
                if (!ventilator.DelayOffTimerStart) ventilator.DelayOffTimerStart = 1;
            }
        }
    }

    old_trigger_state = current_trigger_state;
}
/**
 * @brief Upravlja promenama stanja i šalje Modbus komande na bus.
 */
static void HandleVentilatorStatusChanges(void)
{
    // NOVO: Logic za slanje komandi
    static uint8_t old_flags = 0;
    if (old_flags != ventilator.flags)
    {
        if(ventilator.config.relay > 0)
        {
            uint8_t sendDataBuff[3] = {0};
            sendDataBuff[0] = (ventilator.config.relay >> 8) & 0xFF;
            sendDataBuff[1] = ventilator.config.relay & 0xFF;
            sendDataBuff[2] = VENTILATOR_IS_ACTIVE() ? BINARY_ON : BINARY_OFF;
            AddCommand(&binaryQueue, BINARY_SET, sendDataBuff, 3);
        }
        old_flags = ventilator.flags;
    }
}

/* Public Service Function ---------------------------------------------------*/

/**
 * @brief Glavna servisna funkcija za ventilator, upravlja tajmerima i stanjem.
 */
void Ventilator_Service()
{
    // PROMJENA: Provjeri da li je ventilator konfigurisan prije pokretanja logike
    if (!Ventilator_isConfigured(&ventilator)) {
        return;
    }
    // PROMJENA: Dodata provjera da li je Defroster odabran u meniju.
    if (g_display_settings.selected_control_mode != MODE_VENTILATOR) {
        return;
    }
    HandleDelayOffTimer();
    HandleDelayOnTimer();
    HandleTriggerSources();
    HandleVentilatorStatusChanges();

    // ISPRAVKA: Fizicka kontrola pina se sada dešava na osnovu statusnog flag-a
    // i provjerava se `local_pin`, a ne `relay`.
    if (ventilator.config.local_pin > 0)
    {
        if (VENTILATOR_IS_ACTIVE())
        {
            SetPin(ventilator.config.local_pin, 1);
        }
        else
        {
            SetPin(ventilator.config.local_pin, 0);
        }
    }
}
