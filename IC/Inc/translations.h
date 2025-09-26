/**
 ******************************************************************************
 * @file    translations.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Centralizovana datoteka za sve prevodive stringove i mapiranja u aplikaciji.
 *
 * @note    Ova datoteka sadrži tabelu `language_strings` koja je ključna za
 * internacionalizaciju (i18n), kao i `icon_mapping_table` i
 * `scene_appearance_table` koje fleksibilno povezuju ID izbora
 * korisnika sa vizuelnim prikazom ikonica i njihovim opisima.
 ******************************************************************************
 */

#ifndef __TRANSLATIONS_H__
#define __TRANSLATIONS_H__

#include "display.h" 
#include "scene.h"

/**
 * @brief Glavna tabela za mapiranje ikonica svjetala.
 * @note  Ova tabela je "Jedinstveni Izvor Istine" za prikaz ikonica u meniju za podešavanje.
 * Vrijednost iz SPINBOX-a (`iconID`) se koristi kao direktan indeks u ovom nizu.
 * Svaki element niza definiše koju sličicu (bitmapu) treba prikazati i koje
 * tekstualne opise koristiti, omogućavajući višestruke opise za istu sličicu.
 */
static const IconMapping_t icon_mapping_table[] = {
    // --- Plafonska rasvjeta ---
    /* LUSTER */
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_GLAVNI_SECONDARY },
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_AMBIJENT_SECONDARY },
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_TRPEZARIJA_SECONDARY },
    { ICON_CHANDELIER,          TXT_LUSTER,     TXT_DNEVNA_SOBA_SECONDARY },
    /* SPOT (Ugradbeno svjetlo) */
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_LIJEVI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_DESNI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_CENTRALNI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_PREDNJI_SECONDARY },
    { ICON_SPOT_SINGLE,         TXT_SPOT,       TXT_ZADNJI_SECONDARY },
    { ICON_SPOT_CONSOLE,        TXT_SPOT,       TXT_HODNIK_SECONDARY },
    { ICON_SPOT_CONSOLE,        TXT_SPOT,       TXT_KUHINJA_SECONDARY },
    /* VISILICA */
    { ICON_HANGING,             TXT_VISILICA,   TXT_IZNAD_SANKA_SECONDARY },
    { ICON_HANGING,             TXT_VISILICA,   TXT_IZNAD_STola_SECONDARY },
    { ICON_HANGING,             TXT_VISILICA,   TXT_PORED_KREVETA_1_SECONDARY },
    { ICON_HANGING,             TXT_VISILICA,   TXT_PORED_KREVETA_2_SECONDARY },
    /* PLAFONJERA */
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_GLAVNA_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_SOBA_1_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_SOBA_2_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_KUPATILO_SECONDARY },
    { ICON_CEILING_LED_FIXTURE, TXT_PLAFONJERA, TXT_HODNIK_SECONDARY },
    
    // --- Zidna rasvjeta ---
    /* ZIDNA LAMPA */
    { ICON_WALL,                TXT_ZIDNA,      TXT_LIJEVA_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_DESNA_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_GORE_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_DOLE_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_CITANJE_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_AMBIJENT_SECONDARY },
    { ICON_WALL,                TXT_ZIDNA,      TXT_OGLEDALO_SECONDARY },

    // --- Specijalizovana rasvjeta ---
    /* LED TRAKA */
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_ISPOD_ELEMENTA_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_IZNAD_ELEMENTA_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_ORMAR_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_STEPENICE_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_TV_SECONDARY },
    { ICON_LED_STRIP,           TXT_LED_TRAKA,  TXT_AMBIJENT_SECONDARY },
    /* VENTILATOR SA SVJETLOM */
    { ICON_VENTILATOR_ICON,     TXT_VENTILATOR, TXT_DUMMY }
};

/**
 ******************************************************************************
 * @brief       Tabela predefinisanih izgleda za scene.
 * @author      Gemini & [Vaše Ime]
 * @note        Ova tabela služi kao biblioteka izgleda koje korisnik može
 * odabrati prilikom kreiranja ili editovanja scene. Povezuje
 * jedinstveni vizuelni ID ikonice (`IconID`) sa odgovarajućim
 * nazivom scene (`TextID`). Koristi se u "čarobnjaku" za scene.
 ******************************************************************************
 */
static const SceneAppearance_t scene_appearance_table[] = {
    { ICON_SCENE_WIZZARD,     TXT_SCENE_WIZZARD     },
    { ICON_SCENE_MORNING,     TXT_SCENE_MORNING     },
    { ICON_SCENE_SLEEP,       TXT_SCENE_SLEEP       },
    { ICON_SCENE_LEAVING,     TXT_SCENE_LEAVING     },
    { ICON_SCENE_HOMECOMING,  TXT_SCENE_HOMECOMING  },
    { ICON_SCENE_MOVIE,       TXT_SCENE_MOVIE       },
    { ICON_SCENE_DINNER,      TXT_SCENE_DINNER      },
    { ICON_SCENE_READING,     TXT_SCENE_READING     },
    { ICON_SCENE_RELAXING,    TXT_SCENE_RELAXING    },
    { ICON_SCENE_GATHERING,   TXT_SCENE_GATHERING   }
};

/**
 * @brief Glavna tabela sa prevodima.
 * @note  KONAČNA ISPRAVLJENA VERZIJA. Svaki red odgovara jednom ID-ju iz `TextID` enuma.
 * Dodati su redovi koji nedostaju da bi se redoslijed uskladio.
 */
