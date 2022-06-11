#ifndef aqua_led_h
#define aqua_led_h

#include "globals.h"
#include "Arduino.h"
#include "serverlib.h"

/*
** The channels (aka light modules outputs) that can be individually controlled. 
** Schedules are channel-centric, meaning you schedule each channel's "on" periods independantly. 
** You can create multiple schedules for the same channel, however common sense should be taken in that the schedules do not overlap or conflict.
** The begining of the "on" period is defined by the specific hour/minute, and (optional) day of the week.
** The end of the "on" period (aka "off") is defined by specifying a Duration (length in seconds from the start of the "on" period).
** Schedules define "on" periods only, as the default state for all channels is "off".
** An optional Ramping effect (transistional period off-to-on & on-to-off) can be defined.
** The Ramp option is defined in total seconds and is prepended to the front of the scheduled "on" period, and appended to the Duration.
** Setting Ramp to "0" disables Ramp feature, thus allowing instant "on" instant "off" periods
** Note: enabling the Ramp feature extends the total "on" period by twice the total Ramp seconds defined.
** Note: Total "on" periods (Duration plus Ramps) cannot cross into the following day (extend passed 23:59:59).


** > Reset all the PWM Channels. Default all PWM tracking states to off.
**    Clear the schedule flag. This provides a base-line behavior that
**    the schedules would need to modify in the next step.

** > Check all schedules against the current time. If the current time
**    falls within the range of a schedule, the schedule acts upon the
**    respective channel's state settings by assigning a PWM value. To
**    prevent other schedules from acting on this channel's state (aka
**    conflicting/overlaping) the "in schedule" flag is also toggled.
** > Apply all channel states to the PWM pin outputs. This scans the channel
**    tracking states and changes the respective PWM output of the
**    respective pin assignment if the state differs from the current setting.
** > Update the LCD. Examines the reflesh tracking vars and prints to the
**    LCD if needed.
** > Wait for a while.
***/

/*** Define input button/pin assignments ***/




class ledclass
{
  public:
    ledclass();

    void init(Functie *led, uint8_t id, bool opstart);    
    
    void lcdInfo(char *regel); 
    void resetLeds();
    void checkAllePlanningen();
    void ledAan    (uint8_t index);
    void ledHerstel(uint8_t index);   
    void zelfTest  (int sdelay);
  private:
  
/* Custom Struct:
** 5 channels [0,1,2,3,4], elk hiervan is toegewezen aan een specifieke PWM pin.
** Includes additional vars to track the channel's 'state' to
** include current and suggested PWM levels.
*/
    Functie   _led[aantalLeds];
    uint8_t   _lcd;
    
    void    debugLed    (uint8_t id);         
    uint8_t zoekLedIndex(uint8_t pin); 
    
    void    checkPlanning(Planning *this_event);
    void    setPWM(uint8_t id, uint8_t gewenst);
    void    rampPWM(uint8_t id, uint8_t gewenst, int stap, int sdelay);
};




extern ledclass aqua_led;
#endif
