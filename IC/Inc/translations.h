#ifndef __TRANSLATIONS_H__
#define __TRANSLATIONS_H__

/**
 * @file    translations.h
 * @author  Gemini & [Vaše Ime]
 * @brief   Centralizovana datoteka za sve prevodive stringove i mapiranja u aplikaciji.
 *
 * @note
 * Ova datoteka sadrži tabelu `language_strings` koja je ključna za
 * internacionalizaciju (i18n), kao i novu `icon_mapping_table` koja
 * fleksibilno povezuje ID izbora korisnika sa vizuelnim prikazom ikonica
 * i njihovim opisima.
 */
#include "display.h" // Uključujemo display.h da bismo vidjeli enum TextID i IconID

/**
 * @brief Glavna tabela za mapiranje ikonica svjetala.
 * @note  Ova tabela je "Jedinstveni Izvor Istine" za prikaz ikonica u meniju za podešavanje.
 * Vrijednost iz SPINBOX-a (`iconID`) se koristi kao direktan indeks u ovom nizu.
 * Svaki element niza definiše koju sličicu (bitmapu) treba prikazati i koje
 * tekstualne opise koristiti, omogućavajući višestruke opise za istu sličicu.
 * Redoslijed je preuzet iz `todo.txt` fajla.
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
 * @brief Glavna tabela sa prevodima.
 * @note  Svaki red odgovara jednom ID-ju iz `TextID` enuma, a svaka kolona jednom jeziku iz `Languages` enuma.
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
    /* TXT_ALL */                       { "SVE", "ALL", "ALLE", "TOUT", "TUTTI", "TODOS", "ВСЕ", "ВСІ", "WSZYSTKO", "VŠECHNY", "VŠETKY" },
    /* TXT_DISPLAY_CLEAN_TIME */        { "VRIJEME BRISANJA EKRANA:", "DISPLAY CLEAN TIME:", "BILDSCHIRMREINIGUNGSZEIT:", "TEMPS DE NETTOYAGE:", "TEMPO PULIZIA:", "TIEMPO DE LIMPIEZA:", "ВРЕМЯ ОЧИСТКИ:", "ЧАС ОЧИЩЕННЯ:", "CZAS CZYSZCZENIA:", "ČAS ČIŠTĚNÍ:", "ČAS ČISTENIA:" },
    /* TXT_FIRMWARE_UPDATE */           { "AŽURIRANJE...", "UPDATING...", "AKTUALISIERUNG...", "MISE À JOUR...", "AGGIORNAMENTO...", "ACTUALIZANDO...", "ОБНОВЛЕНИЕ...", "ОНОВЛЕННЯ...", "AKTUALIZACJA...", "AKTUALIZACE...", "AKTUALIZÁCIA..." },
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