static const char* language_strings[TEXT_COUNT][LANGUAGE_COUNT] = {
    /* TXT_DUMMY */                     { "", "", "","", "", "", "", "", "", "", "" },
    /* TXT_LIGHTS */                    { "SVJETLA", "LIGHTS", "LICHTER", "LUMIÈRES", "LUCI", "LUCES", "СВЕТ", "СВІТЛО", "ŚWIATŁA", "SVĚTLA", "SVETLÁ" },
    /* TXT_THERMOSTAT */                { "TERMOSTAT", "THERMOSTAT", "THERMOSTAT", "THERMOSTAT", "TERMOSTATO", "TERMOSTATO", "ТЕРМОСТАТ", "ТЕРМОСТАТ", "TERMOSTAT", "TERMOSTAT", "TERMOSTAT" },
    /* TXT_BLINDS */                    { "ROLETNE", "BLINDS", "JALOUSIEN", "STORES", "PERSIANE", "PERSIANAS", "ЖАЛЮЗИ", "ЖАЛЮЗІ", "ROLETY", "ŽALUZIE", "ŽALÚZIE" },
    /* TXT_DEFROSTER */                 { "ODMRZIVAČ", "DEFROSTER", "ENTEISER", "DÉGIVREUR", "SBRINATORE", "DESCONGELADOR", "РАЗМОРОЗКА", "РОЗМОРОЖУВАННЯ", "ODSZRANIACZ", "ODMRAZOVAČ", "ODMRAZOVAČ" },
    /* TXT_VENTILATOR */                { "VENTILATOR", "FAN", "LÜFTER", "VENTILATEUR", "VENTOLA", "VENTILADOR", "ВЕНТИЛЯТОР", "ВЕНТИЛЯТОР", "WENTYLATOR", "VENTILÁTOR", "VENTILÁTOR" },
    /* TXT_CLEAN */                     { "ČIŠĆENJE", "CLEAN", "REINIGEN", "NETTOYER", "PULIRE", "LIMPIAR", "ЧИСТКА", "ЧИЩЕННЯ", "CZYŚĆ", "ČISTIT", "ČISTIŤ" },
    /* TXT_WIFI */                      { "Wi-Fi", "Wi-Fi", "WLAN", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI" },
    /* TXT_APP */                       { "APP", "APP", "APP", "APPLI", "APP", "APP", "ПРИЛ.", "ДОДАТОК", "APLIKACJA", "APLIKACE", "APLIKÁCIA" },
    
    /* TXT_GATE */                      { "KAPIJA", "GATE", "TOR", "PORTAIL", "CANCELLO", "PUERTA", "ВОРОТА", "ВОРОТА", "BRAMA", "BRÁNA", "BRÁNA" },
    /* TXT_TIMER */                     { "TAJMER", "TIMER", "TIMER", "MINUTERIE", "TIMER", "TEMPORIZADOR", "ТАЙМЕР", "ТАЙМЕР", "MINUTNIK", "ČASOVAČ", "ČASOVAČ" },
    /* TXT_SECURITY */                  { "ALARM", "SECURITY", "SICHERHEIT", "SÉCURITÉ", "SICUREZZA", "SEGURIDAD", "ОХРАНА", "ОХОРОНА", "ALARM", "ZABEZPEČENÍ", "ZABEZPEČENIE" },
    /* TXT_SCENES */                    { "SCENE", "SCENES", "SZENEN", "SCÈNES", "SCENE", "ESCENAS", "СЦЕНЫ", "СЦЕНИ", "SCENY", "SCÉNY", "SCÉNY" },
    /* TXT_LANGUAGE_SOS_ALL_OFF */      { "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS", "SOS" },

    /* TXT_ALL */                       { "SVE", "ALL", "ALLE", "TOUT", "TUTTI", "TODOS", "ВСЕ", "ВСІ", "WSZYSTKO", "VŠECHNY", "VŠETKY" },
    /* TXT_SETTINGS */                  { "POSTAVKE", "SETTINGS", "EINSTELLUNGEN", "RÉGLAGES", "IMPOSTAZIONI", "AJUSTES", "НАСТРОЙКИ", "НАЛАШТУВАННЯ", "USTAWIENIA", "NASTAVENÍ", "NASTAVENIA" },
    /* TXT_GLOBAL_SETTINGS */           { "Globalno Podešavanje", "Global Settings", "Globale Einstellungen", "Réglages Globaux", "Impostazioni Globali", "Ajustes Globales", "Глобальные Настройки", "Глобальні Налаштування", "Ustawienia Globalne", "Globální Nastavení", "Globálne Nastavenia" },
    /* TXT_SAVE */                      { "SNIMI", "SAVE", "SPEICHERN", "SAUVEGARDER", "SALVA", "GUARDAR", "СОХРАНИТЬ", "ЗБЕРЕГТИ", "ZAPISZ", "ULOŽIT", "ULOŽIŤ" },
    /* TXT_ENTER_NEW_NAME */            { "UNESITE NOVI NAZIV", "ENTER NEW NAME", "NEUEN NAMEN EINGEBEN", "ENTREZ LE NOUVEAU NOM", "INSERIRE NUOVO NOME", "INTRODUZCA NUEVO NOMBRE", "ВВЕДИТЕ НОВОЕ ИМЯ", "ВВЕДІТЬ НОВУ НАЗВУ", "WPROWADŹ NOWĄ NAZWĘ", "ZADEJTE NOVÝ NÁZEV", "ZADAJTE NOVÝ NÁZOV" },
    /* TXT_CANCEL */                    { "OTKAŽI", "CANCEL", "ABBRECHEN", "ANNULER", "ANNULLA", "CANCELAR", "ОТМЕНА", "СКАСУВАТИ", "ANULUJ", "ZRUŠIT", "ZRUŠIŤ" },
    /* TXT_DELETE */                    { "OBRIŠI", "DELETE", "LÖSCHEN", "SUPPRIMER", "ELIMINA", "ELIMINAR", "УДАЛИТЬ", "ВИДАЛИТИ", "USUŃ", "SMAZAT", "VYMAZAŤ" },
    /* TXT_CONFIGURE_DEVICE_MSG */      { "Uređaj nije konfigurisan.", "Device not configured.", "Gerät nicht konfiguriert.", "Appareil non configuré.", "Dispositivo non configurato.", "Dispositivo no configurado.", "Устройство не настроено.", "Пристрій не налаштовано.", "Urządzenie nie jest skonfigurowane.", "Zařízení není nakonfigurováno.", "Zariadenie nie je nakonfigurované." },
    /* TXT_SCENE_SAVED_MSG */           { "Scena snimljena.", "Scene saved.", "Szene gespeichert.", "Scène enregistrée.", "Scena salvata.", "Escena guardada.", "Сцена сохранена.", "Сцену збережено.", "Scena zapisana.", "Scéna uložena.", "Scéna uložená." },
    /* TXT_PLEASE_CONFIGURE_SCENE_MSG */{ "Molimo konfigurišite scenu.", "Please configure the scene.", "Bitte konfigurieren Sie die Szene.", "Veuillez configurer la scène.", "Si prega di configurare la scena.", "Por favor, configure la escena.", "Пожалуйста, настройте сцену.", "Будь ласка, налаштуйте сцену.", "Proszę skonfigurować scenę.", "Nakonfigurujte prosím scénu.", "Nakonfigurujte prosím scénu." },
    /* TXT_TIMER_ENABLED */             { "Tajmer je uključen", "Timer is enabled", "Timer ist aktiviert", "Minuterie activée", "Timer abilitato", "Temporizador activado", "Таймер включен", "Таймер увімкнено", "Timer jest włączony", "Časovač je zapnutý", "Časovač je zapnutý" },
    /* TXT_TIMER_DISABLED */            { "Tajmer je isključen", "Timer is disabled", "Timer ist deaktiviert", "Minuterie désactivée", "Timer disabilitato", "Temporizador desactivado", "Таймер выключен", "Таймер вимкнено", "Timer jest wyłączony", "Časovač je vypnutý", "Časovač je vypnutý" },
    /* TXT_TIMER_EVERY_DAY */           { "Svaki dan", "Every day", "Jeden Tag", "Chaque jour", "Ogni giorno", "Cada día", "Каждый день", "Кожен день", "Codziennie", "Každý den", "Každý deň" },
    /* TXT_TIMER_WEEKDAYS */            { "Radnim danima", "Weekdays", "Wochentags", "En semaine", "Giorni feriali", "De lunes a viernes", "По будням", "У будні", "W dni robocze", "Všední dny", "Pracovné dni" },
    /* TXT_TIMER_WEEKEND */             { "Vikendom", "Weekends", "Wochenende", "Le week-end", "Fine settimana", "Fines de semana", "По выходным", "На вихідних", "W weekendy", "Víkendy", "Víkendy" },
    /* TXT_TIMER_ONCE */                { "Jednokratno", "Once", "Einmal", "Une fois", "Una volta", "Una vez", "Один раз", "Один раз", "Jednorazowo", "Jednou", "Jedenkrát" },
    /* TXT_TIMER_USE_BUZZER */          { "BUZZER", "BUZZER", "SUMMER", "BUZZER", "CICALINO", "ZUMBADOR", "ЗУММЕР", "ЗУМЕР", "BRZĘCZYK", "BZUČÁK", "BZUČIAK" },
    /* TXT_TIMER_TRIGGER_SCENE */       { "SCENA", "SCENE", "SZENE", "SCÈNE", "SCENA", "ESCENA", "СЦЕНА", "СЦЕНА", "SCENA", "SCÉNA", "SCÉNA" },
    /* TXT_ALARM_WAKEUP */              { "BUĐENJE!!!", "WAKE UP!!!", "AUFWACHEN!!!", "RÉVEILLEZ-VOUS!!!", "SVEGLIA!!!", "¡DESPIERTA!", "ПОДЪЕМ!!!", "ПРОКИДАЙТЕСЬ!!!", "POBUDKA!!!", "VSTÁVAT!!!", "VSTÁVAŤ!!!" },
    /* TXT_DISPLAY_CLEAN_TIME */        { "VRIJEME BRISANJA EKRANA:", "DISPLAY CLEAN TIME:", "BILDSCHIRMREINIGUNGSZEIT:", "TEMPS DE NETTOYAGE:", "TEMPO PULIZIA:", "TIEMPO DE LIMPIEZA:", "ВРЕМЯ ОЧИСТКИ:", "ЧАС ОЧИЩЕННЯ:", "CZAS CZYSZCZENIA:", "ČAS ČIŠTĚNÍ:", "ČAS ČISTENIA:" },
    /* TXT_FIRMWARE_UPDATE */           { "AŽURIRANJE...", "UPDATING...", "AKTUALISIERUNG...", "MISE À JOUR...", "AGGIORNAMENTO...", "ACTUALIZANDO...", "ОБНОВЛЕНИЕ...", "ОНОВЛЕННЯ...", "AKTUALIZACJA...", "AKTUALIZACE...", "AKTUALIZÁCIA..." },
    /* TXT_UPDATE_IN_PROGRESS */        { "Ažuriranje u toku, molimo sacekajte...", "Update in progress, please wait...", "Aktualisierung läuft, bitte warten...", "Mise à jour en cours, veuillez patienter...", "Aggiornamento in corso, attendere prego...", "Actualización en curso, por favor espere...", "Идет обновление, подождите...", "Триває оновлення, зачекайте...", "Aktualizacja w toku, proszę czekać...", "Probíhá aktualizace, čekejte prosím...", "Prebieha aktualizácia, čakajte prosím..." },
    /* TXT_MONDAY */                    { "Ponedjeljak", "Monday", "Montag", "Lundi", "Lunedì", "Lunes", "Понедельник", "Понеділок", "Poniedziałek", "Pondělí", "Pondelok" },
    /* TXT_TUESDAY */                   { "Utorak", "Tuesday", "Dienstag", "Mardi", "Martedì", "Martes", "Вторник", "Вівторок", "Wtorek", "Úterý", "Utorok" },
    /* TXT_WEDNESDAY */                 { "Srijeda", "Wednesday", "Mittwoch", "Mercredi", "Mercoledì", "Miércoles", "Среда", "Середа", "Środa", "Středa", "Streda" },
    /* TXT_THURSDAY */                  { "Četvrtak", "Thursday", "Donnerstag", "Jeudi", "Giovedì", "Jueves", "Четверг", "Четвер", "Czwartek", "Čtvrtek", "Štvrtok" },
    /* TXT_FRIDAY */                    { "Petak", "Friday", "Freitag", "Vendredi", "Venerdì", "Viernes", "Пятница", "П'ятниця", "Piątek", "Pátek", "Piatok" },
    /* TXT_SATURDAY */                  { "Subota", "Saturday", "Samstag", "Samedi", "Sabato", "Sábado", "Суббота", "Субота", "Sobota", "Sobota", "Sobota" },
    /* TXT_SUNDAY */                    { "Nedjelja", "Sunday", "Sonntag", "Dimanche", "Domenica", "Domingo", "Воскресенье", "Неділя", "Niedziela", "Neděle", "Nedeľa" },
    /* TXT_MONTH_JAN */                 { "Januar", "January", "Januar", "Janvier", "Gennaio", "Enero", "Январь", "Січень", "Styczeń", "Leden", "Január" },
    /* TXT_MONTH_FEB */                 { "Februar", "February", "Februar", "Février", "Febbraio", "Febrero", "Февраль", "Лютий", "Luty", "Únor", "Február" },
    /* TXT_MONTH_MAR */                 { "Mart", "March", "März", "Mars", "Marzo", "Marzo", "Март", "Березень", "Marzec", "Březen", "Marec" },
    /* TXT_MONTH_APR */                 { "April", "April", "April", "Avril", "Aprile", "Abril", "Апрель", "Квітень", "Kwiecień", "Duben", "Apríl" },
    /* TXT_MONTH_MAY */                 { "Maj", "May", "Mai", "Mai", "Maggio", "Mayo", "Май", "Травень", "Maj", "Květen", "Máj" },
    /* TXT_MONTH_JUN */                 { "Juni", "June", "Juni", "Juin", "Giugno", "Junio", "Июнь", "Червень", "Czerwiec", "Červen", "Jún" },
    /* TXT_MONTH_JUL */                 { "Juli", "July", "Juli", "Juillet", "Luglio", "Julio", "Июль", "Липень", "Lipiec", "Červenec", "Júl" },
    /* TXT_MONTH_AUG */                 { "August", "August", "August", "Août", "Agosto", "Agosto", "Август", "Серпень", "Sierpień", "Srpen", "August" },
    /* TXT_MONTH_SEP */                 { "Septembar", "September", "September", "Septembre", "Settembre", "Septiembre", "Сентябрь", "Вересень", "Wrzesień", "Září", "September" },
    /* TXT_MONTH_OCT */                 { "Oktobar", "October", "Oktober", "Octobre", "Ottobre", "Octubre", "Октябрь", "Жовтень", "Październik", "Říjen", "Október" },
    /* TXT_MONTH_NOV */                 { "Novembar", "November", "November", "Novembre", "Novembre", "Noviembre", "Ноябрь", "Листопад", "Listopad", "Listopad", "November" },
    /* TXT_MONTH_DEC */                 { "Decembar", "December", "Dezember", "Décembre", "Dicembre", "Diciembre", "Декабрь", "Грудень", "Grudzień", "Prosinec", "December" },
    /* TXT_LANGUAGE_NAME */             { "Bos/Cro/Srb/Mon", "English", "Deutsch", "Français", "Italiano", "Español", "Русский", "Українська", "Polski", "Čeština", "Slovenčina" },
    /* TXT_DATETIME_SETUP_TITLE */      { "Podesite Datum i Vrijeme", "Set Date and Time", "Datum und Uhrzeit einstellen", "Régler la date et l'heure", "Imposta data e ora", "Configurar fecha y hora", "Установить дату и время", "Встановити дату та час", "Ustaw datę i godzinę", "Nastavte datum a čas", "Nastavte dátum a čas" },
    /* TXT_TIMER_SETTINGS_TITLE */      { "Podešavanje tajmera", "Timer Settings", "Timer-Einstellungen", "Réglages de la minuterie", "Impostazioni timer", "Configuración del temporizador", "Настройки таймера", "Налаштування таймера", "Ustawienia timera", "Nastavení časovače", "Nastavenia časovača" }, // DODAJTE OVAJ RED
    /* TXT_DAY */                       { "Dan", "Day", "Tag", "Jour", "Giorno", "Día", "День", "День", "Dzień", "Den", "Deň" },
    /* TXT_MONTH */                     { "Mjesec", "Month", "Monat", "Mois", "Mese", "Mes", "Месяц", "Місяць", "Miesiąc", "Měsíc", "Mesiac" },
    /* TXT_YEAR */                      { "Godina", "Year", "Jahr", "Année", "Anno", "Año", "Год", "Рік", "Rok", "Rok", "Rok" },
    /* TXT_HOUR */                      { "Sat", "Hour", "Stunde", "Heure", "Ora", "Hora", "Час", "Година", "Godzina", "Hodina", "Hodina" },
    /* TXT_MINUTE */                    { "Minuta", "Minute", "Minute", "Minute", "Minuto", "Minuto", "Минута", "Хвилина", "Minuta", "Minuta", "Minúta" },
    /* TXT_LUSTER */                    { "LUSTER", "CHANDELIER", "KRONLEUCHTER", "LUSTRE", "LAMPADARIO", "ARAÑA", "ЛЮСТРА", "ЛЮСТРА", "ŻYRANDOL", "LUSTR", "LUSTER" },
    /* TXT_SPOT */                      { "SPOT", "SPOT", "STRAHLER", "SPOT", "FARETTO", "FOCO", "ТОЧЕЧНЫЙ", "ТОЧКОВИЙ", "PUNKTOWE", "BODOVÉ", "BODOVÉ" },
    /* TXT_VISILICA */                  { "VISILICA", "PENDANT", "HÄNGELEUCHTE", "SUSPENSION", "SOSPENSIONE", "COLGANTE", "ПОДВЕС", "ПІДВІС", "WISZĄCA", "ZÁVĚSNÉ", "ZÁVESNÉ" },
    /* TXT_PLAFONJERA */                { "PLAFONJERA", "CEILING", "DECKENLEUCHTE", "PLAFONNIER", "PLAFONIERA", "PLAFÓN", "ПОТОЛОЧНЫЙ", "СТЕЛЬОВИЙ", "PLAFON", "STROPNÍ", "STROPNÉ" },
    /* TXT_ZIDNA */                     { "ZIDNA", "WALL", "WANDLEUCHTE", "APPLIQUE", "DA PARETE", "DE PARED", "НАСТЕННЫЙ", "НАСТІННИЙ", "KINKIET", "NÁSTĚNNÉ", "NÁSTENNÉ" },
    /* TXT_SLIKA */                     { "SLIKA", "PICTURE", "BILDERLEUCHTE", "TABLEAU", "QUADRO", "CUADRO", "КАРТИНА", "КАРТИНА", "OBRAZ", "OBRAZ", "OBRAZ" },
    /* TXT_PODNA */                     { "PODNA", "FLOOR", "STEHLEUCHTE", "LAMPADAIRE", "DA TERRA", "DE PIE", "НАПОЛЬНЫЙ", "ПІДЛОГОВИЙ", "PODŁOGOWA", "STOJACÍ", "STOJACA" },
    /* TXT_STOLNA */                    { "STOLNA", "TABLE", "TISCHLEUCHTE", "DE TABLE", "DA TAVOLO", "DE MESA", "НАСТОЛЬНЫЙ", "НАСТІЛЬНИЙ", "STOŁOWA", "STOLNÍ", "STOLNÁ" },
    /* TXT_LED_TRAKA */                 { "LED TRAKA", "LED STRIP", "LED-STREIFEN", "BANDE LED", "STRISCIA LED", "TIRA LED", "ЛЕД-ЛЕНТА", "ЛЕД-СТРІЧКА", "TAŚMA LED", "LED PÁSEK", "LED PÁSIK" },
    /* TXT_VENTILATOR_IKONA */          { "VENTILATOR", "FAN", "VENTILATOR", "VENTILATEUR", "VENTOLA", "VENTILADOR", "ВЕНТИЛЯТОР", "ВЕНТИЛЯТОР", "WENTYLATOR", "VENTILÁTOR", "VENTILÁTOR" },
    /* TXT_FASADA */                    { "FASADA", "FACADE", "FASSADE", "FAÇADE", "FACCIATA", "FACHADA", "ФАСАД", "ФАСАД", "FASADA", "FASÁDA", "FASÁDA" },
    /* TXT_STAZA */                     { "STAZA", "PATH", "WEG", "CHEMIN", "SENTIERO", "CAMINO", "ДОРОЖКА", "ДОРІЖКА", "ŚCIEŻKA", "CESTA", "CESTA" },
    /* TXT_REFLEKTOR */                 { "REFLEKTOR", "FLOODLIGHT", "SCHEINWERFER", "PROJECTEUR", "PROIETTORE", "REFLECTOR", "ПРОЖЕКТОР", "ПРОЖЕКТОР", "REFLEKTOR", "REFLEKTOR", "REFLEKTOR" },
    /* TXT_SCENE_WIZZARD */             { "Dodaj Scenu", "Add Scene", "Szene hinzufügen", "Ajouter une scène", "Aggiungi scena", "Añadir escena", "Добавить сцену", "Додати сцену", "Dodaj scenę", "Přidat scénu", "Pridať scénu" },
    /* TXT_SCENE_MORNING */             { "Jutro", "Morning", "Morgen", "Matin", "Mattina", "Mañana", "Утро", "Ранок", "Poranek", "Ráno", "Ráno" },
    /* TXT_SCENE_SLEEP */               { "Spavanje", "Sleep", "Schlafen", "Dormir", "Sonno", "Dormir", "Сон", "Сон", "Sen", "Spánek", "Spánok" },
    /* TXT_SCENE_LEAVING */             { "Odlazak", "Leaving", "Verlassen", "Départ", "Uscita", "Salida", "Уход", "Вихід", "Wyjście", "Odchod", "Odchod" },
    /* TXT_SCENE_HOMECOMING */          { "Povratak", "Homecoming", "Heimkehr", "Retour", "Ritorno a casa", "Regreso a casa", "Возвращение", "Повернення", "Powrót do domu", "Návrat domů", "Návrat domov" },
    /* TXT_SCENE_MOVIE */               { "Film", "Movie", "Film", "Film", "Film", "Película", "Фильм", "Фільм", "Film", "Film", "Film" },
    /* TXT_SCENE_DINNER */              { "Večera", "Dinner", "Abendessen", "Dîner", "Cena", "Cena", "Ужин", "Вечеря", "Kolacja", "Večeře", "Večera" },
    /* TXT_SCENE_READING */             { "Čitanje", "Reading", "Lesen", "Lecture", "Lettura", "Lectura", "Чтение", "Читання", "Czytanie", "Čtení", "Čítanie" },
    /* TXT_SCENE_RELAXING */            { "Opuštanje", "Relaxing", "Entspannen", "Détente", "Rilassante", "Relajación", "Расслабление", "Розслаблення", "Relaks", "Odpočinek", "Oddych" },
    /* TXT_SCENE_GATHERING */           { "Druženje", "Gathering", "Treffen", "Rassemblement", "Incontro", "Reunión", "Сбор", "Збори", "Spotkanie", "Setkání", "Stretnutie" },
    /* TXT_GLAVNI_SECONDARY */          { "GLAVNI", "MAIN", "HAUPT", "PRINCIPAL", "PRINCIPALE", "PRINCIPAL", "ГЛАВНЫЙ", "ГОЛОВНИЙ", "GŁÓWNY", "HLAVNÍ", "HLAVNÝ" },
    /* TXT_AMBIJENT_SECONDARY */        { "AMBIJENT", "AMBIENT", "AMBIENTE", "AMBIANCE", "AMBIENTE", "AMBIENTE", "АТМОСФЕРА", "АТМОСФЕРА", "NASTRÓJ", "PROSTŘEDÍ", "PROSTREDIE" },
    /* TXT_TRPEZARIJA_SECONDARY */      { "TRPEZARIJA", "DINING", "ESSZIMMER", "SALLE À MANGER", "SALA DA PRANZO", "COMEDOR", "СТОЛОВАЯ", "ЇДАЛЬНЯ", "JADALNIA", "JÍDELNA", "JEDÁLEŇ" },
    /* TXT_DNEVNA_SOBA_SECONDARY */     { "DNEVNA SOBA", "LIVING RM", "WOHNZIMMER", "SALON", "SOGGIORNO", "SALA DE ESTAR", "ГОСТИНАЯ", "ВІТАЛЬНЯ", "SALON", "OBÝVACÍ POKOJ", "OBÝVAČKA" },
    /* TXT_LIJEVI_SECONDARY */          { "LIJEVI", "LEFT", "LINKS", "GAUCHE", "SINISTRA", "IZQUIERDA", "ЛЕВЫЙ", "ЛІВИЙ", "LEWY", "LEVÝ", "ĽAVÝ" },
    /* TXT_DESNI_SECONDARY */           { "DESNI", "RIGHT", "RECHTS", "DROITE", "DESTRA", "DERECHA", "ПРАВЫЙ", "ПРАВИЙ", "PRAWY", "PRAVÝ", "PRAVÝ" },
    /* TXT_CENTRALNI_SECONDARY */       { "CENTRALNI", "CENTRAL", "ZENTRAL", "CENTRAL", "CENTRALE", "CENTRAL", "ЦЕНТРАЛЬНЫЙ", "ЦЕНТРАЛЬНИЙ", "CENTRALNY", "STŘEDNÍ", "STREDNÝ" },
    /* TXT_PREDNJI_SECONDARY */         { "PREDNJI", "FRONT", "VORNE", "AVANT", "ANTERIORE", "FRONTAL", "ПЕРЕДНИЙ", "ПЕРЕДНІЙ", "PRZEDNI", "PŘEDNÍ", "PREDNÝ" },
    /* TXT_ZADNJI_SECONDARY */          { "ZADNJI", "REAR", "HINTEN", "ARRIÈRE", "POSTERIORE", "TRASERA", "ЗАДНИЙ", "ЗАДНІЙ", "TYLNY", "ZADNÍ", "ZADNÝ" },
    /* TXT_HODNIK_SECONDARY */          { "HODNIK", "HALLWAY", "FLUR", "COULOIR", "CORRIDOIO", "PASILLO", "КОРИДОР", "КОРИДОР", "KORYTARZ", "CHODBA", "CHODBA" },
    /* TXT_KUHINJA_SECONDARY */         { "KUHINJA", "KITCHEN", "KÜCHE", "CUISINE", "CUCINA", "COCINA", "КУХНЯ", "КУХНЯ", "KUCHNIA", "KUCHYNĚ", "KUCHYŇA" },
    /* TXT_IZNAD_SANKA_SECONDARY */     { "IZNAD ŠANKA", "ABOVE BAR", "ÜBER THEKE", "AU-DESSUS BAR", "SOPRA BANCONE", "SOBRE BARRA", "НАД БАРОМ", "НАД БАРОМ", "NAD BAREM", "NAD BAREM", "NAD BAROM" },
    /* TXT_IZNAD_STola_SECONDARY */     { "IZNAD STOLA", "ABOVE TABLE", "ÜBER TISCH", "AU-DESSUS TABLE", "SOPRA TAVOLO", "SOBRE MESA", "НАД СТОЛОМ", "НАД СТОЛОМ", "NAD STOŁEM", "NAD STOLEM", "NAD STOLOM" },
    /* TXT_PORED_KREVETA_1_SECONDARY */ { "PORED KREVETA 1", "BESIDE BED 1", "NEBEN BETT 1", "À CÔTÉ DU LIT 1", "ACCANTO AL LETTO 1", "JUNTO A CAMA 1", "У КРОВАТИ 1", "БІЛЯ ЛІЖКА 1", "OBOK ŁÓŻKA 1", "VEDLE POSTELE 1", "VEDĽA POSTELE 1" },
    /* TXT_PORED_KREVETA_2_SECONDARY */ { "PORED KREVETA 2", "BESIDE BED 2", "NEBEN BETT 2", "À CÔTÉ DU LIT 2", "ACCANTO AL LETTO 2", "JUNTO A CAMA 2", "У КРОВАТИ 2", "БІЛЯ ЛІЖКА 2", "OBOK ŁÓŻKA 2", "VEDLE POSTELE 2", "VEDĽA POSTELE 2" },
    /* TXT_GLAVNA_SECONDARY */          { "GLAVNA", "MAIN", "HAUPT", "PRINCIPALE", "PRINCIPALE", "PRINCIPAL", "ГЛАВНАЯ", "ГОЛОВНА", "GŁÓWNA", "HLAVNÍ", "HLAVNÁ" },
    /* TXT_SOBA_1_SECONDARY */          { "SOBA 1", "ROOM 1", "ZIMMER 1", "CHAMBRE 1", "CAMERA 1", "HABITACIÓN 1", "КОМНАТА 1", "КІМНАТА 1", "POKÓJ 1", "POKOJ 1", "IZBA 1" },
    /* TXT_SOBA_2_SECONDARY */          { "SOBA 2", "ROOM 2", "ZIMMER 2", "CHAMBRE 2", "CAMERA 2", "HABITACIÓN 2", "КОМНАТА 2", "КІМНАТА 2", "POKÓJ 2", "POKOJ 2", "IZBA 2" },
    /* TXT_KUPATILO_SECONDARY */        { "KUPATILO", "BATHROOM", "BADEZIMMER", "SALLE DE BAIN", "BAGNO", "BAÑO", "ВАННАЯ", "ВАННА", "ŁAZIENKA", "KOUPELNA", "KÚPEĽŇA" },
    /* TXT_LIJEVA_SECONDARY */          { "LIJEVA", "LEFT", "LINKE", "GAUCHE", "SINISTRA", "IZQUIERDA", "ЛЕВАЯ", "ЛІВА", "LEWA", "LEVÁ", "ĽAVÁ" },
    /* TXT_DESNA_SECONDARY */           { "DESNA", "RIGHT", "RECHTE", "DROITE", "DESTRA", "DERECHA", "ПРАВАЯ", "ПРАВА", "PRAWA", "PRAVÁ", "PRAVÁ" },
    /* TXT_GORE_SECONDARY */            { "GORE", "UP", "OBEN", "HAUT", "SU", "ARRIBA", "ВВЕРХ", "ВГОРУ", "GÓRA", "NAHORU", "HORE" },
    /* TXT_DOLE_SECONDARY */            { "DOLE", "DOWN", "UNTEN", "BAS", "GIÙ", "ABAJO", "ВНИЗ", "ВНИЗ", "DÓŁ", "DOLŮ", "DOLE" },
    /* TXT_CITANJE_SECONDARY */         { "ČITANJE", "READING", "LESEN", "LECTURE", "LETTURA", "LECTURA", "ЧТЕНИЕ", "ЧИТАННЯ", "CZYTANIE", "ČTENÍ", "ČÍTANIE" },
    /* TXT_OGLEDALO_SECONDARY */        { "OGLEDALO", "MIRROR", "SPIEGEL", "MIROIR", "SPECCHIO", "ESPEJO", "ЗЕРКАЛО", "ДЗЕРКАЛО", "LUSTRO", "ZRCADLO", "ZRKADLO" },
    /* TXT_UGAO_SECONDARY */            { "UGAO", "CORNER", "ECKE", "COIN", "ANGOLO", "ESQUINA", "УГОЛ", "КУТ", "NAROŻNIK", "ROH", "ROH" },
    /* TXT_PORED_FOTELJE_SECONDARY */   { "PORED FOTELJE", "BY ARMCHAIR", "AM SESSEL", "PRÈS DU FAUTEUIL", "VICINO POLTRONA", "JUNTO AL SILLÓN", "У КРЕСЛА", "БІЛЯ КРІСЛА", "PRZY FOTELU", "U KŘESLA", "PRI KRESLE" },
    /* TXT_RADNI_STO_SECONDARY */       { "RADNI STO", "DESK", "SCHREIBTISCH", "BUREAU", "SCRIVANIA", "ESCRITORIO", "РАБОЧИЙ СТОЛ", "РОБОЧИЙ СТІЛ", "BIURKO", "PRACOVNÍ STŮL", "PRACOVNÝ STÔL" },
    /* TXT_NOCNA_1_SECONDARY */         { "NOĆNA 1", "NIGHT 1", "NACHT 1", "NUIT 1", "NOTTE 1", "NOCHE 1", "НОЧНАЯ 1", "НІЧНА 1", "NOCNA 1", "NOČNÍ 1", "NOČNÁ 1" },
    /* TXT_NOCNA_2_SECONDARY */         { "NOĆNA 2", "NIGHT 2", "NACHT 2", "NUIT 2", "NOTTE 2", "NOCHE 2", "НОЧНАЯ 2", "НІЧНА 2", "NOCNA 2", "NOČNÍ 2", "NOČNÁ 2" },
    /* TXT_ISPOD_ELEMENTA_SECONDARY */  { "ISPOD ELEMENTA", "UNDER-CABINET", "UNTERSCHRANK", "SOUS-MEUBLE", "SOTTOMOBILE", "BAJO GABINETE", "ПОД ШКАФОМ", "ПІД ШАФОЮ", "PODSZAFKOWA", "POD SKŘÍŇKOU", "POD SKRINKOU" },
    /* TXT_IZNAD_ELEMENTA_SECONDARY */  { "IZNAD ELEMENTA", "OVER-CABINET", "OBERSCHRANK", "SUR-MEUBLE", "SOPRAMOBILE", "SOBRE GABINETE", "НАД ШКАФОМ", "НАД ШАФОЮ", "NADSZAFKOWA", "NAD SKŘÍŇKOU", "NAD SKRINKOU" },
    /* TXT_ORMAR_SECONDARY */           { "ORMAR", "WARDROBE", "KLEIDERSCHRANK", "ARMOIRE", "GUARDAROBA", "ARMARIO", "ШКАФ", "ШАФА", "SZAFKA", "ŠATNÍ SKŘÍŇ", "ŠATNÍK" },
    /* TXT_STEPENICE_SECONDARY */       { "STEPENICE", "STAIRS", "TREPPE", "ESCALIERS", "SCALE", "ESCALERAS", "ЛЕСТНИЦА", "СХОДИ", "SCHODY", "SCHODY", "SCHODY" },
    /* TXT_TV_SECONDARY */              { "TV", "TV", "FERNSEHER", "TÉLÉ", "TV", "TELE", "ТВ", "ТБ", "TELEWIZOR", "TELEVIZE", "TELEVÍZOR" },
    /* TXT_ULAZ_SECONDARY */            { "ULAZ", "ENTRANCE", "EINGANG", "ENTRÉE", "INGRESSO", "ENTRADA", "ВХОД", "ВХІД", "WEJŚCIE", "VCHOD", "VCHOD" },
    /* TXT_TERASA_SECONDARY */          { "TERASA", "TERRACE", "TERRASSE", "TERRASSE", "TERRAZZA", "TERRAZA", "ТЕРРАСА", "ТЕРАСА", "TARAS", "TERASA", "TERASA" },
    /* TXT_BALKON_SECONDARY */          { "BALKON", "BALCONY", "BALKON", "BALCON", "BALCONE", "BALCÓN", "БАЛКОН", "БАЛКОН", "BALKON", "BALKON", "BALKÓN" },
    /* TXT_ZADNJA_SECONDARY */          { "ZADNJA", "REAR", "HINTERE", "ARRIÈRE", "POSTERIORE", "TRASERA", "ЗАДНЯЯ", "ЗАДНЯ", "TYLNA", "ZADNÍ", "ZADNÁ" },
    /* TXT_PRILAZ_SECONDARY */          { "PRILAZ", "DRIVEWAY", "AUFFAHRT", "ALLÉE", "VIALE", "ENTRADA", "ПОДЪЕЗД", "ПІД'ЇЗД", "PODJAZD", "PŘÍJEZDOVÁ CESTA", "PRÍJAZDOVÁ CESTA" },
    /* TXT_DVORISTE_SECONDARY */        { "DVORIŠTE", "YARD", "HOF", "COUR", "CORTILE", "PATIO", "ДВОР", "ДВІР", "PODWÓRKO", "DVŮR", "DVOR" },
    /* TXT_DRVO_SECONDARY */            { "DRVO", "TREE", "BAUM", "ARBRE", "ALBERO", "ÁRBOL", "ДЕРЕВО", "ДЕРЕВО", "DRZEWO", "STROM", "STROM" },
};

static const char* _acContent[LANGUAGE_COUNT][7] = {
    { "PON", "UTO", "SRI", "ČET", "PET", "SUB", "NED" },
    { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" },
    { "MO", "DI", "MI", "DO", "FR", "SA", "SO" },
    { "LUN", "MAR", "MER", "JEU", "VEN", "SAM", "DIM" },
    { "LUN", "MAR", "MER", "GIO", "VEN", "SAB", "DOM" },
    { "LUN", "MAR", "MIÉ", "JUE", "VIE", "SÁB", "DOM" },
    { "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС" },
    { "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "НД" },
    { "PON", "WT", "ŚR", "CZW", "PT", "SOB", "ND" },
    { "PO", "ÚT", "ST", "ČT", "PÁ", "SO", "NE" },
    { "PO", "UT", "ST", "ŠT", "PI", "SO", "NE" }
};

#endif // __TRANSLATIONS_H__
