/**
 ******************************************************************************
 * @file    update_manager.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Javni interfejs za asinhroni menadžer za FW update sesije.
 *
 * @note
 * Ovaj header fajl deklariše funkcije koje su dostupne ostatku sistema
 * za pokretanje i upravljanje procesom ažuriranja firmvera.
 * Drugi moduli, kao što su HTTP server i glavna aplikaciona petlja,
 * uključuju ovaj fajl kako bi mogli komunicirati sa Update Manager-om.
 ******************************************************************************
 */

#ifndef __UPDATE_MANAGER_H__
#define __UPDATE_MANAGER_H__

#include <stdint.h>
#include <stdbool.h>
#include "TinyFrame.h" // Potrebno zbog TF_Msg tipa

/**
 * @brief Inicijalizuje Update Manager.
 * @note  Mora se pozvati jednom pri startu sistema kako bi se interni niz
 * sesija postavio u početno stanje (S_IDLE).
 * @param None
 * @retval None
 */
void UpdateManager_Init(void);

/**
 * @brief Pokreće novu sesiju ažuriranja firmvera za određenog klijenta.
 * @note  Ovu funkciju tipično poziva CGI handler (`httpd_cgi_ssi.c`) nakon što
 * primi HTTP zahtjev za update. Funkcija je ne-blokirajuća; ona samo
 * pronalazi slobodan slot, priprema sesiju i postavlja je u S_STARTING stanje.
 * Stvarni transfer se odvija u pozadini preko `UpdateManager_Service`.
 *
 * @param clientAddress Adresa (1-254) klijenta koji se ažurira.
 * @param filePath      Puna putanja do .BIN fajla na uSD kartici (npr. "IC.BIN").
 * @retval bool         Vraća `true` ako je sesija uspješno inicirana, ili `false`
 * ako nema slobodnih slotova ili ako fajl ne postoji.
 */
bool UpdateManager_StartSession(uint8_t clientAddress, const char* filePath);

/**
 * @brief Glavna servisna funkcija (drajver) koja upravlja svim aktivnim sesijama.
 * @note  Ovu funkciju je neophodno pozivati periodično iz glavne `while(1)` petlje.
 * Ona prolazi kroz sve aktivne sesije i na osnovu njihovog stanja izvršava
 * sljedeći korak (npr. šalje paket, provjerava tajmer, ponovo šalje paket).
 * Ovo je ključna funkcija koja omogućava asinhroni rad.
 * @param None
 * @retval None
 */
void UpdateManager_Service(void);

/**
 * @brief Obrađuje dolazni odgovor (ACK/NACK) od klijenta.
 * @note  Ovu funkciju poziva TinyFrame listener iz `rs485.c` svaki put kada stigne
 * poruka tipa `FIRMWARE_UPDATE`. Funkcija pronalazi odgovarajuću sesiju
 * na osnovu adrese pošiljaoca i ažurira njeno stanje u mašini stanja.
 *
 * @param msg Pokazivač na primljenu TinyFrame poruku.
 * @retval None
 */
void UpdateManager_ProcessResponse(TF_Msg *msg);

#endif // __UPDATE_MANAGER_H__
