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

#include "update_manager.h"
#include "main.h"
#include "rs485.h"
#include "ff.h" // Za rad sa fajlovima na uSD kartici

//=============================================================================
// Definicije
//=============================================================================

/** @brief Maksimalan broj istovremenih update sesija koje menadžer podržava. */
#define MAX_SESSIONS 5
/** @brief Broj ponovnih pokušaja slanja jednog paketa prije odustajanja. */
#define MAX_RETRIES 10
/** @brief Timeout u ms za čekanje na ACK nakon slanja DATA paketa. */
#define T_WAIT_FOR_DATA_ACK 100
/** @brief Timeout u ms za čekanje na START ACK nakon slanja zahtjeva. */
#define T_WAIT_FOR_START_ACK 6000
/** @brief Timeout u ms za čekanje na FINISH ACK nakon slanja finalnog zahtjeva. */
#define T_WAIT_FOR_FINISH_ACK 1000
/** @brief Timeout u ms za čekanje na restart klijenta prije finalne provjere. */
#define T_FINAL_VERIFICATION_DELAY 10000

/** @brief Enumeracija stanja za mašinu stanja svake sesije na serveru. */
typedef enum {
    S_IDLE,                     /**< Slot za sesiju je slobodan. */
    S_STARTING,                 /**< Iniciran, čeka se slanje START_REQUEST. */
    S_WAITING_FOR_START_ACK,    /**< Čeka se ACK na START_REQUEST. */
    S_SENDING_DATA,             /**< Stanje slanja paketa sa podacima. */
    S_WAITING_FOR_DATA_ACK,     /**< Čeka se ACK na poslati DATA paket. */
    S_FINISHING,                /**< Svi podaci poslati, šalje se FINISH_REQUEST. */
    S_WAITING_FOR_FINISH_ACK,   /**< Čeka se ACK na FINISH_REQUEST. */
    S_WAITING_FOR_VERIFICATION, /**< Čeka se da prođe 10s za finalnu provjeru. */
    S_COMPLETED_OK,             /**< Sesija uspješno završena. */
    S_FAILED                    /**< Sesija neuspješna. */
} ServerSessionState_e;

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

/** @brief Struktura koja sadrži sve podatke za jednu update sesiju. */
typedef struct {
    ServerSessionState_e state;             /**< Trenutno stanje sesije. */
    uint8_t         clientAddress;          /**< Adresa klijenta koji se ažurira. */
    FwInfoTypeDef   fwInfo;                 /**< Metapodaci o firmveru. */
    FIL             fileObject;             /**< File handle za .BIN fajl na uSD. */
    uint32_t        bytesSent;              /**< Ukupan broj poslatih bajtova. */
    uint32_t        currentSequenceNum;     /**< Sekvencijalni broj paketa koji se trenutno šalje. */
    uint32_t        timeoutStart;           /**< Vrijeme početka tajmera za trenutnu operaciju. */
    uint8_t         retryCount;             /**< Brojač preostalih pokušaja za trenutni paket. */
} UpdateSession_t;

/** @brief Niz koji čuva stanje za sve potencijalne sesije. */
static UpdateSession_t sessions[MAX_SESSIONS];


//=============================================================================
// Implementacija Funkcija
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
 * @note  Ovu funkciju poziva HTTP_CGI_Handler. Funkcija pronalazi slobodan slot,
 * otvara fajl, čita metapodatke i priprema sesiju za početak.
 * @param clientAddress Adresa uređaja koji treba ažurirati.
 * @param filePath      Putanja do .BIN fajla na uSD kartici.
 * @retval bool         `true` ako je sesija uspješno pokrenuta, `false` ako ne.
 */
bool UpdateManager_StartSession(uint8_t clientAddress, const char* filePath)
{
    // Pronađi prvi slobodan slot za novu sesiju
    int session_index = -1;
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        if (sessions[i].state == S_IDLE)
        {
            session_index = i;
            break;
        }
    }
    if (session_index == -1) return false; // Nema slobodnih slotova

    UpdateSession_t* s = &sessions[session_index];
    
    // Otvori fajl i pročitaj FwInfoTypeDef
    // TODO: Implementirati logiku za otvaranje fajla i čitanje pečata...
    // if (f_open(...) != FR_OK) return false;
    // f_read(...) -> s->fwInfo;
    
    // Inicijalizuj sesiju
    s->clientAddress = clientAddress;
    s->bytesSent = 0;
    s->currentSequenceNum = 0;
    
    // Postavi stanje na STARTING. `UpdateManager_Service` će preuzeti odavde.
    s->state = S_STARTING;
    
    return true;
}

/**
 * @brief Glavna servisna funkcija koja upravlja svim aktivnim sesijama.
 * @note  Ovo je ne-blokirajući drajver mašine stanja. Poziva se iz `main()`.
 */
void UpdateManager_Service(void)
{
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        UpdateSession_t* s = &sessions[i];
        if (s->state == S_IDLE || s->state == S_COMPLETED_OK || s->state == S_FAILED)
        {
            continue; // Preskoči neaktivne sesije
        }

        switch (s->state)
        {
            case S_STARTING:
                // TODO: Formiraj i pošalji SUB_CMD_START_REQUEST...
                // s->timeoutStart = HAL_GetTick();
                // s->state = S_WAITING_FOR_START_ACK;
                break;

            case S_WAITING_FOR_DATA_ACK:
                if ((HAL_GetTick() - s->timeoutStart) > T_WAIT_FOR_DATA_ACK)
                {
                    // Tajmer je istekao, probaj ponovo poslati
                    s->retryCount--;
                    if (s->retryCount > 0)
                    {
                        // TODO: Ponovo pošalji isti paket...
                        // s->timeoutStart = HAL_GetTick(); // Restartuj tajmer
                    }
                    else
                    {
                        s->state = S_FAILED; // Potrošeni svi pokušaji
                    }
                }
                break;
            
            // TODO: Implementirati logiku za ostala stanja...
            // S_SENDING_DATA, S_WAITING_FOR_VERIFICATION, itd.
            
            default:
                break;
        }
    }
}

