#ifndef __TRANSLATIONS_H__
#define __TRANSLATIONS_H__

/**
 * @file    translations.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Centralizovana datoteka za sve prevodive stringove u aplikaciji.
 *
 * @note
 * Ova datoteka sadrži tabelu `language_strings` koja je ključna za
 * internacionalizaciju (i18n). Odvajanjem prevoda od logike u `display.c`,
 * olakšava se održavanje i dodavanje novih jezika.
 */

#include "display.h" // Uključujemo display.h da bismo vidjeli enum-e Languages i TextID

/**
 * @brief Tabela koja sadrži sve tekstualne stringove za sve podržane jezike.
 * @note Redovi odgovaraju TextID enum-u iz display.h, a kolone Languages enum-u.
 */
static const char* language_strings[TEXT_COUNT][LANGUAGE_COUNT] = {
    /* TXT_DUMMY */             { "", "", "", "", "", "", "", "", "", "", "" },
    /* TXT_LIGHTS */            { "SVJETLA", "LIGHTS", "LICHTER", "LUMIÈRES", "LUCI", "LUCES", "СВЕТ", "СВІТЛО", "ŚWIATŁA", "SVĚTLA", "SVETLÁ" },
    /* TXT_THERMOSTAT */        { "TERMOSTAT", "THERMOSTAT", "THERMOSTAT", "THERMOSTAT", "TERMOSTATO", "TERMOSTATO", "ТЕРМОСТАТ", "ТЕРМОСТАТ", "TERMOSTAT", "TERMOSTAT", "TERMOSTAT" },
    /* TXT_BLINDS */            { "ROLETNE", "BLINDS", "JALOUSIEN", "STORES", "PERSIANE", "PERSIANAS", "ЖАЛЮЗИ", "ЖАЛЮЗІ", "ROLETY", "ŽALUZIE", "ŽALÚZIE" },
    /* TXT_DEFROSTER */         { "ODMRZIVAČ", "DEFROSTER", "ENTEISER", "DÉGIVREUR", "SBRINATORE", "DESCONGELADOR", "РАЗМОРОЗКА", "РОЗМОРОЖУВАННЯ", "ODSZRANIACZ", "ODMRAZOVAČ", "ODMRAZOVAČ" },
    /* TXT_VENTILATOR */        { "VENTILATOR", "FAN", "LÜFTER", "VENTILATEUR", "VENTOLA", "VENTILADOR", "ВЕНТИЛЯТОР", "ВЕНТИЛЯТОР", "WENTYLATOR", "VENTILÁTOR", "VENTILÁTOR" },
    /* TXT_CLEAN */             { "ČIŠĆENJE", "CLEAN", "REINIGEN", "NETTOYER", "PULIRE", "LIMPIAR", "ЧИСТКА", "ЧИЩЕННЯ", "CZYŚĆ", "ČISTIT", "ČISTIŤ" },
    /* TXT_WIFI */              { "Wi-Fi", "Wi-Fi", "WLAN", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI", "WI-FI" },
    /* TXT_APP */               { "APP", "APP", "APP", "APPLI", "APP", "APP", "ПРИЛ.", "ДОДАТОК", "APLIKACJA", "APLIKACE", "APLIKÁCIA" },
    /* TXT_SETTINGS */          { "POSTAVKE", "SETTINGS", "EINSTELLUNGEN", "RÉGLAGES", "IMPOSTAZIONI", "AJUSTES", "НАСТРОЙКИ", "НАЛАШТУВАННЯ", "USTAWIENIA", "NASTAVENÍ", "NASTAVENIA" },
    /* TXT_NEXT */              { "SLJEDEĆE", "NEXT", "NÄCHSTE", "SUIVANT", "PROSSIMO", "SIGUIENTE", "ДАЛЕЕ", "ДАЛІ", "DALEJ", "DALŠÍ", "ĎALEJ" },
    /* TXT_SAVE */              { "SPASI", "SAVE", "SPEICHERN", "SAUVER", "SALVA", "GUARDAR", "СОХРАНИТЬ", "ЗБЕРЕГТИ", "ZAPISZ", "ULOŽIT", "ULOŽIŤ" },
    /* TXT_ALL */               { "SVE", "ALL", "ALLE", "TOUT", "TUTTI", "TODOS", "ВСЕ", "ВСІ", "WSZYSTKO", "VŠECHNY", "VŠETKY" },
    /* TXT_ALARM */             { "ALARM", "ALARM", "ALARM", "ALARME", "ALLARME", "ALARMA", "ТРЕВОГА", "ТРИВОГА", "ALARM", "ALARM", "ALARM" },
    /* TXT_ENTER_PASSWORD */    { "UNESI ŠIFRU", "ENTER PASSWORD", "PASSWORT EINGEBEN", "ENTREZ MOT DE PASSE", "INSERIRE PASSWORD", "INTRODUZCA CLAVE", "ВВЕДИТЕ ПАРОЛЬ", "ВВЕДІТЬ ПАРОЛЬ", "WPROWADŹ HASŁO", "ZADEJTE HESLO", "ZADAJTE HESLO" },
    /* TXT_PASSWORD_CORRECT */  { "ŠIFRA TAČNA", "PASSWORD CORRECT", "PASSWORT KORREKT", "MOT DE PASSE CORRECT", "PASSWORD CORRETTA", "CLAVE CORRECTA", "ПАРОЛЬ ВЕРЕН", "ПАРОЛЬ ВІРНИЙ", "HASŁO POPRAWNE", "HESLO JE SPRÁVNÉ", "HESLO JE SPRÁVNE" },
    /* TXT_WRONG_PASSWORD */    { "POGREŠNA ŠIFRA", "WRONG PASSWORD", "FALSCHES PASSWORT", "MOT DE PASSE INCORRECT", "PASSWORD ERRATA", "CLAVE INCORRECTA", "НЕВЕРНЫЙ ПАРОЛЬ", "НЕПРАВИЛЬНИЙ ПАРОЛЬ", "BŁĘDNE HASŁO", "ŠPATNÉ HESLO", "NESPRÁVNE HESLO" },
    /* TXT_DISPLAY_CLEAN_TIME */{ "VRIJEME BRISANJA EKRANA:", "DISPLAY CLEAN TIME:", "BILDSCHIRMREINIGUNGSZEIT:", "TEMPS DE NETTOYAGE:", "TEMPO PULIZIA:", "TIEMPO DE LIMPIEZA:", "ВРЕМЯ ОЧИСТКИ:", "ЧАС ОЧИЩЕННЯ:", "CZAS CZYSZCZENIA:", "ČAS ČIŠTĚNÍ:", "ČAS ČISTENIA:" },
    /* TXT_FIRMWARE_UPDATE */   { "AŽURIRANJE...", "UPDATING...", "AKTUALISIERUNG...", "MISE À JOUR...", "AGGIORNAMENTO...", "ACTUALIZANDO...", "ОБНОВЛЕНИЕ...", "ОНОВЛЕННЯ...", "AKTUALIZACJA...", "AKTUALIZACE...", "AKTUALIZÁCIA..." },
    /* TXT_MONDAY */            { "Ponedjeljak", "Monday", "Montag", "Lundi", "Lunedì", "Lunes", "Понедельник", "Понеділок", "Poniedziałek", "Pondělí", "Pondelok" },
    /* TXT_TUESDAY */           { "Utorak", "Tuesday", "Dienstag", "Mardi", "Martedì", "Martes", "Вторник", "Вівторок", "Wtorek", "Úterý", "Utorok" },
    /* TXT_WEDNESDAY */         { "Srijeda", "Wednesday", "Mittwoch", "Mercredi", "Mercoledì", "Miércoles", "Среда", "Середа", "Środa", "Středa", "Streda" },
    /* TXT_THURSDAY */          { "Četvrtak", "Thursday", "Donnerstag", "Jeudi", "Giovedì", "Jueves", "Четверг", "Четвер", "Czwartek", "Čtvrtek", "Štvrtok" },
    /* TXT_FRIDAY */            { "Petak", "Friday", "Freitag", "Vendredi", "Venerdì", "Viernes", "Пятница", "П'ятниця", "Piątek", "Pátek", "Piatok" },
    /* TXT_SATURDAY */          { "Subota", "Saturday", "Samstag", "Samedi", "Sabato", "Sábado", "Суббота", "Субота", "Sobota", "Sobota", "Sobota" },
    /* TXT_SUNDAY */            { "Nedjelja", "Sunday", "Sonntag", "Dimanche", "Domenica", "Domingo", "Воскресенье", "Неділя", "Niedziela", "Neděle", "Nedeľa" },
    /* TXT_MONTH_JAN */         { "Januar", "January", "Januar", "Janvier", "Gennaio", "Enero", "Январь", "Січень", "Styczeń", "Leden", "Január" },
    /* TXT_MONTH_FEB */         { "Februar", "February", "Februar", "Février", "Febbraio", "Febrero", "Февраль", "Лютий", "Luty", "Únor", "Február" },
    /* TXT_MONTH_MAR */         { "Mart", "March", "März", "Mars", "Marzo", "Marzo", "Март", "Березень", "Marzec", "Březen", "Marec" },
    /* TXT_MONTH_APR */         { "April", "April", "April", "Avril", "Aprile", "Abril", "Апрель", "Квітень", "Kwiecień", "Duben", "Apríl" },
    /* TXT_MONTH_MAY */         { "Maj", "May", "Mai", "Mai", "Maggio", "Mayo", "Май", "Травень", "Maj", "Květen", "Máj" },
    /* TXT_MONTH_JUN */         { "Juni", "June", "Juni", "Juin", "Giugno", "Junio", "Июнь", "Червень", "Czerwiec", "Červen", "Jún" },
    /* TXT_MONTH_JUL */         { "Juli", "July", "Juli", "Juillet", "Luglio", "Julio", "Июль", "Липень", "Lipiec", "Červenec", "Júl" },
    /* TXT_MONTH_AUG */         { "August", "August", "August", "Août", "Agosto", "Agosto", "Август", "Серпень", "Sierpień", "Srpen", "August" },
    /* TXT_MONTH_SEP */         { "Septembar", "September", "September", "Septembre", "Settembre", "Septiembre", "Сентябрь", "Вересень", "Wrzesień", "Září", "September" },
    /* TXT_MONTH_OCT */         { "Oktobar", "October", "Oktober", "Octobre", "Ottobre", "Octubre", "Октябрь", "Жовтень", "Październik", "Říjen", "Október" },
    /* TXT_MONTH_NOV */         { "Novembar", "November", "November", "Novembre", "Novembre", "Noviembre", "Ноябрь", "Листопад", "Listopad", "Listopad", "November" },
    /* TXT_MONTH_DEC */         { "Decembar", "December", "Dezember", "Décembre", "Dicembre", "Diciembre", "Декабрь", "Грудень", "Grudzień", "Prosinec", "December" },
    /* TXT_LANGUAGE_NAME */     { "Bosanski", "English", "Deutsch", "Français", "Italiano", "Español", "Русский", "Українська", "Polski", "Čeština", "Slovenčina" },
    /* TXT_CURTAINS */          { "ZAVJESE", "CURTAINS", "VORHÄNGE", "RIDEAUX", "TENDE", "CORTINAS", "ШТОРЫ", "ШТОРИ", "ZASŁONY", "ZÁVĚSY", "ZÁVESY" },
    /* TXT_TV */                { "TV", "TV", "FERNSEHER", "TÉLÉ", "TV", "TELEVISIÓN", "ТВ", "ТБ", "TELEWIZOR", "TELEVIZE", "TELEVÍZOR" },
    /* TXT_HOURS */             { "Sati", "Hours", "Stunden", "Heures", "Ore", "Horas", "Часы", "Години", "Godziny", "Hodiny", "Hodiny" },
    /* TXT_MINUTES */           { "Minute", "Minutes", "Minuten", "Minutes", "Minuti", "Minutos", "Минуты", "Хвилини", "Minuty", "Minuty", "Minúty" },
    /* TXT_RESET */             { "PONIŠTI", "RESET", "ZURÜCKSETZEN", "RÉINITIALISER", "RIPRISTINA", "REINICIAR", "СБРОС", "СКИНУТИ", "RESETUJ", "RESETOVAT", "RESETOVAŤ" },
    /* TXT_ACTIVATE */          { "AKTIVIRAJ", "ACTIVATE", "AKTIVIEREN", "ACTIVER", "ATTIVA", "ACTIVAR", "АКТИВИРОВАТЬ", "АКТИВУВАТИ", "AKTYWUJ", "AKTIVOVAT", "AKTIVOVAŤ" },
    /* TXT_ALARM_TIME */        { "VRIJEME ALARMA", "ALARM TIME", "ALARMZEIT", "HEURE DE L'ALARME", "ORA SVEGLIA", "HORA DE ALARMA", "ВРЕМЯ БУДИЛЬНИКА", "ЧАС БУДИЛЬНИКА", "CZAS ALARMU", "ČAS ALARMU", "ČAS ALARMU" },
    /* TXT_MUSIC */             { "MUZIKA", "MUSIC", "MUSIK", "MUSIQUE", "MUSICA", "MÚSICA", "МУЗЫКА", "МУЗИКА", "MUZYKA", "HUDBA", "HUDBA" },
    /* TXT_LIGHT */             { "SVJETLO", "LIGHT", "LICHT", "LUMIÈRE", "LUCE", "LUZ", "СВЕТ", "СВІТЛО", "ŚWIATŁO", "SVĚTLO", "SVETLO" },
    /* TXT_BED */               { "SPAVAĆA", "BED", "BETT", "LIT", "LETTO", "CAMA", "КРОВАТЬ", "ЛІЖКО", "ŁÓŻKO", "POSTEL", "POSTEĽ" },
    /* TXT_HALLWAY */           { "HODNIK", "HALLWAY", "FLUR", "COUULOIR", "CORRIDOIO", "PASILLO", "КОРИДОР", "КОРИДОР", "KORYTARZ", "CHODBA", "CHODBA" },
    /* TXT_WC */                { "WC", "WC", "WC", "TOILETTES", "BAGNO", "BAÑO", "ТУАЛЕТ", "ТУАЛЕТ", "TOALETA", "TOALETA", "TOALETA" },
    /* TXT_TERRACE */           { "TERASA", "TERRACE", "TERRASSE", "TERRASSE", "TERRAZZA", "TERRAZA", "ТЕРРАСА", "ТЕРАСА", "TARAS", "TERASA", "TERASA" },
    /* TXT_KITCHEN */           { "KUHINJA", "KITCHEN", "KÜCHE", "CUISINE", "CUCINA", "COCINA", "КУХНЯ", "КУХНЯ", "KUCHNIA", "KUCHYNĚ", "KUCHYŇA" },
    /* TXT_STAIRS */            { "STEP.", "STAIRS", "TREPPE", "ESCALIERS", "SCALE", "ESCALERAS", "ЛЕСТНИЦА", "СХОДИ", "SCHODY", "SCHODY", "SCHODY" },
    /* TXT_LIVING_R_1 */        { "DNEVNI B. 1", "LIVING R. 1", "WOHNZ. 1", "SALON 1", "SOGGIORNO 1", "SALA 1", "ГОСТИНАЯ 1", "ВІТАЛЬНЯ 1", "SALON 1", "OBÝVÁK 1", "OBÝVAČKA 1" },
    /* TXT_LIVING_R_2 */        { "DNEVNI B. 2", "LIVING R. 2", "WOHNZ. 2", "SALON 2", "SOGGIORNO 2", "SALA 2", "ГОСТИНАЯ 2", "ВІТАЛЬНЯ 2", "SALON 2", "OBÝVÁK 2", "OBÝVAČKA 2" },
    /* TXT_LIVING_R_3 */        { "DNEVNI B. 3", "LIVING R. 3", "WOHNZ. 3", "SALON 3", "SOGGIORNO 3", "SALA 3", "ГОСТИНАЯ 3", "ВІТАЛЬНЯ 3", "SALON 3", "OBÝVÁK 3", "OBÝVAČKA 3" },
    /* TXT_TERR_L */            { "TER. L.", "TERR. L.", "TER. L.", "TER. G.", "TER. S.", "TER. IZQ.", "ТЕР. Л.", "ТЕР. Л.", "TAR. L.", "TER. L.", "TER. L." },
    /* TXT_TERR_R */            { "TER. D.", "TERR. R.", "TER. R.", "TER. D.", "TER. D.", "TER. DER.", "ТЕР. П.", "ТЕР. П.", "TAR. P.", "TER. P.", "TER. P." },
    /* TXT_SIDE_WIN */          { "BOČ. PRO.", "SIDE WIN.", "SEITENF.", "FEN. LAT.", "FIN. LAT.", "VENT. LAT.", "БОК. ОКНО", "БІЧНЕ ВІКНО", "OKNO BOCZNE", "BOČNÍ OKNO", "BOČNÉ OKNO" },
    /* TXT_WINDOWS */           { "PROZORI", "WINDOWS", "FENSTER", "FENÊTRES", "FINESTRE", "VENTANAS", "ОКНА", "ВІКНА", "OKNA", "OKNA", "OKNÁ" },
    /* TXT_FACADE */            { "FASADA", "FACADE", "FASSADE", "FAÇADE", "FACCIATA", "FACHADA", "ФАСАД", "ФАСАД", "FASADA", "FASÁDA", "FASÁDA" },
    /* TXT_BEDROOM */           { "SPAVAĆA", "BEDROOM", "SCHLAFZIMMER", "CHAMBRE", "CAMERA", "DORMITORIO", "СПАЛЬНЯ", "СПАЛЬНЯ", "SYPIALNIA", "LOŽNICE", "SPÁLŇA" },
    /* TXT_BEDROOM_1 */         { "SPAVAĆA 1", "BEDROOM 1", "SCHLAFZ. 1", "CHAMBRE 1", "CAMERA 1", "DORMITORIO 1", "СПАЛЬНЯ 1", "СПАЛЬНЯ 1", "SYPIALNIA 1", "LOŽNICE 1", "SPÁLŇA 1" },
    /* TXT_BEDROOM_2 */         { "SPAVAĆA 2", "BEDROOM 2", "SCHLAFZ. 2", "CHAMBRE 2", "CAMERA 2", "DORMITORIO 2", "СПАЛЬНЯ 2", "СПАЛЬНЯ 2", "SYPIALNIA 2", "LOŽNICE 2", "SPÁLŇA 2" },
    /* TXT_TERRACE_1 */         { "TERASA 1", "TERRACE 1", "TERRASSE 1", "TERRASSE 1", "TERRAZZA 1", "TERRAZA 1", "ТЕРРАСА 1", "ТЕРАСА 1", "TARAS 1", "TERASA 1", "TERASA 1" },
    /* TXT_TERRACE_2 */         { "TERASA 2", "TERRACE 2", "TERRASSE 2", "TERRASSE 2", "TERRAZZA 2", "TERRAZA 2", "ТЕРРАСА 2", "ТЕРАСА 2", "TARAS 2", "TERASA 2", "TERASA 2" },
    /* TXT_LIVING_ROOM_1 */     { "DNEVNA 1", "LIVING R. 1", "WOHNZ. 1", "SALON 1", "SOGGIORNO 1", "SALA 1", "ГОСТИНАЯ 1", "ВІТАЛЬНЯ 1", "SALON 1", "OBÝVÁK 1", "OBÝVAČKA 1" },
    /* TXT_LIVING_ROOM_2 */     { "DNEVNA 2", "LIVING R. 2", "WOHNZ. 2", "SALON 2", "SOGGIORNO 2", "SALA 2", "ГОСТИНАЯ 2", "ВІТАЛЬНЯ 2", "SALON 2", "OBÝVÁK 2", "OBÝVAČKA 2" },
    /* TXT_POOL_1 */            { "BAZEN 1", "POOL 1", "POOL 1", "PISCINE 1", "PISCINA 1", "PISCINA 1", "БАССЕЙН 1", "БАСЕЙН 1", "BASEN 1", "BAZÉN 1", "BAZÉN 1" },
    /* TXT_POOL_2 */            { "BAZEN 2", "POOL 2", "POOL 2", "PISCINE 2", "PISCINA 2", "PISCINA 2", "БАССЕЙН 2", "БАСЕЙН 2", "BASEN 2", "BAZÉN 2", "BAZÉN 2" },
    /* TXT_POOL_3 */            { "BAZEN 3", "POOL 3", "POOL 3", "PISCINE 3", "PISCINA 3", "PISCINA 3", "БАССЕЙН 3", "БАСЕЙН 3", "BASEN 3", "BAZÉN 3", "BAZÉN 3" },
    /* TXT_LEFT */              { "LIJEVE", "LEFT", "LINKS", "GAUCHE", "SINISTRA", "IZQUIERDA", "ЛЕВЫЕ", "ЛІВІ", "LEWE", "LEVÉ", "ĽAVÉ" },
    /* TXT_MIDDLE */            { "SREDNJE", "MIDDLE", "MITTE", "MILIEU", "CENTRO", "MEDIO", "СРЕДНИЕ", "СЕРЕДНІ", "ŚRODKOWE", "STŘEDNÍ", "STREDNÉ" },
    /* TXT_RIGHT */             { "DESNE", "RIGHT", "RECHTS", "DROITE", "DESTRA", "DERECHA", "ПРАВЫЕ", "ПРАВІ", "PRAWE", "PRAVÉ", "PRAVÉ" },
    /* TXT_LIVING */            { "DNEVNI", "LIVING", "WOHN-", "SALON", "SOGGIORNO", "SALA", "ГОСТИНАЯ", "ВІТАЛЬНЯ", "SALON", "OBÝVACÍ", "OBÝVACIA" }
};
/**
 * @brief Tabela sa prevodima za dane u sedmici, za dropdown meni.
 */
static const char* _acContent[LANGUAGE_COUNT][7] = {
    { "PON", "UTO", "SRI", "ČET", "PET", "SUB", "NED" }, // BSHC
    { "MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN" }, // ENG
    { "MO", "DI", "MI", "DO", "FR", "SA", "SO" },        // GER
    { "LUN", "MAR", "MER", "JEU", "VEN", "SAM", "DIM" }, // FRA
    { "LUN", "MAR", "MER", "GIO", "VEN", "SAB", "DOM" }, // ITA
    { "LUN", "MAR", "MIÉ", "JUE", "VIE", "SÁB", "DOM" }, // SPA
    { "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС" },        // RUS
    { "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "НД" },        // UKR
    { "PON", "WT", "ŚR", "CZW", "PT", "SOB", "ND" },     // POL
    { "PO", "ÚT", "ST", "ČT", "PÁ", "SO", "NE" },        // CZE
    { "PO", "UT", "ST", "ŠT", "PI", "SO", "NE" }         // SLO
};

#endif // __TRANSLATIONS_H__
