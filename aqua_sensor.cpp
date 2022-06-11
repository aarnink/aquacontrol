#include "aqua_sensor.h" 
#include "serverlib.h" 
#include "aqua_led.h"
#include "aqua_relais.h"
#include <Wire.h>
#include "OneWire.h"
#include "DallasTemperature.h"

#define DEBUG    // veel informatie geven via de seriele monitor (deze regel commentaar maken om debug uit te zetten)

#define TEMPERATURE_PRECISION 9

OneWire                   oneWire(tempPin);
DallasTemperature         temperatuur(&oneWire);  // Pass our oneWire reference to Dallas Temperature. 
DeviceAddress             insideThermometer;      // arrays to hold device address 

ping topOff(topOffPin,topOffPin+2, 20, 1000);     // Each sensor's trigger pin, echo pin, and max distance to ping. 
ping sump(sumpPin,sumpPin+2, 20, 1000); 

// ------------------------------------------------------------------------------------------------------------------//
tempclass::tempclass() {
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void tempclass::init(Functie *temp, bool opstart)
{ 
  _temp.pin       = temp->pin;       // pinnummer waarop deze is aangesloten
  _temp.dbLevel   = temp->dbLevel;   // niet gebruikt
  _temp.maxLevel  = temp->maxLevel;  // niet gebruikt
  _temp.aantal    = temp->aantal;    // bevat het pinnummer van de bijbehorende sensor 
  _temp.correctie = temp->correctie; // niet gebruikt
  _temp.isAan     = temp->isAan;     // hiermee schakelen we een relais in het geheel aan of uit (qua planning). 
  _temp.logAan    = temp->logAan;    // wel of niet vastleggen in de database.
  _temp.datumAan  = temp->datumAan;
  _temp.datumUit  = temp->datumUit;
  strcpy(_temp.kort,temp->kort);     // korte omschrijving voor op het display 

 
  if (opstart) {                     // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _temp.level     = 0;            // level = 0 als de onderhoudsknop UIT staat en 255 als deze ACTIEF is. 
     _temp.suggested = 0;            // planning houdt hierin bij of de relais aan of uit moet zijn  
     _temp.isActief  = false;        // wordt bepaald vanuit de planning
  }

  if ((_temp.isAan) and (opstart)) {
      temperatuur.begin();
      if (!temperatuur.getAddress(insideThermometer, 0)) Serial.println(F("Temperatuur fout: geen geheugenadres voor sensor"));
      temperatuur.setResolution(insideThermometer, 10);

  }
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
float tempclass::gemiddelde() {
  float          meet;        
  
  if (_temp.isAan) {  // alleen als de temperatuursensor aan staat
      
      if (_aantal > 60) {                         // elke 60 metingen reset en opnieuw gemiddelde bepalen
         _som     = _gem;                         // laatste gemiddelde meting wordt nu het totaal
         _aantal  = 1;
      }
      delay(10);
      temperatuur.requestTemperatures();
      meet = temperatuur.getTempC(insideThermometer);
    
      if ((meet > 0) and (meet < 40)) {           // zorgen dat we een geldige waarde hebben
        _aantal++; 
        _som = _som + meet;
        _gem = _som / _aantal;
      }

      #ifdef DEBUG                 
        Serial.println(F("")); Serial.println(F("Temperatuur"));
        Serial.print(F("meting nu : "));Serial.println(String(meet,DEC));
        Serial.print(F("Som meting: "));Serial.println(String(_som,DEC));
        Serial.print(F("# metingen: "));Serial.println(String(_aantal,DEC));
        Serial.print(F("Gemiddeld : "));Serial.println(String(_gem,DEC));
        Serial.println(F(""));
      #endif

      if (_temp.logAan)   {
          if (((minute() % _temp.aantal) == 0) and (_temp.timer != minute())) {  
              if ( server.setMeting(aquaID, tempPin, _gem, "Ok") != 0 )  {
                  Serial.println("TMP fout");
              }
          }    
          _temp.timer = minute();
      }    

      if (_aantal > 9) {
          if ((_gem < 24) or (_gem > 26.5)) {       // waarschuwen als we buiten de veilige waarden komen van een zeeaquarium
              if (!_temp.prowl)  {
                  server.PROWLmelding(13, "Temperatuur", _gem, 0);
                  _temp.prowl = true;
              } 
          } else { _temp.prowl = false; }
      }

      return _gem;
  } else return -127;
}




// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
topoffclass::topoffclass() {
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void topoffclass::init(Functie *top, bool opstart)
{ 
  _topOff.pin       = top->pin;       // pinnummer waarop deze is aangesloten
  _topOff.dbLevel   = top->dbLevel;   // als deze waarde overschreven wordt, wordt een waarschuwing afgegeven
  _topOff.maxLevel  = top->maxLevel;  // als deze waarde overschreven wordt, wordt alarm geslagen
  _topOff.aantal    = top->aantal;    // aantal minuten tussen 2 metingen 
  _topOff.correctie = top->correctie; // cm aftrekken van de meetwaarde
  _topOff.isAan     = top->isAan;     // hiermee schakelen we de sensor aan of uit 
  _topOff.logAan    = top->logAan;    // wel of niet vastleggen in de database.
  _topOff.datumAan  = top->datumAan;  // niet gebruikt
  _topOff.datumUit  = top->datumUit;  // niet gebruikt
  strcpy(_topOff.kort,top->kort);     // korte omschrijving voor op het display 
  
  if (opstart) {                      // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _topOff.level     = 0;           // level = 0 als de onderhoudsknop UIT staat en 255 als deze ACTIEF is. 
     _topOff.suggested = 0;           // planning houdt hierin bij of de relais aan of uit moet zijn  
     _topOff.isActief  = false;       // wordt bepaald vanuit de planning
     _topOff.meting    = 0;           // bevat de Osmose voorraad (in liters)    
  }

  if ((_topOff.isAan) and (opstart)) { 
  }
}



// ------------------------------------------------------------------------------------------------------------------//
// foutmeldingen hier aanbrengen.
// ------------------------------------------------------------------------------------------------------------------//
float topoffclass::voorraad() 
{
    return _topOff.meting;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void topoffclass::sorteer(int waarde[], uint8_t aantal) 
{
    uint8_t i,j  = 0;
    int     temp = 0;  
    
    for (i = aantal-1; i > 0; i--)   // bubble sorteer;
    {
        for (j = 0; j < i; j++)
        {  
            if (waarde[j] > waarde[j+1]) { 
               temp        = waarde[j+1];
               waarde[j+1] = waarde[j]; 
               waarde[j]   = temp;
            }
        }
    }       
}


// ------------------------------------------------------------------------------------------------------------------//
// foutmeldingen hier aanbrengen.
// ------------------------------------------------------------------------------------------------------------------//
bool  topoffclass::vulnodig() 
{
  float  waarde = meet(false);  
  aqua_iot.setFloatWaarde('o', waarde);
  #ifdef DEBUG                 
        Serial.println(F("OSMOSE: controleren of voldoende water op voorraad is."));
        Serial.print(F("OSMOSE: waarde = ")); Serial.println(String(waarde,DEC));
        Serial.print(F("Prowl melding  = ")); Serial.println(String(_topOff.prowl,DEC));
  #endif 
  
  if (waarde > -100) {                                  
      if  (waarde >= _topOff.dbLevel)     {               // als er genoeg water is (boven minimum) 
          return false; 
      }
      
      if ((waarde <  _topOff.dbLevel) and           
          (waarde >= _topOff.maxLevel))   {               // waarschuwing voor te weinig osmose water 
          if (!_topOff.prowl) {
              server.PROWLmelding(9, "Waarschuwing:Osmose", waarde, 0);
              _topOff.prowl = true; 
          }
          return true;                                    // bijvullen als waterPin actief is (bijvullen = ingeschakeld)
      }
      
      if  (waarde < _topOff.maxLevel)    {                // alarm voor te weinig osmose water 
          if (!_topOff.prowl)  {                 
              server.PROWLmelding(9, "ALARM:Osmose", waarde, 0); 
              _topOff.prowl = true; 
          }
          aqua_functie.nieuweFout(9);                     // code 9 is osmose water is op, maar aquarium is nog vol genoeg
          return true; 
      } 
  } else {
      if (!_topOff.prowl) {                               // alleen als er nog geen melding is verstuurd
          server.PROWLmelding(14, "Osmose meting", waarde, 0);    
          _topOff.prowl = true;                           // we hebben alarm afgegeven, voorkomen dat we een storm van meldingen veroorzaken.
      }
      aqua_functie.nieuweFout(23);                        // code 23 is osmose sensor werkt niet
      return false;
  }
  return false;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
float topoffclass::meet(bool forcelog) 

//   -200 meetfout, meer dan aantal+5 metingen uitgevoerd zonder resultaat
//   -100 sensor is bewust uitgezet in de instellingen (zie database)
//    0 of hoger = aantal gemeten liters Osmose water

{  
  const uint8_t aantal      = 9;
  const uint8_t deler       = 3;
  uint8_t       i,t         = 0; 
  float         gem,ltr,x   = 0;
  int           waarde[aantal], m;     

  _vakantie = aqua_iot.getBoolWaarde('h');
  #ifdef DEBUG                 
        Serial.println(F("OSMOSE: meting uitvoeren."));
  #endif
  ltr = 0;
  if (_topOff.isAan) {                                                // alleen als de TopOff sensor aan staat

    for (i=0; i < aantal; i++) { waarde[i] = 0; }                     // met een schone lei beginnen
    i = 0; t = 0; 
    
    do {                  
        m         = topOff.meet_ping();                               // 
        if (m > 0) {                                                  // alleen bij correcte meetwaarde toevoegen aan de lijst
            x = round(m / deler);                                     // afronden op mm (in stappen bepaald door de deler, bij deler = 3 wordt afgerond op 3mm)
            m = x * deler;                                                
            waarde[i] = m; 
            i++; 
        }                                                             
        t++;
        delay(100);
    } while ((i < aantal) and (t <= (aantal+5)) );                    // na maximaal 5 extra pogingen stoppen

    
    #ifdef DEBUG                 
        Serial.println(F(""));
        Serial.println(F("Osmose: gemeten waarden "));
        for (uint8_t i = 0; i < aantal; i++) {  Serial.print(String(waarde[i],DEC)+" ");  } Serial.println(F(" (in cm) "));
    #endif

    if (t > (aantal+5)) {                                             // stoppen, geen of onvolledige lijst meetwaarden
        #ifdef DEBUG                 
          Serial.println(F("Osmose: teveel meetfouten. (exitcode -200)"));
          aqua_iot.setCharWaarde('d',"Osmose: teveel meetfouten");
        #endif
        server.logDebug(24,"topOff.meet()", "", t, m); 
        server.PROWLmelding(24, "TopOff.meet()", t, m);               // melding maken over teveel meetfouten.        
        return -200;  
    }                                      
    
    #ifdef DEBUG                 
          Serial.println(F("Osmose: meetwaarden sorteren"));     
    #endif
    sorteer(waarde,aantal);                                           // sorteren van laag naar hoog
    gem = 0;
    for (i = 2; i < aantal-2; i++) {                                  // laagste en hoogste 2 metingen blijven buiten beschouwing
        gem = gem + waarde[i];                                        // bij het berekenen van het gemiddelde.
    } 
         
    gem = (gem / (aantal - 4));                                       // 4 van het aantal aftrekken (hoogste en laagste 2 metingen bleven buiten beschouwing)
    gem = (round(gem / deler) * deler); 
    
    if (!_vakantie) {                                                 // als vakantiestand is uitgeschakeld, normale osmose voorraad aanspreken  
 //       gem = round(gem / 10) ;                                     // afronden op hele cm
        if ((gem - _topOff.correctie) <= 400) {
          ltr = (400 - (gem - _topOff.correctie)) * 0.025;            // 40cm is de hoogte van de bak; elke cm komt overeen met 250 cl (bij benadering)
        } else {
          ltr = 0;   
        }   
    } else {                                                          // in vakantiestand wordt een extern vat gebruikt van 60cm hoog. 
        if (gem > 600) { gem = 600; }                                 // we werken hier met mm omdat 1cm al bijna 1 liter is, bij afronden wordt de meting altijd minimaal 1 liter.
        ltr = (600 - gem) * 0.09;                                      // else cm komt overeen met 0,9 liter.
    }

    ltr = round(ltr * 10);                                            // we willen afronden op 1 decimaal, dat moet helaas in 2 stappen
    _topOff.meting = ltr / 10;                                        // wordt bijgehouden om het aantal liters voorraad te kunnen tonen in het display

    if (_topOff.logAan) 
    {
        if ((((minute() % _topOff.aantal) == 0) and (_topOff.timer != minute())) or (forcelog)) {  
            if ( server.setMeting(aquaID, _topOff.pin, ltr, "Ok") != 0 )  {
                  Serial.println(F("Osmose meting niet opgeslagen."));
            }
        }
        _topOff.timer = minute();
    }      


    #ifdef DEBUG                 
        Serial.print(F("vakantie stand    : ")); Serial.println(String(_vakantie,DEC));
        Serial.print(F("Osmose: gemiddeld : ")); Serial.println(String(gem,DEC));
        Serial.print(F("Osmose: ltr (x10) : ")); Serial.println(String(ltr,DEC));
        Serial.print(F("Osmose: afgerond  : ")); Serial.println(String(_topOff.meting,DEC));
        Serial.print(F("Osmose: dbLevel   : ")); Serial.println(String(_topOff.dbLevel,DEC));
        Serial.print(F("Osmose: maxLevel  : ")); Serial.println(String(_topOff.maxLevel,DEC));
        Serial.print(F("Osmose: PMP osmose: ")); Serial.println(String(_pompIsAan,DEC));
        Serial.println(F(""));
    #endif

    
    if  (_topOff.meting >= _topOff.dbLevel)     {                     // als er genoeg water is (boven minimum) 
        aqua_functie.verwijderFout(9);                                // 
        aqua_functie.verwijderFout(10);                               // verwijder eventuele fouten 
        aqua_functie.verwijderFout(23);                               // en geef OK code (0) terug
        _topOff.prowl = false;             
    }
          
    return _topOff.meting;
  } else {
      server.logDebug(19,"TopOff.meet()", "", 0, 0); 
      server.PROWLmelding(19, "TopOff.meet()", 0, 0);                 // melding maken dat sensor is uitgeschakeld.   
      return -100;
  }
}

bool  topoffclass::pompIsAan() 
{
    return _pompIsAan;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void  topoffclass::pompStarten() 
{
    _osmose_voor_vullen = _topOff.meting;
    float waarde = meet(false);
    int32_t nu    = now();

    if ((hour() > 0) and (hour() < 4)) {                              // tussen 0:00 en 04:00 uur in de nacht geen pomp activeren ivm zomer/wintertijd              
        server.logDebug(31, "osmose", "", waarde, 0);                 // omschakeling en de looptijd berekening van de pomp (kan niet tegen verspringen van de tijd)
        return;                                                       // wegschrijven in de log, zodat we weten dat dit zich heeft voorgedaan
    }


    if (_pompTijd != 0) {                                             // er was iets fout gegaan, we gaan de pomp niet aanzetten als dit op dezelfde  
        if ((nu - _pompTijd) < 86400) {                               // dag is. (86400 seconden in een dag)
            return; 
        } else {
            server.PROWLmelding(30, "osmose", waarde, 0);             // melding maken             
            server.logDebug(30, "osmose", "", waarde, 0);             // melding maken op SD card (log)  
        }
    }

    analogWrite(osmosePomp, 255); 
    #ifdef DEBUG                 
        Serial.println(F("Pomp osmosewater bijvullen ==> AAN")); 
    #endif
    _pompIsAan = true;
    _pompTijd  = nu;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void  topoffclass::pompStoppen() 
{
    float waarde = meet(true);
    int32_t nu    = now();

    if ((nu - _pompTijd) > 120) {                                     // als de pomp langer dan 120 seconden loopt, is er iets fout
        analogWrite(osmosePomp, 0);                                   
        _pompIsAan = false;

        server.PROWLmelding(28, "osmose", waarde, 0);                 // melding maken             
        server.logDebug(28, "osmose", "", waarde, 0);                 // melding maken op SD card (log)  
    } else {            
        if (( waarde > 9 ) or ( waarde < 0 ))  {                      // als 9 liter in het Osmose voorraad vat aanwezig is, dan stoppen. 
                                                                      // ook stoppen als er een foutcode is teruggegeven (-100, -200))
            analogWrite(osmosePomp, 0); 
            #ifdef DEBUG                 
                Serial.println(F("Pomp osmosewater bijvullen ==> UIT")); 
            #endif  
            _pompIsAan = false;
            _pompTijd  = 0;

            if (waarde > 0) {
                server.PROWLmelding(7, "Osmose gevuld", (waarde - _osmose_voor_vullen), waarde);  
                server.setDosering(aquaID, osmosePomp, (waarde - _osmose_voor_vullen), "liter+toegevoegd");
//                server.logDebug(7, "osmose gevuld", "", (waarde - _osmose_voor_vullen), waarde);      // melding maken op SD card (log)    
            } else {
                server.PROWLmelding(14, "Osmose meetfout", waarde, 0);                    
//                server.logDebug(14, "osmose meetfout", "", waarde, 0);                                // melding maken op SD card (log)    
            }
        }   
    }
}






// ------------------------------------------------------------------------------------------------------------------//
sumpclass::sumpclass() {
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void sumpclass::init(Functie *sump, bool opstart)
{ 
  _sump.pin       = sump->pin;       // pinnummer waarop deze is aangesloten
  _sump.dbLevel   = sump->dbLevel;   // als deze waarde overschreven wordt, wordt een waarschuwing afgegeven
  _sump.maxLevel  = sump->maxLevel;  // als deze waarde overschreven wordt, wordt alarm geslagen
  _sump.aantal    = sump->aantal;    // aantal minuten tussen 2 metingen 
  _sump.correctie = sump->correctie; // cm aftrekken van de meetwaarde
  _sump.isAan     = sump->isAan;     // hiermee schakelen we de sensor aan of uit 
  _sump.logAan    = sump->logAan;    // wel of niet vastleggen in de database.
  _sump.datumAan  = sump->datumAan;  // niet gebruikt
  _sump.datumUit  = sump->datumUit;  // niet gebruikt
  strcpy(_sump.kort,sump->kort);     // korte omschrijving voor op het display 
  
 
  if (opstart) {                    // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _sump.level     = 0;           // level = 0 als de onderhoudsknop UIT staat en 255 als deze ACTIEF is. 
     _sump.suggested = 0;           // planning houdt hierin bij of de relais aan of uit moet zijn  
     _sump.isActief  = false;       // wordt bepaald vanuit de planning
     _sump.meting    = 0;           // bevat de Osmose voorraad (in liters)    
  }

  if ((_sump.isAan) and (opstart)) {
      
  }
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
float sumpclass::stand() 
{
    return _sump.meting;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void sumpclass::sorteer(int waarde[], uint8_t aantal) 
{
    uint8_t i,j  = 0;
    int     temp = 0;  
    
    for (i = aantal-1; i > 0; i--)   // bubble sorteer;
    {
        for (j = 0; j < i; j++)
        {  
            if (waarde[j]  > waarde[j+1]) { 
               temp        = waarde[j+1];
               waarde[j+1] = waarde[j]; 
               waarde[j]   = temp;
            }
        }
    }       
}


// ------------------------------------------------------------------------------------------------------------------//
// foutmeldingen hier aanbrengen.
//    
//              1. alarm hoog water   
//   niveau   ________________________ => (0 - maxLevel)  (een negatief getal is dus hoog water)
//              2. veilige marge 
//            ======================== => 0 normale waterstand in de sump
//              2. veilige marge
//            ________________________ => dbLevel (bijv. +2)
//              3. bijvullen nodig 
//            ________________________ => maxLevel (bijv +5)
//              4. alarm laag water   
//  
// ------------------------------------------------------------------------------------------------------------------//
bool sumpclass::vulnodig(float correctie) 
{
  float waarde = meet();
  aqua_iot.setFloatWaarde('s',waarde);
  
  #ifdef DEBUG                 
        Serial.println(F("SUMP: controleren of aquarium moet worden bijgevuld."));
        Serial.print(F("SUMP: waarde     = ")); Serial.println(String(waarde,DEC));
        Serial.print(F("SUMP: dblevel    = ")); Serial.println(String(_sump.dbLevel,DEC));
        Serial.print(F("SUMP: maxlevel   = ")); Serial.println(String(_sump.maxLevel,DEC));
        Serial.print(F("SUMP: pmp bijvul = ")); Serial.println(String(_pompIsAan,DEC));       
  #endif 
  
  if (waarde > -100) {                                            // -100 en -200 zijn de stopcodes uit de functie meet() er is dan iets fout gegaan
      if  (waarde <= (0 - _sump.maxLevel)) {                      // [situtatie 1]: er is teveel water in de sump aanwezig, geef waarschuwing af.
          if (! (_sump.prowl) ) {                                 // alleen als er nog geen melding is verstuurd
              server.PROWLmelding(11, "SUMP meting", abs(waarde), 0 );                 
              _sump.prowl = true;
          }
          aqua_functie.nieuweFout(15);                            // code 15
          return false;    
      }

      if ( ( waarde > (0 - _sump.maxLevel)) and                   // [situtatie 2]: na correctie moet 0 de normale stand zijn
          (waarde < (_sump.dbLevel - correctie) )) {              // we hebben dus onder deze conditie niet teveel en niet te weinig water !                                         
          aqua_functie.verwijderFout(10);                         // verwijder eventuele fouten
          aqua_functie.verwijderFout(15);                              
          aqua_functie.verwijderFout(33);   
          _sump.prowl = false;                           
          return false; 
      }         
      
      if ((waarde >= (_sump.dbLevel - correctie)) and             // [situtatie 3]: is normaal, door verdamping moeten we steeds osmose water aanvullen
          (waarde < _sump.maxLevel))        { 
          return true;                                            // bijvullen is noodzakelijk 
      }         
                                       
      if  (waarde >  _sump.maxLevel)         {                    // [situtatie 4]: dit is een afwijking, blijkbaar hadden we geen mogelijkheid water aan 
          if (! (_sump.prowl) ) {                                 // te vullen als hierboven; alleen als er nog geen melding is verstuurd
              server.PROWLmelding(10, "SUMP meting", (waarde-correctie), 0);
              _sump.prowl = true;
          }  
          aqua_functie.nieuweFout(10);                            // code 10 is osmose water is op, bijvullen niet meer mogelijk
          return true; 
      }       
        
  } else {    
    if (! (_sump.prowl) ) {                                       // alleen als er nog geen melding is verstuurd
        server.PROWLmelding(14, "SUMP meting", waarde, 0);
        _sump.prowl = true;
    }  
    aqua_functie.nieuweFout(33);                                  // code 33 is sump sensor werkt niet of waterstand te hoog
    return false;
  }
  return false;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
float sumpclass::meet() 

//   -2 meetfout, meer dan 50 metingen uitgevoerd zonder resultaat
//   -1 sensor is bewust uitgezet in de instellingen (zie database)
//    0 of hoger = aantal gemeten liters Osmose water

{  
  const uint8_t aantal        = 9;
  const uint8_t deler         = 3;
  uint8_t       i,t           = 0; 
  float         gem,x         = 0;
  int           waarde[aantal], m;     

  #ifdef DEBUG                 
        Serial.println(F("SUMP: meting uitvoeren."));
  #endif

  if (_sump.isAan) {                                                  // alleen als de TopOff sensor aan staat

    for (i=0; i < aantal; i++) { waarde[i] = 0; }                     // met een schone lei beginnen
    i = 0; t = 0; 
    do {                  
        m         = sump.meet_ping();                                 //  
        if (m > 0) {                                                  // alleen bij correcte meetwaarde toevoegen aan de lijst
            x = round(m / deler);                                   // afronden op mm (in stappen bepaald door de deler, bij deler = 3 wordt afgerond op 3mm)
            m = x * deler;                                    
            waarde[i] = m; 
            i++; 
        }                                                             
        t++;
        delay(125);
    } while ((i < aantal) and (t <= (aantal+5)) );                    // na maximaal 5 extra pogingen stoppen


    #ifdef DEBUG                 
        Serial.println(F(""));
        Serial.println(F("Sump: gemeten waarden "));
        for (uint8_t i = 0; i < aantal; i++) {
            Serial.print(String(waarde[i],DEC)+" ");  
        } Serial.println(F(" (in mm) "));
    #endif

    if (t > (aantal+5)) {                                         // stoppen, geen of onvolledige lijst meetwaarden
        #ifdef DEBUG                 
          Serial.println(F("Sump: teveel meetfouten. (exitcode -200)"));                
          aqua_iot.setCharWaarde('d',"Sump: teveel meetfouten.");

        #endif
        server.logDebug(24,"sump.meet()", "", t, m); 
        server.PROWLmelding(24, "sump.meet()", t, m);             // melding maken over teveel meetfouten.        
        return -200; 
    }                                  
    
    sorteer(waarde,aantal);                                       // sorteren van laag naar hoog
    gem = 0;
    for (i = 1; i < aantal-1; i++) {                              // laagste en hoogste meting blijven buiten beschouwing
        gem = gem + waarde[i];                                    // bij het berekenen van het gemiddelde.
    }          
    gem = round ((gem / (aantal - 2)) / 10);

    gem = gem - _sump.correctie;                                  // correctie verwerken, let op: kan dus ook negatief zijn als de waterstand iets te hoog is
    _sump.meting = gem;                                           // wordt bijgehouden om het niveau van de SUMP te kunnen tonen in het display

    #ifdef DEBUG                 
        Serial.print(F("Sump: correctie= ")); Serial.println(String(_sump.correctie,DEC));
        Serial.print(F("Sump: gemiddeld= ")); Serial.print(String(gem,DEC)); Serial.println(F(" (na correctie)"));
    #endif


    return gem;
  } else {
    server.logDebug(19,"sump.meet()", "", 0, 0); 
    server.PROWLmelding(19, "sump.meet()", 0, 0);                 // melding maken over teveel meetfouten.        
    return -100;
  }
}



// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool  sumpclass::pompIsAan() 
{
    return _pompIsAan;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void  sumpclass::pompStarten() 
{
    float   voorraad   = aqua_topoff.meet(false);
    int32_t nu         = now();
    #ifdef DEBUG                 
        Serial.println(F(""));
        Serial.println(F(">>-------------------------------------->>"));
        Serial.println(F("Controle SUMP pomp starten "));
        Serial.print(F("pomp inschakeld  : ")); Serial.println(String(_pompTijd,DEC));
        Serial.print(F("tijd is nu       : ")); Serial.println(String(nu,DEC));
        Serial.print(F("pomp looptijd    : ")); Serial.println(String((nu - _pompTijd),DEC));
        Serial.print(F("Osmose voorraad  : ")); Serial.println(String(voorraad,DEC));
    #endif
    if (voorraad > 0) {
        _osmose_voor_vullen = voorraad;
    } else {
        _osmose_voor_vullen = 0;      
    }
    
    if ((hour() > 0) and (hour() < 4)) {                                        // tussen 0:00 en 04:00 uur in de nacht geen pomp activeren ivm zomer/wintertijd              
        server.logDebug(31, "sump", "", voorraad, 0);                           // omschakeling en de looptijd berekening van de pomp (kan niet tegen verspringen van de tijd)
        return;                                                                 // wegschrijven in de log, zodat we weten dat dit zich heeft voorgedaan
    }


   if (_pompTijd != 0) {                                                        // er was iets fout gegaan, we gaan de pomp niet aanzetten als dit op dezelfde  
        if ((nu - _pompTijd) < 86400) {                                         // dag is. (86400 seconden in een dag)
            Serial.println(F("afbreken, pomp was al eerder in noodstop."));
            return; 
        } else {
            server.PROWLmelding(29, "sump", voorraad, _sump.meting);            // melding maken             
            server.logDebug(29, "sump", "", voorraad, _sump.meting);            // melding maken op SD card (log)    
        }
    }
    
    if (voorraad > 1.5) {                                                       // alleen starten als de osmose voorraad > 1,5 ltr 
        analogWrite(sumpPomp, 255); 
        _pompIsAan = true;
        _pompTijd  = nu;

        #ifdef DEBUG                 
            Serial.println(F("Pomp aquarium bijvullen ==> AAN")); 
        #endif
    } 

    if ((voorraad <= 1.5) and (voorraad > -1)) {                                // voorkomen dat we actie ondernemen als er een foutcode is teruggeven            
         Serial.println(F("Fout: er is onvoldoende voorraaad Osmose-water"));
         server.PROWLmelding(10, "Aquarium vullen", voorraad, 0);             
         server.logDebug(10, "Aquarium vullen", "", voorraad, 0);               // melding maken op SD card (log)    
    }   
    #ifdef DEBUG                 
      Serial.println(F("<<--------------------------------------<<"));
      Serial.println(F(""));
    #endif                 
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void  sumpclass::pompStoppen() 
{
    float   voorraad   = 0;
    int32_t nu         = now();
    
    voorraad = aqua_topoff.meet(false);
    #ifdef DEBUG                 
        Serial.println(F(""));
        Serial.println(F(">>-------------------------------------->>"));
        Serial.println(F("Controle SUMP pomp stoppen "));
        Serial.print(F("pomp inschakeld  : ")); Serial.println(String(_pompTijd,DEC));
        Serial.print(F("tijd is nu       : ")); Serial.println(String(nu,DEC));
        Serial.print(F("pomp looptijd    : ")); Serial.println(String((nu - _pompTijd),DEC));
        Serial.print(F("Osmose voorraad  : ")); Serial.println(String(voorraad,DEC));
    #endif    

    if ((nu - _pompTijd) > 30) {                                                    // als de pomp langer dan 30 seconden loopt, is er iets fout
        analogWrite(sumpPomp, 0);                                   
        _pompIsAan = false;
        Serial.println(F("NOODSTOP: pomp loopt te lang."));
        aqua_iot.setCharWaarde('d',"NOODSTOP: pomp loopt te lang.");
        server.PROWLmelding(27, "sump", voorraad, _sump.meting);                    // melding maken             
//        server.logDebug(27, "sump", "", voorraad, _sump.meting);                    // melding maken op SD card (log)    
        
    } else {        
        if (  (!vulnodig(0) ) or (voorraad < 1) ) {                                 // houd een correctie aan van 0cm boven het minimum
            analogWrite(sumpPomp, 0);   
            _pompIsAan = false;
            _pompTijd  = 0;
            delay(1000);                                                            // pomp tot rust laten komen, zodat we daarna een 
            voorraad = aqua_topoff.meet(true);                                      // betere meting en dus melding maken van wat er toegevoegd is, we loggen dit ook 
            if (voorraad > 0) {
                server.PROWLmelding(8, "Aquarium gevuld", abs(_osmose_voor_vullen - voorraad), voorraad);             
                server.setDosering(aquaID, sumpPomp, abs(_osmose_voor_vullen - voorraad), "liter+toegevoegd");
                server.logDebug(8, "Aquarium gevuld", "Ok", abs(_osmose_voor_vullen - voorraad), voorraad);      // melding maken op SD card (log)    
            } else {
                server.PROWLmelding(8, "Aquarium gevuld-meting onjuist", 0, 0 );             
                server.logDebug(8, "Aquarium gevuld", "Fout", _osmose_voor_vullen, voorraad);                    // melding maken op SD card (log)                  
            }
            #ifdef DEBUG                 
                Serial.println(F("Pomp aquarium bijvullen ==> UIT")); 
            #endif  
        }
    } 
    #ifdef DEBUG                 
      Serial.println(F("<<--------------------------------------<<"));
      Serial.println(F(""));
    #endif                 
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
ondhoudclass::ondhoudclass() {
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void ondhoudclass::init(Functie *temp, bool opstart)
{ 
  _onderhoud.pin       = temp->pin;       // pinnummer waarop deze is aangesloten
  _onderhoud.dbLevel   = temp->dbLevel;   // niet gebruikt
  _onderhoud.maxLevel  = temp->maxLevel;  // niet gebruikt
  _onderhoud.aantal    = temp->aantal;    // bevat de inhoud van de beslistabel (welke relais moeten worden uitgezet)
  _onderhoud.correctie = temp->correctie; // niet gebruikt
  _onderhoud.isAan     = temp->isAan;     // hiermee schakelen we een relais in het geheel aan of uit (qua planning). 
  _onderhoud.logAan    = temp->logAan;    // wel of niet vastleggen in de database.
  _onderhoud.datumAan  = temp->datumAan;
  _onderhoud.datumUit  = temp->datumUit;
  _onderhoud.prowl     = false;
  strcpy(_onderhoud.kort,temp->kort);     // korte omschrijving voor op het display 

 
  if (opstart) {                          // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _onderhoud.level     = 0;            // level = 0 als de onderhoudsknop UIT staat en 255 als deze ACTIEF is. 
     _onderhoud.suggested = 0;            // planning houdt hierin bij of de relais aan of uit moet zijn  
     _onderhoud.isActief  = false;        // wordt bepaald vanuit de planning
  }

  if ((_onderhoud.isAan) and (opstart)) {
  }
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void ondhoudclass::checkRelais(uint8_t id, uint32_t begintijd, bool isOnderhoud)
{
  uint16_t wacht = aqua_relais.relaisWacht(relaisPins[id]);
  if (wacht > 0) {
      if ( (abs(now() - begintijd)) >= wacht) {
          aqua_relais.relaisHerstel(relaisPins[id], isOnderhoud);    
      }
  } else {                
    aqua_relais.relaisHerstel(relaisPins[id], isOnderhoud); 
  }
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool ondhoudclass::actie() 
{
    bool actief = false; 
    // eerst de waarde van de knop uitlezen (aan of uit)    
    delay(10);
    bool onderhoud_aan = aqua_iot.getBoolWaarde('r');               
    
    actief = ((!digitalRead(onderhoudPin)) or (onderhoud_aan));     
    aqua_iot.setBoolWaarde('m',actief);                                   // 'r' is de knop waarde, deze geven we mee aan NodeRED ter evaluatie
    
    Serial.print(F("actief status       : ")); Serial.println(String(actief,DEC));
    
    if (actief) {    
        _onderhoud.timer    = 0;
        _onderhoud.isActief = true;

        if (aantalRelais > 0) {
          if (  (_onderhoud.aantal == 1) or  (_onderhoud.aantal == 255) or 
               ((_onderhoud.aantal > 10) and (_onderhoud.aantal < 20))  or 
                (_onderhoud.aantal == 123) )
          { aqua_relais.relaisAan(relaisPins[0], actief); }                     // relais 1
          delay(25);     
        }
        
        if (aantalRelais > 1) {
            if (  (_onderhoud.aantal == 2) or  (_onderhoud.aantal > 100) or (_onderhoud.aantal == 12) or 
                 ((_onderhoud.aantal > 20) and (_onderhoud.aantal < 30)) )
            { aqua_relais.relaisAan(relaisPins[1], actief); }                   // relais 2
            delay(25);     
        }
        
        if (aantalRelais > 2) {
            if (  (_onderhoud.aantal == 3)  or  (_onderhoud.aantal > 100) or 
                  (_onderhoud.aantal == 13) or  (_onderhoud.aantal == 34) or  (_onderhoud.aantal == 23) )
            { aqua_relais.relaisAan(relaisPins[2], actief); }                     // relais 3
            delay(25);     
        }
        
        if (aantalRelais > 3) {
            if (  (_onderhoud.aantal == 4)  or  (_onderhoud.aantal > 200) or 
                  (_onderhoud.aantal == 14) or  (_onderhoud.aantal == 24) or (_onderhoud.aantal == 34) )
            { aqua_relais.relaisAan(relaisPins[3], actief); }                     // relais 4
            delay(25);     
        }
 
        aqua_led.ledAan(0);                                           // dwing SUMP led aan, zodat we goed licht hebben bij de werkzaamheden
        
        delay(10);
        if (!_onderhoud.prowl) {                                      // de vorige "loop" was de onderhoudschakelaar nog uitgeschakeld.
          server.PROWLmelding(5, "ONDERHOUD", 0, 0);                  // AAN
          _onderhoud.prowl = true;
        }
    } else {
        if (_onderhoud.timer == 0) {                                  // als de timer nog niet loopt, nu inschakelen
            _onderhoud.timer = now();                                 // timer neemt de huidige tijd over.
        }   
        if ((now() - _onderhoud.timer) >= _onderhoud.correctie) {
           _onderhoud.isActief = false;
        }

          if (aantalRelais > 0) {
          if (  (_onderhoud.aantal == 1)  or  (_onderhoud.aantal == 255) or 
               ((_onderhoud.aantal > 10) and  (_onderhoud.aantal < 20))  or 
                (_onderhoud.aantal == 123) )
          { checkRelais(0,_onderhoud.timer, actief);     }                          // relais 1
                       
        }
        
        if (aantalRelais > 1) {
          if (  (_onderhoud.aantal == 2) or  (_onderhoud.aantal > 100) or (_onderhoud.aantal == 12) or 
               ((_onderhoud.aantal > 20) and (_onderhoud.aantal < 30)) )
          { checkRelais(1,_onderhoud.timer, actief); }                              // relais 2
        }     
        
        if (aantalRelais > 2) {
          if (  (_onderhoud.aantal == 3)  or  (_onderhoud.aantal > 100) or 
                (_onderhoud.aantal == 13) or  (_onderhoud.aantal == 34) or  (_onderhoud.aantal == 23) )
          { checkRelais(2,_onderhoud.timer, actief); }                              // relais 3
        }
        
        if (aantalRelais > 3) {
          if (  (_onderhoud.aantal == 4)  or  (_onderhoud.aantal > 200) or 
                (_onderhoud.aantal == 14) or  (_onderhoud.aantal == 24) or (_onderhoud.aantal == 34) )
          { checkRelais(3,_onderhoud.timer, actief); }                              // relais 4
        }        
              
        if (_onderhoud.prowl) {                                           // de vorige "loop" was de onderhoudschakelaar nog ingeschakeld.
          server.PROWLmelding(6, "ONDERHOUD", 0, 0);                      // UIT    
          _onderhoud.prowl  = false;
        }

        aqua_led.ledHerstel(0);                                           // terug naar de orginele (planning) waarde
    }
    aqua_iot.setBoolWaarde('m',actief);                                   // m = maintenance/onderhoud
    return actief;  
}


// ------------------------------------------------------------------------------------------------------------------//
koelclass::koelclass() {
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void koelclass::init(Functie *koeler, bool opstart)
{ 
  _koeler.pin       = koeler->pin;        // pinnummer waarop deze is aangesloten
  _koeler.dbLevel   = koeler->dbLevel;    // temperatuur waarop de koeler aan gaat
  _koeler.maxLevel  = koeler->maxLevel;   // temperatuur waarop de koeler uit gaat
  _koeler.aantal    = koeler->aantal;     // niet gebruikt 
  _koeler.correctie = koeler->correctie;  // niet gebruikt
  _koeler.isAan     = false;              // hiermee schakelen we de sensor aan of uit 
  _koeler.logAan    = koeler->logAan;     // wel of niet vastleggen in de database.
  _koeler.datumAan  = koeler->datumAan;   // niet gebruikt
  _koeler.datumUit  = koeler->datumUit;   // niet gebruikt
  strcpy(_koeler.kort,koeler->kort);      // korte omschrijving voor op het display 
  
 
  if (opstart) {                          // bij de 1ste initialisatie moeten we alles even uitzetten;  
     _koeler.level     = 0;               // level = 0 als de onderhoudsknop UIT staat en 255 als deze ACTIEF is. 
     _koeler.suggested = 0;               // planning houdt hierin bij of de relais aan of uit moet zijn  
     _koeler.isActief  = false;           // wordt bepaald vanuit de planning
     _koeler.meting    = 0;               // bevat de Osmose voorraad (in liters)    
  }

  if ((_koeler.isAan) and (opstart)) {
      
  }
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool  koelclass::koelerControle() 
{
    float _graden = aqua_temp.gemiddelde();
    bool   zet_koeler_aan = aqua_iot.getBoolWaarde('u');
    
    #ifdef DEBUG                 
      Serial.println(F(""));
      Serial.println(F(">>-------------------------------------->>"));
      Serial.println(F("Controle koeling "));
      Serial.print(F("temperatuur  : ")); Serial.println(String(_graden,DEC));
      Serial.print(F("Blynk knop   : ")); Serial.println(String(zet_koeler_aan,DEC));
      Serial.print(F("ventilatoren : ")); Serial.println(String(_koelerIsAan,DEC));
    #endif

    if (_graden <= 0)    {                                               // foute meting, uitschakelen
      _koelerIsAan = false;  
      koelerStoppen();
      return false;
    }

    if (_koelerIsAan) {
        if ((_graden < 24.5) or ((!zet_koeler_aan) and (_graden <= 25.7))) {  
            koelerStoppen();
            server.PROWLmelding(17, "KOELING", _graden, 0);   // Koeling UIT melden
            _koelerIsAan = false;
        }
    } 

    if ((!_koelerIsAan) and ((_graden >= 25.9) or (zet_koeler_aan))) {    // als ventilator uit is en temp > 26 dan inschakelen
        koelerStarten();
        _koelerIsAan = true;
        server.PROWLmelding(16, "KOELING", _graden, 0);   // Koeling AAN melden
    } 

    aqua_iot.setBoolWaarde('k', _koelerIsAan);
        
    #ifdef DEBUG                 
      Serial.println(F("<<--------------------------------------<<"));
    #endif
    
    return _koelerIsAan;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void  koelclass::koelerStarten() 
{
    int32_t nu         = now();
    #ifdef DEBUG                 
      Serial.println(F(""));
      Serial.println(F("Ventilatoren starten "));
      Serial.print(F("tijd is nu   : ")); Serial.println(String(nu,DEC));       
    #endif            

    analogWrite(koelerPin, 255); 
    _koelerTijd       = now();
      
    #ifdef DEBUG                 
      Serial.println(F(""));
    #endif                 
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void  koelclass::koelerStoppen() 
{
    int32_t nu         = now();
    
    #ifdef DEBUG                 
      Serial.println(F(""));
      Serial.println(F("Ventilatoren stoppen "));
      Serial.print(F("ingeschakeld : ")); Serial.println(String(_koelerTijd,DEC));
      Serial.print(F("tijd is nu   : ")); Serial.println(String(nu,DEC));
    #endif    

    analogWrite(koelerPin, 0); 
    _koelerTijd = 0;
    
    #ifdef DEBUG                 
      Serial.println(F(""));
    #endif                 
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
ping::ping(uint8_t trigger, uint8_t echos, uint16_t minRange, uint16_t maxRange)
{
    _trigger  = trigger;
    _echo     = echos;
    _minRange = minRange;
    _maxRange = maxRange;
}

unsigned long ping::echoInMicroseconds()
{    
    pinMode(_trigger, OUTPUT);
    pinMode(_echo, INPUT);

    // Make sure that trigger pin is LOW.
    digitalWrite(_trigger, LOW);
    delayMicroseconds(4);
    // Hold trigger for 10 microseconds, which is signal for sensor to measure distance.
    digitalWrite(_trigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigger, LOW); //was _trigger
    // Measure the length of echo signal, which is equal to the time needed for sound to go there and back.
    noInterrupts();
    long durationMicroSec = pulseIn(_echo, HIGH, 1000000UL);
    interrupts();
    return durationMicroSec;
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
float ping::meet_ping()
{
    unsigned long duration = echoInMicroseconds();
    
    // Given the speed of sound in air is 343,6m/s at 21 Celius = 3436cm/s = 0.03436cm/us).
    if (duration > 0) {
      float distance = (duration / 2) * 0.34438;  
      if (_minRange == -1 && _maxRange == -1) { return distance; }
  
      if (distance >= _minRange && distance <= _maxRange)
      {
          return distance;
      } else {
          return -1;  
      }
  } else {
     #ifdef DEBUG                 
          Serial.println(F("Ongeldige meting : (-1) "));
     #endif             
     return -1;
  }
}



// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
tempclass    aqua_temp       = tempclass();
topoffclass  aqua_topoff     = topoffclass();
sumpclass    aqua_sump       = sumpclass();
ondhoudclass aqua_onderhoud  = ondhoudclass();
koelclass    aqua_koeler     = koelclass();
// ------------------------------------------------------------------------------------------------------------------//
