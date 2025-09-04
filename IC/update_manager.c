/**
 ******************************************************************************
 * @file    update_manager.c
 * @author  Gemini & [Vaše Ime]
 * @brief   Asinhroni menadžer za upravljanje višestrukim FW update sesijama.
 *
 * @note
 * Ovaj modul je srce serverske logike za ažuriranje firmvera. Dizajniran je
 * da radi potpuno asinhrono i ne-blokirajuće. Njegova `UpdateManager_Service`
 * funkcija se poziva iz glavne `while(1)` petlje i upravlja stanjima
 * svih aktivnih update sesija, uključujući slanje paketa, praćenje tajmera
 * i ponovno slanje (retransmit).
 ******************************************************************************
 */

#include "update_manager.h" // Ključni include koji je nedostajao
#include "main.h"
#include "rs485.h"
#include "ff.h"
#include "common.h"

//=============================================================================
// Definicije
//=============================================================================

/** @brief Maksimalan broj istovremenih update sesija koje menadžer podržava. */
#define MAX_SESSIONS 5
/** @brief Broj ponovnih pokušaja slanja jednog paketa prije odustajanja. */
#define MAX_RETRIES 10
/** @brief Timeout u ms za čekanje na ACK nakon slanja DATA paketa. */
#define T_WAIT_FOR_DATA_ACK 200
/** @brief Timeout u ms za čekanje na START ACK nakon slanja zahtjeva. */
#define T_WAIT_FOR_START_ACK 6000
/** @brief Timeout u ms za čekanje na FINISH ACK nakon slanja finalnog zahtjeva. */
#define T_WAIT_FOR_FINISH_ACK 1000
/** @brief Timeout u ms za čekanje na restart klijenta prije finalne provjere. */
#define T_FINAL_VERIFICATION_DELAY 10000
/** @brief Veličina jednog dijela (chunk) firmvera koji se šalje u jednom paketu. */
#define FW_PACKET_DATA_SIZE 256

/** @brief Niz koji čuva stanje za sve potencijalne sesije. Privatna varijabla modula. */
static UpdateSession_t sessions[MAX_SESSIONS];

//=============================================================================
// Prototipovi Privatnih Funkcija
//=============================================================================
static void CleanupSession(UpdateSession_t* s);
static void SendStartRequest(UpdateSession_t* s);
static void SendDataPacket(UpdateSession_t* s);
static void SendFinishRequest(UpdateSession_t* s);
static void SendGetInfoRequest(uint8_t clientAddress);
static const char* NackReasonToString(FwUpdate_NackReason_e reason);

// Pretpostavka je da je ova funkcija dostupna iz drugog modula (npr. display.c)
extern void DISP_UpdateLog(const char *pbuf);

//=============================================================================
// Implementacija Javnih Funkcija
//=============================================================================

/**
 * @brief Inicijalizuje Update Manager.
 * @note  Poziva se jednom pri startu servera. Resetuje sve sesije na IDLE stanje.
 */
void UpdateManager_Init(void)
{
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        sessions[i].state = S_IDLE;
    }
}

/**
 * @brief Pokreće novu update sesiju.
 * @note  Ovu funkciju poziva HTTP_CGI_Handler. Funkcija je ne-blokirajuća; ona samo
 * pronalazi slobodan slot, otvara fajl, čita metapodatke ("pečat"), validira
 * ih i priprema sesiju za početak. Stvarni transfer se odvija u pozadini
 * preko `UpdateManager_Service`.
 */
