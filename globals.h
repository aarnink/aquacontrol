#ifndef HEADER_GLOBALS
#define HEADER_GLOBALS

#include "TimeLib.h"
#include "Arduino.h"

// ------------------------------------------------------------------------------------------------------------------//

const bool    sdcard                   = true;              // [true] -> schrijven naar SD card  [false] geen SDcard, dus geen logfile

const uint8_t aquaID                   = 1;                 // aquarium ID waarmee alle settings uit de mySQL database worden gehaald.
const uint8_t max_fout                 = 10;                // aantal alarmcodes achtereen dat wordt gegeven
uint8_t const alarmPin                 = 23;                // op welke arduino pin is het alarm aangesloten

uint8_t const max_planning             = 12;                // maximaal aantal planningen per 24uur, voor alle Leds opgeteld.
uint8_t const max_pins                 = 9;                 // aantal functies waarvoor een planning kan gelden 
uint8_t const sumpPomp                 = 4;                 // pomp voor het bijvullen van het aquarium met osmose water of pin=5?
uint8_t const osmosePomp               = 5;                 // osmose bijvullen
uint8_t const aantalPompen             = 4;
uint8_t const aantalLeds               = 1;
uint8_t const aantalRelais             = 4;
const uint8_t ledPins[aantalLeds]      = {22};              // dit zijn de pinnummers waarop Leds zijn aangesloten.
const uint8_t pompPins[aantalPompen]   = {6, 7, 8, 9};      
const uint8_t relaisPins[aantalRelais] = {34, 35, 36, 37};
const uint8_t tempPin                  = 11;                // Temperatuur
const uint8_t waterPin                 = 26;                // bijvul schakelaar
const uint8_t onderhoudPin             = 24;
const uint8_t topOffPin                = 30;                // Trigger en Echo (alleen 1ste pin noemen)              
const uint8_t sumpPin                  = 31;                // Trigger en Echo (alleen 1ste pin noemen)  
const uint8_t koelerPin                = 2;                 // verbonden met relais voor 12V koelventilator (doel is temperatuur gestuurd koelen)            
const uint8_t csPin                    = 53;                // chipselect pin voor de SD card

const uint8_t testModePin              = 29;                // lees schakelaar uit voor testmodus 
const uint8_t vakantiePin              = 25;                // lees schakelaar uit voor vakantiemodus

const uint8_t sdcardLed                = 28;
const uint8_t backupLed                = 27;


const int     max_buffer               = 512;               // maximale lengte van de PHP GET of POST data
const size_t  BUFFER_SIZE              = 512;       
  
const unsigned long   BAUD_RATE        = 115200;            // serial connection speed
const unsigned long   HTTP_TIMEOUT     = 20000;             // max respone time from server
const size_t          MAX_CONTENT_SIZE = 512;               // max size of the HTTP response (was 512 maar verlaagd voor MQTT)

const char    prowlkey[]               = "d9bd101ab47999c93cf7bb8ad2f39b3b54cc49a8";
const char    application[]            = "AquaControl-Eheim";
const char    apikeyParam[]            = "apikey=";
const char    applicationParam[]       = "&application=";
const char    eventParam[]             = "&event=";
const char    descriptionParam[]       = "&description=";
const uint8_t fixedContentLength       = 95;                //Remember to re-calculate this if you change any of the text fields above  (lengte van de apikey is 40 en deze zit in deze telling)
const char    prowlserver[]            = "api.prowlapp.com";
const uint8_t prowlport                = 80;
const char    path[15]                 = "/publicapi/add";

// ------------------------------------------------------------------------------------------------------------------//



typedef struct
{
  long        wifisignal;           // uitgaand  HTML GET = "w"  Blynk pin# V0
  bool        alarm_status;         // uitgaand  HTML GET = "a"  Blynk pin# V1
  bool        sdcard;               // uitgaand  HTML GET = "c"  Blynk pin# V2
  float       temperatuur;          // uitgaand  HTML GET = "t"  Blynk pin# V3
  float       osmose;               // uitgaand  HTML GET = "o"  Blynk pin# V4
  float       sump;                 // uitgaand  HTML GET = "s"  Blynk pin# V5
  bool        koeling_status;       // uitgaand  HTML GET = "k"  Blynk pin# V6
  bool        onderhoud_status;     // uitgaand  HTML GET = "m"  Blynk pin# V8
  bool        opvoerpomp;           // uitgaand  HTML GET = "p"  Blynk pin# V12
  bool        skimmer;              // uitgaand  HTML GET = "e"  Blynk pin# V13
  bool        wavemaker;            // uitgaand  HTML GET = "v"  Blynk pin# V14
  bool        vakantiestand;        // uitgaand  HTML GET = "h"  Blynk pin# V7
  bool        bijvullen;            // uitgaand  HTML GET = "b"  Blynk pin# V9
  char        debugregel[40];       // uitgaand  HTML GET = "d"  Blynk pin# V20
  char        ipaddress[16];        // uitgaand  HTML GET = "i   Blynk pin# V21     

  bool        onderhoud_btn;        // inkomend HTML GET = "r"   Blynk pin# V11 
  bool        koeling_btn;          // inkomend HTML GET = "u"   Blynk pin# V10
  bool        vakantie_btn;         // inkomend HTML GET = "x"   Blynk pin# V15
  bool        bijvullen_btn;        // inkomend HTML GET = "y"   Blynk pin# V16

} IoT_data; // http://192.168.10.210:1880/eheim/sync/?w=-71&t=25.5&o=7.8&s=-1.3&k=1&a=0&m=0&p=1&i=1&v=1&h=0&b=0&c=1



