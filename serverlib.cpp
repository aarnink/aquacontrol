#include "Arduino.h"
#include "serverlib.h"
#include "globals.h"
#include "WiFiEsp.h"
#include "SD.h"

#define DEBUG 
// #define TESTMODE

//  Meldingen voor PROWL (voor gebruik in de Prowlfunctie in aqua_mysql)
//  00  Systeemstart
//  01  PHP lees fout:<spec>
//  02  JSON parsing mislukt<element>
//  03  SQL Database update mislukt  
//  04  SQL database schoning issue
//  05  SQL error
//  06  HTTP header fout
//  10  pomp automatisch bijvullen top-off gestart
//  11  bijvullen top-off systeem is uitgeschakeld
//  13  noodstop op bijvullen top-off systeem
//  14  waterniveau afwijkend
//  25  te hoge dosering gevraagd
//  90  onderhoudfunctie in-uitgeschakeld
// -------------------------------------------------------------------------------------------------

// uitleg over PROGMEM:   https://www.arduino.cc/reference/en/language/variables/utilities/progmem/
const char string_00[] PROGMEM = "Het systeem is opgestart. ";

// ----------- Webserver ---------------------------------------------------------------------------
const char string_01[] PROGMEM = "De server reageert niet.";
const char string_02[] PROGMEM = "HTTP header bevat fouten en is niet verwerkt.";
const char string_03[] PROGMEM = "ongeldig dataformaat, JSON parsing mislukt.";
const char string_04[] PROGMEM = "De update/read van de database is mislukt. [aquaID:%s pinID:%s].";

// ----------- Onderhoud ---------------------------------------------------------------------------
const char string_05[] PROGMEM = "De onderhoudsfunctie is AAN.";
const char string_06[] PROGMEM = "De onderhoudsfunctie is UIT.";

// ----------- Metingen ----------------------------------------------------------------------------
const char string_07[] PROGMEM = "%s liter osmosewater toegevoegd.\nEr is nu ±%s liter op voorraad.";
const char string_08[] PROGMEM = "%s liter water aan het aquarium toegevoegd. \nEr is nu nog ±%s liter osmosewater op voorraad.";
const char string_09[] PROGMEM = "Nog %s liter osmosewater op voorraad. ";
const char string_10[] PROGMEM = "Alarm: het aquarium heeft ±%s cm te weinig water en kan NIET worden bijgevuld!!";
const char string_11[] PROGMEM = "Fout, er is ±%s cm teveel water in de SUMP aanwezig!";
                                                                          
const char string_12[] PROGMEM = "De dosering plannning bevat een fout:\nGevraagd %s ml gevraagd \nMaximaal %s ml toegestaan.";
const char string_13[] PROGMEM = "De temperatuur wijkt teveel af. \nGemeten: %s°C.";
const char string_14[] PROGMEM = "Fout: ongeldige meting. \nDebug code: %s.";
const char string_15[] PROGMEM = "Interne clock sync, de tijd was:%s en is nu:%s.";
const char string_16[] PROGMEM = "Ventilatoren [AAN]; temperatuur wordt te hoog  [%s°C].";
const char string_17[] PROGMEM = "Ventilatoren [UIT]; temperatuur is weer normaal [%s°C].";
const char string_18[] PROGMEM = "";
const char string_19[] PROGMEM = "Ultrasensor (afstand) is uitgeschakeld.";

// ----------- Logmeldingen ------------------------------------------------------------------------
const char string_20[] PROGMEM = "Geen verbinding met WIFI: %s";
const char string_21[] PROGMEM = "Waarschuwing: backup file ingelezen: ";
const char string_22[] PROGMEM = "systeem heeft automatische RESET uitgevoerd!";
const char string_23[] PROGMEM = "Fout: kan interne clock niet initialiseren.";
const char string_24[] PROGMEM = "Fout: het aantal meetfouten is te hoog:[%s]  meetwaarde:[%s].";
const char string_25[] PROGMEM = "Fout: PROWL melding niet verstuurd:";
const char string_26[] PROGMEM = "Fout: overlappende of dubbele planning.\n[planID:%s en PinID:%s].";
const char string_27[] PROGMEM = "Fout: de pomp is te lang aan en is uit voorzorg uitgeschakeld. \n[voorraad osmose ±%s ltr]-[sump stand ±%s cm].";
const char string_28[] PROGMEM = "Fout: de pomp is te lang aan en is uit voorzorg uitgeschakeld. \n[voorraad osmose ±%s ltr].";
const char string_29[] PROGMEM = "Reset: de pomp is gereset en wordt weer ingschakeld. \n[voorraad osmose ±%s ltr]-[sump stand %s cm].";
const char string_30[] PROGMEM = "Reset: de pomp is gereset en wordt weer ingschakeld. \n[voorraad osmose ±%s ltr].";
const char string_31[] PROGMEM = "Pomp wordt tussen 0:00u en 4:00u niet gestart.\n[voorraad osmose ±%s ltr].";



const char* const meldingen[] PROGMEM = {string_00, string_01, string_02, string_03, string_04, string_05, string_06, string_07, string_08, string_09,
                                         string_10, string_11, string_12, string_13, string_14, string_15, string_16, string_17, string_18, string_19,
                                         string_20, string_21, string_22, string_23, string_24, string_25, string_26, string_27, string_28, string_29,
                                         string_30, string_31};

const uint8_t     max_melding = 32;       // aantal meldingen in de lijst hierboven  
const int         timeout     = 2000;     // timeout in msec tussen 2 pogingen de database server te benaderen
const uint8_t     retry       = 2;        // aantal pogingen die worden gedaan 
const char        logFile[]   = "/DEBUG.LOG";


