/**
 ******************************************************************************
 * Description 		: LuxNET Protocol header
 ******************************************************************************
 *
 *
 ******************************************************************************
 */
typedef enum {
    BINARY_GET          = 1,    // vraća stanje adresiranog izlaza
    BINARY_SET          = 2,    // podešava novo stanje adresiranog izlaza
    BINARY_RESET        = 3,    // softverski restart uređaja 
    BINARY_SETUP        = 4,    // nema trenutno postavki ovog uređaja nije isključeno da budu timer funkcija ili toggle ili kombinacija
    // ostavi prostora za dopune
    DIMMER_GET          = 8,    // vraća cijelu strukturu adresiranog kanala dimera
    DIMMER_SET          = 9,    // podešava novu vrijednost jednog kanala dimera
    DIMMER_RESET        = 10,   // softverski restart jednog kanala dimera
    DIMMER_SETUP        = 11,   // cijela nova dimmer struktura parametara za jedan kanal dimera
    DIMMER_RESTART      = 12,   // softverski restart modula dimera čiji je adresirani kanal 
    // ostavi prostora za dopune
    JALOUSIE_GET        = 16,   // vraća stanje i podešeni timeout adresirane žaluzine
    JALOUSIE_SET        = 17,   // podešava novo stanje adresirane žaluzine
    JALOUSIE_RESET      = 18,   // softverski restart modula žaluzina čiji je adresirani izlaza žaluzine 
    JALOUSIE_SETUP      = 19,   // podešava timout za adresiranu žaluzinu
    // ostavi prostora za dopune
    RGB_GET             = 24,	// vraća strukturu adresiranog registrovanog daljinskog upravljača, klijent uzima šta mu treba
    RGB_SET             = 25,   // podesi novu vrijednost adresiranog milight registrovanog daljinskog upravljača 
    RGB_RESET           = 26,   // softverski restart esp-m2 milight kontrolera 
    RGB_SETUP           = 27,   // cijela struktura ili više uzastopnih za podešavanje... treba definisat setup strukturu
    RGB_INFO            = 28,   // promjena sa web interfejsa... uređaji koji imaju lokalne izmjene imaju info kanal... treba definisat info strukturu
    // ostavi prostora za dopune
    PWM_GET             = 32,
    PWM_SET             = 33,
    PWM_RESET           = 34,   // softverski restart uređaja
    PWM_SETUP           = 35,
    // ostavi prostora za dopune
    THERMOSTAT_GET      = 40,   // vraća cijelu termostat strukturu adresiranog termostata, klijent uzima šta mu treba
    THERMOSTAT_SET      = 41,   // podesi novu zadanu temperaturu adresiranog termostata
    THERMOSTAT_RESET    = 42,   // reinicijalizacija termostat aplikacije ne cijelog kontrolera, prinudni prolazak kroz init funkciju termostata
    THERMOSTAT_SETUP    = 43,   // cijela nova termostat struktura parametara....  treba definisat termostat strukturu
    THERMOSTAT_INFO     = 44,   // izmjerena nova temperature senzora, promjenjena zadana temeratura, termostat isključen.... treba definisat info strukturu
    // ostavi prostora za dopune
    CUSTOM              = 48,
    QR_REQUEST			= 49,	// zahtjev za qr kod sa uSD kartice ili upis novog qr koda
	FIRMWARE_REQUEST    = 50,   // zahtjev za update firmware-a sa dragona i uSD kartice 
    FIRMWARE_UPDATE     = 51,   // upit i provjera verzije, backup trenutne verzije, formatiranje i početak transfera
    TIME_INFO           = 52,   // svi koje interesuje tačno vrijeme i datum imaju registrovan ovaj tip
    CONTEROLLER_GET     = 53,   // uzmi cijelu strukturu kontrolera sve pinove sve registre
    CONTROLLER_SET      = 54,   // upiši cijelu strukturu kontrolera i reinicijalizuj 
    SCENE_CONTROL       = 55,   // Poruka za sinhronizaciju aktivacije scena između displeja.
    DIGITAL_INPUT_EVENT = 56    // Poruka koju šalje modul sa ulazima kada detektuje promjenu stanja.
} tf_types_t;
