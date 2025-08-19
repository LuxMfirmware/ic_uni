/**
 ******************************************************************************
 * Project          : KuceDzevadova
 ******************************************************************************
 *
 *
 ******************************************************************************
 */

#if (__CURTAIN_CTRL_H__ != FW_BUILD)
#error "curtain header version mismatch"
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

// Globalna varijabla koja sadrži EEPROM podatke
static Curtains_EepromData_t curtains_eeprom_data;
/* Private Defines -----------------------------------------------------------*/
// Removed: CURTAIN_SWITCH_DIRECTION_WAIT_TIME - not used in the provided code.
// Removed: CURTAIN_MOVE_TIME - replaced by upDownDurationSeconds from EEPROM.

/* Private Variables ---------------------------------------------------------*/
// Removed: curtains_send - not used
uint8_t curtains_count = 0;
Curtain curtains[CURTAINS_SIZE];
// curtain_selected is declared in display.h/c, so it's not here.

/* Private Function Prototypes -----------------------------------------------*/
static void HandleCurtainMovement(Curtain* const cur);
static void HandleCurtainDirectionChange(Curtain* const cur);
/* Public Functions ----------------------------------------------------------*/
/**
 * @brief Pronalazi pointer na roletnu na osnovu njenog rednog broja.
 * @param logical_index Redni broj roletne (0 = prva, 1 = druga, itd.).
 * @return Pokazivac na strukturu Curtain* ili NULL ako indeks nije validan.
 */
Curtain* Curtain_GetByLogicalIndex(const uint8_t logical_index) {
    uint8_t found_count = 0;
    for (uint8_t i = 0; i < CURTAINS_SIZE; i++) {
        if (Curtain_hasRelays(curtains + i)) {
            if (found_count == logical_index) {
                return &curtains[i]; // Vraca pointer na ispravnu roletnu
            }
            found_count++;
        }
    }
    return NULL; // Vraca NULL ako roletna nije pronadena
}
/**
 * @brief Selects a specific curtain.
 * @param curtain The index of the curtain to select.
 */
void Curtain_Select(const uint8_t curtain)
{
    curtain_selected = curtain;
}

/**
 * @brief Gets the currently selected curtain.
 * @return The index of the selected curtain.
 */
uint8_t Curtain_getSelected()
{
    return curtain_selected;
}

/**
 * @brief Checks if all curtains are selected.
 * @return True if all curtains are selected, false otherwise.
 */
bool Curtain_areAllSelected()
{
    return curtain_selected == Curtains_getCount();
}

/**
 * @brief Resets the curtain selection to "all curtains".
 */
void Curtain_ResetSelection()
{
    curtain_selected = Curtains_getCount();
}

/**
 * @brief Counts the number of configured curtains.
 */
void Curtains_Count(void)
{
    curtains_count = 0;
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        if(Curtain_hasRelays(curtains + i)) ++curtains_count;
        //else break; // Assuming curtains are contiguous in the array
    }
}

/**
 * @brief Gets the number of configured curtains.
 * @return The count of configured curtains.
 */
uint8_t Curtains_getCount(void)
{
    return curtains_count;
}

/**
 * @brief Initializes a single curtain structure from EEPROM.
 * @param cur Pointer to the Curtain structure.
 * @param addr EEPROM address to read from.
 */
void Curtains_Init(void)
{
    // Ucitaj cijeli blok podataka
    EE_ReadBuffer((uint8_t*)&curtains_eeprom_data, EE_CURTAINS, sizeof(Curtains_EepromData_t));

    // Validacija pomocu magicnog broja i CRC-a
    if (curtains_eeprom_data.magic_number != EEPROM_MAGIC_NUMBER) {
        Curtains_SetDefault();
        Curtains_Save();
    } else {
        uint16_t received_crc = curtains_eeprom_data.crc;
        curtains_eeprom_data.crc = 0;
        uint16_t calculated_crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&curtains_eeprom_data, sizeof(Curtains_EepromData_t));
        if (received_crc != calculated_crc) {
            Curtains_SetDefault();
            Curtains_Save();
        }
    }

    // Nakon uspješne validacije, prebaci konfiguraciju u runtime niz i resetuj stanje
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        curtains[i].config = curtains_eeprom_data.curtains[i];
        // Resetuj runtime stanje za svaku zavjesu
        Curtain_Reset(&curtains[i]);
    }

    Curtains_Count();
}
/**
 * @brief Saves a single curtain structure to EEPROM.
 * @param cur Pointer to the Curtain structure.
 * @param addr EEPROM address to write to.
 */