//constructor hierin worden de variabelen geinitialiseerd die nodig zijn voor deze class 
serverclass::serverclass() { 
  verzoek[0]            = '\0';           // verzoek is het server request 
  poort                 = 81;             // server poort  
  nodered               = 11880;           // IoT Node-Red server
  _geen_verbinding_tijd = 0;
  strcpy(richtech,"192.168.10.210");      // IP adres van de MySQL server (poort zie boven)
  strcpy(ssid,"AARNINK");                 // wireless network name
  strcpy(password,"koenthijs3618");       // wireless password 
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::leegFunctie(Functie *functie) {
  functie->kort[0]   = {'\0'};  // korte omschrijving voor op het LCD display  
  functie->pin       =  0;      // PWM pin assigned 
  functie->level     =  0;      // PWM current setting [0-255]
  functie->maxLevel  =  0;      // Highest PWM level allowed for this Channel [0-255], is waarde1 in de database
  functie->aantal    =  0;      // aantal is waarde2 in de database 
  functie->dbLevel   =  0;
  functie->suggested =  0;      // PWM seggested setting [0-255]  
  functie->correctie =  0;
  functie->timer     =  0;
  functie->logAan    =  0;
  functie->schoonAan =  0;
  //  functie->isActief  mag niet worden gereset, waarde wordt bepaalt in de functie zelf
  functie->isAan     =  false;  // false = helemaal uitgezet in de instellingen
  functie->datumAan  =  0;      // datum waarop dit apparaat aan staat;
  functie->datumUit  =  0; 
  functie->prowl     =  0;
}



// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::leegPlanning() 
{
  for (uint8_t i = 0; i < max_planning; i++) {
     schedule[i].id       = 0;      // planningID  
     schedule[i].uur      = 0;      // Start Hour [0-23]
     schedule[i].min      = 0;      // Start Minute [0-59]
     schedule[i].dagen    = 0;      // Dagen vd week --> 0=elke dag | 1=Zo, 2=Ma, 3=Di, 4=Woe, 5=Do, 6=Vr, 7=Za | 8=werkdagen (Ma-Vr) | 9=weekend (Za,Zo)
     schedule[i].duur     = 0;      // how long the Event lasts in seconds
     schedule[i].fadeIn   = 0;      // Seconds to fade-in before event, and fade-out after duration (0 = no fading)
     schedule[i].fadeUit  = 0;      // Seconds to fade-uit before event, and fade-out after duration (0 = no fading)
     schedule[i].maximaal = 0;      // maximale waarde in dit specifieke programma

     for (uint8_t a = 0; a < max_pins; a++) {
          schedule[i].pins[a].pin = 0;        // PWM channel assigned to this event [0,1,2,3]
          schedule[i].pins[a].pin = false;    // plannning nog niet uitgevoerd
     }
  }
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::testMode ()
{

    #ifdef TESTMODE 
        _isTestmode = true;
    #else                                       
        _isTestmode = digitalRead(testModePin);
    #endif
    #ifdef DEBUG  
        Serial.print(F("Testmode      : "));  
        Serial.println(String(_isTestmode,DEC));
    #endif
    
    return _isTestmode;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::resetCount  (uint8_t increment)
{
    if (!sdcard)  { return false; }                                        // als we geen SD card hebben geinstalleerd, dan direct stoppen
    uint8_t resetcount = 0;

    StaticJsonDocument<64> doc1;
    StaticJsonDocument<64> doc2;
        
    if (!SD.exists("/RESET.CNT")) {                                   
        File file = SD.open("/RESET.CNT", FILE_WRITE);               // Open file for writing
        if (file) {
//            JsonObject &object = doc1.createObject();
            doc1["reset"]    = 0;
            if (serializeJson(doc1, file) == 0) {
                Serial.println(F("Fout: RESET.CNT kan niet worden gereset."));
            }
            file.close();
        } else {
            Serial.println(F("Fout: RESET.CNT kan niet worden aangemaakt."));          
        }   
        return false;
          
    } else {              
        resetcount = 0;
        File lees  = SD.open("/RESET.CNT");                         // Open file for read
        if (lees) {
            DeserializationError error = deserializeJson(doc1, lees);
            if (!error) {                      
                resetcount     = doc1["reset"];
            }
            lees.close();
        }
        
//        JsonObject &nieuw  = doc2.createObject();                           
        if (increment > 0) {
            doc2["reset"] = (resetcount + 1);
        } else {
            doc2["reset"] = 0;
            resetcount     = 0;
        }
        SD.remove("/RESET.CNT");
        File file  = SD.open("/RESET.CNT", FILE_WRITE);             // Open file for write
        if (file) {
            if (serializeJson(doc2,file) == 0) {
                Serial.println(F("Fout: RESET.CNT kan niet worden geupdated."));
            }
            file.close();
            Serial.print(F("RESET.CNT counter bijgewerkt, counter= ")); 
            Serial.println(String(resetcount,DEC));          
        }
        if (resetcount > 3) {                                       // vaker dan 3 keer gereset, dan is er een loop en gaan we niet langer door
            return false; 
        } else {
            return true;                                            // we 'mogen' nog een reset doen 
        }
    }  
}



String serverclass::ip2Str(IPAddress ip) 
{
  String s="";
  for (uint8_t i=0; i<4; i++) {
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  }
  return s;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::verbind() 
{
  uint8_t  tel = 0;

  #ifdef DEBUG  
    Serial.println(F("verbinden met WIFI."));  
  #endif


  if (status == WL_CONNECTED) {  
      #ifdef DEBUG  
        Serial.println(F("WiFi: al verbonden."));  
     #endif
     _geen_verbinding_tijd = 0;
     return true;
  }
  
  Serial2.begin(115200);
  WiFi.init(&Serial2);
  delay(100);                                     // even wachten

  tel = 0;
  while ((WiFi.status() == WL_NO_SHIELD) and (tel < retry)) {
     delay(timeout);   tel++;    
  }
  
  if (tel >= retry) { 
    logDebug(20,"verbind()","geen WIFI module.", 0, 0);
    if (_geen_verbinding_tijd == 0) {                            // alleen als er tot nu toe een verbinding was, dan vanaf nu gaan tellen (aftellen voor reset)
        _geen_verbinding_tijd = millis();
    }
    return false; 
  }
  
  byte mac[6]={0,0,0,0,0,0};
  WiFi.macAddress(mac);
  Serial.print(F("MAC Address : ")); 
  Serial.print(mac[5], HEX);  Serial.print(":");
  Serial.print(mac[4], HEX);  Serial.print(":");
  Serial.print(mac[3], HEX);  Serial.print(":");
  Serial.print(mac[2], HEX);  Serial.print(":");
  Serial.print(mac[1], HEX);  Serial.print(":");
  Serial.println(mac[0], HEX);Serial.println();
  
  tel = 0;
  do {
    status = WiFi.begin(ssid, password);
    if (status != WL_CONNECTED) { delay(2000); } 
    tel++;
  } while ( (status != WL_CONNECTED) and (tel < retry) );

  if (status == WL_CONNECTED) {  
      Serial.print(F("IP Address  : ")); 
      Serial.println( WiFi.localIP() );
      aqua_iot.setCharWaarde('i', ip2Str(WiFi.localIP()).c_str() );
      return true; 
  } else { 
      logDebug(20,"verbind()","geen verbinding.", 0, 0);
      if (_geen_verbinding_tijd == 0) {                            // alleen als er tot nu toe een verbinding was, dan vanaf nu gaan tellen (aftellen voor reset)
        _geen_verbinding_tijd = millis();
      }
      return false; 
  }
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
long serverclass::getWifiStatus() {
  long rssi = WiFi.RSSI();
  return rssi;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
/*
char serverclass::getIPnummer() {
  return (char) String(WiFi.localIP()).c_str();
}
*/

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
long serverclass::runs_from_backup() {
  unsigned long nu = millis();
  unsigned long in_backup   = 0; 
  
  if (_geen_verbinding_tijd != 0) {
    in_backup = (nu - _geen_verbinding_tijd);
  }
  in_backup = (in_backup / 1000);  // naar seconden.
  return in_backup;
}


// ------------------------------------------------------------------------------------------------------------------//
// Opbouwen van de http parameter string die naar de Blyck API service wordt gestuurd
//
// Uitgaande status velden vanuit controller 
// a  = alarm (boolean)
// c  = SDcard (boolean)
// d  = melding (url encoded string)
// e  = eiwitafschuimer (boolean)
// h  = vakantiestand (boolean)
// i  = 
// k  = koeling status (boolean)
// m  = onderhoud (boolean)
// o  = osmose voorraad (float) 
// p  = pomp (boolean)
// s  = sump stand (float)
// t  = temperatuur (float)
// v  = wavemaker (boolean)
// w  = wifi status (long)
// 
//
// Inkomende knop status velden vanuit de Blynck app
// r = onderhoudsknop (boolean)
// u = koeling knop (boolean)
// x = vakantie knop (boolean)
// y = bijvullen knop (boolean)
// ------------------------------------------------------------------------------------------------------------------//

bool serverclass::iotdata_to_request(char *out) {  
  char temp[51] = {'\0'}; 


// vanuit controller naar App
  strcat(out,"?w="); 
  dtostrf(aqua_iot.getLongWaarde('w'),1, 0, temp);   // wifi status
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&c=");
  itoa(aqua_iot.getBoolWaarde('c'), temp, 10);      // sdcard
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&t="); 
  dtostrf(aqua_iot.getFloatWaarde('t'), 1, 1, temp); // temperatuur
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&o=");
  dtostrf(aqua_iot.getFloatWaarde('o'), 1, 1, temp); // osmose
  strcat(out,temp); temp[0] = {'\0'};
   
  strcat(out,"&s=");  
  dtostrf(aqua_iot.getFloatWaarde('s'), 1, 1, temp); // sump 
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&k=");
  itoa(aqua_iot.getBoolWaarde('k'), temp, 10);      // koeling
  strcat(out,temp); temp[0] = {'\0'};
 
  strcat(out,"&a=");  
  itoa(aqua_iot.getBoolWaarde('a'), temp, 10);      // alarm
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&m=");
  itoa(aqua_iot.getBoolWaarde('m'), temp, 10);      // onderhoud status
  strcat(out,temp); temp[0] = {'\0'};
   
  strcat(out,"&p=");  
  itoa(aqua_iot.getBoolWaarde('p'), temp, 10);      // pomp
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&e=");  
  itoa(aqua_iot.getBoolWaarde('e'), temp, 10);      // skimmer/eiwitafschuimer
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&v=");  
  itoa(aqua_iot.getBoolWaarde('v'), temp, 10);      // wavemaker
  strcat(out,temp); temp[0] = {'\0'};
  
  strcat(out,"&h=");  
  itoa(aqua_iot.getBoolWaarde('h'), temp, 10);      // vakantie
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&b=");  
  itoa(aqua_iot.getBoolWaarde('b'), temp, 10);      // bijvullen status
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&i=");   
  aqua_iot.getCharWaarde('i', temp);
  strcat(out, temp);
  temp[0] = {'\0'};  


// inkomend vanuit de App
  strcat(out,"&r=");
  itoa(aqua_iot.getBoolWaarde('r'), temp, 10);      // onderhoud knop
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&u=");
  itoa(aqua_iot.getBoolWaarde('u'), temp, 10);      // koeling knop
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&x=");
  itoa(aqua_iot.getBoolWaarde('x'), temp, 10);      // vakantie knop
  strcat(out,temp); temp[0] = {'\0'};

  strcat(out,"&y=");
  itoa(aqua_iot.getBoolWaarde('y'), temp, 10);      // bijvullen knop
  strcat(out,temp); temp[0] = {'\0'};

// meldingen
  strcat(out,"&d=");   
  aqua_iot.getCharWaarde('d', temp);
  strcat(verzoek, aqua_functie.urlencode(temp).c_str());
}



// ------------------------------------------------------------------------------------------------------------------//
// r = onderhoudsknop (boolean)
// u = koeling knop (boolean)
// x = vakantie knop (boolean)
// y = bijvullen knop (boolean)
// ------------------------------------------------------------------------------------------------------------------//

bool serverclass::json_to_iotdata(JsonObject& in) {

  if (in["i"].as<int>() == aquaID) {                       // i is aquaid en wordt teruggegeven door Node Red
    aqua_iot.setBoolWaarde('r',in["r"].as<int>());
    aqua_iot.setBoolWaarde('u',in["u"].as<int>());
    aqua_iot.setBoolWaarde('x',in["x"].as<int>());    
    aqua_iot.setBoolWaarde('y',in["y"].as<int>());
  } else {
    Serial.println(F("antwoord bevat onjuist AquaID"));
  }
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//

bool serverclass::synchroniseer_IoT()
{
  bool succes = false;
  verzoek[0] = {'\0'}; 
  iotdata_to_request(verzoek);
  
  #ifdef DEBUG   
    Serial.println(F("IoT onderhoud status sync ")); 
    
    Serial.println(F("IoT ** uitgaand ** "));  
    Serial.print(F("IoT onderhoud stat : "));  Serial.println(String(aqua_iot.getBoolWaarde('m'),DEC));
    Serial.print(F("IoT koeling stat   : "));  Serial.println(String(aqua_iot.getBoolWaarde('k'),DEC));
    Serial.print(F("IoT vakantie stat  : "));  Serial.println(String(aqua_iot.getBoolWaarde('h'),DEC));
    Serial.print(F("IoT bijvullen stat : "));  Serial.println(String(aqua_iot.getBoolWaarde('b'),DEC));
    Serial.println(F("IoT ** inkomend ** "));  
    Serial.print(F("IoT onderhoud knop : "));  Serial.println(String(aqua_iot.getBoolWaarde('r'),DEC));
    Serial.print(F("IoT koeling knop   : "));  Serial.println(String(aqua_iot.getBoolWaarde('u'),DEC));
    Serial.print(F("IoT vakantie knop  : "));  Serial.println(String(aqua_iot.getBoolWaarde('x'),DEC));
    Serial.print(F("IoT bijvullen knop : "));  Serial.println(String(aqua_iot.getBoolWaarde('y'),DEC));

    char temp[41] = {'\0'}; aqua_iot.getCharWaarde('d',temp);
    Serial.print(F("IoT melding   : "));  Serial.println(temp) ;
    
    Serial.println(F("--------------------------")); 
    Serial.print(F("IoT HTTP verzoek : "));  Serial.println(verzoek);
    Serial.println(F("--------------------------")); 
  #endif
  
  
  if (verbind_met_RichTech(nodered)) { 
     if (sendNodeRed(verzoek)) {
        if ((skipResponseHeaders() ))  {   
            StaticJsonDocument<256> doc;                                                  // Allocate a temporary memory pool
            DeserializationError error = deserializeJson(doc, client);
            verbreek_RichTech();                                                          // eerder vrijgeven
            if (error) {
                Serial.print(F("Fout bij verwerken IoT response: "));
                Serial.println(error.c_str());
            } else {
                aqua_iot.setCharWaarde('d',"");                                           // na verzending is dit oud nieuws !
                succes = (!doc.containsKey("error"));
                if (succes) { 
                    #ifdef DEBUG 
                      serializeJsonPretty(doc, Serial);
                      Serial.println();
                    #endif 
                    JsonObject result = doc.as<JsonObject>();
                    json_to_iotdata(result);
                } else {
                    char fout[40];
                    strcpy(fout, doc["error"]);   
                }
            }  
        }
     }
//     verbreek_RichTech(); 
  }
  return succes;
}
  
// ------------------------------------------------------------------------------------------------------------------//
// Deze functie haalt de omschrijving van het aquarium op uit de database.
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::getAquarium(char *tekst, uint8_t aquaID, uint16_t *ververs, bool *gewijzigd) 
{
  verzoek[0] = {'\0'};
  strcpy(verzoek,"/AD_aquarium.php?aquarium="); 

  // vertaal het Integer ID naar een char 
  char id[5] = {'\0'}; 
  itoa(aquaID, id, 10);
  strcat(verzoek, id);
      
  bool succes = false;
  *gewijzigd  = false;
  bool loaded = false;

  #ifdef DEBUG   
    Serial.print(F("SQL HTTP verzoek : "));  Serial.println(verzoek);
  #endif

  bool    get_ok;
  uint8_t t = 0; 
  do {
      if (verbind_met_RichTech(poort)) { 
          get_ok = sendRequest(verzoek);
          if (!get_ok) { 
              delay(10);  
              verbreek_RichTech(); 
          }
      } else {
          get_ok = false; 
          delay(timeout);  
      }
      t++;
  } while ((!get_ok) and (t < retry));

  if (get_ok) {    
      if ((skipResponseHeaders() ))  {                                                     // er is verbinding met de server en een geldig antwoord van de server 
        // Allocate a temporary memory pool
        DynamicJsonDocument doc(BUFFER_SIZE);                                              // alloceer geheugen voor het json object met het antwoord van de server 
        DeserializationError error = deserializeJson(doc, client);                        
        verbreek_RichTech();                                                               // eerder vrijgeven
        if (error) {                                                                       // json object voldoet niet 
            PROWLmelding(3, "getAquarium", 0, 0);                                          // resultaat van het GET request (JSON parsing) is mislukt
        } 
        else {
            #ifdef DEBUG  
              serializeJsonPretty(doc, Serial); 
            #endif

            succes = (!doc.containsKey("error"));                                          // controle op de inhoud van het bericht zelf
            if (succes) { 
                strcpy(tekst, doc["omschrijving"]);         
                *ververs   = doc["leesInstelling"];
                *gewijzigd = doc["isGewijzigd"];
                loaded     = true;

               if (sdcard) {
                  String filename    =  "/MAIN.CFG";
                  SD.remove(filename);                                                    // Delete existing file, otherwise the configuration is appended to the file
                  File file = SD.open(filename, FILE_WRITE);                              // Open file for writing
                  if (file) {
                      if (serializeJson(doc,file) == 0) {                                 // Serialize JSON to file
                          Serial.println(F("Fout, kan configuratie niet opslaan."));
                      }
                      file.close();                                                       // Close the file (File's destructor doesn't close the file)
                  } else { Serial.println(F("Fout, configuratiefile kan niet worden gemaakt.")); } 
               }              
                
            } else {          
                char fout[40];
                strcpy(fout, doc["error"]);   
                PROWLmelding(4, "getAquarium", aquaID, 0);                
            }
        }  
      } 
 //     verbreek_RichTech();
  } else { 
      PROWLmelding(1, "getAquarium", 0, 0);
      logDebug(1,"getAquarium()",verzoek, 0, 0);   
      if (sdcard) {          
            String filename    =  "/MAIN.CFG";
            File file = SD.open(filename);                                          // Open file for writing
            if (file) {
                StaticJsonDocument<BUFFER_SIZE> doc;
                DeserializationError error = deserializeJson(doc, file);

                succes = (!doc.containsKey("error"));
                if (succes) {                                                       // Kopieer de waarden uit de buffer waarin we geintereseerd zijn.
                    #ifdef DEBUG 
                      serializeJsonPretty(doc,Serial);  
                    #endif 

                    strcpy(tekst, doc["omschrijving"]);         
                    *ververs   = 43200;                                             // 30 dagen vasthouden, dus niets verversen de komend 30 dagen. 
                    *gewijzigd = false;                                             // op de backup (sdcard) wijzigt niets, dus dit moet standaard false zijn
                    loaded     = true;
                }               
                file.close();                                                       // Close the file (File's destructor doesn't close the file)
                logDebug(21,"getAquarium()", filename.c_str(), aquaID, 0);   
                _backup = true;
                            
            } else { 
                Serial.print(F("Fout, Aquarium configuratie niet leesbaar:")); 
                Serial.println(filename); 
                
            } 
        }              
  }
  return loaded;
}
 

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::setMeting(uint8_t aquaID, uint8_t pinID, float waarde, const char *opm)
{

  if (_isTestmode) { return true; }                                 // niets opslaan als het systeem in testmode staat.
  
  char id[12]; 
  verzoek[0] = {'\0'};
  bool ok  = false;          // 9 = nog geen acties ondernomen
  
  strcpy(verzoek,"/AD_meting.php?aquarium="); 

  id[0] = {'\0'};   // vertaal het Integer ID naar een char
  itoa(aquaID, id, 10);
  strcat(verzoek, id);
  
  id[0] = {'\0'};   // vertaal het Integer ID naar een char
  itoa(pinID, id, 10);
  strcat(verzoek, "&pin=");
  strcat(verzoek, id);

  id[0] = {'\0'};   // vertaal het float naar een char
  dtostrf(waarde ,3, 1, id);
  strcat(verzoek, "&meet=");
  strcat(verzoek, id);

  strcat(verzoek, "&opm=");
  strcat(verzoek, opm);
  strcat(verzoek, "");

  #ifdef DEBUG   
    Serial.print(F("SQL HTTP verzoek : "));  Serial.println(verzoek);
  #endif

  bool    get_ok;
  uint8_t t = 0; 
  do {
      if (verbind_met_RichTech(poort)) { 
          get_ok = sendRequest(verzoek);
          if (!get_ok) { 
              delay(timeout);  
              verbreek_RichTech(); 
          }
      } else {
          get_ok = false; 
          delay(timeout);  
      }
      t++;
  } while ((!get_ok) and (t < retry));

  ok = false;    
  if (get_ok) {
      if (skipResponseHeaders()) {
        // Allocate a temporary memory pool
        DynamicJsonDocument doc(BUFFER_SIZE);
        DeserializationError error = deserializeJson(doc, client);
        verbreek_RichTech();                                                // eerder vrijgeven?
        if (error) {
            #ifdef DEBUG         
                Serial.println(F("setMeting: JSON parsing mislukt!"));
            #endif
            PROWLmelding(3, "setMeting", 0, 0);
        } 
        else {
            if (!doc.containsKey("error")) {
               ok = true; // aquarium informatie met succes opgehaald.
            } else {
               char fout[40];
//               strcpy(fout, result.get<char *>("error"));
               strcpy(fout, doc["error"]);    
               PROWLmelding(4, "setMeting", aquaID, pinID);
            }
        }  
      }
//      verbreek_RichTech();
   } else { 
      PROWLmelding(1, "setMeting", 0, 0);
      logDebug(1,"setMeting()",verzoek, 0, 0);   
   }
   return ok;
}



// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::setDosering(uint8_t aquaID, uint8_t pinID, float waarde, const char *opm)
{
  if (_isTestmode) { return true; }                                 // niets opslaan als het systeem in testmode staat.

  char id[12];  
  verzoek[0] = {'\0'};
  bool succes = false;
  bool ok     = false;          // 9 = nog geen acties ondernomen
  
  strcpy(verzoek,"/AD_dosering.php?aquarium="); 

  id[0] = {'\0'};   // vertaal het Integer ID naar een char
  itoa(aquaID, id, 10);
  strcat(verzoek, id);
  
  id[0] = {'\0'};   // vertaal het Integer ID naar een char
  itoa(pinID, id, 10);
  strcat(verzoek, "&pin=");
  strcat(verzoek, id);

  id[0] = {'\0'};   // vertaal het float naar een char
  dtostrf(waarde ,3, 1, id);
  strcat(verzoek, "&meet=");
  strcat(verzoek, id);

  strcat(verzoek, "&opm=");
  strcat(verzoek, opm);
  strcat(verzoek, "");

  #ifdef DEBUG   
    Serial.print(F("SQL HTTP verzoek : "));  Serial.println(verzoek);
  #endif
  
  bool    get_ok;
  uint8_t t = 0; 
  do {
      if (verbind_met_RichTech(poort)) { 
          get_ok = sendRequest(verzoek);
          if (!get_ok) { 
              delay(timeout);  
              verbreek_RichTech(); 
          }
      } else {
          get_ok = false; 
          delay(timeout);  
      }
      t++;
  } while ((!get_ok) and (t < retry));

  ok = false;    
  if (get_ok) {      
      if (skipResponseHeaders()) {
        // Allocate a temporary memory pool
        DynamicJsonDocument doc(BUFFER_SIZE);
        DeserializationError error = deserializeJson(doc, client);
        verbreek_RichTech();                                                      // eerder vrijgeven?
        if (error) {
            #ifdef DEBUG         
                Serial.println(F("setDosering: JSON parsing mislukt!"));
            #endif
            PROWLmelding(3, "setDosering", 0, 0);
        } 
        else {
            succes = (!doc.containsKey("error"));
            if (succes) { 
               ok = true; // aquarium informatie met succes opgehaald.
            } else {
               char fout[40];
//               strcpy(fout, result.get<char *>("error"));   
               strcpy(fout, doc["error"]);   
               PROWLmelding(4, "setMeting", aquaID, pinID);
            }
        }  
      }
//      verbreek_RichTech();
   } else { 
      PROWLmelding(1, "setDosering", 0, 0);
      logDebug(1,"setDosering()",verzoek, 0, 0);   
      ok = false; // geen verbinding met de server
   }
   return ok;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::setUpdated(uint8_t aquaID)
{
  char id[5];
  bool ok        = false;
  verzoek[0]     = {'\0'};
  
  strcpy(verzoek,"/AD_updated.php?aquarium="); 
  id[0] = {'\0'};   // vertaal het Integer ID naar een char
  itoa(aquaID, id, 10);
  strcat(verzoek, id);

  #ifdef DEBUG   
    Serial.print(F("SQL HTTP verzoek : "));  Serial.println(verzoek);
  #endif
  
  bool    get_ok;
  uint8_t t = 0; 
  do {
      if (verbind_met_RichTech(poort)) { 
          get_ok = sendRequest(verzoek);
          if (!get_ok) { 
              delay(timeout);  
              verbreek_RichTech(); 
          }
      } else {
          get_ok = false; 
          delay(timeout);  
      }
      t++;
  } while ((!get_ok) and (t < retry));
  
  verbreek_RichTech();      
  if (get_ok) {      
      ok = true;
  } else {
      PROWLmelding(4, "setUpdated", aquaID, 0);
      ok = false;  
  }
   #ifdef DEBUG   
      Serial.println("Result="+String(ok,DEC));
   #endif
   
   return ok;
}




// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::copyFunctie(Functie *functie, JsonObject& result)
{  
    strcpy(functie->kort, result["kort"].as<const char *>());   
//    strcpy(functie->kort, result["kort"]);   

    functie->aantal    = result["aantal"].as<int>();
    functie->dbLevel   = result["level"].as<int>(); 
    functie->maxLevel  = result["maxLevel"].as<int>();
    functie->correctie = result["correctie"].as<int>(); 
    functie->isAan     = result["isAan"].as<int>(); 
    functie->logAan    = result["logAan"].as<int>(); 
    functie->schoonAan = result["schoonAan"].as<int>();
    functie->pin       = result["pinID"].as<int>();
    functie->datumAan  = result["start"].as<long>();
    functie->datumUit  = result["einde"].as<long>();
}


// ------------------------------------------------------------------------------------------------------------------//
// Deze functie haalt de omschrijving van het aquarium op uit de database.
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::getFunctie(Functie *functie, uint8_t aquaID, uint8_t pinID)  
{
//  const  size_t BUFFER_SIZE = 512; 

  leegFunctie(functie);
  verzoek[0] = {'\0'};
  strcpy(verzoek,"/AD_functie.php?aquarium="); 

  char id[5] = {'\0'};   // vertaal het Integer ID naar een char
  itoa(aquaID, id, 10);
  strcat(verzoek, id);
  itoa(pinID, id, 10);
  strcat(verzoek, "&pin=");
  strcat(verzoek, id);
     
  #ifdef DEBUG   
    Serial.print(F("SQL HTTP verzoek : "));  Serial.println(verzoek);
  #endif

  bool ok = false;

  bool    get_ok;
  uint8_t t = 0; 
  do {
      if (verbind_met_RichTech(poort)) { 
          get_ok = sendRequest(verzoek);
          if (!get_ok) { 
              delay(timeout);  
              verbreek_RichTech(); 
          }
      } else {
          get_ok = false; 
          delay(timeout);  
      }
      t++;
  } while ((!get_ok) and (t < retry));

  if (get_ok) {                                                     
    if (skipResponseHeaders()) {
       StaticJsonDocument<BUFFER_SIZE> doc;
       DeserializationError error = deserializeJson(doc,client);
       JsonObject result = doc.as<JsonObject>();

       verbreek_RichTech();  // eerder vrijgeven?

       if (error) {
            #ifdef DEBUG         
                Serial.println(F("JSON parsing mislukt!"));
            #endif
            PROWLmelding(3, "getFunctie", 0, 0);
        } 
        else {
            ok = (!doc.containsKey("error"));
            if (ok) { // Kopieer de waarden uit de buffer waarin we geintereseerd zijn.
               #ifdef DEBUG 
                  serializeJsonPretty(doc, Serial);
               #endif 

               copyFunctie(functie, result);                                              // velden kopieren naar de functie

               if (sdcard) {
                  String filename    =  "/PIN"+String(pinID,DEC)+".CFG";
                  SD.remove(filename);                                                    // Delete existing file, otherwise the configuration is appended to the file
                  File file = SD.open(filename, FILE_WRITE);                              // Open file for writing
                  if (file) {
                      if (serializeJson(doc, file) != 0) {                                // Serialize JSON to file
                          Serial.println(F("Failed to write to file"));
                      }
                      file.close();                                                       // Close the file (File's destructor doesn't close the file)
                  } else { Serial.println(F("Failed to create file")); } 
               }              
            } else {
               PROWLmelding(4, "getFunctie", aquaID, pinID);
            }         
        }    
    }
//    verbreek_RichTech();
   } else {                                                                         // lees de backup van de SD card
        if (sdcard) {          
            String filename    =  "/PIN"+String(pinID,DEC)+".CFG";
            File file = SD.open(filename);                                          // Open file for writing
            if (file) {
                StaticJsonDocument<BUFFER_SIZE> doc;
                DeserializationError error = deserializeJson(doc, file);        
                JsonObject result = doc.as<JsonObject>();        

                ok = (!doc.containsKey("error"));
                if (ok) {                                                           // Kopieer de waarden uit de buffer waarin we geintereseerd zijn.
                    #ifdef DEBUG              
                        serializeJsonPretty(doc, Serial);  
                    #endif 

                    copyFunctie(functie, result);                                   // velden kopieren naar de functie
                }               
                file.close();                                                       // Close the file (File's destructor doesn't close the file)
                logDebug(21,"getfunctie()", filename.c_str(), 0, 0);                   
            } else { 
                Serial.print(F("Failed to read functie config file:")); 
                Serial.println(filename);             
            } 
        }              
   }
   return ok;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
uint32_t serverclass::volgendeTijd(uint8_t pin)
{
  unsigned long aan, nu, volg, eerst;
  uint8_t       i,p;


  // stap 1:  zoek naar de inschakeltijd van de 1ste planning van de dag voor de gevraagde PIN#id
  i     = 0;
  eerst = 0;                                                                
  while ((eerst == 0) and (i < max_planning)) {            // planning is gesorteerd op inschakeltijd (oplopend)
      aan = aqua_functie.Time2Sec(schedule[i].uur, schedule[i].min, 0);   // tijdstip in seconden berekenen 
      for (p = 0; p < max_pins; p++) {
          if (schedule[i].pins[p].pin == pin) {
              eerst = aan;
          }
      }    
      i++;   
  }

  // stap 2:  zoek naar de inschakeltijd van de 1ste planning, vanaf NU voor de gevraagde PIN#id
  nu    = aqua_functie.Time2Sec(hour(), minute(), second()); 
  volg  = 0;
  i     = 0;
  while ((volg == 0) and (i < max_planning)) {
          aan = aqua_functie.Time2Sec(schedule[i].uur, schedule[i].min, 0);    
          if (aan > nu) {                                                     // eerst volgende tijd na NU
              for (p = 0; p < max_pins; p++) {
                  if (schedule[i].pins[p].pin == pin) {
                      volg  = aan;
                  }
              } 
          } 
      i++;   
  }

  #ifdef DEBUG 
    Serial.print(F("Planning PIN   #"));  Serial.println(String(pin,DEC));
    Serial.print(F("Tijd nu       : "));  Serial.println(String(nu,DEC));
    Serial.print(F("volgende tijd : "));  Serial.println(String(volg,DEC));
    Serial.print(F("eerste tijd   : "));  Serial.println(String(eerst,DEC));  
  #endif         


  if (volg > 0) {                                                             // als er geen planning meer actief wordt vandaag, dan is volg 0,
      return volg;                                                            // maar anders is volg het tijdstip dat moet worden gemeld
  } else {
    if (eerst > 0) {                                                          // in dat geval moeten we kijken of er uberhaupt een planning  
      return eerst;                                                           // actief is op de dag (en geven we dus de 1ste planning van de dag)
    } else {
      return 0;                                                               // als geen enkele planning geldt, dan is het resultaat 0
    }
  }
}

// ------------------------------------------------------------------------------------------------------------------//
// functie 0  = eindtijd als returnwaarde als de planning is uitgevoerd of loopt
//         1  = planning ID, veld maximaal (aantal ml voor doseerpomp)
// 
// ------------------------------------------------------------------------------------------------------------------//

uint32_t serverclass::eindPlanning(uint8_t pin, uint8_t functie)
{
  unsigned long aan, nu, uit;
  uint8_t       i,p;

  // zoek naar de inschakeltijd van de 1ste planning, vanaf NU voor de gevraagde PIN#id
  nu    = aqua_functie.Time2Sec(hour(), minute(), second()); 
  uit   = 0;
  aan   = 0;
  i     = 0;

  while (i < max_planning) {
      aan = aqua_functie.Time2Sec(schedule[i].uur, schedule[i].min, 0);    
      uit = aan + schedule[i].duur;
      if ((nu >= aan) and (nu <= uit))            {                           // deze planning is actief
          for (p = 0; p < max_pins; p++) {                                    // nu nog bepalen of deze planning ook geldt voor de gevraagde PIN
              if (schedule[i].pins[p].pin == pin) { 

                  #ifdef DEBUG 
                    Serial.print(F("Planning PIN   #"));  Serial.println(String(pin,DEC));
                    Serial.print(F("Tijd nu       : "));  Serial.println(String(nu,DEC));
                    Serial.print(F("Planning ID   : "));  Serial.println(String(schedule[i].id,DEC));
                    Serial.print(F("Aan           : "));  Serial.println(String(aan,DEC));
                    Serial.print(F("Uit           : "));  Serial.println(String(uit,DEC));  
                    Serial.print(F("Maximaal      : "));  Serial.println(String(schedule[i].maximaal,DEC));                     
                  #endif 
                         
                  switch (functie) {
                      case  0 : return uit;                                   // functie 0 = eindtijd van de planning
                      case  1 : return schedule[i].maximaal;
                      default : return uit;          
                  }   
                  break;
              }
          } 
      } 
      i++;   
  }
  return 0;
}



// -------------------------------------------------------------------------------------------------
// zoek in het tijdvlak (nu) naar de planning waarvoor de PIN ID is gepland
// -------------------------------------------------------------------------------------------------

uint8_t serverclass::zoekPlanningIndex(uint8_t pin) 
{   
  unsigned long aan, uit, nu;
  uint8_t a,i = 0;
  
  nu = aqua_functie.Time2Sec(hour(), minute(), second());                     // exacte tijdstip nu in seconden  
  for (i = 0; i < max_planning; i++) {
      aan = aqua_functie.Time2Sec(schedule[i].uur, schedule[i].min, 0);  
      uit = aan + schedule[i].duur;                                           // voor de pompen willen we doseren binnen 1 minuut
      if ((nu >= aan) and (nu <= uit)) {                                      // we zijn binnen het tijdvlak van de planning
          for (a = 0; a < max_pins; a++) {     
              if (pin == schedule[i].pins[a].pin) {
                  return i;
                  break;
              }
          }
      }
  }
  return 255; //max waarde betekent dat we niets gevonden hebben.
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
int serverclass::tekst_int(const char* invoer, uint8_t s, uint8_t a) 
{
  char temp[60] = "";                        // tijdelijke velden 
  int  m,t      = 0;

  m = strlen(invoer) - a - 1;
  if (a > m ) { a = m; }                     // beveilig tegen een overflow
  if (s >= m) { s = m + 1; }

  m = 0;
  for (t = s; t < (s+a); t++) {
      if (isDigit(invoer[t])) {
         strcat(temp, &invoer[t]);
      }
  }
  if (strlen(temp) > 0) {
      m = atoi(temp);
      return m;  
  }
  else {
      return -1;
  }    
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::sortPlanning()
{
    uint8_t i,k,m     = 0;
    unsigned long aan = 0;
    Planning _temp;
    
    i = max_planning-1;
    m = 0;
    while ((i > 0) and (m == 0)) {
      if (schedule[i].id > 0) {
         m = i; 
      }
      i--;
    }
    for (i = m; i > 0; i--)   // bubble sorteer;
    {
        for (k = 0; k < i; k++)
        { 
            aan = aqua_functie.Time2Sec(schedule[k].uur, schedule[k].min, 0);   // tijdstip in seconden berekenen                      
            if (aan > aqua_functie.Time2Sec(schedule[k+1].uur, schedule[k+1].min, 0)) {
               _temp      = schedule[k+1];
               schedule[k+1] = schedule[k];
               schedule[k]   = _temp;            
            }
        }
    } 
}




// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::copyPlanning(JsonArray& result)
{
    bool    planchange       = false;
    uint8_t h,m,i,a,tel      = 0;
     
    for (JsonObject rij : result) 
    {
        #ifdef DEBUG      
          Serial.println(F(""));             
          Serial.print(F("Huidig PlanID= ")); Serial.println(String(schedule[tel].id,DEC));
        #endif

        planchange = (schedule[tel].id != rij["ID"].as<int>()); // stel vast of er op deze positie een nieuwe planning wordt geladen
                                                        
        schedule[tel].id        = rij["ID"].as<unsigned int>();           // extract veld "PlanID" als een unsigned integer 
                                       
        schedule[tel].uur       = tekst_int(rij["Aan"].as<const char*>(),0,2); 
        schedule[tel].min       = tekst_int(rij["Aan"].as<const char*>(),3,2);
                  
        h = tekst_int(rij["Uit"].as<const char*>(),0,2);
        m = tekst_int(rij["Uit"].as<const char*>(),3,2);
               
        schedule[tel].duur  = (((h - schedule[tel].uur) * 60) - (schedule[tel].min - m)) * 60;
        schedule[tel].fadeIn   = rij["fIn"].as<unsigned int>(); 
        schedule[tel].fadeUit  = rij["fUit"].as<unsigned int>();
        schedule[tel].maximaal = rij["max"].as<unsigned int>();              
        schedule[tel].dagen    = rij["dgn"].as<unsigned int>();

        JsonArray pinnen       = rij["pID"];                                  // in het JSON bericht staat een array.
        
        #ifdef DEBUG  
          serializeJsonPretty(rij, Serial);
          serializeJsonPretty(pinnen, Serial); 
          Serial.println(F(""));      
          Serial.print(F("Teller       = ")); Serial.println(String(tel,DEC));      
          Serial.print(F("Nieuw PlanID = ")); Serial.println(String(schedule[tel].id,DEC));   
          Serial.print(F("Gewijzigd    = ")); Serial.println(String(planchange,DEC));                                         
          Serial.println(F(""));
          Serial.print(F("Huidige pins = "));                  
          for (uint8_t l = 0; l < max_pins; l++) { Serial.print("["+String(schedule[tel].pins[l].pin, DEC)+"]"); }  
          Serial.println(F(""));
        #endif  

        i = 0;
        for (auto value : pinnen) {
             // als er geen wijzigingen zijn, houden we de status afgerond vast, anders resetten we dit
             // dit betekent dat wijzigingen in tijden bij een bestaand plan pas na 24 uur ingaan of na een reset.
             // doen we dit niet, dan leidt elke wijziging ertoe dat 2x dezelfde planning kan worden uitgevoerd, omdat de afgerond status wordt gereset.
                          
            if ((schedule[tel].pins[i].pin != value.as<unsigned int>()) or (planchange))  {
                schedule[tel].pins[i].pin  = value.as<unsigned int>();
                schedule[tel].pins[i].afgerond = false;    
            } else {
                #ifdef DEBUG  
                  Serial.print(F("Status pin   = ")); Serial.print(String(schedule[tel].pins[i].pin,DEC));Serial.println(F(" is ongewijzigd.")); 
                #endif                                         
            }
            if (i < max_pins) { i++; }
        }
                
        for (a = i; a < max_pins; a++) {                                // de rest van de pinnen op 0 zetten;
            schedule[tel].pins[a].pin      = 0; 
            schedule[tel].pins[a].afgerond = false;  
        }
                
        #ifdef DEBUG  
          Serial.print  (F("Nieuwe pins  = ")); 
          for (uint8_t l = 0; l < max_pins; l++) { Serial.print("["+String(schedule[tel].pins[l].pin, DEC)+"]"); }  
          Serial.println(F(""));  
        #endif
        tel++;
    }
}

// ------------------------------------------------------------------------------------------------------------------//
// Deze procedure vult de array in de classe met de aanwezige planning voor een specifiek aquarium ID
// in de query (getPlanningen.php) is het aantal records gemaximaliseerd op 10 --> bufferoverflow voorkomen.
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::getPlanning(uint8_t aquaID, bool opstart)  
{
  const   size_t BUFFER_SIZE = 1536; //was 2048
  uint8_t t                  = 0;
  char id[12]                = {'\0'};              // vertaal het Integer ID naar een char
 
  if (opstart) { leegPlanning(); }                  // alleen leegmaken bij initiele opstart.

  verzoek[0] = {'\0'};
  strcpy(verzoek,"/AD_planningen.php?aquarium="); 
  id[0] =      {'\0'};   // vertaal het Integer ID naar een char
  itoa(aquaID, id, 10); strcat(verzoek, id);
  
  #ifdef DEBUG   
      Serial.println(F(">>=========== Planning Laden ===========>>"));
      Serial.print(F("SQL opdracht =")); Serial.println(verzoek);
  #endif

  bool    get_ok;
  t = 0; 
  do {
      if (verbind_met_RichTech(poort)) { 
          get_ok = sendRequest(verzoek);
          if (!get_ok) { 
              delay(timeout);  
              verbreek_RichTech(); 
          }
      } else {
          get_ok = false; 
          delay(timeout);  
      }
      t++;
  } while ((!get_ok) and (t < retry));
      
  if (get_ok) 
  {      
      if (skipResponseHeaders()) 
      {
         DynamicJsonDocument doc(BUFFER_SIZE);
         DeserializationError error = deserializeJson(doc, client);          
         JsonArray result = doc.as<JsonArray>();    
         verbreek_RichTech();                                                     // eerder vrijgeven?
         if (error) {
               #ifdef DEBUG          
                    Serial.println(F("Foutmelding  = JSON parsing mislukt!"));
               #endif
               PROWLmelding(3, "getPlanning [all] ", 0, 0);
          } 
          else  {
             copyPlanning(result);                                                // velden kopieren naar de Planning

             if (sdcard) {
                String filename    =  "/PLANNING.CFG";
                SD.remove(filename);                                              // Delete existing file, otherwise the configuration is appended to the file
                File file = SD.open(filename, FILE_WRITE);                        // Open file for writing
                if (file) {
                    if (serializeJson(doc, file) == 0) {                              // Serialize JSON to file
                        Serial.println(F("Failed to write to file"));
                    }
                    file.close();                                                 // Close the file (File's destructor doesn't close the file)
                } else { Serial.println(F("Failed to create file")); }               
             } 

        }      
      }     
//      verbreek_verbinding();
   } else {                                                                       // lees de backup van de SD card
        if (sdcard) {          
            String filename    =  "/PLANNING.CFG";
            File file = SD.open(filename);                                        // Open file for writing
            if (file) {
                DynamicJsonDocument doc(BUFFER_SIZE);
                DeserializationError error = deserializeJson(doc, file);    
                JsonArray result = doc.as<JsonArray>();            
                if (!error) {
                    #ifdef DEBUG 
                      serializeJsonPretty(doc, Serial);
                    #endif 
                    copyPlanning(result);                                              // velden kopieren naar de Planning
                }               
                file.close();                                                       // Close the file (File's destructor doesn't close the file)
                logDebug(21,"getPlanning()", filename.c_str(), 0, 0);   
                _backup = true;
            } else { 
                Serial.print(F("Fout, Aquarium configuratie niet leesbaar:")); 
                Serial.println(filename); 
            } 
      }
   }
      
   #ifdef DEBUG
        Serial.println(F("<<======================================<<"));     
   #endif
      
}

// *************************************** LOGGING & DEBUGGING FUNCTIES *********************************************//

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::parseMelding(uint8_t id, char *melding, float in1, float in2)
{
    uint8_t digits  = 0;
    char temp[200]  = {'\0'};
    char t1[20]     = {'\0'}; 
    char t2[20]     = {'\0'}; 

    Serial.print(F("Debug melding parse :"));
    Serial.println(String(melding)+" id="+String(id,DEC));

    strcpy(temp, melding);
    if (id < max_melding) {                                         // nu vanuit het programma geheugen. 
        strcat_P(temp, (char*)pgm_read_word(&(meldingen[id])));     // Necessary casts and dereferencing, just copy.
    } else {
        strcat(temp, "Onbekende meldingcode:");                     // mag eigenlijk niet voorkomen.
        strcat(temp, String(id,DEC).c_str());
    }
 
    digits = 0;                                                     // standaard geen cijfers achter de comma     
    if ((id >= 7)  and (id <= 9))  { digits = 2; }                  // voor meldingen met liters geven we 2 decimalen weer
    if ((id >= 10) and (id <= 11)) { digits = 1; }                  // voor meldingen met centimeters 1 decimaal 
    if ((id >= 16) and (id <= 17)) { digits = 1; }                  // voor meldingen over temperatuur 1 decimaal
    
    dtostrf(in1, (4 - digits), digits, t1);
    dtostrf(in2, (4 - digits), digits, t2);
    sprintf(melding, temp, t1, t2);
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::logDebug(uint8_t id, const char *event, const char *melding, const float in1, const float in2)
{
     
    if (!sdcard)  {                                                 // als we geen SD card hebben geinstalleerd, dan direct stoppen
        Serial.println(F("Debug melding: geen SD card"));
        return; 
    }                                      
    if (_isTestmode) {                                              // niets loggen als het systeem in testmode staat.
        Serial.println(F("Debug melding: systeem draait in testmodus"));        
        return; 
    }                                    
    
    char temp[200] = {'\0'};
    char datum[30] = {'\0'};
     
    strcpy(datum, "["); 
    aqua_functie.netteDatumTijd(datum);                            // opmaak datum tijd = dd-mm-jjjj uu:mm:ss
    strcat(datum, "] "); 
    
    parseMelding(id, temp, in1, in2);                                                                

    #ifdef DEBUG         
        Serial.println(F(">>-------------------------------------->>"));
        Serial.println(F("Debug melding:"));
        Serial.println(String(datum));
        Serial.println(String(event));
        Serial.println(String(temp));
        Serial.println(String(melding));
    #endif    

    File file = SD.open(logFile, FILE_WRITE);                      // Open file for writing
    if (file) {
        file.println(String(datum)+" ID:"+String(event)+ " "+String(temp)+" "+String(melding));
        file.close();                                              // Close the file (File's destructor doesn't close the file)
    } else { 
        #ifdef DEBUG         
            Serial.println(F("Fout bij opslaan debug info op sdcard.")); 
        #endif
    }               
    #ifdef DEBUG         
        Serial.println(F("<<--------------------------------------<<"));
    #endif
}


// ------------------------------------------------------------------------------------------------------------------//
// voor de 'vaste' meldingen moet PROGMEM worden gelezen, zie bovenaan deze pagina.
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::PROWLmelding(uint8_t id, const char *event, const float in1, const float in2) 
{
    if (_isTestmode) {             // geen berichten sturen als het systeem in testmode staat.
        Serial.println(F("PROWL melding: systeem draait in testmodus"));        
    return; }                                
    
    char melding[300] = {'\0'};
    char tijd[13]     = {'\0'}; 

    strcpy(tijd, "["); 
    aqua_functie.netteTijd(tijd);  
    strcat(tijd, "]  "); 
   
    parseMelding(id, melding, in1, in2);
    #ifdef DEBUG         
        Serial.println("melding="+String(melding));
        Serial.print(F("Connecting to Prowl..."));
    #endif

    client.stop();
    if ( client.connect(prowlserver,prowlport) ) {

      int contentLength = fixedContentLength + strlen(tijd) + strlen(event) + strlen(melding) + 1;
      #ifdef DEBUG         
         Serial.println("Prowl:" + String(event) + " (" + String(contentLength) + " bytes)");
      #endif  
      client.print(F("POST "));
      client.print(path); 
      client.println(F(" HTTP/1.1"));
  
      client.print("Host: ");
      client.println(prowlserver); // Important for HTTP/1.1 server, even though we decalre HTTP/1.0 
  
      client.println(F("User-Agent: Arduino-WiFly/1.0"));
      client.println(F("Content-Type: application/x-www-form-urlencoded"));
  
      client.print(F("Content-Length: "));
      client.println(contentLength);
  
      client.println(); // Important blank line between HTTP headers and body
  
      client.print(apikeyParam);
      client.print(prowlkey);
  
      client.print(applicationParam);
      client.print(application);
  
      client.print(eventParam);
      client.print(String(tijd)+String(event));
  
      client.print(descriptionParam);
      client.println(String(melding));
  
      #ifdef DEBUG  
        Serial.println(F("Client->disconnect"));
      #endif
      client.flush();  
      client.stop();
    } else {
     
      logDebug(25,"PROWLmelding()",melding,0,0);  
      #ifdef DEBUG         
          Serial.println(F("PROWL connection failed"));
      #endif
    }
}


// ************************************** VANAF HIER DE PRIVATE FUNCTIES ********************************************//


// ------------------------------------------------------------------------------------------------------------------//
// Open connection to the HTTP server
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::verbind_met_RichTech(uint16_t tcp) {
    client.stop();
    #ifdef DEBUG 
        Serial.print(F("Verbinden met "));
        Serial.println(richtech);
    #endif

    bool ok = client.connect(richtech, tcp);
    if (!ok) {  
        #ifdef DEBUG 
            Serial.print(F("Fout: client.connect()"));
            Serial.println(richtech);
        #endif
        verbreek_RichTech(); 
    }   
    return ok;
}


// -------------------------------------------------------------------------------------------------
// Send the HTTP GET request to the server
// -------------------------------------------------------------------------------------------------
bool serverclass::sendRequest(const char* resource) 
{
    
  #ifdef DEBUG 
    Serial.print(F("Client->connect: GET /aquacontrol"));
    Serial.println(String(resource)+" HTTP/1.0");
  #endif
    client.print(F("GET /aquacontrol"));
    client.print(resource);
    client.println(F(" HTTP/1.0"));
    client.print(F("Host: "));
    client.println(richtech);
    client.println(F("Connection: close"));
    bool ok = (client.println() != 0);
    if (ok) {
        // Check HTTP status
        char state[32] = {'\0'};
        client.readBytesUntil('\r', state, sizeof(state));
        bool is_ok = (strcmp(state, "HTTP/1.1 200 OK") == 0);

        if (!is_ok) {              
            Serial.print(F("Onverwachte HTTP reactie: "));
            Serial.println(state);  
//            PROWLmelding(1, "SendRequest", 0, 0);
            logDebug(2,"SendRequest()",state, 0, 0);         
            return false;
        } else {
            return true;
        }
    } else { 
      logDebug(2,"SendRequest()",resource, 0, 0);         
      return false; 
    }
}

// -------------------------------------------------------------------------------------------------
// Send the HTTP GET request to the NodeRed
// -------------------------------------------------------------------------------------------------
bool serverclass::sendNodeRed(const char* resource) 
{
    
  #ifdef DEBUG 
    Serial.print(F("Client->connect: GET /eheim/sync"));
    Serial.println(String(resource)+" HTTP/1.0");
  #endif
    client.print(F("GET /eheim/sync"));
    client.print(resource);
    client.println(F(" HTTP/1.0"));
    client.print(F("Host: "));
    client.println(richtech);
    client.println(F("Connection: close"));
    bool ok = (client.println() != 0);
    if (ok) {
        // Check HTTP status
        char state[32] = {'\0'};
        client.readBytesUntil('\r', state, sizeof(state));
        bool is_ok = (strcmp(state, "HTTP/1.1 200 OK") == 0);
        if (!is_ok) {              
            Serial.print(F("Onverwachte HTTP reactie: "));
            Serial.println(state);  
            logDebug(2,"SendRequest()",state, 0, 0);  
            return false;
        } else {
            Serial.println("state:"+String(state));
            return true;
        }    
    } else { 
      logDebug(2,"SendRequest()",resource, 0, 0);         
      return false; 
    }
}


// ------------------------------------------------------------------------------------------------------------------//
// Skip HTTP headers so that we are at the beginning of the response's body
// ------------------------------------------------------------------------------------------------------------------//
bool serverclass::skipResponseHeaders() {
  // HTTP headers end with an empty line
  char endOfHeaders[] = "\r\n\r\n";

  client.setTimeout(HTTP_TIMEOUT);
  bool ok = client.find(endOfHeaders);

  if (!ok) {
      Serial.println(F("HTTP header probleem"));
      logDebug(2,"HTTP /GET()","", 0, 0);   
  }
  return ok;
}

// ------------------------------------------------------------------------------------------------------------------//
// Close the connection with the HTTP server
// ------------------------------------------------------------------------------------------------------------------//
void serverclass::verbreek_RichTech() {  
  #ifdef DEBUG  
        Serial.println(F("Client->disconnect"));
  #endif
  client.flush();  
  client.stop();
  delay(1);
}



// server is nu de instantiatie van de class
serverclass server = serverclass();
