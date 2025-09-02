/*
----------------------------------------------------------------------
File        : Resource.h
Content     : Main resource header file of weather forecast demo
---------------------------END-OF-HEADER------------------------------
*/

#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdlib.h>
#include "GUI.h"

#ifndef GUI_CONST_STORAGE
  #define GUI_CONST_STORAGE const
#endif
extern GUI_CONST_STORAGE GUI_FONT GUI_FontVerdana20;
extern GUI_CONST_STORAGE GUI_FONT GUI_FontVerdana32;
  
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_ceiling_led_fixture_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_ceiling_led_fixture_on;  
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_chandelier_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_chandelier_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_hanging_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_hanging_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_led_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_led_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_console_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_console_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_single_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_spot_single_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_stairs_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_stairs_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_wall_off;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_lights_wall_on;
extern GUI_CONST_STORAGE GUI_BITMAP bmicons_settings;
  
extern GUI_CONST_STORAGE GUI_BITMAP bmSijalicaOn;
extern GUI_CONST_STORAGE GUI_BITMAP bmSijalicaOff;
extern GUI_CONST_STORAGE GUI_BITMAP bmTermometar;
extern GUI_CONST_STORAGE GUI_BITMAP bmHome;

extern GUI_CONST_STORAGE GUI_BITMAP bmCLEAN;
extern GUI_CONST_STORAGE GUI_BITMAP bmblindMedium;
extern GUI_CONST_STORAGE GUI_BITMAP bmbaflag;
extern GUI_CONST_STORAGE GUI_BITMAP bmengflag;
extern GUI_CONST_STORAGE GUI_BITMAP bmnext;
extern GUI_CONST_STORAGE GUI_BITMAP bmalHamliQR;
extern GUI_CONST_STORAGE GUI_BITMAP bmprevious;
extern GUI_CONST_STORAGE GUI_BITMAP bmVENTILATOR_OFF;
extern GUI_CONST_STORAGE GUI_BITMAP bmVENTILATOR_ON;
extern GUI_CONST_STORAGE GUI_BITMAP bmwifi;
extern GUI_CONST_STORAGE GUI_BITMAP bmmobilePhone;
extern GUI_CONST_STORAGE GUI_BITMAP bmcolorSpectrum;
extern GUI_CONST_STORAGE GUI_BITMAP bmblackWhiteGradient;
extern GUI_CONST_STORAGE GUI_BITMAP bmdefrosterico;
extern GUI_CONST_STORAGE GUI_BITMAP bmdefrostericoOn;

extern const unsigned long thstat_size;
extern const unsigned char thstat[];

#endif // RESOURCE_H
/************************ (C) COPYRIGHT JUBERA D.O.O Sarajevo ************************/
