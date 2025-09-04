/**
 ******************************************************************************
 * @file    update_manager.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni interfejs za asinhroni menadžer za FW update sesije.
 *
 * @note
 * Ovaj header fajl deklariše javne strukture i funkcije koje su dostupne
 * ostatku sistema (HTTP server, glavna petlja, displej modul) za upravljanje
 * procesom ažuriranja firmvera na udaljenim klijentima.
 ******************************************************************************
 */

#ifndef __UPDATE_MANAGER_H__
#define __UPDATE_MANAGER_H__

#include <stdint.h>
#include <stdbool.h>
#include "TinyFrame.h"      // Potrebno zbog TF_Msg tipa
#include "common.h"         // Potrebno zbog FwInfoTypeDef strukture
#include "ff.h"             // Potrebno zbog FIL (File Object) strukture

//=============================================================================
// JAVNE DEFINICIJE PROTOKOLA (za server i klijenta)
//=============================================================================

/**
 * @brief Enumeracija koja definiše sve moguće sub-komande unutar FIRMWARE_UPDATE poruke.
 * @note  Ova enumeracija je ključna za protokol i mora biti identična na serveru i klijentu.
 */
typedef enum {
    SUB_CMD_START_REQUEST   = 0x01, /**< Server -> Klijent: Zahtjev za početak ažuriranja. Payload sadrži FwInfoTypeDef. */
    SUB_CMD_START_ACK       = 0x02, /**< Klijent -> Server: Potvrda da je zahtjev prihvaćen i da je memorija spremna. */
    SUB_CMD_START_NACK      = 0x03, /**< Klijent -> Server: Odbijanje zahtjeva, payload sadrži razlog. */
    SUB_CMD_DATA_PACKET     = 0x10, /**< Server -> Klijent: Paket koji sadrži dio firmvera. */
    SUB_CMD_DATA_ACK        = 0x11, /**< Klijent -> Server: Potvrda prijema DATA paketa. */
    SUB_CMD_FINISH_REQUEST  = 0x20, /**< Server -> Klijent: Signal da je transfer završen, zahtjev za finalnu validaciju. */
    SUB_CMD_FINISH_ACK      = 0x21, /**< Klijent -> Server: Potvrda da je finalna validacija uspješna i da slijedi restart. */
    SUB_CMD_FINISH_NACK     = 0x22, /**< Klijent -> Server: Prijava da finalna validacija nije uspjela. */
} FwUpdate_SubCommand_e;

/**
 * @brief Enumeracija koja definiše moguće razloge za odbijanje (NACK) update-a.
 */
typedef enum {
    NACK_REASON_NONE = 0,               /**< Nema greške. */
    NACK_REASON_FILE_TOO_LARGE,         /**< Firmver fajl je veći od raspoložive memorije na klijentu. */
    NACK_REASON_INVALID_VERSION,        /**< Verzija firmvera je ista, starija, ili pogrešnog tipa. */
    NACK_REASON_ERASE_FAILED,           /**< Brisanje QSPI memorije na klijentu nije uspjelo. */
    NACK_REASON_WRITE_FAILED,           /**< Upisivanje podataka u QSPI memoriju nije uspjelo. */
    NACK_REASON_CRC_MISMATCH,           /**< Finalna CRC32 provjera na klijentu nije uspjela. */
    NACK_REASON_UNEXPECTED_PACKET,      /**< Klijent je primio paket koji nije očekivao u trenutnom stanju. */
    NACK_REASON_SIZE_MISMATCH,          /**< Ukupan broj primljenih bajtova se ne poklapa sa očekivanom veličinom. */
    NACK_REASON_SERVER_TIMEOUT,         /**< Interna greška na serveru: klijent se nije odazvao u predviđenom roku. */
} FwUpdate_NackReason_e;

//=============================================================================
// JAVNE ENUMERACIJE I STRUKTURE
//=============================================================================

/**
 * @brief Enumeracija stanja za mašinu stanja svake sesije na serveru.
 * @note  Ova enumeracija je javna kako bi display modul mogao tumačiti
 * stanje sesije i na osnovu njega iscrtati odgovarajući UI element.
 */
typedef enum {
    S_IDLE,                     /**< Slot za sesiju je slobodan. */
    S_STARTING,                 /**< Iniciran, čeka se slanje START_REQUEST. */
    S_WAITING_FOR_START_ACK,    /**< Čeka se ACK na START_REQUEST. */
    S_SENDING_DATA,             /**< Stanje slanja paketa sa podacima. */
    S_WAITING_FOR_DATA_ACK,     /**< Čeka se ACK na poslati DATA paket. */
    S_FINISHING,                /**< Svi podaci poslati, šalje se FINISH_REQUEST. */
    S_WAITING_FOR_FINISH_ACK,   /**< Čeka se ACK na FINISH_REQUEST. */
    S_WAITING_FOR_VERIFICATION, /**< Čeka se da prođe 10s za finalnu provjeru. */
    S_COMPLETED_OK,             /**< Sesija uspješno završena, čeka se čišćenje. */
    S_FAILED                    /**< Sesija neuspješna, čeka se čišćenje. */
} ServerSessionState_e;

/** @brief Struktura koja sadrži sve podatke za jednu update sesiju.
 * @note  Ova struktura je javna kako bi display modul imao "read-only"
 * pristup podacima za iscrtavanje progress barova i statusnih poruka.
 */
typedef struct {
    ServerSessionState_e    state;              /**< Trenutno stanje sesije. */
    uint8_t                 clientAddress;      /**< Adresa klijenta koji se ažurira. */
    FwInfoTypeDef           fwInfo;             /**< Metapodaci o firmveru koji se šalje. */
    FIL                     fileObject;         /**< File handle za .BIN fajl na uSD kartici. */
    uint32_t                bytesSent;          /**< Ukupan broj poslatih bajtova podataka (payload). */
    uint16_t                lastPacketSize;     /**< Veličina podataka u posljednjem poslatom paketu. */
    uint32_t                currentSequenceNum; /**< Sekvencijalni broj paketa koji se trenutno šalje. */
    uint32_t                timeoutStart;       /**< Vrijeme početka tajmera za trenutnu operaciju. */
    uint8_t                 retryCount;         /**< Brojač preostalih pokušaja za trenutni paket. */
    FwUpdate_NackReason_e   failReason;         /**< Razlog neuspjeha sesije (koristi se za dijagnostiku). */
} UpdateSession_t;


//=============================================================================
// JAVNI API - PROTOTIPOVI FUNKCIJA
//=============================================================================

void UpdateManager_Init(void);
bool UpdateManager_StartSession(uint8_t clientAddress, const char* filePath);
void UpdateManager_Service(void);
void UpdateManager_ProcessResponse(TF_Msg *msg);
const UpdateSession_t* UpdateManager_GetSessionInfo(uint8_t session_index);


#endif // __UPDATE_MANAGER_H__
