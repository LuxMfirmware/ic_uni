/**
 ******************************************************************************
 * @file    firmware_update_agent.c
 * @author  Gemini
 * @brief   Implementacija "Firmware Update Agent" modula.
 *
 * @note
 * Sadrži kompletnu implementaciju mašine stanja (State Machine) za pouzdan
 * prijem i validaciju firmvera preko RS485 protokola. Koristi interne tajmere
 * za detekciju grešaka i prekida u komunikaciji.
 ******************************************************************************
 */

#include "firmware_update_agent.h"
#include "main.h"
#include "common.h"
#include "rs485.h" // Potrebno za slanje ACK/NACK odgovora
#include "stm32746g_qspi.h"
#include "stm32746g_eeprom.h"

//=============================================================================
// Definicije Vremenskih Ograničenja (Timeouts) i Parametara
//=============================================================================

/**
 * @brief Vrijeme (u ms) koje klijent čeka na sljedeći paket od servera
 * prije nego što samostalno prekine proces ažuriranja.
 */
#define T_INACTIVITY_TIMEOUT 5000 // 5 sekundi

/**
 * @brief Adresa u EEPROM-u gdje se upisuje marker za bootloader.
 * @note  Bootloader pri startu provjerava ovu lokaciju.
 */
#define EE_BOOTLOADER_MARKER_ADDR 0x10 // Primjer, odabrati slobodnu adresu

//=============================================================================
// Definicije za Mašinu Stanja (State Machine)
//=============================================================================

/**
 * @brief Enumeracija koja definiše sve moguće sub-komande unutar FIRMWARE_UPDATE poruke.
 */
typedef enum {
    SUB_CMD_START_REQUEST   = 0x01,
    SUB_CMD_START_ACK       = 0x02,
    SUB_CMD_START_NACK      = 0x03,
    SUB_CMD_DATA_PACKET     = 0x10,
    SUB_CMD_DATA_ACK        = 0x11,
    SUB_CMD_FINISH_REQUEST  = 0x20,
    SUB_CMD_FINISH_ACK      = 0x21,
    SUB_CMD_FINISH_NACK     = 0x22,
} FwUpdate_SubCommand_e;


/**
 * @brief Enumeracija koja definiše moguće razloge za odbijanje (NACK) update-a.
 */
typedef enum {
    NACK_REASON_NONE = 0,
    NACK_REASON_FILE_TOO_LARGE,
    NACK_REASON_INVALID_VERSION,
    NACK_REASON_ERASE_FAILED,
    NACK_REASON_WRITE_FAILED,
    NACK_REASON_CRC_MISMATCH,
    NACK_REASON_UNEXPECTED_PACKET,
    NACK_REASON_SIZE_MISMATCH
} FwUpdate_NackReason_e;

/**
 * @brief Enumeracija koja definiše sva moguća stanja u kojima se Agent može naći.
 */
typedef enum
{
    FSM_IDLE,           /**< Agent je neaktivan i čeka komandu za početak. */
    FSM_RECEIVING,      /**< Agent je prihvatio update, obrisao memoriju i prima pakete. */
    FSM_ERROR           /**< Desila se greška, Agent je u stanju greške do reseta. */
} FSM_State_e;


/**
 * @brief Struktura koja čuva sve runtime podatke potrebne za rad Agenta.
 */
typedef struct
{
    FSM_State_e     currentState;           /**< Trenutno stanje mašine. */
    FwInfoTypeDef   fwInfo;                 /**< Metapodaci o firmveru koji se prima. */
    uint32_t        expectedSequenceNum;    /**< Redni broj sljedećeg paketa koji očekujemo. */
    uint32_t        currentWriteAddr;       /**< Trenutna adresa za upis u QSPI. */
    uint32_t        bytesReceived;          /**< Ukupan broj primljenih bajtova. */
    uint32_t        inactivityTimerStart;   /**< Vrijeme kada je primljen posljednji paket. */
} FwUpdateAgent_t;

/**
 * @brief Statička, privatna instanca Agenta. Jedina u sistemu.
 */
static FwUpdateAgent_t agent;


