
// -------------------------------------------------------------------------------------------------
//  PROJECT AQUA CONTROL (aquaID = 1)
// -------------------------------------------------------------------------------------------------
//
//  De aansluitingen voor de Arduino MEGA
//  zie globals.h voor de PIN nummers.
// 
//  Aansluitingen
// 	Analoge Pin (20)		RTC (SDA)
//	Analoge Pin (21)		RTC (SCL)
//	5V Pin							All 5V rails
//	GND Pin							All Ground rails

//  Onderhouds codes - beslistabel
//  RELAIS 1--2--3--4   (X=AAN,O=UIT)  
//      0  X--X--X--X   
//      1  O--X--X--X   
//      2  X--O--X--X
//      3  X--X--O--X
//      4  X--X--X--O
//     12  O--O--X--X
//     13  O--X--O--X
//     14  O--X--X--O
//     23  X--O--O--X
//     24  X--O--X--O
//     34  X--X--O--O
//    123  O--O--O--X
//    234  X--O--O--O
//    255  O--O--O--O

#include "serverlib.h" 
#include "aqua_led.h"
#include "aqua_pomp.h"
#include "aqua_relais.h"
#include "aqua_sensor.h"
#include "globals.h"
#include <LiquidCrystal.h> // voor LCD aansturing 
#include "MemoryFree.h"
#include <SPI.h>
#include "SD.h"
#include "Timezone.h"                 //https://github.com/JChristensen/Timezone
#include "WiFiEsp.h"
#include "WiFiEspUdp.h"
// #include <ArduinoTrace.h>



// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
// #define DEBUG                                                 // veel informatie geven via de seriele monitor (deze regel commentaar maken om debug uit te zetten)

char           versie[]        = "4.2.2";                     // versienummer van de software

TimeChangeRule  myDST = {"CET-Z", Last, Sun, Mar, 2, +120};   // Zomertijd  Central European Time = CET = UTC + 2// 
TimeChangeRule  mySTD = {"CET-W", Last, Sun, Oct, 3, +60};    // Wintertijd Central European Time = CET = UTC + 1
Timezone        myTZ(myDST, mySTD);
TimeChangeRule  *tcr;                                         // pointer to the time change rule, use to get TZ abbrev
char            timeServer[]     = "de.pool.ntp.org";         // NTP server
tmElements_t    tm;

unsigned int    localPort        = 2390;                      // local port to listen for UDP packets 
const    int    NTP_PACKET_SIZE  = 48;                        // NTP timestamp is in the first 48 bytes of the message
const    int    UDP_TIMEOUT      = 10000;                     // timeout in miliseconds to wait for an UDP packet to arrive
byte            packetBuffer[NTP_PACKET_SIZE];                // buffer to hold incoming and outgoing packets

char            aquarium[21];                                 // reserveren voor de titel (of naam) van het aquarium in het display      
bool            verbonden        = false;                     // is er een WIFI verbinding
bool            isGewijzigd      = false;                     // zijn er wijzigingen aangebracht in de database
bool            isOnderhoud      = false;                     // is de onderhoudsfunctie ingeschakeld
bool            vakantie         = false;                     // systeem is in vakantiemodus (als true).
bool            bijvullen        = false;                     // knop in App of controller voor bijvullen is ingedrukt/aan
bool            isTestmode       = false;

uint32_t        lcdsec           = 0;                         // LCD display wordt elke hele seconden ververst
uint8_t         lcdregel         = 0;                         // voor de LCD, hier kunnen we dan de vakantiestand tonen 
uint16_t        verversDB        = 5;                         // standaard elke 5 minuten checken we de database 
int             upd_uur          = 254;                       // tijd wordt elk heel uur gesynchroniseerd met de internettijd
int             upd_server       = 254;                       // database check 
int             upd_sensor       = 254;                       // sensoren worden elke x minuten uitgelezen
int             upd_dag          = 254;                       // 1x per dag

uint8_t         lcdr2            = 1;                         // LCD rgl 2 toont de planning van de led(s) en de  doseringspomp(en) deze teller houdt bij wat
uint8_t         lcdr3            = 1;                         // er nu getoond moet worden. Voor regel 3 worden de sensoren (sump/osmose) en de relais getoond.

