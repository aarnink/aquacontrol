#ifndef aqua_relais_h
#define aqua_relais_h

#include "globals.h"
#include "Arduino.h"


/*** Define input button/pin assignments ***/

class relaisclass
{
  public:
    relaisclass();

    void     init(Functie *relais, uint8_t id, bool opstart);

    void     lcdInfo(char *regel); 
    void     checkAllePlanningen(bool isOnderhoud);   

    void     relaisAan    (uint8_t pinID, bool isOnderhoud);
    void     relaisUit    (uint8_t pinID, bool isOnderhoud);
    void     relaisHerstel(uint8_t pinID, bool isOnderhoud);
    uint16_t relaisWacht(uint8_t pinID);

   
  private:
  
/* Custom Struct:
** 5 channels [0,1,2,3,4], elk hiervan is toegewezen aan een specifieke PWM pin.
** Includes additional vars to track the channel's 'state' to
** include current and suggested PWM levels.
*/
    Functie  _relais[aantalRelais];
    uint8_t  _lcd;
    
    void     debugRelais(uint8_t id); 
    uint32_t volgendeTijd(uint8_t pin);
    uint8_t  zoekRelaisIndex(uint8_t pin); 

    void     checkPlanning(Planning *this_event, bool isOnderhoud);
};




extern relaisclass aqua_relais;
#endif