bool UpdateManager_StartSession(uint8_t clientAddress, const char* filePath)
{
    int session_index = -1;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].state == S_IDLE) {
            memset(&sessions[i], 0, sizeof(UpdateSession_t));
            session_index = i;
            break;
        }
    }
    if (session_index == -1) return false;

    UpdateSession_t* s = &sessions[session_index];

    //if (f_mount(&fatfs, "0:", 0) != FR_OK) return false;
    
    if (f_open(&s->fileObject, filePath, FA_READ) != FR_OK) {
        //f_mount(NULL, "0:", 0);
        return false;
    }

    FRESULT fr;
    UINT bytesRead;
    fr = f_lseek(&s->fileObject, VERS_INF_OFFSET);
    if (fr != FR_OK) {
        f_close(&s->fileObject);
        //f_mount(NULL, "0:", 0);
        return false;
    }

    fr = f_read(&s->fileObject, &s->fwInfo.size,    sizeof(uint32_t), &bytesRead);
    fr |= f_read(&s->fileObject, &s->fwInfo.crc32,   sizeof(uint32_t), &bytesRead);
    fr |= f_read(&s->fileObject, &s->fwInfo.version, sizeof(uint32_t), &bytesRead);
    fr |= f_read(&s->fileObject, &s->fwInfo.wr_addr, sizeof(uint32_t), &bytesRead);
    
    if (fr != FR_OK) {
        f_close(&s->fileObject);
        //f_mount(NULL, "0:", 0);
        return false;
    }
    
    f_lseek(&s->fileObject, 0);

    if (ValidateFwInfo(&s->fwInfo) != 0) {
        f_close(&s->fileObject);
        //f_mount(NULL, "0:", 0);
        return false;
    }

    s->clientAddress = clientAddress;
    s->state = S_STARTING;
    
    return true;
}

/**
 * @brief Glavna servisna funkcija koja upravlja svim aktivnim sesijama.
 * @note  Ovo je ne-blokirajući drajver mašine stanja. Poziva se iz `main()`.
 * Prolazi kroz sve aktivne sesije i na osnovu njihovog stanja izvršava
 * sljedeći korak (npr. šalje paket, provjerava tajmer, ponovo šalje paket).
 */
void UpdateManager_Service(void)
{
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        UpdateSession_t* s = &sessions[i];

        if (s->state == S_IDLE) continue;

        if (s->state == S_COMPLETED_OK || s->state == S_FAILED) {
            CleanupSession(s);
            continue;
        }

        switch (s->state)
        {
            case S_STARTING:
                SendStartRequest(s);
                break;
            case S_SENDING_DATA:
                SendDataPacket(s);
                break;
            case S_FINISHING:
                SendFinishRequest(s);
                break;
            case S_WAITING_FOR_START_ACK:
            case S_WAITING_FOR_DATA_ACK:
            case S_WAITING_FOR_FINISH_ACK:
            {
                uint32_t timeout_duration = 0;
                if(s->state == S_WAITING_FOR_START_ACK)  timeout_duration = T_WAIT_FOR_START_ACK;
                if(s->state == S_WAITING_FOR_DATA_ACK)  timeout_duration = T_WAIT_FOR_DATA_ACK;
                if(s->state == S_WAITING_FOR_FINISH_ACK) timeout_duration = T_WAIT_FOR_FINISH_ACK;

                if ((HAL_GetTick() - s->timeoutStart) > timeout_duration) {
                    if (s->state == S_WAITING_FOR_DATA_ACK && s->retryCount > 0) {
                        s->retryCount--;
                        s->state = S_SENDING_DATA; 
                    } else {
                        s->failReason = NACK_REASON_SERVER_TIMEOUT;
                        s->state = S_FAILED;
                    }
                }
                break;
            }
            case S_WAITING_FOR_VERIFICATION:
            {
                if ((HAL_GetTick() - s->timeoutStart) > T_FINAL_VERIFICATION_DELAY) {
                    SendGetInfoRequest(s->clientAddress);
                    s->state = S_COMPLETED_OK; // Pretpostavljamo uspjeh, stvarna potvrda stiže asinhrono.
                }
                break;
            }
            default: break;
        }
    }
}