void Curtains_Save(void)
{
    // Prije snimanja, prebaci trenutne konfiguracije iz runtime niza u EEPROM strukturu
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++) {
        curtains_eeprom_data.curtains[i] = curtains[i].config;
    }
    // Ažuriraj vrijeme kretanja iz getter-a (ako se mijenja izvan postavki)
    curtains_eeprom_data.upDownDurationSeconds = Curtain_GetMoveTime();

    // Izracunaj i snimi CRC
    curtains_eeprom_data.magic_number = EEPROM_MAGIC_NUMBER;
    curtains_eeprom_data.crc = 0;
    curtains_eeprom_data.crc = HAL_CRC_Calculate(&hcrc, (uint32_t*)&curtains_eeprom_data, sizeof(Curtains_EepromData_t));
    EE_WriteBuffer((uint8_t*)&curtains_eeprom_data, EE_CURTAINS, sizeof(Curtains_EepromData_t));

    // Ponovo prebroji konfigurisane zavjese nakon snimanja
    Curtains_Count();
}

/**
 * @brief Checks if a curtain has valid up and down relays configured.
 * @param cur Pointer to the Curtain structure.
 * @return True if relays are configured, false otherwise.
 */
bool Curtain_hasRelays(const Curtain* const cur)
{
    return cur->config.relayUp && cur->config.relayDown;
}

/**
 * @brief Gets the up-relay index for a curtain.
 * @param cur Pointer to the Curtain structure.
 * @return The up-relay index.
 */
uint16_t Curtain_GetRelayUp(const Curtain* const cur)
{
    return cur->config.relayUp;
}

/**
 * @brief Sets the up-relay index for a curtain.
 * @param cur Pointer to the Curtain structure.
 * @param val The up-relay index to set.
 */
void Curtain_SetRelayUp(Curtain* const cur, const uint16_t val)
{
    cur->config.relayUp = val;
}

/**
 * @brief Gets the down-relay index for a curtain.
 * @param cur Pointer to the Curtain structure.
 * @return The down-relay index.
 */
uint16_t Curtain_GetRelayDown(const Curtain* const cur)
{
    return cur->config.relayDown;
}

/**
 * @brief Sets the down-relay index for a curtain.
 * @param cur Pointer to the Curtain structure.
 * @param val The down-relay index to set.
 */
void Curtain_SetRelayDown(Curtain* const cur, const uint16_t val)
{
    cur->config.relayDown = val;
}

/**
 * @brief Gets the old direction of a curtain.
 * @param cur Pointer to the Curtain structure.
 * @return The old direction (CURTAIN_STOP, CURTAIN_UP, CURTAIN_DOWN).
 */
uint8_t Curtain_getDirection(const Curtain* const cur)
{
    return cur->upDown_old;
}

/**
 * @brief Gets the new desired direction of a curtain.
 * @param cur Pointer to the Curtain structure.
 * @return The new direction (CURTAIN_STOP, CURTAIN_UP, CURTAIN_DOWN).
 */
uint8_t Curtain_getNewDirection(const Curtain* const cur)
{
    return cur->upDown;
}

/**
 * @brief Sets the global move time for curtains.
 * @param seconds The move time in seconds.
 */
void Curtain_SetMoveTime(const uint8_t seconds)
{
    curtains_eeprom_data.upDownDurationSeconds = seconds;
}

/**
 * @brief Gets the global move time for curtains.
 * @return The move time in seconds.
 */
uint8_t Curtain_GetMoveTime(void)
{
    return curtains_eeprom_data.upDownDurationSeconds;
}

/**
 * @brief Checks if the direction of a curtain has changed.
 * @param cur Pointer to the Curtain structure.
 * @return True if direction has changed, false otherwise.
 */
bool Curtain_hasDirectionChanged(const Curtain* const cur)
{
    return cur->upDown != cur->upDown_old;
}

