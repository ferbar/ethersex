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
#include "home-assistant-mqtt.h"
#include "protocols/ecmd/ecmd-base.h"

#include "core/eeprom.h"

/*
int16_t parse_cmd_home_assistant_mqtt_init(char *cmd, char *output, uint16_t len) 
{
  return home_assistant_mqtt_init();
}

int16_t parse_cmd_home_assistant_mqtt_periodic(char *cmd, char *output, uint16_t len) 
{
  return home_assistant_mqtt_periodic();
}
*/

int16_t parse_cmd_home_assistant_mqtt_status(char *cmd, char *output, uint16_t len) 
{
  uint8_t HAm_adc_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_adc_pin_mask, &HAm_adc_pin_mask, sizeof(HAm_adc_pin_mask));
  uint8_t HAm_read_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_read_pin_mask, &HAm_read_pin_mask, sizeof(HAm_read_pin_mask));
  MYDEBUGF("parse_cmd_home_assistant_mqtt_status adc pin mask:%d read digital pin mask:%d",HAm_adc_pin_mask,HAm_read_pin_mask);

  uint8_t HAm_pullup_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_pullup_pin_mask, &HAm_pullup_pin_mask, sizeof(HAm_pullup_pin_mask));
  uint8_t HAm_pullup_changeable_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_pullup_changeable_pin_mask, &HAm_pullup_changeable_pin_mask, sizeof(HAm_pullup_changeable_pin_mask));
  MYDEBUGF("parse_cmd_home_assistant_mqtt_status pullup on reset pin mask:%d changeable pin mask:%d",HAm_pullup_pin_mask,HAm_pullup_changeable_pin_mask);

  char *p=output;
  p+=snprintf_P(p, len, PSTR("adc:%d r:%d p:%d cp:%d "),HAm_adc_pin_mask, HAm_read_pin_mask, HAm_pullup_pin_mask, HAm_pullup_changeable_pin_mask);
  for(int i=0; i < HOME_ASSISTANT_MQTT_MAX_PINS; i++) {
    if((HAm_adc_pin_mask & 1 << i) || (HAm_read_pin_mask & 1 << i)) {
      p+=snprintf_P(p, len - (p-output), PSTR("pin%d=%d "),i,home_assistant_mqtt_lastAdc[i]);
    } if(HAm_pullup_pin_mask & 1 << i) {
      p+=snprintf_P(p, len - (p-output), PSTR("p:%d "),i);
    } if(HAm_pullup_changeable_pin_mask & 1 << i) {
      p+=snprintf_P(p, len - (p-output), PSTR("pc:%d "),i);
    }
  }
  *p=0;
  MYDEBUGF("parse_cmd_home_assistant_mqtt_status ret:%d",p-output);
  return ECMD_FINAL(p-output);
}

int16_t parse_cmd_home_assistant_mqtt_pin_mask(char *cmd, char *output, uint16_t len, int offset)
{

  MYDEBUGF("parse_cmd_home_assistant_mqtt_pin_mask() called with string %s offset=%d", cmd, offset);

  while (*cmd == ' ')
	cmd++;

    uint8_t HAm_pin_mask=0;
    if (*cmd != '\0') {
      HAm_pin_mask=atoi(cmd);
      eeprom_write_block_hack(EEPROM_CONFIG_BASE + offset, &HAm_pin_mask, sizeof(HAm_pin_mask));
      eeprom_update_chksum();
      MYDEBUGF("parse_cmd_home_assistant_mqtt_pin_mask() changed mask %d", HAm_pin_mask);
      return ECMD_FINAL_OK;
    } else {
      eeprom_read_block(&HAm_pin_mask, EEPROM_CONFIG_BASE + offset, sizeof(HAm_pin_mask));

      return ECMD_FINAL(snprintf_P(output, len, PSTR("%d"),HAm_pin_mask));
    }
}


int16_t parse_cmd_home_assistant_mqtt_adc_pin_mask(char *cmd, char *output, uint16_t len)
{

  MYDEBUGF("parse_cmd_home_assistant_mqtt_adc_pin_mask() called with string %s", cmd);
  return parse_cmd_home_assistant_mqtt_pin_mask(cmd, output, len, offsetof(struct eeprom_config_t, home_assistant_mqtt_adc_pin_mask));
}

int16_t parse_cmd_home_assistant_mqtt_read_pin_mask(char *cmd, char *output, uint16_t len)
{

  MYDEBUGF("parse_cmd_home_assistant_mqtt_read_pin_mask() called with string %s", cmd);
  return parse_cmd_home_assistant_mqtt_pin_mask(cmd, output, len, offsetof(struct eeprom_config_t, home_assistant_mqtt_read_pin_mask));
}

int16_t parse_cmd_home_assistant_mqtt_pullup_pin_mask(char *cmd, char *output, uint16_t len)
{

  MYDEBUGF("parse_cmd_home_assistant_mqtt_pullup_pin_mask() called with string %s", cmd);
  return parse_cmd_home_assistant_mqtt_pin_mask(cmd, output, len, offsetof(struct eeprom_config_t, home_assistant_mqtt_pullup_pin_mask));
}

int16_t parse_cmd_home_assistant_mqtt_pullup_changeable_pin_mask(char *cmd, char *output, uint16_t len)
{
  MYDEBUGF("parse_cmd_home_assistant_mqtt_pullup_changeable_pin_mask() called with string %s", cmd);
  return parse_cmd_home_assistant_mqtt_pin_mask(cmd, output, len, offsetof(struct eeprom_config_t, home_assistant_mqtt_pullup_changeable_pin_mask));
}

// ecmd_feature(home_assistant_mqtt_init, "home-assistant-mqtt_init",, Manually call application home-assistant mqtt init method)
// ecmd_feature(home_assistant_mqtt_periodic, "home-assistant-mqtt_periodic",, Manually call application home-assistant mqtt periodic method)

/*
-- Ethersex META --
block([[Application_Sample]])
ecmd_feature(home_assistant_mqtt_status, "HA-mqtt-status",, get status)
ecmd_feature(home_assistant_mqtt_adc_pin_mask, "HA-mqtt-adc-pin-mask",, get/set number of monitored adc mask)
ecmd_feature(home_assistant_mqtt_read_pin_mask, "HA-mqtt-read-pin-mask",, get/set number of monitored digital pin mask)
ecmd_feature(home_assistant_mqtt_pullup_pin_mask, "HA-mqtt-pullup-pin-mask",, get/set number of digital pin pullup mask)
ecmd_feature(home_assistant_mqtt_pullup_changeable_pin_mask, "HA-mqtt-pullup-changeable-pin-mask",, get/set number of monitored adc mask)
*/
