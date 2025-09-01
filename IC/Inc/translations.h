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
#include "display.h" // Uključujemo display.h da bismo vidjeli enum TextID

// Maksimalan broj unosa po jednoj ikoni
#define MAX_ICON_DESCRIPTIONS  20
// Maksimalan broj stringova po jednom unosu
#define MAX_DESCRIPTION_STRINGS 2

/**
 * @brief Trodimenzionalni niz za mapiranje ikona i tekstualnih opisa.
 * @note  Struktura je: [ICON_ID][REDI_OPISA][PRIMARY/SECONDARY].
 */
static const TextID icon_strings[ICON_COUNT][MAX_ICON_DESCRIPTIONS][MAX_DESCRIPTION_STRINGS] = {
    [ICON_CHANDELIER] = { // LUSTER
        { TXT_LUSTER, TXT_GLAVNI_SECONDARY },
        { TXT_LUSTER, TXT_AMBIJENT_SECONDARY },
        { TXT_LUSTER, TXT_TRPEZARIJA_SECONDARY },
        { TXT_LUSTER, TXT_DNEVNA_SOBA_SECONDARY },
    },
    [ICON_SPOT] = { // SPOT
        { TXT_SPOT, TXT_LIJEVI_SECONDARY },
        { TXT_SPOT, TXT_DESNI_SECONDARY },
        { TXT_SPOT, TXT_CENTRALNI_SECONDARY },
        { TXT_SPOT, TXT_PREDNJI_SECONDARY },
        { TXT_SPOT, TXT_ZADNJI_SECONDARY },
        { TXT_SPOT, TXT_HODNIK_SECONDARY },
        { TXT_SPOT, TXT_KUHINJA_SECONDARY },
    },
    [ICON_HANGING] = { // VISILICA
        { TXT_VISILICA, TXT_IZNAD_SANKA_SECONDARY },
        { TXT_VISILICA, TXT_IZNAD_STOLA_SECONDARY },
        { TXT_VISILICA, TXT_PORED_KREVETA_1_SECONDARY },
        { TXT_VISILICA, TXT_PORED_KREVETA_2_SECONDARY },
    },
    [ICON_CEILING_LIGHT] = { // PLAFONJERA
        { TXT_PLAFONJERA, TXT_GLAVNA_SECONDARY },
        { TXT_PLAFONJERA, TXT_SOBA_1_SECONDARY },
        { TXT_PLAFONJERA, TXT_SOBA_2_SECONDARY },
        { TXT_PLAFONJERA, TXT_KUPATILO_SECONDARY },
        { TXT_PLAFONJERA, TXT_HODNIK_SECONDARY },
    },
    [ICON_WALL_LIGHT] = { // ZIDNA
        { TXT_ZIDNA, TXT_LIJEVA_SECONDARY },
        { TXT_ZIDNA, TXT_DESNA_SECONDARY },
        { TXT_ZIDNA, TXT_GORE_SECONDARY },
        { TXT_ZIDNA, TXT_DOLE_SECONDARY },
        { TXT_ZIDNA, TXT_CITANJE_SECONDARY },
        { TXT_ZIDNA, TXT_AMBIJENT_SECONDARY },
        { TXT_ZIDNA, TXT_OGLEDALO_SECONDARY },
    },
    [ICON_PICTURE_LIGHT] = { // SLIKA
        { TXT_SLIKA, TXT_DUMMY },
    },
    [ICON_FLOOR_LAMP] = { // PODNA
        { TXT_PODNA, TXT_UGAO_SECONDARY },
        { TXT_PODNA, TXT_PORED_FOTELJE_SECONDARY },
    },
    [ICON_TABLE_LAMP] = { // STOLNA
        { TXT_STOLNA, TXT_RADNI_STO_SECONDARY },
        { TXT_STOLNA, TXT_NOCNA_1_SECONDARY },
        { TXT_STOLNA, TXT_NOCNA_2_SECONDARY },
    },
    [ICON_LED_STRIP] = { // LED TRAKA
        { TXT_LED_TRAKA, TXT_ISPOD_ELEMENTA_SECONDARY },
        { TXT_LED_TRAKA, TXT_IZNAD_ELEMENTA_SECONDARY },
        { TXT_LED_TRAKA, TXT_ORMAR_SECONDARY },
        { TXT_LED_TRAKA, TXT_STEPENICE_SECONDARY },
        { TXT_LED_TRAKA, TXT_TV_SECONDARY },
        { TXT_LED_TRAKA, TXT_AMBIJENT_SECONDARY },
    },
    [ICON_VENTILATOR_ICON] = { // VENTILATOR SA SVJETLOM
        { TXT_VENTILATOR_IKONA, TXT_DUMMY },
    },
    [ICON_FACADE_LIGHT] = { // FASADA
        { TXT_FASADA, TXT_ULAZ_SECONDARY },
        { TXT_FASADA, TXT_TERASA_SECONDARY },
        { TXT_FASADA, TXT_BALKON_SECONDARY },
        { TXT_FASADA, TXT_LIJEVA_SECONDARY },
        { TXT_FASADA, TXT_DESNA_SECONDARY },
        { TXT_FASADA, TXT_ZADNJA_SECONDARY },
    },
    [ICON_PATH_LIGHT] = { // STAZA
        { TXT_STAZA, TXT_PRILAZ_SECONDARY },
        { TXT_STAZA, TXT_DVORISTE_SECONDARY },
    },
    [ICON_REFLECTOR] = { // REFLEKTOR
        { TXT_REFLEKTOR, TXT_DRVO_SECONDARY },
        { TXT_REFLEKTOR, TXT_DVORISTE_SECONDARY },
    },
};

