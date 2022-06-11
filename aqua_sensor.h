#ifndef aqua_sensor_h
#define aqua_sensor_h

#include "Arduino.h"
#include "globals.h"


// ------------------------------------------------------------------------------------------------------------------//
class tempclass
{
  public:
    tempclass();

    void  init(Functie *temp, bool opstart);
    float gemiddelde();

  private:  
    float     _som      = 0;
    float     _gem      = 0;
    uint16_t  _aantal   = 0;
    Functie   _temp;
};


// ------------------------------------------------------------------------------------------------------------------//
class topoffclass
{
  public:
    topoffclass();

    void     init(Functie *top, bool opstart);
  
    float    meet(bool forcelog);
    bool     vulnodig();
    float    voorraad();

    bool     pompIsAan();

    void     pompStarten();
    void     pompStoppen();

  private:  
    Functie  _topOff;
    bool     _vakantie  = false;
    float    _osmose_voor_vullen;    
    bool     _pompIsAan = false;
    uint32_t _pompTijd  = 0;
    
    void     sorteer (int waarde[], uint8_t aantal);
};


// ------------------------------------------------------------------------------------------------------------------//
class sumpclass
{
  public:
    sumpclass();

    void     init(Functie *sump, bool opstart);
    float    stand(); 
  
    float    meet();
    bool     vulnodig(float correctie);

    bool     pompIsAan();
    void     pompStarten();
    void     pompStoppen();
  private:  
    Functie  _sump;
    bool     _pompIsAan = false;
    uint32_t _pompTijd  = 0;
    float    _osmose_voor_vullen;    
    
    void     sorteer (int waarde[], uint8_t aantal);
};


// ------------------------------------------------------------------------------------------------------------------//
class koelclass
{
  public:
    koelclass();

    void     init(Functie *koeler, bool opstart);

    bool     koelerControle();
    void     koelerStarten();
    void     koelerStoppen();
  private:  
    Functie  _koeler;
    bool     _koelerIsAan = false;
    uint32_t _koelerTijd  = 0;
};


// ------------------------------------------------------------------------------------------------------------------//
class ondhoudclass
{
  public:
    ondhoudclass();

    void     init(Functie *temp, bool opstart);
    bool     actie();
  private:  
    Functie  _onderhoud;

    void     checkRelais(uint8_t id, uint32_t begintijd, bool isOnderhoud);
    
};


// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//

class ping
{
  public:
    ping(uint8_t trigger, uint8_t echos, uint16_t minRange, uint16_t maxRange);  
    float meet_ping();
  private:
    unsigned long echoInMicroseconds();
    int _trigger;
    int _echo;
    int _minRange = -1;
    int _maxRange = -1;
};

// ------------------------------------------------------------------------------------------------------------------//
// ------------------------------------------------------------------------------------------------------------------//

extern tempclass    aqua_temp;
extern topoffclass  aqua_topoff;
extern sumpclass    aqua_sump;
extern ondhoudclass aqua_onderhoud;
extern koelclass    aqua_koeler;



#endif