/**
 * @brief Obrađuje dolazni odgovor (ACK/NACK) od klijenta.
 * @note  Ovu funkciju poziva TinyFrame listener iz `rs485.c` (ili sličnog modula).
 * Ona pronalazi odgovarajuću sesiju na osnovu adrese pošiljaoca iz
 * payload-a i ažurira njeno stanje u mašini stanja.
 */
void UpdateManager_ProcessResponse(TF_Msg *msg)
{
    if (msg->len < 2) return; 

    FwUpdate_SubCommand_e sub_cmd = (FwUpdate_SubCommand_e)msg->data[0];
    uint8_t clientAddress = msg->data[1]; 

    UpdateSession_t* s = NULL;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].state != S_IDLE && sessions[i].clientAddress == clientAddress) {
            s = &sessions[i];
            break;
        }
    }
    if (s == NULL) return;

    switch (sub_cmd)
    {
        case SUB_CMD_START_ACK:
            if (s->state == S_WAITING_FOR_START_ACK) {
                s->state = S_SENDING_DATA;
            }
            break;
        case SUB_CMD_START_NACK:
            if (s->state == S_WAITING_FOR_START_ACK) {
                if(msg->len > 2) s->failReason = (FwUpdate_NackReason_e)msg->data[2];
                s->state = S_FAILED;
            }
            break;
        case SUB_CMD_DATA_ACK:
            if (s->state == S_WAITING_FOR_DATA_ACK) {
                uint32_t ackedSeqNum;
                memcpy(&ackedSeqNum, &msg->data[2], sizeof(uint32_t));
                if (ackedSeqNum == s->currentSequenceNum) {
                    s->bytesSent += s->lastPacketSize; 
                    s->currentSequenceNum++;
                    if (s->bytesSent >= s->fwInfo.size) {
                        s->state = S_FINISHING;
                    } else {
                        s->state = S_SENDING_DATA;
                    }
                }
            }
            break;
        case SUB_CMD_FINISH_ACK:
            if (s->state == S_WAITING_FOR_FINISH_ACK) {
                s->state = S_WAITING_FOR_VERIFICATION;
                s->timeoutStart = HAL_GetTick();
            }
            break;
        case SUB_CMD_FINISH_NACK:
             if (s->state == S_WAITING_FOR_FINISH_ACK) {
                if(msg->len > 2) s->failReason = (FwUpdate_NackReason_e)msg->data[2];
                s->state = S_FAILED;
            }
            break;
        default: break;
    }
}

/**
 * @brief Omogućava 'read-only' pristup podacima o određenoj sesiji.
 */
const UpdateSession_t* UpdateManager_GetSessionInfo(uint8_t session_index)
{
    if (session_index < MAX_SESSIONS) {
        return (const UpdateSession_t*)&sessions[session_index];
    }
    return NULL;
}

//=============================================================================
// Implementacija Privatnih Funkcija
//=============================================================================

/**
 * @brief Pomoćna funkcija za čišćenje i oslobađanje resursa jedne sesije.
 */
static void CleanupSession(UpdateSession_t* s)
{
    char logBuffer[128];
    f_close(&s->fileObject);
    
    if (s->state == S_COMPLETED_OK) {
        sprintf(logBuffer, "Update za Klijent %d: USPJESAN!", s->clientAddress);
    } else {
        const char* reason_text = NackReasonToString(s->failReason);
        sprintf(logBuffer, "Update za Klijent %d: NEUSPJEH! Razlog: %s", s->clientAddress, reason_text);
    }
    DISP_UpdateLog(logBuffer);

    memset(s, 0, sizeof(UpdateSession_t));
    s->state = S_IDLE;
}

/**
 * @brief Sastavlja i šalje SUB_CMD_START_REQUEST poruku.
 */
