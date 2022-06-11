#include "aqua_led.h"

// #define DEBUG                // veel informatie geven via de seriele monitor (deze regel commentaar maken om debug uit te zetten)


ledclass::ledclass() {
}



// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void ledclass::debugLed(uint8_t id) 
{
  #ifdef DEBUG  
    Serial.println(F("= stamgegevens = "));
    Serial.print(F("Led PIN   : ")); Serial.println(String(_led[id].pin,DEC));       // pinnummer waarop deze is aangesloten
    Serial.print(F("max level : ")); Serial.println(String(_led[id].maxLevel,DEC));  // 
    Serial.print(F("level     : ")); Serial.println(String(_led[id].level,DEC));  // 
    Serial.print(F("aantal    : ")); Serial.println(String(_led[id].aantal,DEC));    // wordt niet gebruikt
    Serial.print(F("isAan     : ")); Serial.println(String(_led[id].isAan,DEC));     // 
    Serial.print(F("isActief  : ")); Serial.println(String(_led[id].isActief,DEC));  // 
    Serial.print(F("datum Aan : ")); Serial.println(String(_led[id].datumAan,DEC));  // 
    Serial.print(F("datum Uit : ")); Serial.println(String(_led[id].datumUit,DEC));  // 
    Serial.println(F(""));
  #endif
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void ledclass::lcdInfo(char *regel)
{
  uint8_t       i = 0;
  unsigned long tijd;
  uint32_t      h = 0;
  uint16_t      m = 0;
  
  regel[0]     = {'\0'}; 
  char temp[5] = {'\0'};

  if (strlen(_led[_lcd].kort) != 0)  { 
      aqua_functie.strcopylen(regel, _led[_lcd].kort, 9); 
  } else {           
      strcpy(regel, "[-leeg-] ");
  }

  if (!(_led[_lcd].isAan)) {
      strcat(regel, " blijft uit");
  } else {  
    if (_led[_lcd].isActief) {
      strcat(regel, "[" );
      if (_led[_lcd].level > 0) {
          itoa(_led[_lcd].level, temp, 10);         
          for (i = strlen(temp); i < 3; i++) { strcat(regel," "); }
          strcat(regel, temp);
          strcat(regel, "]-[");
      } 
      itoa(_led[_lcd].maxLevel, temp, 10);
      for (i = strlen(temp); i < 3; i++) { strcat(regel," "); }
      strcat(regel, temp);
      strcat(regel, "]");
    } else {
      tijd = server.volgendeTijd(_led[_lcd].pin);
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
      } else {           
          strcat(regel, " blijft uit");  
      }
    }
  }  
  if (_lcd < (aantalLeds-1)) { _lcd++; } else { _lcd = 0; }                       // de volgende ronde geven we info over de volgende pomp
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void ledclass::init(Functie *led, uint8_t id, bool opstart) 
{ 
  _led[id].pin       = led->pin;       // pinnummer waarop deze is aangesloten
  _led[id].maxLevel  = led->maxLevel;  // ophalen vanuit de database
  _led[id].aantal    = led->aantal;    // voor volledigheid ophalen, maar wordt niet gebruikt
  _led[id].isAan     = led->isAan;     // hiermee schakelen we een led in het geheel aan of uit. 
  _led[id].datumAan  = led->datumAan;
  _led[id].datumUit  = led->datumUit;

  strcpy(_led[id].kort,led->kort);     // korte omschrijving voor op het display 

  if (opstart) {                       // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _led[id].level     = 0;           // daarna niet meer anders knipperen de leds bij elke keer bij het inlezen van evt. wijzigingen 
     _led[id].suggested = 0;           // waarde wordt berekend als onderdeel van de planning 
     _led[id].isActief  = 0;           // wordt bepaald vanuit de planning       
     _lcd               = 0;
   }
  #ifdef DEBUG    
      debugLed(id);      
  #endif

}


// -------------------------------------------------------------------------------------------------
// Reset alle Led statussen
// -------------------------------------------------------------------------------------------------
void ledclass::resetLeds()
{
   uint8_t i = 0;

  for (i = 0; i < aantalLeds; i++)
  {
    _led[i].isActief = false;
    _led[i].suggested = 0;                  // bais all channel to an "off" state
  }
}


// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void ledclass::ledAan(uint8_t index)
{
  if (index < aantalLeds) {
      setPWM(index, 255); 
  }
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void ledclass::ledHerstel(uint8_t index)
{
  if (index < aantalLeds) {
      setPWM(index, _led[index].suggested); 
  }
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
void ledclass::checkAllePlanningen()
{
  resetLeds();                                               
  #ifdef DEBUG         
    Serial.println(F(">>-------------------------------------->>"));
    Serial.println(F(">>   LED planning controle uitvoeren    >>"));    
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
void ledclass::zelfTest(int sdelay)
{
  #ifdef DEBUG  
    Serial.println(F("zelf-test uitvoeren"));
  #endif
  
  for (uint8_t i = 0; i < aantalLeds; i++)
  {
    // ramp up
    rampPWM(i, 255, 8, sdelay);  // channel, setting, increments, speed-delay
    // ramp down
    rampPWM(i, 0, 8, sdelay);
  }
}


// -------------------------------------------------------------------------------------------------
/*
** Applies the PWM setting to the specified channel by setting the respective PWM pin.
** Updates the specified channels PWM setting tracking var.
** Toggles the LCD needs refresh flag
*/
// -------------------------------------------------------------------------------------------------
void ledclass::setPWM(uint8_t id, uint8_t gewenst)
{
  // Adjust PWM pin
  #ifdef DEBUG
    Serial.print(F("Stel LED ")); Serial.print(String(_led[id].pin,DEC)); 
    Serial.print(F(" in op "));   Serial.println(String(gewenst,DEC));
  #endif
    
  analogWrite(_led[id].pin, gewenst);
  // update the tracking var
  _led[id].level = gewenst;
}



// -------------------------------------------------------------------------------------------------
/*
** Adjusts the channels current PWM level to a new level.
** Adjustment is made transisionally via a ramping method.
*/
// -------------------------------------------------------------------------------------------------
void ledclass::rampPWM(uint8_t id, uint8_t gewenst, int stap, int sdelay)
{
  int huidig = _led[id].level;

  if (huidig > gewenst) {              //ramp down
    while (huidig > gewenst)
    {
      setPWM(id, huidig);
      delay(sdelay);
      huidig -= stap;
    }
  } else {                             //ramp up
    while (huidig < gewenst)
    {
      setPWM(id, huidig);
      delay(sdelay);
      huidig += stap;
    }
  }
  setPWM(id, gewenst);                // insure we stop on the exact desired setting
}



// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
uint8_t ledclass::zoekLedIndex(uint8_t pin) 
{
  for (uint8_t i = 0; i < aantalLeds; i++) {
      if (pin == _led[i].pin) {
          return i;
          break;
      }
  }
  return 255; //max waarde betekent dat we niets gevonden hebben.
}

// -------------------------------------------------------------------------------------------------
// Checks if this Channel's schedule falls within the current time/day
// -------------------------------------------------------------------------------------------------

void ledclass::checkPlanning(Planning *dezePlanning)
{
  unsigned long volAanBegin, volAanEind;
  unsigned long fadeInBegin, fadeUitEind;
  uint8_t       newlevel   = 0;
  uint8_t       led_idx,i  = 0;
  unsigned long sec_into, secNow;
  float         factor;
  bool          bereken    = true;
  uint32_t      nu         = now();
  
  #ifdef DEBUG
      Serial.print(F("planning ID: ")); Serial.println(String(dezePlanning->id, DEC));
  #endif


// hier moet een while do lus komen om voor deze planning alle leds te testen en bij te stellen.
  i = 0;
  while ((dezePlanning->pins[i].pin != 0) and (i < max_pins)) {                         // pin 0 bestaat niet
      led_idx = zoekLedIndex(dezePlanning->pins[i].pin);                                // zoek in de array van leds naar de led met dit specifieke PinNummer
      if (led_idx < 255) {                                                              // led_Idx 255 betekent dat de led niet is gevonden 
   
          bool inDatum = (((_led[led_idx].datumAan <= nu ) or (_led[led_idx].datumAan == 0)) and ((_led[led_idx].datumUit >= nu ) or (_led[led_idx].datumUit == 0)));
          
          #ifdef DEBUG                        
            Serial.print(F("Led PIN      : ")); Serial.println(String(_led[led_idx].pin, DEC));
            Serial.print(F("binnen datum : ")); Serial.println(String(inDatum, DEC));
            debugLed(led_idx);
          #endif

          if ( (!_led[led_idx].isActief) and (_led[led_idx].isAan) and (inDatum) ) {
          
              // Bereken het aantal seconden in het tijdvlak van deze planning
              // bepaal de begin (volAanBegin) en einddtijd (volAanEind) van het schema, bepaal dit in seconden.

              fadeInBegin = aqua_functie.Time2Sec(dezePlanning->uur, dezePlanning->min, 0);           // start van het programma (met een ramp-up tijd)
              
              volAanBegin = fadeInBegin +  dezePlanning->fadeIn;                         // nu is de maximale sterkte bereikt
              volAanEind  = fadeInBegin + (dezePlanning->duur - dezePlanning->fadeUit);  // tot deze tijd moet de sterkte maximaal blijven
    
              fadeUitEind  = fadeInBegin + dezePlanning->duur;

              secNow = aqua_functie.Time2Sec(hour(), minute(), second());                             // exacte tijdstip nu in seconden  
                
              bereken = false; 
              
              if ((secNow >= fadeInBegin) && (secNow < volAanBegin))                     // nu zijn we in het fade-in tijdvlak
              {
                  sec_into = secNow - fadeInBegin;                                        // how many seconds are we into the ramp period
                  factor   = (float)dezePlanning->fadeIn / (float)dezePlanning->maximaal; // slice the ramp window into equal portions to the max level allowed
                  newlevel = sec_into / factor; // how many slices are we into so far
                  bereken  = true; 
              }  
              
              if ((secNow >= volAanBegin) && (secNow <= volAanEind))                     // check of dit het tijdvlak is waarin de led volledig aan staat
              {
                  newlevel = dezePlanning->maximaal;                                     // in deze tijd brandt de led op maximale sterkte, later checken we of dit niet de max waarde van deze led overschrijdt         
                  bereken  = true; 
              } 
              
              if ((secNow > volAanEind) && (secNow <= fadeUitEind))                      // nu zijn we in het fade-out tijdvlak
              {
                  sec_into = secNow - volAanEind;                                        // how many seconds are we into the ramp period
                  factor   = (float)dezePlanning->fadeUit / (float)dezePlanning->maximaal; // slice the ramp window into equal portions to the max level allowed
                  newlevel = dezePlanning->maximaal - (sec_into / factor);              // how many slices are we into so far          
                  bereken  = true; 
              }  

              #ifdef DEBUG        
                  Serial.print(F("tijd nu    : "));Serial.println(String(secNow, DEC));
                  Serial.print(F("fadeIn     : "));Serial.println(String(fadeInBegin, DEC));
                  Serial.print(F("volAan     : "));Serial.println(String(volAanBegin, DEC));
                  Serial.print(F("volUit     : "));Serial.println(String(volAanEind, DEC));
                  Serial.print(F("fadeUit    : "));Serial.println(String(fadeUitEind, DEC));
                  Serial.print(F("berekenen? : "));Serial.println(String(bereken, DEC));
              #endif

              if (bereken) {
                  // If we got to this point, we calculated a level other than "off" state, so let's notify this channel of the newely desired PWM level.
                  // Make sure we are not above the desired max level threshold (we should never be if the math above is correct)
                  
                  if (newlevel > _led[led_idx].maxLevel)
                      newlevel = _led[led_idx].maxLevel;    // corrigeer als de max_level voor deze led wordt overschreden

                  #ifdef DEBUG         
                        Serial.print(F("oud   level: ")); Serial.println(String(_led[led_idx].level, DEC)); 
                        Serial.print(F("nieuw level: ")); Serial.println(String(newlevel, DEC)); 
                  #endif

                  _led[led_idx].suggested = newlevel;                                 // sla de berekende waarde voor de led op voor de volgende ronde
                  _led[led_idx].isActief  = true;                                     // markeer dat er nu een planning wordt uitgevoerd voor deze LED, om overlap met andere planningen te voorkomen

                  if (_led[led_idx].suggested != _led[led_idx].level) {
                      setPWM(led_idx, _led[led_idx].suggested);                       // stel de LED in op de nieuwe berekende waarde
                  }
              }
          } else {                                                                    // dubbel planningblok geconstateerd, meldt overlap
            #ifdef DEBUG  
                  // als isActief dan dubbele planning, anders uitgeschakeld of buiten datum          
                  Serial.print(F("Overslaan  : dubbele planning of uitgeschakeld voor Led "));
                  Serial.println(String(dezePlanning->pins[i].pin,DEC)); 
            #endif
            server.PROWLmelding(26, "Planning_led()", dezePlanning->id, dezePlanning->pins[i].pin);
            // TODO: netjes zou zijn hier ook een foutmelding te tonen (later uit te werken)           
          }
          #ifdef DEBUG 
            Serial.println(F(""));
          #endif
      }                                                                         
      i++; // controleer of voor deze planning nog meer leds actief moeten worden gemaakt. 
  }
}



ledclass aqua_led = ledclass();
