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

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "config.h"
#include "alarm-push.h"
#include "protocols/ecmd/ecmd-base.h"

#include "core/eeprom.h"

int16_t parse_cmd_alarm_push_init(char *cmd, char *output, uint16_t len) 
{
  return alarm_push_init();
}

int16_t parse_cmd_alarm_push_periodic(char *cmd, char *output, uint16_t len) 
{
  return alarm_push_periodic();
}

int16_t parse_cmd_alarm_push_status(char *cmd, char *output, uint16_t len) 
{
  uint8_t alarm_push_adcs=0;
  eeprom_restore(alarm_push_adcs, &alarm_push_adcs, sizeof(alarm_push_adcs));
  MYDEBUGF("parse_cmd_alarm_push_status adcs:%d first:%d\n",alarm_push_adcs,ALARM_PUSH_FIRST_ADC);
  char *p=output;
  p+=snprintf_P(p, len, PSTR("adcs:%d first:%d "),alarm_push_adcs,ALARM_PUSH_FIRST_ADC);
  for(int i=0; i < alarm_push_adcs; i++) {
    p+=snprintf_P(p, len - (p-output), PSTR("adc%d=%d&"),ALARM_PUSH_FIRST_ADC+i,alarm_push_lastAdc[i]);
  }
  *p=0;
  MYDEBUGF("parse_cmd_alarm_push_status ret:%d\n",p-output);
  return ECMD_FINAL(p-output);
}


int16_t parse_cmd_alarm_push_adcs(char *cmd, char *output, uint16_t len)
{

    debug_printf("parse_cmd_alarm_push_adcs() called with string %s\n", cmd);

    while (*cmd == ' ')
	cmd++;

    uint8_t alarm_push_adcs=0;
    if (*cmd != '\0') {
	alarm_push_adcs=atoi(cmd);
	eeprom_save(alarm_push_adcs, &alarm_push_adcs, sizeof(alarm_push_adcs) );
	eeprom_update_chksum();
	return ECMD_FINAL_OK;
    }
    else
    {
	eeprom_restore(alarm_push_adcs, &alarm_push_adcs, sizeof(alarm_push_adcs));

	return ECMD_FINAL(snprintf_P(output, len, PSTR("ADCs:%d"),alarm_push_adcs));
    }
}

/*
-- Ethersex META --
block([[Application_Sample]])
ecmd_feature(alarm_push_init, "alarm-push_init",, Manually call application sample init method)
ecmd_feature(alarm_push_periodic, "alarm-push_periodic",, Manually call application sample periodic method)
ecmd_feature(alarm_push_status, "alarm-push-status",, get status)
ecmd_feature(alarm_push_adcs, "alarm-push-adcs",, get/set number of monitored adcs)
*/
