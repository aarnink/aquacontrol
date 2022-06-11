#ifndef aqua_pomp_h
#define aqua_pomp_h

#include "globals.h"
#include "Arduino.h"


/*** Define input button/pin assignments ***/

class pompclass // t.b.v. de doseringspompen
{
  public:
    pompclass();
    void            init(Functie *pomp, uint8_t id, bool opstart);

    void            lcdInfo(char *regel); 
    void            checkAllePlanningen();   
  private:
  
    Functie         _pomp[aantalPompen];
    uint8_t         _lcd;
    
    void            debugPomp(uint8_t id); 
    uint8_t         zoekPompIndex(uint8_t pin); 
    
    void            checkPlanning(Planning *this_event);
};




extern pompclass aqua_pomp;
#endif
