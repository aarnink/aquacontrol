#include "globals.h" 
#include <TimeLib.h>


// ------------------------------------------------------------------------------------------------------------------//
// #define DEBUG   // veel informatie geven via de seriele monitor (deze regel commentaar maken om debug uit te zetten)
// ------------------------------------------------------------------------------------------------------------------//




// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
functieclass::functieclass() {
    for (uint8_t i = 0; i < max_fout; i++) {
        _fout[i] = 0;
    }
    _foutOpLCD   = 0;
    foutenTeller = 0;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void functieclass::init()
{
    for (uint8_t i = 0; i < max_fout; i++) {
        _fout[i] = 0;
    }
    _foutOpLCD   = 0;
    foutenTeller = 0;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void functieclass::strcopylen(char *target, char *source, uint8_t len) {
  target[0] = {'\0'};
  if (strlen(source) > len) {
    strncpy(target, source, len);
  } else 
  {
    strcpy(target, source);
    for (uint8_t i = strlen(target); i < len; i++) {
        strcat(target," ");  
    }
  }
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void functieclass::nieuweFout(uint8_t code)
{
  uint8_t i      = 0;                             // teller om de reeks te doorlopen
  bool    gemeld = false;                         // vaststellen of de code al gemeld is (niet 2x zelfde alarm melden)
 
  for (i = 0; i < max_fout; i++) {
     if (_fout[i] == code) { 
        gemeld = true; }      
  }

  if (gemeld == false) {                          // alleen als voor deze specifieke code nog geen alarm gemeld was, dan toevoegen
      _fout[foutenTeller] = code;                 // nieuwe foutcode toevoegen aan de reeks
      foutenTeller++;                             // er is een reden om het alarm te zetten, bij meerdere redenen wordt de teller verhoogd 
      switch (code) {
          case  9 : aqua_iot.setCharWaarde('d',"[A09] osmose vullen ");           break;       
          case 10 : aqua_iot.setCharWaarde('d',"[A10] osmosewater op ");          break;
          case 14 : aqua_iot.setCharWaarde('d',"[A14] Temperatuur wijkt af ");    break;
          case 15 : aqua_iot.setCharWaarde('d',"[A15] Teveel water in sump ");    break;
          case 23 : aqua_iot.setCharWaarde('d',"[A23] osmose sensor storing ");   break;
          case 33 : aqua_iot.setCharWaarde('d',"[A33] sump sensor storing ");     break;
          case 99 : aqua_iot.setCharWaarde('d',"[A99] geen WiFi ");               break;
          default : aqua_iot.setCharWaarde('d',"[A--] fout onbekend ");           break;           
      }
  }
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void functieclass::verwijderFout(uint8_t code)
{
  uint8_t  i,a    = 0;                            // tellers om de reeks te doorlopen en te sorteren
  bool     gemeld = false;                        // vaststellen of de code al gemeld is (niet 2x zelfde alarm melden)

  for (i = 0; i < max_fout; i++) {                // zoeken of er voor deze specifieke code een alarm is gezet
     if (_fout[i] == code) { 
        gemeld = true; 
        _fout[i] = 0;                             // reset de foutcode hier, om deze reden hoeft het alarm immers niet meer af te gaan
     }      
  }

  if (gemeld == true) {
      for (a = max_fout; a > 1; a--) {            // de foutcodes sorteren, alarm dat nu is uitgezet was misschien niet de laatste code in de rij
          for (i = 0; i < (a-1); i++) {
              if ((_fout[i] == 0) and (_fout[i+1] != 0))  {  // dan wisselen
                   _fout[i]   = _fout[i+1];
                   _fout[i+1] = 0;
              }
          }
      }      
      foutenTeller--;
   }
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
bool  functieclass::toonFouten(char* fout)
{
  #ifdef DEBUG                                    // controle van de foutmeldingen tonen
      Serial.print(F("Alarm codes   "));
      for (uint8_t i = 0; i < max_fout; i++) {
          Serial.print(F("[")); Serial.print(String(_fout[i],DEC)); Serial.print(F("] "));
      }
      Serial.println(F(""));
      Serial.print(F("FoutenTeller = ")); 
      Serial.println(String(foutenTeller,DEC));
      Serial.println(F(""));
  #endif

  if (foutenTeller > 0) {
     switch (_fout[_foutOpLCD]) {
                                  //12345678901234567890 
            case  9 : strcpy(fout, "[A09] osmose vullen ");
                      break;
            case 10 : strcpy(fout, "[A10] osmosewater op");
                      break;
            case 14 : strcpy(fout, "[A14] Temp wijkt af ");
                      break;
            case 15 : strcpy(fout, "[A15] Teveel water!");
                      break;
            case 23 : strcpy(fout, "[A23] osmose sensor ");
                      break;
            case 33 : strcpy(fout, "[A33] sump sensor   ");
                      break;
            case 99 : strcpy(fout, "[A99] geen WIFI     ");
                      break;
            default : strcpy(fout, "[A--] fout onbekend ");
                      break;
      }
      if (_foutOpLCD < foutenTeller-1) { _foutOpLCD++; } else { _foutOpLCD = 0; }
      return true;
  } else {
      return false;
  }
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void functieclass::netteDatumTijd(char* tijd)
{         
    char temp[10] = {'\0'}; 

    itoa(day(), temp, 10);              // dag
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);
    strcat(tijd, "-");

    itoa(month(), temp, 10);            // maand
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);
    strcat(tijd, "-");

    itoa(year(), temp, 10);            // jaar
    strcat(tijd, temp);
    strcat(tijd, " ");
    
    itoa(hour(), temp, 10);             // uur
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);     
    strcat(tijd, ":");
    
    itoa(minute(), temp, 10);          // minuten      
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);
    strcat(tijd, ":");
    
    itoa(second(), temp, 10);         // seconden
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);
}

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void functieclass::netteTijd(char* tijd)
{         
    char temp[4] = {'\0'};

    itoa(hour(), temp, 10);      
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);
    strcat(tijd, ":");
    
    itoa(minute(), temp, 10);      
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);
    strcat(tijd, ":");
    
    itoa(second(), temp, 10);      
    if ( strlen(temp) < 2 ) {
        strcat(tijd, "0");
    }
    strcat(tijd, temp);
}