/**
 * @brief Obrađuje dolazni odgovor (ACK/NACK) od klijenta.
 * @note  Ovu funkciju poziva TinyFrame listener iz `rs485.c` svaki put kada stigne
 * poruka tipa `FIRMWARE_UPDATE`. Funkcija pronalazi odgovarajuću sesiju
 * na osnovu adrese pošiljaoca iz payload-a i ažurira njeno stanje.
 *
 * @param msg Primljena TinyFrame poruka.
 */
void UpdateManager_ProcessResponse(TF_Msg *msg)
{
    // Svaka poruka od klijenta mora imati bar 2 bajta (sub-komanda + adresa)
    if (msg->len < 2) return; 

    // Čitamo sub-komandu i adresu klijenta iz payload-a
    FwUpdate_SubCommand_e sub_cmd = (FwUpdate_SubCommand_e)msg->data[0];
    uint8_t clientAddress = msg->data[1]; 

    // Pronađi sesiju koja odgovara adresi `clientAddress`
    UpdateSession_t* s = NULL;
    for (int i = 0; i < MAX_SESSIONS; i++)
    {
        if (sessions[i].state != S_IDLE && sessions[i].clientAddress == clientAddress)
        {
            s = &sessions[i];
            break;
        }
    }
    
    // Ako sesija za ovog klijenta nije aktivna, ignorišemo poruku.
    if (s == NULL) return;

    // Na osnovu sub-komande, odlučujemo šta dalje.
    switch (sub_cmd)
    {
        /**
         * Klijent je potvrdio da je primio START_REQUEST, obrisao memoriju
         * i da je spreman za prijem podataka.
         */
        case SUB_CMD_START_ACK:
            // Ovaj ACK je validan samo ako smo ga očekivali.
            if (s->state == S_WAITING_FOR_START_ACK)
            {
                // Odlično, prelazimo u stanje slanja podataka.
                // UpdateManager_Service će u sljedećem ciklusu poslati prvi paket.
                s->state = S_SENDING_DATA;
            }
            break;

        /**
         * Klijent je odbio zahtjev za ažuriranje. Sesija je neuspješna.
         */
        case SUB_CMD_START_NACK:
            if (s->state == S_WAITING_FOR_START_ACK)
            {
                // Klijent je odbio update, sesija je propala.
                s->state = S_FAILED;
                // Ovdje možemo pročitati i razlog iz msg->data[2] i ispisati na displeju servera.
            }
            break;

        /**
         * Klijent je potvrdio prijem jednog paketa sa podacima.
         */
        case SUB_CMD_DATA_ACK:
            // Ovaj ACK je validan samo ako smo ga očekivali.
            if (s->state == S_WAITING_FOR_DATA_ACK)
            {
                uint32_t ackedSeqNum;
                memcpy(&ackedSeqNum, &msg->data[2], sizeof(uint32_t));

                // Provjeravamo da li se broj potvrđenog paketa poklapa sa onim koji smo poslali.
                if (ackedSeqNum == s->currentSequenceNum)
                {
                    // ACK je ispravan. Ažuriramo stanje sesije.
                    // NAPOMENA: Potrebno je dodati `lastPacketSize` u UpdateSession_t strukturu
                    // s->bytesSent += s->lastPacketSize; 
                    s->currentSequenceNum++;

                    // Provjeravamo da li smo poslali cijeli fajl.
                    if (s->bytesSent >= s->fwInfo.size)
                    {
                        // Svi podaci su poslati, prelazimo u fazu finalizacije.
                        s->state = S_FINISHING;
                    }
                    else
                    {
                        // Nismo još gotovi, pripremi se za slanje sljedećeg paketa.
                        s->state = S_SENDING_DATA;
                    }
                }
                // Ako ackedSeqNum ne odgovara, ignorišemo ga. Naš retransmit tajmer će
                // svakako ponovo poslati paket ako pravi ACK ne stigne na vrijeme.
            }
            break;

        /**
         * Klijent je potvrdio da je primio cijeli fajl, da je CRC provjera
         * uspješna i da se restartuje kako bi bootloader preuzeo.
         */
        case SUB_CMD_FINISH_ACK:
            if (s->state == S_WAITING_FOR_FINISH_ACK)
            {
                // Odlično. Naš dio posla je skoro gotov.
                // Sada pokrećemo tajmer od 10 sekundi i čekamo da se uređaj vrati online.
                s->state = S_WAITING_FOR_VERIFICATION;
                s->timeoutStart = HAL_GetTick();
            }
            break;

        /**
         * Klijent javlja da finalna CRC provjera nije uspjela. Sesija je neuspješna.
         */
        case SUB_CMD_FINISH_NACK:
            if (s->state == S_WAITING_FOR_FINISH_ACK)
            {
                s->state = S_FAILED;
                // Ovdje ispisujemo grešku na displeju servera.
            }
            break;
            
        default:
            // Nepoznata ili neočekivana sub-komanda. Ignorišemo.
            break;
    }
}