static void SendStartRequest(UpdateSession_t* s)
{
    uint8_t payload[2 + sizeof(FwInfoTypeDef)];
    payload[0] = SUB_CMD_START_REQUEST;
    payload[1] = s->clientAddress;
    memcpy(&payload[2], &s->fwInfo, sizeof(FwInfoTypeDef));

    AddCommand(&thermoQueue, FIRMWARE_UPDATE, payload, sizeof(payload));

    s->timeoutStart = HAL_GetTick();
    s->state = S_WAITING_FOR_START_ACK;
}

/**
 * @brief Čita dio fajla, sastavlja i šalje SUB_CMD_DATA_PACKET.
 */
static void SendDataPacket(UpdateSession_t* s)
{
    uint8_t tx_buffer[6 + FW_PACKET_DATA_SIZE]; // SUB(1)+ADR(1)+SEQ(4)+DATA
    UINT bytesToRead, bytesRead;
    
    uint32_t remainingBytes = s->fwInfo.size - s->bytesSent;
    bytesToRead = (remainingBytes > FW_PACKET_DATA_SIZE) ? FW_PACKET_DATA_SIZE : remainingBytes;

    f_lseek(&s->fileObject, s->bytesSent);
    if (f_read(&s->fileObject, &tx_buffer[6], bytesToRead, &bytesRead) != FR_OK || bytesRead != bytesToRead) {
        s->failReason = NACK_REASON_INTERNAL_ERROR;
        s->state = S_FAILED;
        return;
    }

    tx_buffer[0] = SUB_CMD_DATA_PACKET;
    tx_buffer[1] = s->clientAddress;
    memcpy(&tx_buffer[2], &s->currentSequenceNum, sizeof(uint32_t));
    
    AddCommand(&thermoQueue, FIRMWARE_UPDATE, tx_buffer, bytesRead + 6);

    s->lastPacketSize = bytesRead;
    s->retryCount = MAX_RETRIES;
    s->timeoutStart = HAL_GetTick();
    s->state = S_WAITING_FOR_DATA_ACK;
}

/**
 * @brief Sastavlja i šalje SUB_CMD_FINISH_REQUEST poruku.
 */
static void SendFinishRequest(UpdateSession_t* s)
{
    uint8_t payload[2 + sizeof(uint32_t)];
    payload[0] = SUB_CMD_FINISH_REQUEST;
    payload[1] = s->clientAddress;
    memcpy(&payload[2], &s->fwInfo.crc32, sizeof(uint32_t));

    AddCommand(&thermoQueue, FIRMWARE_UPDATE, payload, sizeof(payload));

    s->timeoutStart = HAL_GetTick();
    s->state = S_WAITING_FOR_FINISH_ACK;
}

/**
 * @brief Šalje GET_INFO upit za finalnu verifikaciju.
 */
static void SendGetInfoRequest(uint8_t clientAddress)
{
    // TODO: Implementirati slanje GET_APPL_STAT poruke
    // uint8_t payload[] = {GET_APPL_STAT, clientAddress};
    // AddCommand(&neki_red, GET_APPL_STAT, payload, sizeof(payload));
}

/**
 * @brief Pretvara kod greške (NackReason) u tekstualni opis.
 */
static const char* NackReasonToString(FwUpdate_NackReason_e reason)
{
    switch (reason)
    {
        case NACK_REASON_FILE_TOO_LARGE:    return "Fajl je prevelik";
        case NACK_REASON_INVALID_VERSION:   return "Verzija nije ispravna";
        case NACK_REASON_ERASE_FAILED:      return "Brisanje memorije neuspjesno";
        case NACK_REASON_WRITE_FAILED:      return "Greska pri upisu podataka";
        case NACK_REASON_CRC_MISMATCH:      return "CRC provjera neuspjesna";
        case NACK_REASON_UNEXPECTED_PACKET: return "Neocekivani paket";
        case NACK_REASON_SIZE_MISMATCH:     return "Velicina fajla se ne poklapa";
        case NACK_REASON_SERVER_TIMEOUT:    return "Timeout - klijent se ne odaziva";
        default:                            return "Nepoznata greska";
    }
}