/**
 * @brief Equalizes the old and new direction of a curtain.
 * @param cur Pointer to the Curtain structure.
 */
void Curtain_DirectionEqualize(Curtain* const cur)
{
    cur->upDown_old = cur->upDown;
}

/**
 * @brief Checks if a curtain is currently moving.
 * @param cur Pointer to the Curtain structure.
 * @return True if moving, false otherwise.
 */
bool Curtain_isMoving(const Curtain* const cur)
{
    return (Curtain_hasRelays(cur)) ? (cur->upDown_old != CURTAIN_STOP) : false;
}

/**
 * @brief Checks if any curtain is currently moving.
 * @return True if any curtain is moving, false otherwise.
 */
bool Curtains_isAnyCurtainMoving(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMoving(curtains + i)) return true;
    }
    return false;
}

/**
 * @brief Checks if all configured curtains are currently moving.
 * @return True if all configured curtains are moving, false otherwise.
 */
bool Curtains_areAllMoving(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i) && (!Curtain_isMoving(curtains + i))) return false;
    }
    return true;
}

/**
 * @brief Checks if all configured curtains are moving in the same specified direction.
 * @param direction The direction to check against (CURTAIN_UP or CURTAIN_DOWN).
 * @return True if all configured curtains are moving in the same direction, false otherwise.
 */
bool Curtains_areAllMovinginSameDirection(const uint8_t direction)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i) && (curtains[i].upDown_old != direction)) return false;
    }
    return true;
}

/**
 * @brief Checks if a curtain is moving up.
 * @param cur Pointer to the Curtain structure.
 * @return True if moving up, false otherwise.
 */
bool Curtain_isMovingUp(const Curtain* const cur)
{
    return cur->upDown_old == CURTAIN_UP;
}

/**
 * @brief Checks if any curtain is moving up.
 * @return True if any curtain is moving up, false otherwise.
 */
bool Curtains_isAnyCurtainMovingUp(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMovingUp(curtains + i)) return true;
    }
    return false;
}

/**
 * @brief Checks if a curtain is moving down.
 * @param cur Pointer to the Curtain structure.
 * @return True if moving down, false otherwise.
 */
bool Curtain_isMovingDown(const Curtain* const cur)
{
    return cur->upDown_old == CURTAIN_DOWN;
}

/**
 * @brief Checks if any curtain is moving down.
 * @return True if any curtain is moving down, false otherwise.
 */
bool Curtains_isAnyCurtainMovingDown(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_isMovingDown(curtains + i)) return true;
    }
    return false;
}

/**
 * @brief Checks if the new desired direction of a curtain is stop.
 * @param cur Pointer to the Curtain structure.
 * @return True if new direction is stop, false otherwise.
 */
bool Curtain_isNewDirectionStop(const Curtain* const cur)
{
    return cur->upDown == CURTAIN_STOP;
}

/**
 * @brief Checks if the new desired direction of a curtain is up.
 * @param cur Pointer to the Curtain structure.
 * @return True if new direction is up, false otherwise.
 */
bool Curtain_isNewDirectionUp(const Curtain* const cur)
{
    return cur->upDown == CURTAIN_UP;
}

/**
 * @brief Checks if the new desired direction for all curtains is up.
 * @return True if new direction for all is up, false otherwise.
 */
bool Curtains_isNewDirectionUp(void)
{
    uint8_t anyRelay = 0;
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i))
        {
            if(!Curtain_isNewDirectionUp(curtains + i)) return false;
            anyRelay = 1;
        }
    }
    return anyRelay ? true : false;
}

/**
 * @brief Checks if the new desired direction of a curtain is down.
 * @param cur Pointer to the Curtain structure.
 * @return True if new direction is down, false otherwise.
 */
bool Curtain_isNewDirectionDown(const Curtain* const cur)
{
    return cur->upDown == CURTAIN_DOWN;
}

/**
 * @brief Checks if the new desired direction for all curtains is down.
 * @return True if new direction for all is down, false otherwise.
 */
bool Curtains_isNewDirectionDown(void)
{
    uint8_t anyRelay = 0;
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        if(Curtain_hasRelays(curtains + i))
        {
            if(!Curtain_isNewDirectionDown(curtains + i)) return false;
            anyRelay = 1;
        }
    }
    return anyRelay ? true : false;
}