//=============================================================================
// Prototipovi Privatnih Funkcija (Handleri za Stanja)
//=============================================================================
static void HandleMessage_Idle(TinyFrame *tf, TF_Msg *msg);
static void HandleMessage_Receiving(TinyFrame *tf, TF_Msg *msg);
static uint32_t QSPI_CalculateCRC32(uint32_t address, uint32_t size);


//=============================================================================
// Implementacija Javnih Funkcija (API)
//=============================================================================

/**
 * @brief Inicijalizuje Agent.
 */
void FwUpdateAgent_Init(void)
{
    agent.currentState = FSM_IDLE;
    agent.expectedSequenceNum = 0;
    agent.bytesReceived = 0;
    agent.inactivityTimerStart = 0;
}

/**
 * @brief Servisna funkcija za upravljanje tajmerima.
 */
void FwUpdateAgent_Service(void)
{
    // Provjera tajmera za neaktivnost se vrši samo ako smo u stanju prijema.
    if (agent.currentState == FSM_RECEIVING)
    {
        if ((HAL_GetTick() - agent.inactivityTimerStart) > T_INACTIVITY_TIMEOUT)
        {
            // Server predugo nije poslao paket. Prekidamo proces.
            agent.currentState = FSM_ERROR;
            // Ovdje se može dodati logika za logovanje greške.
        }
    }
}

/**
 * @brief Glavni dispečer poruka koji poziva odgovarajući handler.
 */
void FwUpdateAgent_ProcessMessage(TinyFrame *tf, TF_Msg *msg)
{
    // Adresa je uvijek na drugom bajtu, bez obzira na sub-komandu.
    uint8_t target_address = msg->data[1];
    
    // Ako poruka nije za nas, ignorišemo je, osim ako je START_REQUEST
    // koji je namijenjen svima da bi prikazali poruku na ekranu.
    if (msg->data[0] != SUB_CMD_START_REQUEST && target_address != tfifa)
    {
        return;
    }

    switch (agent.currentState)
    {
        case FSM_IDLE:      HandleMessage_Idle(tf, msg);      break;
        case FSM_RECEIVING: HandleMessage_Receiving(tf, msg); break;
        default: break;
    }
}


/**
 * @brief Provjerava da li je Agent aktivan.
 */
bool FwUpdateAgent_IsActive(void)
{
    return (agent.currentState != FSM_IDLE);
}


//=============================================================================
// Implementacija Privatnih Funkcija (Logika Mašine Stanja)
//=============================================================================

/**
 * @brief Handler za obradu poruka kada je Agent u IDLE stanju.
 *
 * @note
 * U ovom stanju, jedina relevantna poruka je `SUB_CMD_START_REQUEST`. Ova funkcija
 * djeluje kao "čuvar kapije" za proces ažuriranja. Njen zadatak je da:
 * 1. Provjeri da li je poruka validan zahtjev za početak ažuriranja.
 * 2. Provjeri da li je zahtjev namijenjen baš ovom uređaju.
 * 3. Izvrši sve neophodne pred-provjere (veličina fajla, verzija) kako bi se
 * proces otkazao što je ranije moguće ako uslovi nisu ispunjeni ("fail fast").
 * 4. Ako su svi uslovi ispunjeni, priprema QSPI memoriju brisanjem.
 * 5. Šalje potvrdan (`ACK`) ili negativan (`NACK`) odgovor serveru.
 * 6. Ako je odgovor potvrdan, prebacuje mašinu stanja u `FSM_RECEIVING`.
 *
 * @param tf    Pokazivač na TinyFrame instancu, potreban za slanje odgovora.
 * @param msg   Pokazivač na primljenu TF_Msg poruku.
 */