// alarm instellingen
int             laag             = 400;                       // frequentie van het alarm (laag en hoog)
int             hoog             = 800;

Functie         waterSwitch;                                  // de 2 knoppen voor Bijvullen (groen) en Onderhoud (geel) 

WiFiEspUDP      Udp;                                          // voor het synchroniseren van de tijd       
LiquidCrystal   lcd(48, 49, 44, 46, 45, 47);                  // LCD 

void(* resetFunc) (void) = 0;                                 // declareer de reset functie @ address 0



// ------------------------------------------------------------------------------------------------------------------//
// eigen characters voor de LCD display worden gebruikt voor de SUMP aanduiding
// ------------------------------------------------------------------------------------------------------------------//
byte up[8] = {        // definitie van eigen character voor LCD display, dit is PIJL OMHOOG
  B00100,             // elk character bestaat uit 5 dots horizontaal en 7 dots verticaal.  
  B01110,             // deze dots vul je hier in met een 1 of een 0.
  B11111,
  B00100,
  B00100,
  B00100,
  B11111,
};


byte down[8] = {      // definitie van eigen character voor LCD display, dit is PIJL OMHOOG-OMLAAG
  B11111,
  B00100,
  B00100,
  B00100,
  B11111,
  B01110,
  B00100,
};

byte norm[8] = {      // definitie van eigen character voor LCD display, dit is PIJL OMLAAG
  B00100,
  B01110,
  B11111,
  B00100,
  B11111,
  B01110,
  B00100,
};
// ------------------------------------------------------------------------------------------------------------------//


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void alarmFunctie()
{  
  uint8_t _alarm = 0;
  if ((!isOnderhoud) and (aqua_functie.foutenTeller > 0) and (!isTestmode) ) {
      tone(alarmPin, hoog);                                           // en laat het alarm maar horen!
      delay(150);
      tone(alarmPin, laag);                                           // en laat het alarm maar horen!
      _alarm = 1;
  } else {
      noTone(alarmPin); 
      _alarm = 0;
  }
  #ifdef DEBUG                                  
      Serial.print(F("status ALARM  : "));Serial.println(String(_alarm,DEC));
  #endif
}