/**
 * @brief Stops a single curtain.
 * @param cur Pointer to the Curtain structure.
 */
void Curtain_Stop(Curtain* const cur)
{
    cur->upDown = CURTAIN_STOP;
}

/**
 * @brief Stops all curtains.
 */
void Curtains_Stop()
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        Curtain_Stop(curtains + i);
    }
}

/**
 * @brief Moves a single curtain in the specified direction.
 * @param cur Pointer to the Curtain structure.
 * @param direction The direction to move (CURTAIN_UP, CURTAIN_DOWN, CURTAIN_STOP).
 */
void Curtain_Move(Curtain* const cur, const uint8_t direction)
{
    if(Curtain_hasRelays(cur))
    {

        cur->external_cmd = 0; // NOVO: Kada se displej dotakne, preuzima se kontrola
        cur->upDown = direction;
        cur->upDownTimer = HAL_GetTick();
        if(!cur->upDownTimer) cur->upDownTimer = 1; // Ensure non-zero
    }
}

/**
 * @brief Sends a move signal to a single curtain, toggling stop if already moving in that direction.
 * @param cur Pointer to the Curtain structure.
 * @param direction The desired direction.
 */
void Curtain_MoveSignal(Curtain* const cur, const uint8_t direction)
{
    if(Curtain_isMoving(cur) && (cur->upDown == direction) && (cur->upDown_old == direction)) Curtain_Stop(cur);
    else Curtain_Move(cur, direction);
}

/**
 * @brief Moves all curtains in the specified direction.
 * @param direction The direction to move.
 */
void Curtains_Move(const uint8_t direction)
{
    // Original code had `curtains_send = 1;` here, but curtains_send is unused. Removed.

    // If any curtain is already moving, set their old_direction to stop to force a new command
    // This handles the "reverse direction" logic by forcing a stop before moving in new direction
    if(Curtains_isAnyCurtainMoving())
    {
        for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
        {
            if(Curtain_hasRelays(curtains + i))
                (curtains + i)->upDown_old = CURTAIN_STOP; // Force a change detection
        }
    }

    for(uint8_t i = 0; i < CURTAINS_SIZE; ++i)
    {
        Curtain_Move(curtains + i, direction);
    }
}

/**
 * @brief Sends a move signal to all curtains, intelligently stopping or moving them.
 * @param direction The desired direction.
 */
void Curtains_MoveSignal(const uint8_t direction)
{
    // PROMENA: Petlja sada prolazi do maksimalnog broja roletni (CURTAINS_SIZE).
    // Proverimo da li se ijedna roletna vec krece u odabranom smjeru
    bool any_moving_in_direction = false;
    for (uint8_t i = 0; i < CURTAINS_SIZE; ++i) {
        if (Curtain_hasRelays(curtains + i) && Curtain_isMoving(curtains + i) && (Curtain_getNewDirection(curtains + i) == direction)) {
            any_moving_in_direction = true;
            break;
        }
    }

    if (any_moving_in_direction) {
        // Ako se ijedna roletna krece u tom smjeru, zaustavi sve koje se krecu u tom smjeru
        // PROMENA: Petlja sada prolazi do maksimalnog broja roletni (CURTAINS_SIZE).
        for (uint8_t i = 0; i < CURTAINS_SIZE; ++i) {
            if (Curtain_hasRelays(curtains + i) && Curtain_isMoving(curtains + i) && (Curtain_getNewDirection(curtains + i) == direction)) {
                Curtain_Stop(curtains + i);
            }
        }
    } else {
        // Ako se nijedna ne krece u tom smjeru, pokreni ih sve u tom smjeru
        // PROMENA: Petlja sada prolazi do maksimalnog broja roletni (CURTAINS_SIZE).
        for (uint8_t i = 0; i < CURTAINS_SIZE; ++i) {
            if (Curtain_hasRelays(curtains + i)) { // Dodata provera da se ne obraduju nekonfigurisane roletne
                Curtain_Move(curtains + i, direction);
            }
        }
    }
}
/**
 * @brief Checks if a curtain index is within the valid range for Modbus operations.
 * @param index The index to check.
 * @return True if the index is valid, false otherwise.
 */