static const char* language_strings[TEXT_COUNT][LANGUAGE_COUNT] = {
    // Ažurirana lista sa novim tekstovima
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
    /* TXT_LANGUAGE_NAME */             { "Bos/Cro/Srb/Mon", "English", "German", "French", "Italian", "Spanish", "Russian", "Ukrainian", "Polish", "Czech", "Slovak" },
    /* TXT_LUSTER */                    { "LUSTER", "CHANDELIER", "KRONLEUCHTER", "LUSTRE", "LAMPADARIO", "ARAÑA", "ЛЮСТРА", "ЛЮСТРА", "ŻYRANDOL", "LUSTA", "LUSTER" },
    /* TXT_SPOT */                      { "SPOT", "SPOT", "SPOT", "SPOT", "SPOT", "FOCO", "ТОЧЕЧНЫЙ", "ТОЧКОВИЙ", "REFLEKTOR", "BOD", "BODOVKA" },
    /* TXT_VISILICA */                  { "VISILICA", "HANGING", "HÄNGEND", "SUSPENSION", "SOSPENSIONE", "COLGANTE", "ПОДВЕСНОЙ", "ПІДВІСНИЙ", "WISZĄCA", "ZÁVĚSNÉ", "ZÁVESNÉ" },
    /* TXT_PLAFONJERA */                { "PLAFONJERA", "CEILING", "DECKENLEUCHTE", "PLAFONNIER", "PLAFONIERA", "PLAFÓN", "ПОТОЛОЧНЫЙ", "СТЕЛЬОВИЙ", "PLAFON", "STROP", "STROP" },
    /* TXT_ZIDNA */                     { "ZIDNA", "WALL", "WAND", "MURALE", "PARETE", "PARED", "СТЕННОЙ", "СТІННИЙ", "ŚCIENNA", "NÁSTĚNNÉ", "NÁSTENNÉ" },
    /* TXT_SLIKA */                     { "SLIKA", "PICTURE", "BILD", "IMAGE", "QUADRO", "IMAGEN", "КАРТИНА", "КАРТИНА", "OBRAZ", "OBRAZ", "OBRAZ" },
    /* TXT_PODNA */                     { "PODNA", "FLOOR", "BODEN", "SOL", "TERRA", "SUELO", "НАПОЛЬНЫЙ", "ПІДЛОГОВИЙ", "PODŁOGOWA", "PODLAHOVÉ", "PODLAHOVÉ" },
    /* TXT_STOLNA */                    { "STOLNA", "TABLE", "TISCH", "TABLE", "TAVOLO", "MESA", "НАСТОЛЬНЫЙ", "НАСТІЛЬНИЙ", "STOŁOWA", "STOLNÍ", "STOLOVÉ" },
    /* TXT_LED_TRAKA */                 { "LED TRAKA", "LED STRIP", "LED-STREIFEN", "BANDE LED", "STRISCIA LED", "TIRA LED", "ЛЕНТА СИД", "СТРІЧКА СІД", "TAŚMA LED", "LED PÁSEK", "LED PÁSIK" },
    /* TXT_VENTILATOR_IKONA */          { "VENTILATOR", "FAN", "LÜFTER", "VENTILATEUR", "VENTOLA", "VENTILADOR", "ВЕНТИЛЯТОР", "ВЕНТИЛЯТОР", "WENTYLATOR", "VENTILÁTOR", "VENTILÁTOR" },
    /* TXT_FASADA */                    { "FASADA", "FACADE", "FASSADE", "FAÇADE", "FACCIATA", "FACHADA", "ФАСАД", "ФАСАД", "FASADA", "FASÁDA", "FASÁDA" },
    /* TXT_STAZA */                     { "STAZA", "PATH", "WEG", "SENTIER", "SENTIERO", "CAMINO", "ДОРОЖКА", "ДОРІЖКА", "ŚCIEŻKA", "STEZKA", "STEZKA" },
    /* TXT_REFLEKTOR */                 { "REFLEKTOR", "REFLECTOR", "REFLEKTOR", "PROJECTEUR", "RIFLETTORE", "REFLECTOR", "ПРОЖЕКТОР", "ПРОЖЕКТОР", "REFLEKTOR", "REFLEKTOR", "REFLEKTOR" },
    /* TXT_GLAVNI_SECONDARY */          { "GLAVNI", "MAIN", "HAUPT", "PRINCIPAL", "PRINCIPALE", "PRINCIPAL", "ГЛАВНЫЙ", "ГОЛОВНИЙ", "GŁÓWNY", "HLAVNÍ", "HLAVNÝ" },
    /* TXT_AMBIJENT_SECONDARY */        { "AMBIJENT", "AMBIENT", "AMBIENTE", "AMBIANTE", "AMBIENTE", "AMBIENTE", "АТМОСФЕРНЫЙ", "АТМОСФЕРНИЙ", "NASTRÓJ", "NÁLADOVÉ", "NÁLADOVÉ" },
    /* TXT_TRPEZARIJA_SECONDARY */      { "TRPEZARIJA", "DINING ROOM", "ESSZIMMER", "SALLE À MANGER", "SALA DA PRANZO", "COMEDOR", "СТОЛОВАЯ", "ЇДАЛЬНЯ", "JADALNIA", "JÍDELNA", "JEDÁLEŇ" },
    /* TXT_DNEVNA_SOBA_SECONDARY */     { "DNEVNA SOBA", "LIVING ROOM", "WOHNZIMMER", "SALON", "SOGGIORNO", "SALA DE ESTAR", "ГОСТИНАЯ", "ВІТАЛЬНЯ", "POKÓJ DZIENNY", "OBÝVACÍ POKOJ", "OBÝVACIA IZBA" },
    /* TXT_LIJEVI_SECONDARY */          { "LIJEVI", "LEFT", "LINKS", "GAUCHE", "SINISTRO", "IZQUIERDA", "ЛЕВЫЙ", "ЛІВИЙ", "LEWY", "LEVÝ", "ĽAVÝ" },
    /* TXT_DESNI_SECONDARY */           { "DESNI", "RIGHT", "RECHTS", "DROITE", "DESTRO", "DERECHA", "ПРАВЫЙ", "ПРАВИЙ", "PRAWY", "PRAVÝ", "PRAVÝ" },
    /* TXT_CENTRALNI_SECONDARY */       { "CENTRALNI", "CENTRAL", "ZENTRAL", "CENTRAL", "CENTRALE", "CENTRAL", "ЦЕНТРАЛЬНЫЙ", "ЦЕНТРАЛЬНИЙ", "CENTRALNY", "CENTRÁLNÍ", "CENTRÁLNY" },
    /* TXT_PREDNJI_SECONDARY */         { "PREDNJI", "FRONT", "VORDERER", "AVANT", "ANTERIORE", "FRONTAL", "ПЕРЕДНИЙ", "ПЕРЕДНІЙ", "PRZEDNI", "PŘEDNÍ", "PREDNÝ" },
    /* TXT_ZADNJI_SECONDARY */          { "ZADNJI", "BACK", "HINTERER", "ARRIÈRE", "POSTERIORE", "TRASERO", "ЗАДНИЙ", "ЗАДНІЙ", "TYLNY", "ZADNÍ", "ZADNÝ" },
    /* TXT_HODNIK_SECONDARY */          { "HODNIK", "HALLWAY", "FLUR", "COUULOIR", "CORRIDOIO", "PASILLO", "КОРИДОР", "КОРИДОР", "KORYTARZ", "CHODBA", "CHODBA" },
    /* TXT_KUHINJA_SECONDARY */         { "KUHINJA", "KITCHEN", "KÜCHE", "CUISINE", "CUCINA", "COCINA", "КУХНЯ", "КУХНЯ", "KUCHNIA", "KUCHYNĚ", "KUCHYŇA" },
    /* TXT_CITANJE_SECONDARY */         { "ČITANJE", "READING", "LESE", "LECTURE", "LETTURA", "LECTURA", "ЧТЕНИЕ", "ЧИТАННЯ", "DO CZYTANIA", "ČTENÍ", "ČÍTANIE" },
    /* TXT_OGLEDALO_SECONDARY */        { "OGLEDALO", "MIRROR", "SPIEGEL", "MIROIR", "SPECCHIO", "ESPEJO", "ЗЕРКАЛО", "ДЗЕРКАЛО", "LUSTRO", "ZRCADLO", "ZRKADLO" },
    /* TXT_UGAO_SECONDARY */            { "UGAO", "CORNER", "ECKE", "COIN", "ANGOLO", "ESQUINA", "УГОЛ", "КУТ", "NAROŻNY", "ROH", "ROH" },
    /* TXT_PORED_FOTELJE_SECONDARY */   { "PORED FOTELJE", "BY ARMCHAIR", "NEBEN SESSEL", "PRÈS DU FAUTEUIL", "VICINO ALLA POLTRONA", "JUNTO AL SILLÓN", "У КРЕСЛА", "БІЛЯ КРІСЛА", "PRZY FOTELU", "U KŘESLA", "PRI KRESLO" },
    /* TXT_RADNI_STO_SECONDARY */       { "RADNI STO", "DESK", "SCHREIBTISCH", "BUREAU", "SCRIVANIA", "ESCRITORIO", "РАБОЧИЙ СТОЛ", "РОБОЧИЙ СТІЛ", "BIURKO", "PRACOVNÍ STŮL", "PRACOVNÝ STÔL" },
    /* TXT_NOCNA_1_SECONDARY */         { "NOĆNA 1", "NIGHT 1", "NACHT 1", "NUIT 1", "NOTTE 1", "NOCHE 1", "НОЧНОЙ 1", "НІЧНИЙ 1", "NOCNA 1", "NOČNÍ 1", "NOČNÁ 1" },
    /* TXT_NOCNA_2_SECONDARY */         { "NOĆNA 2", "NIGHT 2", "NACHT 2", "NUIT 2", "NOTTE 2", "NOCHE 2", "НОЧНОЙ 2", "НІЧНИЙ 2", "NOCNA 2", "NOČNÍ 2", "NOČNÁ 2" },
    /* TXT_ISPOD_ELEMENTA_SECONDARY */  { "ISPOD ELEMENTA", "UNDER CABINET", "UNTER SCHRANK", "SOUS L'ARMOIRE", "SOTTO L'ARMADIO", "BAJO EL GABINETE", "ПОД ШКАФОМ", "ПІД ШАФОЮ", "POD SZAFKĄ", "POD SKŘÍŇKOU", "POD SKRINKOU" },
    /* TXT_IZNAD_ELEMENTA_SECONDARY */  { "IZNAD ELEMENTA", "OVER CABINET", "ÜBER SCHRANK", "SUR L'ARMOIRE", "SOPRA L'ARMADIO", "SOBRE EL GABINETE", "НАД ШКАФОМ", "НАД ШАФОЮ", "NAD SZAFKĄ", "NAD SKŘÍŇKOU", "NAD SKRINKOU" },
    /* TXT_ORMAR_SECONDARY */           { "ORMAR", "CLOSET", "SCHRANK", "PLACARD", "ARMADIO", "ARMARIO", "ШКАФ", "ШАФА", "SZAFA", "ŠATNÍK", "ŠATNÍK" },
    /* TXT_STEPENICE_SECONDARY */       { "STEPENICE", "STAIRS", "TREPPE", "ESCALIERS", "SCALE", "ESCALERAS", "ЛЕСТНИЦА", "СХОДИ", "SCHODY", "SCHODY", "SCHODY" },
    /* TXT_TV_SECONDARY */              { "TV", "TV", "FERNSEHER", "TÉLÉ", "TV", "TELEVISIÓN", "ТВ", "ТБ", "TELEWIZOR", "TELEVIZE", "TELEVÍZOR" },
    /* TXT_ULAZ_SECONDARY */            { "ULAZ", "ENTRANCE", "EINGANG", "ENTRÉE", "INGRESSO", "ENTRADA", "ВХОД", "ВХІД", "WEJŚCIE", "VSTUP", "VSTUP" },
    /* TXT_TERASA_SECONDARY */          { "TERASA", "TERRACE", "TERRASSE", "TERRASSE", "TERRAZZA", "TERRAZA", "ТЕРРАСА", "ТЕРАСА", "TARAS", "TERASA", "TERASA" },
    /* TXT_BALKON_SECONDARY */          { "BALKON", "BALCONY", "BALKON", "BALCON", "BALCONE", "BALCÓN", "БАЛКОН", "БАЛКОН", "BALKON", "BALKÓN", "BALKÓN" },
    /* TXT_DVORISTE_SECONDARY */        { "DVORIŠTE", "YARD", "GARTEN", "JARDIN", "CORTILE", "PATIO", "ДВОР", "ДВІР", "PODWÓRKO", "DVŮR", "DVOR" },
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
