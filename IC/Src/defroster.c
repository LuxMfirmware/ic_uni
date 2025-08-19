/**
 ******************************************************************************
 * Projekt          : KuceDzevadova
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

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


// Globalna instanca strukture odmrzivaca
Defroster defroster;

/* Private Function Prototypes -----------------------------------------------*/
static void HandleDefrosterCycle(void);
static void HandleDefrosterActiveTime(void);

/* Public Functions ----------------------------------------------------------*/
/**
 * @brief Postavlja sve parametre odmrzivaca na sigurne fabricke vrijednosti.
 */
void Defroster_SetDefault(void)
{
    memset(&defroster, 0, sizeof(Defroster));
    // Vrijednosti su vec 0, ali ih eksplicitno postavljamo radi citljivosti
    defroster.config.cycleTime = 0;
    defroster.config.activeTime = 0;
    defroster.config.pin = 0;
}
/**
 * @brief Inicijalizuje konfiguraciju odmrzivaca iz EEPROM-a.
 */
void Defroster_Init(void)
{
    EE_ReadBuffer((uint8_t*)&defroster.config, EE_DEFROSTER, sizeof(Defroster_EepromConfig_t));

    if (defroster.config.magic_number != EEPROM_MAGIC_NUMBER) {
        Defroster_SetDefault();
        Defroster_Save();
    } else {
        uint16_t received_crc = defroster.config.crc;
        defroster.config.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&defroster.config, sizeof(Defroster_EepromConfig_t));
        if (received_crc != calculated_crc) {
            Defroster_SetDefault();
            Defroster_Save();
        }
    }
    // Runtime varijable se uvijek resetuju na pocetku
    defroster.cycleTime_TimerStart = 0;
    defroster.activeTime_TimerStart = 0;
}


/**
 * @brief Cuva konfiguraciju odmrzivaca u EEPROM.
 */
void Defroster_Save(void)
{
    defroster.config.magic_number = EEPROM_MAGIC_NUMBER;
    defroster.config.crc = 0;
    defroster.config.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&defroster.config, sizeof(Defroster_EepromConfig_t));
    EE_WriteBuffer((uint8_t*)&defroster.config, EE_DEFROSTER, sizeof(Defroster_EepromConfig_t));
}

/**
 * @brief Postavlja vreme ciklusa odmrzivaca.
 * @param time Vreme ciklusa u minutama.
 */
void Defroster_SetCycleTime(uint8_t time)
{
    defroster.config.cycleTime = time;

    // Osigurava da aktivno vreme ne bude vece od vremena ciklusa
    if(defroster.config.activeTime > time)
    {
        defroster.config.activeTime = time;
    }
}

/**
 * @brief Postavlja aktivno vreme odmrzivaca.
 * @param time Aktivno vreme u minutama.
 */
void Defroster_SetActiveTime(uint8_t time)
{
    // Osigurava da aktivno vreme ne bude vece od vremena ciklusa
    if(time > defroster.config.cycleTime)
    {
        defroster.config.activeTime = defroster.config.cycleTime;
    }
    else
    {
        defroster.config.activeTime = time;
    }
}

/**
 * @brief Pokrece tajmer za aktivno vreme odmrzivaca.
 */
void Defroster_ActiveTimeTimerStart(void)
{
    uint32_t tick = HAL_GetTick();
    defroster.activeTime_TimerStart = tick ? (tick - 1) : 1; // Osigurava da tajmer ne pocne od nule
}

/**
 * @brief Proverava da li je tajmer za aktivno vreme ukljucen.
 * @return True ako je tajmer ukljucen, false inace.
 */
bool Defroster_isActiveTimeTimerOn(void)
{
    return defroster.activeTime_TimerStart;
}

/**
 * @brief Proverava da li je tajmer za aktivno vreme istekao.
 * @param tick Trenutni sistemski tick.
 * @return True ako je tajmer istekao, false inace.
 */
bool Defroster_hasActiveTimeTimerExpired(const uint32_t tick)
{
    return (tick - defroster.activeTime_TimerStart) >= (defroster.config.activeTime * 1000 * 60); // Konvertuje minute u milisekunde
}

/**
 * @brief Zaustavlja tajmer za aktivno vreme odmrzivaca.
 */