bool Curtain_Modbus_isIndexInRange(const uint8_t index)
{
    return (index < CURTAINS_SIZE); // uint8_t is always >= 0
}

/**
 * @brief Gets the current direction of a curtain by its index for Modbus.
 * @param index The index of the curtain.
 * @return The direction (CURTAIN_STOP, CURTAIN_UP, CURTAIN_DOWN).
 */
uint8_t Curtain_Modbus_Get_byIndex(const uint8_t index)
{
    if(Curtain_isNewDirectionStop(curtains + index)) return CURTAIN_STOP;
    else if(Curtain_isNewDirectionUp(curtains + index)) return CURTAIN_UP;
    else return CURTAIN_DOWN;
}

/**
 * @brief Sends a move signal to a specific curtain by its index.
 * @param index The index of the curtain.
 * @param direction The desired direction.
 */
void Curtain_MoveSignal_byIndex(uint8_t const index, const uint8_t direction)
{
    Curtain_MoveSignal(curtains + index, direction);
}

/**
 * @brief Restarts the movement timer for a curtain.
 * @param cur Pointer to the Curtain structure.
 */
void Curtain_RestartTimer(Curtain* const cur)
{
    cur->upDownTimer = HAL_GetTick();
    if(!cur->upDownTimer) cur->upDownTimer = 1; // Ensure non-zero
}

/**
 * @brief Checks if the switch direction wait time has expired for a curtain.
 * @param cur Pointer to the Curtain structure.
 * @return True if expired, false otherwise.
 */
bool Curtain_HasSwitchDirectionTimeExpired(const Curtain* const cur)
{
    // CURTAIN_SWITCH_DIRECTION_WAIT_TIME is removed as it's not used in the service loop.
    // This function is also not called anywhere in the provided code.
    return (HAL_GetTick() - cur->upDownTimer) >= 500; // Using hardcoded 500ms as per original macro value
}

/**
 * @brief Checks if the total move time has expired for a curtain.
 * @param cur Pointer to the Curtain structure.
 * @return True if expired, false otherwise.
 */
bool Curtain_hasMoveTimeExpired(const Curtain* const cur)
{
    return (HAL_GetTick() - cur->upDownTimer) >= (Curtain_GetMoveTime() * 1000);
}

/**
 * @brief Resets the movement state of a curtain.
 * @param cur Pointer to the Curtain structure.
 */
void Curtain_Reset(Curtain* const cur)
{
    cur->upDown = CURTAIN_STOP;
    cur->upDown_old = CURTAIN_STOP;
    cur->upDownTimer = 0;
}

/**
 * @brief Sets default values for a single curtain.
 * @param cur Pointer to the Curtain structure.
 */
void Curtains_SetDefault(void)
{
    // Obriši cijelu EEPROM strukturu
    memset(&curtains_eeprom_data, 0, sizeof(Curtains_EepromData_t));
    // Postavi logicku default vrijednost za vrijeme kretanja
    curtains_eeprom_data.upDownDurationSeconds = 15; // 15 sekundi
}

/**
 * @brief Updates external curtain state.
 * @param cur Pointer to the Curtain structure.
 * @param val New value for curtain state.
 */
void Curtain_Update_External(Curtain* cur, uint8_t val)
{
    cur->upDown_old = val;
    cur->upDown = val;
    Curtain_RestartTimer(cur);
    cur->external_cmd = 1;// NOVO: Postavi flag da je komanda stigla sa busa
}

/* Private Helper Functions for Curtain_Service ------------------------------*/

/**
 * @brief Handles the movement logic for a single curtain.
 * @param cur Pointer to the Curtain structure.
 */
static void HandleCurtainMovement(Curtain* const cur)
{
    if(Curtain_hasMoveTimeExpired(cur))
    {
        Curtain_Stop(cur);
    }
}
/**
 * @brief Obrada logike kretanja roletni na osnovu željenog smjera.
 *
 * Ova funkcija se poziva iz GUI handlera i sadrži svu poslovnu logiku
 * za upravljanje stanjem roletne, bilo da se radi o pojedinacnoj roletni
 * ili grupi, te šalje odgovarajuce komande.
 *
 * @param direction Željeni smjer kretanja (CURTAIN_UP, CURTAIN_DOWN, CURTAIN_STOP).
 */
