#ifndef serverlib_h
#define serverlib_h

#include "Arduino.h"
#include "ArduinoJson.h"
#include "globals.h"
#include <SPI.h>
#include "WiFiEsp.h"


class serverclass
{
  public:
    serverclass();
    WiFiEspClient client;
    
    bool      verbind();                                                                      // verbind met de centrale mySQL database op de centrale server
    long      runs_from_backup();                                                             // true als minimaal 1 van de backup files is gebruikt.

    long      getWifiStatus();                                                                // lees de WiFi sterkte en geef deze terug, zodat dit op het LCD display kan worden getoond
//    char      getIPnummer();  
    bool      getAquarium (char *tekst, uint8_t aquaID, uint16_t *ververs, bool *gewijzigd);    
    void      getPlanning (uint8_t aquaID, bool opstart);   
    bool      getFunctie  (Functie *functie, uint8_t aquaID, uint8_t pinID);                  //    
    bool      setMeting   (uint8_t aquaID, uint8_t pinID, float waarde, const char *opm);
    bool      setDosering (uint8_t aquaID, uint8_t pinID, float waarde, const char *opm);
    bool      testMode();
    bool      setUpdated  (uint8_t aquaID);
    uint32_t  volgendeTijd(uint8_t pin);
    uint32_t  eindPlanning(uint8_t pin, uint8_t functie);
    
    bool      synchroniseer_IoT();
    
    void      PROWLmelding(uint8_t id, const char *event, const float in1, const float in2); 
    void      logDebug    (uint8_t id, const char *event, const char *melding, const float in1, const float in2);

    bool      resetCount  (uint8_t increment);
    Planning  schedule[max_planning];            // Custom Struct Defines "on" periods to apply to a given channel for a given time and duration. 

  private:
    const uint8_t reqbuf = 255;
    char      richtech[16];
    char      ssid[8];                                                                        // wireless network name
    char      password[14];                                                                   // wireless password
    char      verzoek[255];  // zie reqbuf
    uint8_t   poort;                                                                          // server poort (van de server)  
    uint16_t  nodered;                                                                        // poortnummer Node-Red IoT server.
    bool      _isTestmode               = false;         
    int       status                    = WL_IDLE_STATUS;
    uint8_t   _error                    = 0;
    bool      _backup                   = false;                                              // werken in backup modus (geen verbinding server)
    long      _geen_verbinding_tijd;                                                          // moment dat verbinding met server verloren ging.
    
    void      parseMelding(uint8_t id, char *melding, float in1, float in2);
    String    ip2Str(IPAddress ip);

    bool      verbind_met_RichTech(uint16_t tcp);
    void      verbreek_RichTech();

    bool      iotdata_to_request(char *out);
    bool      json_to_iotdata(JsonObject& in);
    
    void      leegFunctie(Functie *functie);
    void      copyFunctie(Functie *functie, JsonObject& result);
    
    void      leegPlanning();
    void      copyPlanning(JsonArray& result);
    void      sortPlanning();                           //
    uint8_t   zoekPlanningIndex(uint8_t pin);     
    
    bool      sendRequest(const char* resource);
    bool      sendNodeRed(const char* resource);
    bool      skipResponseHeaders();
    int       tekst_int(const char* invoer, uint8_t s, uint8_t a); 

};


extern serverclass server;

#endif