// ------------------------------------------------------------------------------------------------------------------//
// Bereken het aantal seconden voor de specifieke dag.
// ------------------------------------------------------------------------------------------------------------------//
unsigned long functieclass::Time2Sec(unsigned long nhour, uint16_t nmin, uint8_t nsec)
{
  unsigned long total_sec = 0;

  total_sec = total_sec + (nhour * 3600);  // this should evaluate in 32-bit space
  total_sec = total_sec + (nmin * 60);     // this should evaluate in 16-bit space
  total_sec = total_sec + nsec;

  return total_sec;
}


unsigned char functieclass::h2_int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}


//  https://github.com/zenmanenergy/ESP8266-Arduino-Examples/tree/master/helloworld_serial
String functieclass::urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2_int(code0) << 4) | h2_int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

String functieclass::urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}





// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
iotclass::iotclass() {


}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
void iotclass::init()
{
    _iotdata.wifisignal      = 0;          //
    _iotdata.temperatuur     = 0;          // 
    _iotdata.osmose          = 0;          // osmose voorraad
    _iotdata.sump            = 0;
    _iotdata.koeling_status  = false;      // status ventilator (aan/uit)
    _iotdata.alarm_status    = false;
    _iotdata.onderhoud_status= false;
    _iotdata.opvoerpomp      = false;      // 
    _iotdata.skimmer         = false;
    _iotdata.wavemaker       = false;
    _iotdata.vakantiestand   = false;
    _iotdata.sdcard          = false;
    _iotdata.bijvullen       = false;

    _iotdata.onderhoud_btn   = false;         // inkomend HTML GET = "r"   Blynk pin# V11 
    _iotdata.koeling_btn     = false;         // inkomend HTML GET = "u"   Blynk pin# V10
    _iotdata.vakantie_btn    = false;         // inkomend HTML GET = "x"   Blynk pin# V15
    _iotdata.bijvullen_btn   = false;         // inkomend HTML GET = "y"   Blynk pin# V16
    
    _iotdata.ipaddress[0]    = {'\0'};
    _iotdata.debugregel[0]   = {'\0'};

}



void iotclass::setLongWaarde(byte soort, long waarde)
{
    if (soort == 'w') {
          _iotdata.wifisignal = waarde;
    } else {
          _iotdata.wifisignal = 0;
    } 
}