// ------------------------------------------------------------------------------------------------------------------//
//  initialiseer de Arduino en alle sensoren en aangesloten apparaten 
// ------------------------------------------------------------------------------------------------------------------//
void setup() 
{ 
    uint8_t i = 0; 
    Serial.begin(115200); delay(50);                                  // start seriele poort voor voor debug
    Serial.println(F("setup van de AquaController"));
    aqua_iot.init();                                                  // initialiseer de variabelen voor de metingen (Cayenne)

    Serial.println(F("")); Serial.println(F("Pompen initialiseren"));    
    lcd.setCursor(13, 3); lcd.print(F("."));                          // voortgang setup clock is geinitialiseerd
    for (i = 0; i < aantalPompen; i++) {                              // initialiseer de pinnen van de Arduino voor de doseringspompen 
        if (pompPins[i] > 0) {
            pinMode(pompPins[i],   OUTPUT);                           // zie globals.h voor de pinnummers
            analogWrite(pompPins[i], 0);                              //
        }
    }
    pinMode(osmosePomp, OUTPUT);                                      // de osmose pomp (voor bijvullen van het osmosewater)
    analogWrite(osmosePomp, 0);             
     
    pinMode(sumpPomp, OUTPUT);                                        // de sumppomp vult water bij vanuit de osmose voorraad indien nodig.
    analogWrite(sumpPomp, 0);

  
    Serial.println(F("")); Serial.println(F("Leds initialiseren"));      
    for (i = 0; i < aantalLeds; i++) {                                // initialiseer de pinnen van de Arduino voor de Led(s)      
        if (ledPins[i] > 0) {                                         // zie globals.h voor de pinnummers
            pinMode(ledPins[i],   OUTPUT);            
            analogWrite(ledPins[i], 0);
        }
    }

    Serial.println(F("")); Serial.println(F("Relais initialiseren"));      
    for (uint8_t i = 0; i < aantalRelais; i++) {                      // initialiseer de pinnen van de Arduino voor de Relais
        if (relaisPins[i] > 0) {                                      // zie globals.h voor de pinnummers
            pinMode(relaisPins[i], OUTPUT);
            analogWrite(relaisPins[i], 255);                          // direct naar hoog, zodat de relais 220V doorlaat
        }
    }

    pinMode(testModePin, INPUT);
  
    lcd.begin(20, 4);                                                 // start de LCD bibliotheek voor 20 characters en 4 regels
    lcd.createChar(0, up);                                            // zet eigen characters
    lcd.createChar(1, norm);
    lcd.createChar(2, down);
    lcd.clear();                                                      // opstart tekst op de LCD
    lcd.setCursor(0, 0);                                              
    lcd.print(F("     AquaControl    "));
    lcd.setCursor(0, 1);
    uint8_t p = (20 - strlen(versie) ) / 2;                           // centreren van het versienummer op het scherm
    for (i = 0; i < p; i++) {
        lcd.print(" ");      
    }
    lcd.print("v"+String(versie));
    lcd.setCursor(0, 3);
    lcd.print(F("initialiseren"));
        
    delay(250);                                                       // wachten tot het systeem zeker klaar is
    lcd.setCursor(14, 3); lcd.print(F("."));                          // toon voortgang
    pinMode(sdcardLed, OUTPUT);                                       // Led toont dat de SDcard actief is
    pinMode(backupLed, OUTPUT);                                       // led toont dat er een backup van de configuratie in gebruik is (en er dus geen database verbinding is)
    pinMode(csPin,     OUTPUT);                                       // zie globals.h voor de pinnummers
    i = 0;          
    while ((!SD.begin(csPin))  and (i < 3))  {                        // Initialiseer SD library
         Serial.println(F("Fout, SDcard bibliotheek niet geinitialiseerd"));
         delay(500); i++;                      
    }   

    lcd.setCursor(15, 3); lcd.print(F("."));                          // toon voortgang op LCD
    aqua_led.resetLeds();
    pinMode(waterPin, INPUT);                                         // initialiseer de knop voor automatisch water toevoegen (bijvullen)
    pinMode(onderhoudPin, INPUT);                                     // initialiseer de knop voor onderhoud

    if (!LeesServerInstellingen(true)) {                              // Lees de instellingen van de apparaten uit de SQL database, eerste keer opstarten, dan met true als parameter
         lcd.setCursor(0, 2);
         lcd.print(F("! ->Fatale Fout<- !"));
    }

    lcd.setCursor(16, 3); lcd.print(F("."));                          // toon voortgang
    delay(250);
    Serial.println(F("")); Serial.println(F("Datum en tijd instellen"));
    Udp.begin(localPort);                                             // 
    delay(250);
    SyncNTPTijd();                                                    // sychroniseer de tijd met internet server
    aqua_led.zelfTest(25);                                            // test de led(s) door van 0 naar 255 en weer naar 0 aan te sturen, vertraging tussen de stappen is 25 ms

    lcd.setCursor(17, 3); lcd.print(F("."));            
    
    pinMode(vakantiePin, INPUT);
    vakantie      = ( (digitalRead(vakantiePin)) or (aqua_iot.getBoolWaarde('x')) );
    aqua_iot.setBoolWaarde('h', vakantie);
    aqua_iot.setFloatWaarde('o',aqua_topoff.meet(false)); 
    aqua_iot.setFloatWaarde('s',aqua_sump.meet()); 

    lcd.setCursor(18, 3); lcd.print(F("."));                          // toon voortgang

    delay(250);
    verversLCD();

    lcd.setCursor(19, 3); lcd.print(F("."));                          // toon voortgang
    server.logDebug(0, "setup()", versie, 0,0);
    server.PROWLmelding(0 ,String("versie:"+String(versie)).c_str(), 0, 0); // stuur bericht dat het systeem is opgestart 
    aqua_iot.setCharWaarde('d', String("opgestart: versie "+String(versie)).c_str());
    server.synchroniseer_IoT();
}