typedef struct
{
  uint8_t     pin;      // de pin waarvoor de planning geldig is;
  bool        afgerond; // indicatie of deze planning voor deze pin is afgerond;  
} Pins;


  // Define the "Event Period" data-structure 
typedef struct
{
  uint8_t      id;       // planningID  
  bool         isAan;    // the Event is only actioned if isEnabled is true
  uint8_t      uur;      // Start Hour [0-23]
  uint8_t      min;      // Start Minute [0-59]
  uint8_t      dagen;    // Dagen vd week --> 0=elke dag | 1=Zo, 2=Ma, 3=Di, 4=Woe, 5=Do, 6=Vr, 7=Za | 8=werkdagen (Ma-Vr) | 9=weekend (Za,Zo)
  unsigned int duur;     // how long the Event lasts in seconds
  unsigned int fadeIn;   // Seconds to fade-in before event, and fade-out after duration (0 = no fading)
  unsigned int fadeUit;  // Seconds to fade-uit before event, and fade-out after duration (0 = no fading)
  unsigned int maximaal; // maximale waarde in dit specifieke programma
//  bool         afgerond; // voor eenmalige actie tijdens een planningvenster, wordt dit hier bijgehouden (bijv. bij pompen) 
  Pins         pins[max_pins];     // PWM channel assigned to this event [0,1,2,3]
} Planning;

// Define Channels consisting of the PWM pin modulation level
typedef struct
  {
  char      kort[11];    // korte omschrijving voor op het LCD display  
  uint8_t   pin;         // PWM pin assigned 
  uint8_t   level;       // PWM current setting [0-255]
  uint8_t   maxLevel;    // Highest PWM level allowed for this Channel [0-255], is waarde1 in de database
  uint16_t  aantal;      // aantal is waarde2 in de database 
  uint16_t  dbLevel;     // wordt voor meerdere zaken gebruikt.
  uint16_t  correctie;   // 
  int32_t   timer;       // voor eventuele wachttijd bepalingen
  uint8_t   suggested;   // PWM seggested setting [0-255]  
  double    meting;      // gebruikt voor meetapparatuur in aqua_sensor
  bool      isActief;    // Tracking var to eliminate multiple schedules on the same day for this channel
  bool      isAan;       // false = helemaal uitgezet in de instellingen
  bool      logAan;      // moet de evt. meetwaarden worden weggeschreven.
  bool      schoonAan;   // moet de evt. meetwaarden worden weggeschreven.  
  bool      prowl;       // Prowl melding verstuurd 
  uint32_t  datumAan;    // datum waarop dit apparaat aan staat;
  uint32_t  datumUit;
} Functie;    


class functieclass
{
  public:
    functieclass();    
    void            init();
    String          urlencode(String str);
    String          urldecode(String str);
    void            strcopylen(char *target, char *source, uint8_t len);
    void            netteTijd(char* tijd);
    void            netteDatumTijd(char* tijd);
    unsigned long   Time2Sec(unsigned long nhour, uint16_t nmin, uint8_t nsec);

    void            nieuweFout   (uint8_t code);
    void            verwijderFout(uint8_t code);
                
    bool            toonFouten(char* fout);            
    uint8_t         foutenTeller; 
  private:  
    uint8_t         _fout[max_fout];   
    uint8_t         _foutOpLCD;
    unsigned char   h2_int(char c);
};


class iotclass
{
  public:
    iotclass();
    void            init();
    void            setLongWaarde(byte soort, long waarde); 
    void            setFloatWaarde(byte soort, float waarde); 
    void            setBoolWaarde(byte soort, bool waarde); 
    void            setCharWaarde(byte soort, const char *melding);
    
    bool            getBoolWaarde(byte soort);
    float           getFloatWaarde(byte soort);
    long            getLongWaarde(byte soort);
    void            getCharWaarde(byte soort, char *melding);
  private:
    IoT_data         _iotdata;
};



extern functieclass  aqua_functie;
extern iotclass      aqua_iot;


#endif
