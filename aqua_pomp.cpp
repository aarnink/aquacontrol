#include "aqua_pomp.h"
#include "serverlib.h" 

// #define DEBUG                // veel informatie geven via de seriele monitor (deze regel commentaar maken om debug uit te zetten)

pompclass::pompclass() {
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void pompclass::init(Functie *pomp, uint8_t id, bool opstart) 
{ 
  _pomp[id].pin       = pomp->pin;        // pinnummer waarop deze is aangesloten
  _pomp[id].dbLevel   = pomp->dbLevel;    // ophalen vanuit de database
  _pomp[id].maxLevel  = pomp->maxLevel;   // ophalen vanuit de database
  _pomp[id].aantal    = pomp->aantal;     // aantal ml per seconden dat wordt gepompt
  _pomp[id].correctie = pomp->correctie;  // aantal ml per seconden dat wordt gepompt
  _pomp[id].schoonAan = pomp->schoonAan;  // aantal ml per seconden dat wordt gepompt
  _pomp[id].isAan     = pomp->isAan;      // hiermee schakelen we een led in het geheel aan of uit. 
  _pomp[id].logAan    = pomp->logAan;     // hiermee schakelen we een led in het geheel aan of uit. 
  _pomp[id].datumAan  = pomp->datumAan;
  _pomp[id].datumUit  = pomp->datumUit;
  strcpy(_pomp[id].kort,pomp->kort);      // korte omschrijving voor op het display 
    
  if (opstart) {                          // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _pomp[id].level     = 0;             // daarna niet meer anders knipperen de leds bij elke keer bij het inlezen van evt. wijzigingen 
     _pomp[id].suggested = 0;             // waarde wordt berekend als onderdeel van de planning 
     _pomp[id].isActief  = false;         // wordt bepaald vanuit de planning
     _lcd                = 0;
  }
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void pompclass::debugPomp(uint8_t id) 
{
  #ifdef DEBUG   
    Serial.println(F(""));
    Serial.print(F("Pomp PIN      :")); Serial.println(String(_pomp[id].pin,DEC));        // pinnummer waarop deze is aangesloten
    Serial.print(F("max Level     :")); Serial.println(String(_pomp[id].maxLevel,DEC));  // 
    Serial.print(F("ml/msec       :")); Serial.println(String(_pomp[id].aantal,DEC));
    Serial.print(F("dbLevel       :")); Serial.println(String(_pomp[id].dbLevel,DEC));
    Serial.print(F("correctie     :")); Serial.println(String(_pomp[id].correctie,DEC));
    Serial.print(F("isAan         :")); Serial.println(String(_pomp[id].isAan,DEC));     // 
    Serial.print(F("logAan        :")); Serial.println(String(_pomp[id].logAan,DEC));    //      
    Serial.print(F("isActief      :")); Serial.println(String(_pomp[id].isActief,DEC));  //  
    Serial.print(F("schoonAan     :")); Serial.println(String(_pomp[id].schoonAan,DEC)); //  
    Serial.print(F("datumAan      :")); Serial.println(String(_pomp[id].datumAan,DEC));  // 
    Serial.print(F("datumUit      :")); Serial.println(String(_pomp[id].datumUit,DEC));  // 
    Serial.println(F(""));
  #endif 
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void pompclass::lcdInfo(char *regel)
{
  unsigned long tijd,ml   = 0;
  uint32_t      h         = 0;
  uint16_t      m         = 0;
  
  #ifdef DEBUG   
    Serial.println(F("LCD tekst voor Doseerpompen bepalen ")); 
    Serial.print(F("Pomp naam     :")); Serial.println(String(_pomp[_lcd].kort));
    Serial.print(F("Pomp pinID    :")); Serial.println(String(_pomp[_lcd].pin,DEC));
    Serial.print(F("Pomp Actief   :")); Serial.println(String(_pomp[_lcd].isActief,DEC));
  #endif 

  regel[0]     = {'\0'}; 
  char temp[5] = {'\0'};

  if (strlen(_pomp[_lcd].kort) != 0)  { 
      aqua_functie.strcopylen(regel, _pomp[_lcd].kort, 9); 
  } else {           
      strcpy(regel, "[-leeg-] ");
  }

  if (!(_pomp[_lcd].isAan)) {
      strcat(regel, " blijft uit");
  } else {  
      ml = server.eindPlanning(_pomp[_lcd].pin,1);                        // functie = 1, resultaat is veld maximaal van de planning, deze geeft het aantal ml. aan.
      if (ml > 0) {                                                       // isActief geeft aan dat de Pomp is geactiveerd, dus dat er een planning loopt          .
              temp[0] = {'\0'};   
              strcat(regel, " klaar ");                                   
              ltoa(ml, temp, 10);  
              if (ml < 10) { strcat(regel, " "); }                        
              strcat(regel, temp);          
              strcat(regel, "ml");
      }  else {                                                           // de Pomp is niet actief, geen planning actief.
          tijd = server.volgendeTijd(_pomp[_lcd].pin);                    // toon wanneer de Pomp functie actief wordt
          if (tijd != 0) {        
              temp[0] = {'\0'};       
              strcat(regel, " aan  "); 
              h = (tijd / 3600);
              ltoa(h, temp, 10);
              if (h < 10) { strcat(regel, " "); }                        
              strcat(regel, temp);
              strcat(regel, ":");
              m = ((tijd % 3600) / 60);              
              if (m < 10) { strcat(regel, "0"); }          
              ltoa(m, temp, 10);            
              strcat(regel, temp);            
          } else {         //12345678901   
              strcat(regel, " blijft uit");  
          }
      }   
  }  
  if (_lcd < (aantalPompen-1)) { _lcd++; } else { _lcd = 0; }             // de volgende ronde geven we info over de volgende pomp

  #ifdef DEBUG   
    Serial.print(F("LCD tekst     :")); Serial.println(String(regel));
    Serial.println("");        
  #endif 
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
uint8_t pompclass::zoekPompIndex(uint8_t pin) 
{
  uint8_t i = 0;
  for (i = 0; i < aantalPompen; i++) {
      if (pin == _pomp[i].pin) {
          return i;
          break;
      }
  }
  return 255; //max waarde betekent dat we niets gevonden hebben.
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void pompclass::checkAllePlanningen()
{
  #ifdef DEBUG         
    Serial.println(F(">>-------------------------------------->>"));
    Serial.println(F(">>  POMP planning controle uitvoeren    >>"));    
    Serial.println(F(">>-------------------------------------->>"));
  #endif
  
  for (uint8_t i = 0; i < max_planning; i++) {
      if (server.schedule[i].id > 0) {
          checkPlanning(&server.schedule[i]);
      }
  }  
  
  #ifdef DEBUG         
    Serial.println(F("<<--------------------------------------<<"));
    Serial.println(F("")); Serial.println(F("")); 
  #endif

}



// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void pompclass::checkPlanning(Planning *dezePlanning)
{
  uint8_t        pomp_idx = 0;
  unsigned long  aan, secNow;
  char           oms[40];
  char           temp[10];
  uint32_t       nu       = now();

  #ifdef DEBUG
      Serial.print(F("planning ID: ")); Serial.println(String(dezePlanning->id, DEC));
  #endif

  uint8_t i = 0;
  secNow    = aqua_functie.Time2Sec(hour(), minute(), second());                   // exacte tijdstip nu in seconden   
  aan       = aqua_functie.Time2Sec(dezePlanning->uur, dezePlanning->min, 0); 
  
  while ((dezePlanning->pins[i].pin != 0) and (i < max_pins)) {                     // pin 0 bestaat niet
     pomp_idx = zoekPompIndex(dezePlanning->pins[i].pin);                           // zoek in de array van pompen naar de ppmp met dit specifieke PinNummer
     if (pomp_idx < 255) {                                                         // pomp_idx 255 betekent dat de pomp niet is gevonden 

          bool inDatum = (((_pomp[pomp_idx].datumAan <= nu ) or (_pomp[pomp_idx].datumAan == 0)) and ((_pomp[pomp_idx].datumUit >= nu ) or (_pomp[pomp_idx].datumUit == 0)));
          if ( (_pomp[pomp_idx].isAan) and (inDatum)) {
            
              #ifdef DEBUG                 
                    Serial.print(F("Pomp PIN   : ")); Serial.println(String(_pomp[pomp_idx].pin, DEC));
//                  debugPomp(pomp_idx);
              #endif

              // omdat de planning zich elke 24uur herhaalt, moeten we een reset doen aan het einde van de planning,
              // anders komt deze planning morgen niet meer aan de beurt.
          
              if (secNow > (aan + dezePlanning->duur)) {       
                  dezePlanning->pins[i].afgerond = false;                                   // dus reset aan einde van de planning
                  #ifdef DEBUG
                      Serial.println(F("Planning buiten tijdvlak; reset uitgevoerd. "));
                  #endif
              } 

              // controle of we binnen het tijdsvenster van deze planning zijn, we checken het geplande moment 
              if ((secNow >= aan) and (secNow < (aan + dezePlanning->duur))) {                                
                                                                          
                  #ifdef DEBUG
                      Serial.print(F("Planning klaar:")); Serial.println(String(dezePlanning->pins[i].afgerond,DEC));
                  #endif

                  if (!(dezePlanning->pins[i].afgerond)) {
                      if (dezePlanning->maximaal <= _pomp[pomp_idx].maxLevel) {          // als er meer dan x ml moet worden gedoseerd, dan is er iets fout in de database, dit is wel erg veel
                          analogWrite(_pomp[pomp_idx].pin, 255);
                          delay(dezePlanning->maximaal * _pomp[pomp_idx].aantal);       // in aantal is het msec nodig voor 1ml opgeslagen voor deze specifieke pomp
                          analogWrite(_pomp[pomp_idx].pin, 0);  

                          if (_pomp[pomp_idx].logAan) {
                              if ( !(server.setMeting(aquaID, _pomp[pomp_idx].pin, dezePlanning->maximaal, "Ok")) ) {
                                  #ifdef DEBUG
                                      Serial.println(F("AquaPomp: dosering opslaan voor pomp mislukt"));
                                  #endif
                              }
                          }    
                      } else // bij fouten altijd loggen 
                      {            
                          if ( !(server.setMeting(aquaID, _pomp[pomp_idx].pin, dezePlanning->maximaal, "fout+teveel+ml")) ) {
                              Serial.println(F("AquaPomp: update pomp mislukt"));                               
                              aqua_iot.setCharWaarde('d',"dosering: teveel ml gevraagd ");
                          }

                          oms[0]  = {'\0'};  temp[0] = {'\0'};
                          strcpy(oms, _pomp[pomp_idx].kort);
                          strcat(oms, " planning-ID: "); itoa(dezePlanning->id, temp, 10); strcat(oms, temp);
                          server.PROWLmelding(12, oms, dezePlanning->maximaal, _pomp[pomp_idx].maxLevel);        
                      }

                      dezePlanning->pins[i].afgerond = true;                 // dosering uitgevoerd (voorkomen dat we dit nog een keer doen binnen 24 uur)
                  }
              }
          } else {
            #ifdef DEBUG            
                  Serial.print(F("Overslaan  : dubbele planning of uitgeschakeld voor Pomp "));
                  Serial.println(String(dezePlanning->pins[i].pin,DEC)); 
            #endif      
          }
          #ifdef DEBUG 
            Serial.println(F(""));
          #endif          
     }                                                                      // geen fout, maar de planning is niet van toepassing op deze pomp                                                                 
     i++; // controleer of voor deze planning nog meer leds actief moeten worden gemaakt. 
  }
}

pompclass aqua_pomp = pompclass();