// ------------------------------------------------------------------------------------------------------------------//
// In de main loop worden de volgende stappen achter elkaar uitgevoerd
//
//   0. synchroneer de tijd met internet elk heel uur (noodzakelijk ivm zomer- en wintertijd)
//   1. controleer de voorraad osmose en waarschuw als dit te weinig is, vul eventueel ook automatisch aan
//   2. controleer de sump, als de waterstand te laag is, vul aan met osmose. Sla alarm als teveel of teweinig water 
//   3. controleer alarm / de systeem tijd (werk ook de uptime bij)
//   4. voer planning uit van de : Leds
//   5. voer planning uit van de : Doseringspompen
//   6. voer planning uit van de : Relais (hieraan zijn de 220v stopcontacten aangesloten)
//   7. controleer temperatuur en zet evt. ventilatoren aan of uit.
//   8. controleer of er wijzigingen zijn in de planning of functies (lees database instellingen opnieuw)
//   9. werk het LCD scherm netjes bij
// ------------------------------------------------------------------------------------------------------------------//

void loop() 
{  
  
   if (hour() != upd_uur) {                                                       // 0. synchroniseer met internettijd (elk heel uur ivm overgang van zomer/wintertijden)
      unsigned long oud   = now();
      upd_uur = hour();
      SyncNTPTijd();
      server.logDebug(15, "RTC","NTP synchronisatie", oud, now());    
  }
  
  char datumtijd[30] = {'\0'};
  aqua_functie.netteDatumTijd(datumtijd);
  Serial.println(F(""));
  Serial.println(F(">>======================================<<"));
  Serial.print  (F("  HOOFDLUS > ")); Serial.println(String(datumtijd));
  #ifdef DEBUG                                  
    Serial.print(F("  Geheugen > ")); Serial.print(freeMemory()); Serial.println(F(" bytes vrij."));
  #endif
  Serial.println(F(">>======================================<<"));
                                                                        
  unsigned long start = millis(); 

  // hier moet nog een schakelaartest komen, (digitalread), maar er is nog geen schakelaar.
  isTestmode    = server.testMode();                                              // sychronoseer / zet testmode stand;
  isOnderhoud   = aqua_onderhoud.actie();                                         // bepaal of de Onderhoud knop is ingedrukt en als dat zo is, schakel dan de relais uit
      
  vakantie      = ( (digitalRead(vakantiePin)) or (aqua_iot.getBoolWaarde('x')) );
  aqua_iot.setBoolWaarde('h', vakantie);
  
  
  bijvullen     = ( (!digitalRead(waterPin)) or (aqua_iot.getBoolWaarde('y')) );
  aqua_iot.setBoolWaarde('b',bijvullen);
  
  #ifdef DEBUG   
        Serial.print(F("vakantiestand : "));                                      // toon of de vakantiestand is ingeschakeld.      
        Serial.println(String(vakantie,DEC));
        Serial.print(F("onderhoudsknop: "));                                      // toon of de onderhoudsknop is ingeschakeld.
        Serial.println(String(isOnderhoud,DEC));
  #endif  

                                                                                                                                             
  if ( (((minute() % 15) == 0)  and (upd_sensor != minute()) ) or                 // 2 en 3. controleren topoff en sump, maar dit doen we elke 15 minuten
     (aqua_topoff.pompIsAan())  or (aqua_sump.pompIsAan())     or                 //         en natuurlijk altijd als oon maar 1 van de pompen loopt.
     (bijvullen))                                                 {               //         of tot slot als Bijvullen schakelaar is ingedrukt.
   
      upd_sensor = minute();                                                                     
        
      if (aqua_topoff.pompIsAan()) {                                              // 2. Aquarium wordt bijgevuld met osmose water. Bepalen of er voldoende osmose is
          aqua_topoff.pompStoppen();                                              // laat aan de functie over of idd de pomp moet worden gestopt.
      } else {                                                                    // let op: onderstaande IF statements niet in elkaar 'vlechten', het is beter leesbaar als we dit stap voor stap controleren 
          if (!isOnderhoud) {                                                     // als we niet in de onderhoudsmodus zijn (knop is niet ingedrukt) 
              if (aqua_topoff.vulnodig()) {                                       // controleer osmose voorraad -> vulnodig() moet uitgevoerd, ook als waterpin niet actief is
                  if ((!digitalRead(waterPin)) and                                // alleen actie ondernemen als water bijvullen knop is ingeschakeld 
                      (!aqua_sump.pompIsAan()))  {                                // en de pomp voor het bijvullen van de sump loopt nog niet (2 pompen tegelijkertijd laten lopen is niet slim)  
                          aqua_topoff.pompStarten();                              // dan -> osmose water moet worden bijgevuld
                  }     
              }
          }
      } 
                                                                                  //  3. controleren van de SUMP waterstand.    
      if (aqua_sump.pompIsAan()) {                                                // Aquarium wordt bijgevuld met osmose water. Bepalen of dit nog nodig is.
          aqua_sump.pompStoppen();                                                // laat aan de functie over of idd de pomp moet worden gestopt.
      } else { 
          if ( (!isOnderhoud) and (!aqua_topoff.pompIsAan()) )  {                 // alleen bijvullen als onderhoud is uitgeschakeld en de topoff wordt niet bijgevuld.
              if (aqua_sump.vulnodig(0))        {                                 // we moeten voorkomen dat beide pompen tegelijkertijd lopen, dan kunnen meetfouten optreden
                  aqua_sump.pompStarten();                                        // waardoor pompen direct stoppen (foutafhandeling) of te lang doorlopen. 
              }
          }
      } 
  }

  if ( (!aqua_topoff.pompIsAan()) and (!aqua_sump.pompIsAan()) ) {
      aqua_led.checkAllePlanningen();                                             // 4. Led(s)  --> update
      aqua_pomp.checkAllePlanningen();                                            // 5. Doseringspomp(en)
      aqua_relais.checkAllePlanningen(isOnderhoud);                               // 6. Relais
 
      bool koeling_aan = aqua_koeler.koelerControle();                            // 7. check temperatuur en zet evt. ventilatoren aan of uit. 
      server.synchroniseer_IoT();
 
      if (((minute() % verversDB) == 0) and (upd_server != minute())) {           // 8. elke x min instellingen inlezen van de database          
          upd_server = minute();
          verbonden = LeesServerInstellingen(false);    
      }

  }
  
  verversLCD();
  bool sdcard = SD.exists("/MAIN.CFG");
  aqua_iot.setBoolWaarde('c',sdcard);
  alarmFunctie();
  int32_t wacht = (1000 - (millis()-start) );                                     // dynamisch de wachttijd bepalen, in elk geval zorgen dat de wachttijd voor de lus 
  if (wacht < 0)     { wacht = 0;}
  if (wacht > 1000)  { wacht = 1000; }
  delay(wacht);                                                                   // Wacht voordat de loop opnieuw doorlopen wordt, mag nl. niet binnen 1 seconde gebeuren.
  Serial.println(F(">>======================================<<"));
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool LeesServerInstellingen(bool opstart) 
{
  Functie  _functie;
  uint8_t  s,i    = 0;
  isGewijzigd     = false;
  aquarium[0]     = {'\0'};
  
  if ((!verbonden) or (aqua_iot.getLongWaarde('w') == 0)) {           // nog niet verbonden, dan even opnieuw een verbinding maken
      i = 0;
      do {
          i++;
          verbonden   = server.verbind();                     
          if ((!verbonden) or (aqua_iot.getLongWaarde('w') == 0)) {   // w = wifi db score, als deze 0 is, is er geen verbinding
              aqua_functie.nieuweFout(99);  
              delay(2000);

              if (i > 5) {
                  if (server.resetCount(1)) {                         // als we al 3 keer eerder een reset hebben geforceerd dan heeft dit geen zin meer  
                      server.logDebug(22,"","",0,0);                  // vastleggen dat een reset is geforceerd
                      resetFunc();                                    // forceer reset
                  } 
              }             
          }
      } while ((!verbonden) and (i < 6)); 
  }   
  
  if ((verbonden) or (sdcard)) {                                      // als er een WIFI verbinding is, dan moet eventuele foutmelding verwijderd worden   
      aqua_functie.verwijderFout(99);                                 // we kunnen hoe dan ook doorgaan, er is immers een backup aanwezig op de SD card
                                                                      // als er geen WIFI verbinding is, wordt de informatie van de SD card gelezen

      if (!server.getAquarium(aquarium,  aquaID, &verversDB, &isGewijzigd)) {
          aqua_functie.nieuweFout(99);   
      }   
  
      if ((verversDB < 1) or (verversDB > 43200)) {                   // overschrijf als we een rare waarde hebben teruggekregen., 43200 is 30 dagen. 
          verversDB = 15;  
      }
    
      if ((isGewijzigd) or (opstart)) {
          server.getPlanning(aquaID, opstart);   
             
          s = 0;
          for (i = 0; i < aantalLeds; i++) {                          // lees de Leds in (deel van de functietabel)
              if (server.getFunctie(&_functie, aquaID, ledPins[i])) {    
                  aqua_led.init(&_functie, s, opstart); 
                  s++;
              } 
          } 
      
          s = 0;
          for (i = 0; i < aantalPompen; i++) {                        // lees de Pompen in (deel van de functietabel)               
              if (server.getFunctie(&_functie, aquaID, pompPins[i])) {    
                  aqua_pomp.init(&_functie, s, opstart); 
                  s++;
              }  
          }   

          s = 0;
          for (i = 0; i < aantalRelais; i++) {                          // lees de Relais in (deel van de functietabel)                          
              if (server.getFunctie(&_functie, aquaID, relaisPins[i])) {    
                  aqua_relais.init(&_functie, s, opstart); 
                  s++;
              }  
          }   

          if (server.getFunctie(&_functie, aquaID, tempPin)) {
              aqua_temp.init(&_functie, opstart);      
          }
        
          if (server.getFunctie(&_functie, aquaID, topOffPin)) {
              aqua_topoff.init(&_functie, opstart);             
          }
 
          if (server.getFunctie(&_functie, aquaID, sumpPin)) {
              aqua_sump.init(&_functie, opstart);      
          }       

          if (server.getFunctie(&_functie, aquaID, koelerPin)) {
              aqua_koeler.init(&_functie, opstart);             
          }

          if (server.getFunctie(&_functie, aquaID, onderhoudPin)) {
              aqua_onderhoud.init(&_functie, opstart);      
          }       

          if (! server.setUpdated(aquaID)) {                          // zet update-indicator (vlag) op false, zodat we niet onnodig alle instellingen opnieuw verwerken
              Serial.println(F("update vlag niet gereset."));
          }
      }  else {   
          Serial.println(F("geen wijzigingen"));
      }
      return true;
  } else {
      server.logDebug(20,"leesServerInstellingen()","",0,0);
      verversDB     = 43200;                                         // de komende 30 dagen niet meer checken
      isGewijzigd   = false;
      return false;   
  }
}

// ------------------------------------------------------------------------------------------------------------------//
// update de tijd op het LCD scherm
// zorg voor een voorloop 0 indien nodig
// ------------------------------------------------------------------------------------------------------------------//

void p2dig(uint32_t v, uint8_t d, bool zero)
// print 2 digits leading zero
{
  char tmp[2];
  tmp[0]    = {'\0'};
  if (zero) { strcpy(tmp,"0"); } else { strcpy(tmp, " ");}
  
  if (d > 1) { if (v < 10)      lcd.print(tmp); }
  if (d > 2) { if (v < 100)     lcd.print(tmp); }
  if (d > 3) { if (v < 1000)    lcd.print(tmp); }
  if (d > 4) { if (v < 10000)   lcd.print(tmp); }
  if (d > 5) { if (v < 100000)  lcd.print(tmp); }
  if (d > 6) { if (v < 1000000) lcd.print(tmp); }  
  lcd.print(v, DEC);
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void LCDtijd()
{
  lcd.setCursor(0, 3);
  p2dig( hour(), 2, true);
  lcd.print(":");
  p2dig( minute(), 2, true);
}



// ------------------------------------------------------------------------------------------------------------------//
// toon nieuwe of update informatie op het LCD display
// ------------------------------------------------------------------------------------------------------------------//
void verversLCD()
{
  uint8_t  tel     = 0;
  char     tmp[11];
  char     regel[22];

  float temperatuur     = aqua_temp.gemiddelde();
  aqua_iot.setFloatWaarde('t',temperatuur); // t = temperatuur
  lcd.clear();  
  regel[0]  = {'\0'};
  tmp[0]    = {'\0'};
  
  LCDtijd();
  

  if ((vakantie) and (lcdregel == 1)) {
      lcdregel = 0; 
      strcpy(regel,"[vakantiestand]");     
  } else {
      for (tel = strlen(aquarium); tel < 15; tel++) {                        // altijd aanvullen tot 15 posities
        strcat(aquarium," ");
      }
      lcdregel = 1;
      strcpy(regel, aquarium);    
  }

  if (temperatuur > 0) {  dtostrf(temperatuur ,5, 1, tmp); } else { strcpy(tmp," --.-"); }
  strcat(regel,tmp);
  
  #ifdef DEBUG                                  
      Serial.print(F("LCD regel 1   : ")); Serial.println(regel);
  #endif
  
  lcd.setCursor(0,0);
  lcd.print(regel);


  regel[0]  = {'\0'};   
  if (!aqua_functie.toonFouten(regel)) {                                 // als er fouten zijn, worden deze hierdoor al getoond
      if (lcdr2 < (aantalLeds + aantalPompen)) { lcdr2++; } else { lcdr2 = 1; }      
      if (lcdr2 <= aantalLeds) { aqua_led.lcdInfo(regel); } else { aqua_pomp.lcdInfo(regel); }
  }

  #ifdef DEBUG                                  
      Serial.print(F("LCD regel 2   : ")); Serial.println(regel);
  #endif
  lcd.setCursor(0,1); 
  lcd.print(regel); 


  regel[0]  = {'\0'};   
  lcd.setCursor(0,2); 
  if (aqua_topoff.pompIsAan()) {                                          // als de pomp loopt dan alleen stand van Osmose water tonen.

     strcpy(regel,"Osmosewater ");
     tmp[0]    = {'\0'};   
     dtostrf(aqua_topoff.voorraad(),4, 1, tmp);
     strcat(tmp,"l");
     for (uint8_t i = strlen(tmp); i < 8; i++) { strcat(regel," "); }     // Spaties toevoegen, zodat uitlijning perfect is.
     strcat(regel,tmp);

     #ifdef DEBUG                                  
        Serial.print(F("LCD regel 3   : ")); Serial.println(regel);
     #endif
     lcd.print(regel); 

     
  } else {

      if (lcdr3 < (aantalRelais + 2)) { lcdr3++; } else { lcdr3 = 1; }    // er zijn 4 relais, osmose en een sump meting (daarom +2)
  
      if (lcdr3 <= aantalRelais) { 
          aqua_relais.lcdInfo(regel); 
          
          #ifdef DEBUG                                  
            Serial.print(F("LCD regel 3   : ")); Serial.println(regel);
          #endif
          lcd.print(regel); 
      }
      
      if (lcdr3 == (aantalRelais+1)) {
          strcpy(regel,"Osmosewater ");
          tmp[0]    = {'\0'};   
          dtostrf(aqua_topoff.voorraad(),4, 1, tmp);
          strcat(tmp,"l");
          for (uint8_t i = strlen(tmp); i < 8; i++) { strcat(regel," "); }   // Spaties toevoegen, zodat uitlijning perfect is.
          strcat(regel,tmp);

          #ifdef DEBUG                                  
              Serial.print(F("LCD regel 3   : ")); Serial.println(regel);
          #endif
          lcd.print(regel); 
      }
                          
      if (lcdr3 == (aantalRelais+2)) {
          strcpy(regel,"Sump niveau ");
          tmp[0]    = {'\0'};   
          dtostrf(aqua_sump.stand(),4, 1, tmp);
          strcat(tmp," ");
          for (uint8_t i = strlen(tmp); i < 8; i++) { strcat(regel," "); }   // Spaties toevoegen, zodat uitlijning perfect is.
          strcat(regel,tmp);

          #ifdef DEBUG                                  
              Serial.print(F("LCD regel 3   : ")); Serial.println(regel);
          #endif
          lcd.print(regel);
      
          lcd.setCursor(19,2); 
          if  (aqua_sump.stand() <  -1 )                               { lcd.print(char(0)); }      
          if ((aqua_sump.stand() >= -1 ) and (aqua_sump.stand() <= 1)) { lcd.print(char(1)); }      
          if  (aqua_sump.stand() >   1 )                               { lcd.print(char(2)); }        
      }                    
  }
  regel[0]  = {'\0'};   
  if ( (verbonden) and (server.runs_from_backup() == 0 )) {  
      aqua_functie.verwijderFout(99);                  
      aqua_iot.setLongWaarde('w',server.getWifiStatus());           // haal de WiFi signaal sterke op, 

      lcd.setCursor(9, 3);                                          // zodat deze op het LCD scherm kan worden getoond.
      lcd.print("WiFi  "+String(aqua_iot.getLongWaarde('w'),DEC)+"dB");
      digitalWrite(backupLed, LOW);                                 // waarschuwing led aan             
  } else { 
      lcd.setCursor(9, 3);                                          // zodat deze op het LCD scherm kan worden getoond.
      if (server.runs_from_backup() == 0) {
          lcd.print("WiFi:F*02"); 
          aqua_functie.nieuweFout(99);   
      } else {
          tmp[0]    = {'\0'}; 
          long _in_backup = server.runs_from_backup();
          p2dig(_in_backup, 5, false);
          lcd.print(" sec");                                        // 6 t/m 9                       
          
          digitalWrite(backupLed, HIGH);                            // waarschuwing led aan    
          if (_in_backup > 43200) {                                 // na 12 uur reset forceren      
              resetFunc(); 
          }
      }
  }  
} 
           




// ------------------------------------------------------------------------------------------------------------------//
// send an NTP request to the time server at the given address
// ------------------------------------------------------------------------------------------------------------------//
void SyncNTPTijd()
{
  time_t utc, local;
  char     tijd[31];

  sendNTPpacket(timeServer); // send an NTP packet to a time server
  
  // wait for a reply for UDP_TIMEOUT miliseconds
  unsigned long startMs = millis();
  while (!Udp.available() && (millis() - startMs) < UDP_TIMEOUT) {}

  if (Udp.parsePacket()) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);

      // We've received a packet, read the data from it into the buffer
      Udp.read(packetBuffer, NTP_PACKET_SIZE);

      // the timestamp starts at byte 40 of the received packet and is four bytes,
      // or two words, long. First, esxtract the two words:

      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;

      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:

      utc = epoch;
      if (utc != 0) { 
          Serial.print(F("Universele tijd: ")); 
          Serial.println(String(utc,DEC));     

          local = myTZ.toLocal(utc, &tcr);   // vertalen van de UTC tijd naar de CET, incl. correctie voor winter/zomertijd
          Serial.print(F("locale tijd: ")); 
          Serial.println(String(utc,DEC));     
          
          setTime(local);  
          Serial.print(F("Locale tijd ingesteld: ")); 
          Serial.println(String(now(),DEC));     
      } else {
          Serial.print(F("Internet tijd NIET ingesteld, UTC tijd:"));  
          Serial.println(String(utc,DEC));    
      }
  } else {
      Serial.print(F("geen verbinding met NTP-server, tijd NIET ingesteld, nu:"));
      Serial.println(String(now(),DEC));     
  }
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void sendNTPpacket(char *ntpSrv)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)

  packetBuffer[0]   = 0b11100011;   // LI, Version, Mode
  packetBuffer[1]   = 0;            // Stratum, or type of clock
  packetBuffer[2]   = 6;            // Polling Interval
  packetBuffer[3]   = 0xEC;         // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(ntpSrv, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool getTime()
{
  tm.Hour   = hour();
  tm.Minute = minute();
  tm.Second = second();
  return true;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool getDate(const char *str)
{
  const char *monthName[12] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  char    Month[12];
  int     Day, Year;
  uint8_t monthIndex;

  Serial.println(String(str));
  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
      if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day    = Day;
  tm.Month  = monthIndex + 1;
  tm.Year   = CalendarYrToTm(Year);
  return true;
}