static void HandleMessage_Idle(TinyFrame *tf, TF_Msg *msg)
{
    // KORAK 1: Provjera da li je poruka uopšte relevantna za ovo stanje.
    // Očekujemo isključivo `SUB_CMD_START_REQUEST`.
    if (msg->data[0] != SUB_CMD_START_REQUEST)
    {
        return; // Ignoriši sve ostale poruke.
    }

    // KORAK 2: Provjera da li je poruka namijenjena ovom uređaju.
    // `tfifa` je globalna varijabla koja čuva našu RS485 adresu.
    uint8_t target_address = msg->data[1];
    if (target_address != tfifa)
    {
        return; // Poruka nije za nas, ignoriši.
    }

    // KORAK 3: Parsiranje metapodataka o firmveru iz payload-a poruke.
    memcpy(&agent.fwInfo, &msg->data[2], sizeof(FwInfoTypeDef));

    // --- FAZA PRE-VALIDACIJE ---
    
    // KORAK 4: Provjera da li je veličina dolazećeg firmvera prihvatljiva.
    if ((agent.fwInfo.size > RT_APPL_SIZE) || (agent.fwInfo.size == 0))
    {
        uint8_t nack_response[] = {SUB_CMD_START_NACK, tfifa, NACK_REASON_FILE_TOO_LARGE};
        TF_SendSimple(tf, FIRMWARE_UPDATE, nack_response, sizeof(nack_response));
        return;
    }

    // KORAK 5: Provjera da li je firmver noviji i odgovarajućeg tipa.
    FwInfoTypeDef currentFwInfo;
    currentFwInfo.ld_addr = RT_APPL_ADDR; // Adresa trenutno aktivnog firmvera
    GetFwInfo(&currentFwInfo);            // Učitaj info o trenutnom firmveru
    
    if (IsNewFwUpdate(&currentFwInfo, &agent.fwInfo) != 0)
    {
        uint8_t nack_response[] = {SUB_CMD_START_NACK, tfifa, NACK_REASON_INVALID_VERSION};
        TF_SendSimple(tf, FIRMWARE_UPDATE, nack_response, sizeof(nack_response));
        return;
    }

    // --- FAZA PRIPREME MEMORIJE ---
    
    // KORAK 6: Brisanje "staging" particije na QSPI fleš memoriji.
    // Koristimo obrazac koji ste potvrdili kao ispravan.
    MX_QSPI_Init();
    if (QSPI_Erase(agent.fwInfo.wr_addr, agent.fwInfo.wr_addr + agent.fwInfo.size) != QSPI_OK)
    {
        // Ako brisanje ne uspije, vraćamo QSPI u normalan mod i šaljemo grešku.
        MX_QSPI_Init();
        QSPI_MemMapMode();
        
        uint8_t nack_response[] = {SUB_CMD_START_NACK, tfifa, NACK_REASON_ERASE_FAILED};
        TF_SendSimple(tf, FIRMWARE_UPDATE, nack_response, sizeof(nack_response));
        
        // Ulazimo u stanje greške jer je memorija možda u nekonzistentnom stanju.
        agent.currentState = FSM_ERROR;
        return;
    }
    // Obavezno vraćanje QSPI u Memory-Mapped mod nakon operacije.
    MX_QSPI_Init();
    QSPI_MemMapMode();

    // --- FAZA POTVRDE I PRELASKA U NOVO STANJE ---
    
    // KORAK 7: Inicijalizacija varijabli za praćenje transfera.
    agent.expectedSequenceNum = 0;
    agent.bytesReceived = 0;
    agent.currentWriteAddr = agent.fwInfo.wr_addr;
    agent.inactivityTimerStart = HAL_GetTick(); // Pokreni sigurnosni tajmer

    // KORAK 8: Slanje potvrdne poruke (ACK) serveru.
    uint8_t ack_response[] = {SUB_CMD_START_ACK, tfifa};
    TF_SendSimple(tf, FIRMWARE_UPDATE, ack_response, sizeof(ack_response));
    
    // KORAK 9: Prelazak u sljedeće stanje mašine.
    agent.currentState = FSM_RECEIVING;
}

/**
 * @brief Handler za obradu poruka kada je Agent u RECEIVING stanju.
 * @note  U ovom stanju, očekuju se UPDATE_DATA_PACKET i UPDATE_FINISH_REQUEST.
 */