// ------------------------------------------------------------------------------------------------------------------//
//  voor uitgaande velden die moeten worden gevuld voor de App
// ------------------------------------------------------------------------------------------------------------------//
// r = onderhoudsknop (boolean)
// u = koeling knop (boolean)
// x = vakantie knop (boolean)
// y = bijvullen knop (boolean)
// ------------------------------------------------------------------------------------------------------------------//
void iotclass::setBoolWaarde(byte soort, bool waarde)
{
    switch (soort) {
        case 'm':    
          _iotdata.onderhoud_status = waarde;   
          break;
        case 'k':    
          _iotdata.koeling_status   = waarde;   
          break;
        case 'h':    
          _iotdata.vakantiestand    = waarde;
          break;
        case 'b':   
          _iotdata.bijvullen        = waarde;   
          break;
        case 'r':    
          _iotdata.onderhoud_btn    = waarde;   
          break;
        case 'u':    
          _iotdata.koeling_btn      = waarde;   
          break;
        case 'x':    
          _iotdata.vakantie_btn     = waarde;
          break;
        case 'y':   
          _iotdata.bijvullen_btn    = waarde;   
          break;
    }    
}

void iotclass::setFloatWaarde(byte soort, float waarde)
{
    switch (soort) {
        case 't':    
          _iotdata.temperatuur = waarde;
          break;
        case 'o':    
          _iotdata.osmose = waarde;
          break;
        case 's':    
          _iotdata.sump = waarde;
          break;
    }    
}

void iotclass::setCharWaarde(byte soort, const char *melding)
{
  if (soort == 'i') {
      if (strlen(melding) < 17) { 
          strcpy(_iotdata.ipaddress, melding); 
      } else {
          strcpy(_iotdata.ipaddress, "---.---.---.---");
          Serial.print(F("lengte IP address:")); 
          Serial.println(String(strlen(melding),DEC));
          Serial.println(String(melding));   
      }
  }

  if (soort == 'd') {
      if (strlen(melding) < 41) { 
          strcpy(_iotdata.debugregel, melding); 
      } else {
          strcpy(_iotdata.debugregel, "debug > max. lengte 40");
      }
  }
}


void iotclass::getCharWaarde(byte soort, char *melding)
{
  if (soort == 'i') {
      strcpy(melding, _iotdata.ipaddress);
  }  

  if (soort == 'd') {
      strcpy(melding, _iotdata.debugregel);
  }  
}


long iotclass::getLongWaarde(byte soort)
{
  if (soort == 'w') {   
      return _iotdata.wifisignal; }
  else {
      return 0;
  }
}

/* Inkomende velden
  bool        onderhoud_btn         // inkomend HTML GET = "r"   Blynk pin# V9 
  bool        koeling_btn           // inkomend HTML GET = "u"   Blynk pin# V6
  bool        vakantie_btn          // inkomend HTML GET = "x"   Blynk pin# V7
  bool        bijvullen_btn         // inkomend HTML GET = "y"   Blynk pin# V8
*/

bool iotclass::getBoolWaarde(byte soort)
{
    bool waarde = false;
    switch (soort) {
        case 'a':    
          waarde = _iotdata.alarm_status;     break;
        case 'b':   
          waarde = _iotdata.bijvullen;        break;
        case 'c':    
          waarde = _iotdata.sdcard;           break;
        case 'e':   
          waarde = _iotdata.skimmer;          break;
        case 'k':    
          waarde = _iotdata.koeling_status;   break;
        case 'm':    
          waarde = _iotdata.onderhoud_status; break;
        case 'p':    
          waarde = _iotdata.opvoerpomp;       break;
        case 'v':   
          waarde = _iotdata.wavemaker;        break;
        case 'h':   
          waarde = _iotdata.vakantiestand;    break;
        case 'r':   
          waarde = _iotdata.onderhoud_btn;    break;
        case 'u':   
          waarde = _iotdata.koeling_btn;      break;
        case 'x':   
          waarde = _iotdata.vakantie_btn;     break;
        case 'y':   
          waarde = _iotdata.bijvullen_btn;    break;
        default:
          waarde = false; 
          break;
    }    
    return waarde;
}


float iotclass::getFloatWaarde(byte soort)
{
    float waarde;
    switch (soort) {
        case 't':    
          waarde = _iotdata.temperatuur;    break;
        case 'o':    
          waarde = _iotdata.osmose;         break;
        case 's':    
          waarde = _iotdata.sump;           break;
        default:
          waarde = 0; 
          break;
    }    
    return waarde;
}


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//
functieclass    aqua_functie       = functieclass();
iotclass        aqua_iot           = iotclass();
// ------------------------------------------------------------------------------------------------------------------//
