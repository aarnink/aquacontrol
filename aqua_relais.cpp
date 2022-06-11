#include "aqua_relais.h"
#include <Wire.h>
#include "serverlib.h" 

#define DEBUG    // veel informatie geven via de seriele monitor (deze regel commentaar maken om debug uit te zetten)

relaisclass::relaisclass() {
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void relaisclass::init(Functie *relais, uint8_t id, bool opstart) 
{ 
  _relais[id].pin       = relais->pin;       // pinnummer waarop deze is aangesloten
  _relais[id].dbLevel   = relais->dbLevel;   // niet gebruikt
  _relais[id].maxLevel  = relais->maxLevel;  // niet gebruikt
  _relais[id].aantal    = relais->aantal;    // bevat het pinnummer van de bijbehorende LED 
  _relais[id].correctie = relais->correctie; // gebruikt als wachttijd in seconden bij inschakelen na onderhoud.
  _relais[id].isAan     = relais->isAan;     // hiermee schakelen we een relais in het geheel aan of uit (qua planning). 
  _relais[id].logAan    = relais->logAan;    // wel of niet vastleggen in de database.
  _relais[id].datumAan  = relais->datumAan;
  _relais[id].datumUit  = relais->datumUit;
  strcpy(_relais[id].kort,relais->kort);     // korte omschrijving voor op het display 

 
  if (opstart) {                             // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _relais[id].level     = 0;              // level = 0 als de onderhoudsknop UIT staat en 255 als deze ACTIEF is. 
     _relais[id].suggested = 0;              // planning houdt hierin bij of de relais aan of uit moet zijn  
     _relais[id].isActief  = false;          // wordt bepaald vanuit de planning
     _lcd                  = 0;
     relaisUit(_relais[id].pin, false);      // bij de 1ste initalisatie relais goedzetten.
  }
}




// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void relaisclass::debugRelais(uint8_t id) 
{
  #ifdef DEBUG   
      Serial.println("");
      Serial.print(F("relais PIN    :"));Serial.println(String(_relais[id].pin,DEC)+"");   // pinnummer waarop deze is aangesloten
      Serial.print(F("suggested     :"));Serial.println(String(_relais[id].suggested,DEC));  // 
      Serial.print(F("isAan         :"));Serial.println(String(_relais[id].isAan,DEC));     // 
      Serial.print(F("isActief      :"));Serial.println(String(_relais[id].isActief,DEC));  // 
      Serial.print(F("datumAan      :"));Serial.println(String(_relais[id].datumAan,DEC));  // 
      Serial.print(F("datumUit      :"));Serial.println(String(_relais[id].datumUit,DEC));  // 
      Serial.println("");
  #endif 
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void relaisclass::lcdInfo(char *regel)
{
  unsigned long tijd    = 0;
  uint32_t      h       = 0;
  uint16_t      m       = 0;
  bool          aan,uit = false;
  char          temp[9] = {'\0'};
  
  #ifdef DEBUG   
    Serial.println(F("LCD tekst voor Relais bepalen ")); 
    Serial.print(F("Relais naam   :")); Serial.println(String(_relais[_lcd].kort));
    Serial.print(F("Relais pinID  :")); Serial.println(String(_relais[_lcd].pin,DEC));
    Serial.print(F("Relais Actief :")); Serial.println(String(_relais[_lcd].isActief,DEC));
  #endif 

  regel[0]     = {'\0'}; 

  if (strlen(_relais[_lcd].kort) != 0)  { 
      aqua_functie.strcopylen(regel, _relais[_lcd].kort, 9); 
  } else {           
      strcpy(regel, "[-leeg-] ");
  }

  for (uint8_t i = 0; i < aantalRelais; i++) {
      if (_relais[i].kort == "Opv.pomp" ) {
          aqua_iot.setBoolWaarde('p',!_relais[i].isAan);
      }
      if (_relais[i].kort == "Skimmer" ) {
          aqua_iot.setBoolWaarde('e',!_relais[i].isAan);
      }
      
      if (_relais[i].kort == "Wavemaker" ) {
          aqua_iot.setBoolWaarde('v',!_relais[i].isAan);
      }
  }


  if (!(_relais[_lcd].isAan)) {
      strcat(regel, " blijft uit");

  
  } else {  
      aan = false;
      uit = false;
      if (_relais[_lcd].isActief) {                                         // isActief geeft aan dat de Relais is geactiveerd, dus dat er een planning loopt
          tijd = server.eindPlanning(_relais[_lcd].pin,0);                  // toon wanneer de relais functie weer uitgeschakeld wordt
          aan  = true;
      } else {                                                              // de relais is niet actief, geen planning actief.
          tijd = server.volgendeTijd(_relais[_lcd].pin);                    // toon wanneer de relais functie actief wordt
          uit  = true;
      }  
      if (tijd != 0) {        
          temp[0] = {'\0'};      
          if (aan) { strcat(regel, " aan  "); }                             // de tekst is 'andersom', omdat bij activering van het relais
          if (uit) { strcat(regel, " uit  "); }                             // het aangesloten apparaat juist wordt uitgeschakeld.

          h = (tijd / 3600);
          ltoa(h, temp, 10);
          if (h < 10) { strcat(regel, " "); }                        
          strcat(regel, temp);
          strcat(regel, ":");
          m = ((tijd % 3600) / 60);              
          if (m < 10) { strcat(regel, "0"); }          
          ltoa(m, temp, 10);            
          strcat(regel, temp);            
      } else {           
          strcat(regel, " blijft aan");                                             // omgekeerd, een bekrachtigde relais zorgt ervoor dat er geen 220V is.
      }   
  }  
  if (_lcd < (aantalRelais-1)) { _lcd++; } else { _lcd = 0; }                       // de volgende ronde geven we info over de volgende pomp

  #ifdef DEBUG   
    Serial.print(F("LCD tekst     :")); Serial.println(String(regel));
    Serial.println("");        
  #endif 
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
uint16_t relaisclass::relaisWacht(uint8_t pinID) 
{
  for (uint8_t idx = 0; idx < aantalRelais; idx++) {
      if (_relais[idx].pin == pinID) {
         return _relais[idx].correctie;
      }
  }  
  return 0;
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void relaisclass::relaisHerstel(uint8_t pinID, bool isOnderhoud)
{
  for (uint8_t idx = 0; idx < aantalRelais; idx++) {
      if (_relais[idx].pin == pinID) {

          #ifdef DEBUG   
              Serial.println(F("Relais waarden herstellen ")); 
              Serial.print(F("Relais naam   :")); Serial.println(String(_relais[idx].kort));
              Serial.print(F("Relais pinID  :")); Serial.println(String(_relais[idx].pin,DEC));
              Serial.print(F("Relais Actief :")); Serial.println(String(_relais[idx].isActief,DEC));
              Serial.print(F("Relais Suggest:")); Serial.println(String(_relais[idx].suggested,DEC));
          #endif 

          if (_relais[idx].suggested == 255) {
              relaisAan(pinID, isOnderhoud);
          } else {
              relaisUit(pinID, isOnderhoud);
          }          
      }
  }
}


// -------------------------------------------------------------------------------------------------
// Relais aan = actief = apparaat schakelt UIT
// -------------------------------------------------------------------------------------------------
void relaisclass::relaisAan(uint8_t pinID, bool isOnderhoud) 
{
  for (uint8_t idx = 0; idx < aantalRelais; idx++) {
      if (_relais[idx].pin == pinID) {
          if ((_relais[idx].suggested == 255) or (isOnderhoud)) {                             // suggested wordt bijgehouden in de planning

              digitalWrite(_relais[idx].pin, LOW);            
              delay(25);
              _relais[idx].level = 255;
              analogWrite(_relais[idx].aantal, 0);                                            // aantal bevat het pinnummer waarop de led is aangesloten;
  
              #ifdef DEBUG   
                Serial.print(F("Relais aan:   #")); Serial.print(String(_relais[idx].pin,DEC)); // dus onderbreken.
                Serial.print(F(" - Led uit:   #")); Serial.println(String(_relais[idx].aantal,DEC));
              #endif
          } 
      }
  }
}


// -------------------------------------------------------------------------------------------------
// Relais uit = niet actief = apparaat schakelt AAN/IN 
// -------------------------------------------------------------------------------------------------
void relaisclass::relaisUit(uint8_t pinID, bool isOnderhoud) 
{
  for (uint8_t idx = 0; idx < aantalRelais; idx++) {
      if (_relais[idx].pin == pinID) {    
          if ((_relais[idx].suggested == 0) and (!isOnderhoud)) {       // suggested wordt bijgehouden in de planning, als deze 0 is, is er een planning actief en mag de relais niet uit
              digitalWrite(_relais[idx].pin, HIGH);
              delay(25);
              _relais[idx].level = 0;                 // level wordt bijgehouden voor de onderhoudsknop, is de level 0 dan is deze uitgeschakeld.
              analogWrite(_relais[idx].aantal, 255);  // aantal bevat het pinnummer waarop de led is aangesloten;

              #ifdef DEBUG   
                Serial.print(F("Relais uit:   #")); Serial.print(String(_relais[idx].pin,DEC));
                Serial.print(F(" - Led aan:   #")); Serial.println(String(_relais[idx].aantal,DEC));
              #endif
          } 
      }
  }
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
uint8_t relaisclass::zoekRelaisIndex(uint8_t pin) 
{
  for (uint8_t i = 0; i < aantalRelais; i++) {
      if (pin == _relais[i].pin) {
          return i;
          break;
      }
  }
  return 255; //max waarde betekent dat we niets gevonden hebben.
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void relaisclass::checkAllePlanningen(bool isOnderhoud)
{
  #ifdef DEBUG         
    Serial.println(F(">>-------------------------------------->>"));
    Serial.println(F(">> RELAIS planning controle uitvoeren   >>"));    
    Serial.println(F(">>-------------------------------------->>"));
  #endif
  
  for (uint8_t i = 0; i < max_planning; i++) {
      if (server.schedule[i].id > 0) {
          checkPlanning(&server.schedule[i], isOnderhoud);
      }
  }  
  
  #ifdef DEBUG         
    Serial.println(F("<<--------------------------------------<<"));
    Serial.println(F("")); Serial.println(F("")); 
  #endif

}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

void relaisclass::checkPlanning(Planning *dezePlanning, bool isOnderhoud)
{
  uint8_t        relais_idx = 0;
  unsigned long  aan, secNow;
  uint32_t       nu         = now();


  // hier moet een while do lus komen om voor deze planning voor alle Pompen uit te voeren
  uint8_t i = 0;
  secNow    = aqua_functie.Time2Sec(hour(), minute(), second());                            // exacte tijdstip nu in seconden   
  aan       = aqua_functie.Time2Sec(dezePlanning->uur, dezePlanning->min, 0); 


  #ifdef DEBUG
      Serial.print(F("Planning ID   -")); Serial.println(String(dezePlanning->id, DEC));
  #endif
  
  while ((dezePlanning->pins[i].pin != 0) and (i < max_pins)) {                             // pin 0 bestaat niet
     relais_idx = zoekRelaisIndex(dezePlanning->pins[i].pin);                               // zoek in de array van pompen naar de ppmp met dit specifieke PinNummer
     if (relais_idx < 255) {                                                                // pomp_idx 255 betekent dat de pomp niet is gevonden 

          bool inDatum = (((_relais[relais_idx].datumAan <= nu ) or (_relais[relais_idx].datumAan == 0)) and ((_relais[relais_idx].datumUit >= nu ) or (_relais[relais_idx].datumUit == 0)));
          if ( (_relais[relais_idx].isAan) and (inDatum)) {
            
              // BUITEN tijdvak
              if (secNow > (aan + dezePlanning->duur)) {       
                  dezePlanning->pins[i].afgerond = false;                                   // dus reset aan einde van de planning
                  _relais[relais_idx].suggested  = 0;
                  _relais[relais_idx].isActief   = false;
                  relaisUit(_relais[relais_idx].pin, isOnderhoud);
                  #ifdef DEBUG
                    Serial.print(F("Relais: ")); Serial.print(String(_relais[relais_idx].pin,DEC)); Serial.println(F(" is INACTIEF (230V=Aan)."));  
                  #endif
              }
          
              // BINNEN tijdvlak
              if ((secNow >= aan) and (secNow < (aan+dezePlanning->duur)))                  // check of dit het tijdvlak is waarin de relais actief moet worden
              {
                  if (!(dezePlanning->pins[i].afgerond))  { 
                    
                      _relais[relais_idx].suggested = 255;
                      _relais[relais_idx].isActief = true;
                      relaisAan(_relais[relais_idx].pin, isOnderhoud);
                      
                      #ifdef DEBUG
                        Serial.print(F("Relais        :")); Serial.print(String(_relais[relais_idx].pin,DEC)); Serial.println(F(" is ACTIEF (230V=UIT)."));  
                      #endif
                      
                      if (_relais[relais_idx].logAan) {
                          if ( !(server.setMeting(aquaID, _relais[relais_idx].pin, 1, "Ok")) ) {                        
                          }
                      }
                      
                  }
              }  
          } // geen fout, maar de planning is niet van toepassing op dit relais 
     }                                                                
     i++; // controleer of voor deze planning nog meer relais actief moeten worden gemaakt. 
  }
}



relaisclass aqua_relais = relaisclass();