static void HandleMessage_Receiving(TinyFrame *tf, TF_Msg *msg)
{
    agent.inactivityTimerStart = HAL_GetTick();

    switch (msg->data[0])
    {
        case SUB_CMD_DATA_PACKET:
        {
            uint32_t receivedSeqNum;
            memcpy(&receivedSeqNum, &msg->data[2], sizeof(uint32_t));

            if (receivedSeqNum == agent.expectedSequenceNum) {
                uint8_t* data_payload = (uint8_t*)&msg->data[6];
                uint16_t data_len = msg->len - 6;
                
                MX_QSPI_Init();
                if (QSPI_Write(data_payload, agent.currentWriteAddr, data_len) == QSPI_OK) {
                    agent.bytesReceived += data_len;
                    agent.currentWriteAddr += data_len;
                    agent.expectedSequenceNum++;
                    
                    uint8_t ack_payload[6];
                    ack_payload[0] = SUB_CMD_DATA_ACK;
                    ack_payload[1] = tfifa;
                    memcpy(&ack_payload[2], &receivedSeqNum, sizeof(uint32_t));
                    TF_SendSimple(tf, FIRMWARE_UPDATE, ack_payload, sizeof(ack_payload));
                } else {
                    agent.currentState = FSM_ERROR;
                }
                MX_QSPI_Init(); QSPI_MemMapMode();
            } else if (receivedSeqNum < agent.expectedSequenceNum) {
                uint8_t ack_payload[6];
                ack_payload[0] = SUB_CMD_DATA_ACK;
                ack_payload[1] = tfifa;
                memcpy(&ack_payload[2], &receivedSeqNum, sizeof(uint32_t));
                TF_SendSimple(tf, FIRMWARE_UPDATE, ack_payload, sizeof(ack_payload));
            }
            break;
        }

         case SUB_CMD_FINISH_REQUEST:
        {
            // Provjeravamo da li je primljen tačan broj bajtova.
            if (agent.bytesReceived != agent.fwInfo.size) {
                 agent.currentState = FSM_ERROR;
                 uint8_t nack_response[] = {SUB_CMD_FINISH_NACK, tfifa, NACK_REASON_SIZE_MISMATCH}; // Potrebno dodati novi NACK razlog
                 TF_SendSimple(tf, FIRMWARE_UPDATE, nack_response, sizeof(nack_response));
                 break;
            }

            // Kreiramo novu FwInfoTypeDef strukturu za validaciju.
            FwInfoTypeDef receivedFwInfo;
            ResetFwInfo(&receivedFwInfo); // Sigurnosno je postavimo na nule.
            
            // Postavljamo `ld_addr` na adresu gdje smo upravo upisali novi firmver.
            receivedFwInfo.ld_addr = agent.fwInfo.wr_addr;

            // Pozivamo Vašu postojeću, "bulletproof" funkciju za validaciju!
            uint8_t validation_result = GetFwInfo(&receivedFwInfo);

            if (validation_result == 0) // GetFwInfo vraća 0 u slučaju uspjeha
            {
                // SVE JE U REDU! Fajl na QSPI je validan.
                
                // 1. Upisujemo marker za bootloader. Koristimo `receivedFwInfo` jer je svježe validirana.
                EE_WriteBuffer((uint8_t*)&receivedFwInfo, EE_BOOTLOADER_MARKER_ADDR, sizeof(FwInfoTypeDef));

                // 2. Šaljemo finalni ACK serveru.
                uint8_t ack_response[] = {SUB_CMD_FINISH_ACK, tfifa};
                TF_SendSimple(tf, FIRMWARE_UPDATE, ack_response, sizeof(ack_response));

                // 3. Čekamo kratko da se poruka pošalje i onda restart.
                HAL_Delay(100); 
                SYSRestart();
            }
            else
            {
                // `GetFwInfo` je vratio grešku. CRC provjera nije uspjela.
                agent.currentState = FSM_ERROR;
                uint8_t nack_response[] = {SUB_CMD_FINISH_NACK, tfifa, NACK_REASON_CRC_MISMATCH};
                TF_SendSimple(tf, FIRMWARE_UPDATE, nack_response, sizeof(nack_response));
            }
            break;
        }
        default: break;
    }
}

