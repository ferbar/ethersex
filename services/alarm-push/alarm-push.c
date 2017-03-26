/*
 * Copyright (c) 2009 by Stefan Riepenhausen <rhn@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "alarm-push.h"

#include "protocols/ecmd/ecmd-base.h"
#include "hardware/adc/adc.h"
#include "protocols/httplog/httplog.h"

#include "core/eeprom.h"

uint16_t *alarm_push_lastAdc=NULL;

/*
  If enabled in menuconfig, this function is called during boot up of ethersex
*/
int16_t
alarm_push_init(void)
{
  MYDEBUGF ("init\n");
  uint8_t alarm_push_adcs=0;
  eeprom_restore(alarm_push_adcs, &alarm_push_adcs, sizeof(alarm_push_adcs));
  if(alarm_push_lastAdc) {
    free(alarm_push_lastAdc);
  }
  alarm_push_lastAdc = (uint16_t*) malloc(sizeof(int16_t)*alarm_push_adcs);

  return ECMD_FINAL_OK;
}

char *alarm_push_querystring(uint8_t alarm_push_adcs) {
    int len=alarm_push_adcs*10+1; // adc1=1234&
    char *buffer=(char*) malloc(len);
    char *p=buffer;
    for(int i=0; i < alarm_push_adcs; i++) {
      p+=snprintf_P(p, len - (p-buffer), PSTR("adc%d=%d&"),ALARM_PUSH_FIRST_ADC+i,alarm_push_lastAdc[i]);
    }
    *p=0;
    return buffer;
}

/*
  If enabled in menuconfig, this function is periodically called
  change "timer(100,app_sample_periodic)" if needed

  wird alle 5*20ms aufgerufen
*/
int16_t
alarm_push_periodic(void)
{
  static int periodicCounter=0;
  uint8_t alarm_push_adcs=0;
  eeprom_restore(alarm_push_adcs, &alarm_push_adcs, sizeof(alarm_push_adcs));

  for(int i=0; i < alarm_push_adcs; i++) {
    uint16_t adcVal = adc_get(ALARM_PUSH_FIRST_ADC+i); // ADC4 ist mit adc1 beschriftet, eigentlich uint16_t 0...1023 aber dann geht das < 0-5 ned gescheit
    if(adcVal > (alarm_push_lastAdc[i]+5) || adcVal < (alarm_push_lastAdc[i]-5)) {
      periodicCounter=0;
      char *buffer=alarm_push_querystring(alarm_push_adcs);
      MYDEBUGF("changed ADC%d: %d, last=%d buffer=%s\n", ALARM_PUSH_FIRST_ADC+i, adcVal, alarm_push_lastAdc[i], buffer);

      httplog_P(PSTR("%s"),buffer);
      free(buffer);
      alarm_push_lastAdc[i]=adcVal;
      return ECMD_FINAL_OK;
    }
    if(periodicCounter > (30*10)) { // 10 = 1 sekunde - wir senden unten dann das komplette update => ins lastAdc schreiben
      alarm_push_lastAdc[i]=adcVal;
    }
  }
  if(periodicCounter > (30*10)) { // 10 = 1 sekunde
    char *buffer=alarm_push_querystring(alarm_push_adcs);
    MYDEBUGF ("periodic 1min %s\n", buffer);
    httplog_P(PSTR("%s"),buffer);
    periodicCounter=0;
    free(buffer);
  }
  periodicCounter++;
  // MYDEBUGF("adc1: %d\n",adc_get(1));
  return ECMD_FINAL_OK;
}

/*
  This function will be called on request by menuconfig, if wanted...
  You need to enable ECMD_SUPPORT for this.
  Otherwise you can use this function for anything you like 
*/
int16_t
alarm_push_onrequest(char *cmd, char *output, uint16_t len){
  MYDEBUGF ("onrequest\n");
  // enter your code here

  return ECMD_FINAL_OK;
}

// http://www.ethersex.de/index.php/Own_module
// alarm_push_periodic 1. Parameter = 20ms delay schritte

/*
  -- Ethersex META --
  header(services/alarm-push/alarm-push.h)
  ifdef(`conf_ALARM_PUSH_INIT_AUTOSTART',`init(alarm_push_init)')
  ifdef(`conf_ALARM_PUSH_PERIODIC_AUTOSTART',`timer(5,alarm_push_periodic())')
*/