void Defroster_ActiveTimeTimerStop(void)
{
    defroster.activeTime_TimerStart = 0;
}

/**
 * @brief Pokrece ciklicni tajmer odmrzivaca.
 */
void Defroster_CycleTimerStart(void)
{
    uint32_t tick = HAL_GetTick();
    defroster.cycleTime_TimerStart = tick ? (tick - 1) : 1; // Osigurava da tajmer ne pocne od nule
}

/**
 * @brief Proverava da li je ciklicni tajmer ukljucen.
 * @return True ako je tajmer ukljucen, false inace.
 */
bool Defroster_isCycleTimerOn(void)
{
    return defroster.cycleTime_TimerStart;
}

/**
 * @brief Proverava da li je ciklicni tajmer istekao.
 * @param tick Trenutni sistemski tick.
 * @return True ako je tajmer istekao, false inace.
 */
bool Defroster_hasCycleTimerExpired(const uint32_t tick)
{
    return (tick - defroster.cycleTime_TimerStart) >= (defroster.config.cycleTime * 1000 * 60); // Konvertuje minute u milisekunde
}

/**
 * @brief Zaustavlja ciklicni tajmer odmrzivaca.
 */
void Defroster_CycleTimerStop(void)
{
    defroster.cycleTime_TimerStart = 0;
}

/**
 * @brief Proverava da li je odmrzivac aktivan (tj. da li mu je ciklicni tajmer pokrenut).
 * @return True ako je aktivan, false inace.
 */
bool Defroster_isActive(void)
{
    return Defroster_isCycleTimerOn();
}

/**
 * @brief Ukljucuje odmrzivac i pokrece oba tajmera.
 */
void Defroster_On(void)
{
    Defroster_CycleTimerStart();
    Defroster_ActiveTimeTimerStart();
    SetPin(defroster.config.pin, 1); // Postavlja pin na HIGH
}

/**
 * @brief Iskljucuje odmrzivac i zaustavlja oba tajmera.
 */
void Defroster_Off(void)
{
    Defroster_CycleTimerStop();
    Defroster_ActiveTimeTimerStop();
    SetPin(defroster.config.pin, 0); // Postavlja pin na LOW
}

/* Private Helper Functions for Defroster_Service ----------------------------*/

/**
 * @brief Upravlja ciklusom rada odmrzivaca.
 * Ako je ciklicni tajmer istekao, ponovo ga pokrece i ukljucuje pin.
 */
static void HandleDefrosterCycle(void)
{
    const uint32_t tick = HAL_GetTick(); // Dobija trenutni tick jednom

    if(Defroster_isCycleTimerOn() && Defroster_hasCycleTimerExpired(tick))
    {
        Defroster_CycleTimerStart();      // Ponovo pokreni ciklicni tajmer
        Defroster_ActiveTimeTimerStart(); // Pokreni tajmer za aktivno vreme
        SetPin(defroster.config.pin, 1);         // Ukljuci pin odmrzivaca
    }
}

/**
 * @brief Upravlja aktivnim vremenom odmrzivaca.
 * Ako je tajmer za aktivno vreme istekao, zaustavlja ga i iskljucuje pin.
 */
static void HandleDefrosterActiveTime(void)
{
    const uint32_t tick = HAL_GetTick(); // Dobija trenutni tick jednom

    if(Defroster_isActiveTimeTimerOn() && Defroster_hasActiveTimeTimerExpired(tick))
    {
        Defroster_ActiveTimeTimerStop(); // Zaustavi tajmer za aktivno vreme
        SetPin(defroster.config.pin, 0);        // Iskljuci pin odmrzivaca
    }
}

/* Public Service Function ---------------------------------------------------*/

/**
 * @brief Glavna servisna funkcija za odmrzivac, upravlja ciklusima i stanjem.
 * @note Ova funkcija treba da se poziva periodicno u glavnoj petlji.
 */
void Defroster_Service(void)
{
    // PROMJENA: Dodata provjera da li je Defroster odabran u meniju.
    if (g_display_settings.selected_control_mode != MODE_DEFROSTER) {
        return;
    }

    if(Defroster_isActive()) // Samo ako je odmrzivac globalno aktivan
    {
        HandleDefrosterCycle();      // Upravljaj ciklusom
        HandleDefrosterActiveTime(); // Upravljaj aktivnim vremenom
    }
}