void Curtain_HandleTouchLogic(const uint8_t direction) {
    if(Curtain_areAllSelected()) {
        Curtains_MoveSignal(direction);
    } else {
        Curtain* cur = Curtain_GetByLogicalIndex(curtain_selected);
        if (cur) {
            if (Curtain_isMoving(cur) && (Curtain_getNewDirection(cur) == direction)) {
                Curtain_Stop(cur);
            } else if (Curtain_isMoving(cur) && (Curtain_getNewDirection(cur) != direction)) {
                Curtain_Move(cur, direction);
            } else {
                Curtain_Move(cur, direction);
            }
        }
    }
}
/**
 * @brief Handles direction changes for a single curtain and sends Modbus commands.
 * @param cur Pointer to the Curtain structure.
 */
static void HandleCurtainDirectionChange(Curtain* const cur)
{
    if(Curtain_hasDirectionChanged(cur))
    {
        if(Curtain_hasRelays(cur)) // Prvo provjeri da li je roletna uopšte konfigurisana
        {
            uint8_t sendDataBuff[3] = {0};
            uint16_t relay = 0;

            // Odredi relej i popuni prva dva bajta bafera, jer su ista za oba protokola
            if (Curtain_isNewDirectionUp(cur)) {
                relay = Curtain_GetRelayUp(cur);
            } else if (Curtain_isNewDirectionDown(cur)) {
                relay = Curtain_GetRelayDown(cur);
            } else if (Curtain_isMovingUp(cur)) { // Zaustavljanje, provjeri prethodni smjer
                relay = Curtain_GetRelayUp(cur);
            } else if (Curtain_isMovingDown(cur)) {
                relay = Curtain_GetRelayDown(cur);
            }

            // Zajednicki dio koda za smještanje adrese u bafer
            sendDataBuff[0] = (relay >> 8) & 0xFF;
            sendDataBuff[1] = relay & 0xFF;

            // Logika se sada odnosi samo na treci bajt (komandu)
            if(Curtain_GetRelayUp(cur) == Curtain_GetRelayDown(cur))
            {
                // SLUCAJ 1: Jalousie protokol
                sendDataBuff[2] = Curtain_isNewDirectionUp(cur) ? CURTAIN_UP : (Curtain_isNewDirectionDown(cur) ? CURTAIN_DOWN : CURTAIN_STOP);
                AddCommand(&curtainQueue, JALOUSIE_SET, sendDataBuff, 3);
            }
            else
            {
                // SLUCAJ 2: Binarni protokol
                if (Curtain_isNewDirectionUp(cur) || Curtain_isNewDirectionDown(cur)) {
                    sendDataBuff[2] = BINARY_ON;
                    AddCommand(&binaryQueue, BINARY_SET, sendDataBuff, 3);
                } else { // CURTAIN_STOP
                    sendDataBuff[2] = BINARY_OFF;
                    AddCommand(&binaryQueue, BINARY_SET, sendDataBuff, 3);
                }
            }
        }

        if(screen == SCREEN_CURTAINS) shouldDrawScreen = 1;

        if(Curtain_isNewDirectionStop(cur)) Curtain_Reset(cur);
        else Curtain_DirectionEqualize(cur);
    }
}

/* Public Service Function ---------------------------------------------------*/

/**
 * @brief Main service function for curtains, handling movement and state changes.
 * @note This function should be called periodically in the main loop.
 */
void Curtain_Service(void)
{
    for(uint8_t i = 0; i < CURTAINS_SIZE; i++)
    {
        Curtain* const cur = curtains + i;

        if(!Curtain_hasRelays(cur)) continue; // Skip if curtain is not configured

        // PROMJENA: Timer se uvijek mora provjeravati
        // HandleCurtainMovement() se uvijek poziva, bez obzira da li je komanda eksterna ili ne
        HandleCurtainMovement(cur);

        // Ali slanje komandi se radi samo ako displej ima kontrolu
        if (cur->external_cmd == 0) {
            HandleCurtainDirectionChange(cur);
        }
    }
}


