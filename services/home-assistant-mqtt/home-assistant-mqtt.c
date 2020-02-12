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
#include <stdbool.h>

#include "config.h"
#include "home-assistant-mqtt.h"

#include "protocols/ecmd/ecmd-base.h"
#include "hardware/adc/adc.h"
#include "protocols/httplog/httplog.h"

#include "core/eeprom.h"

#include "core/util/fixedpoint.h"

#ifndef MQTT_SUPPORT
#error Please define mqtt support
#endif

#include "protocols/mqtt/mqtt.h"

int16_t *home_assistant_mqtt_lastAdc=NULL;
int home_assistant_mqtt_periodicCounter=0;
uint8_t home_assistant_discover_send=0;

// ======================== Homeassistant mqtt discovery ======================
bool home_assistant_mqtt_send_discover() { 
// round 0: announce ADC + read io pins
// round 1: announce switches
// round 2: subscribe

// all pins announced
  if(home_assistant_discover_send == HOME_ASSISTANT_MQTT_MAX_PINS * 2 + 2) {
    if(home_assistant_mqtt_lastAdc) {
      free(home_assistant_mqtt_lastAdc);
    }
    home_assistant_mqtt_lastAdc = (int16_t*) malloc(sizeof(int16_t)*HOME_ASSISTANT_MQTT_MAX_PINS);
    if(home_assistant_mqtt_lastAdc == NULL) {
      MYDEBUGF("malloc failed");
      return false;
    }
    home_assistant_discover_send++;
    return true;
  } else if(home_assistant_discover_send > HOME_ASSISTANT_MQTT_MAX_PINS * 2 + 2) {
    return true;
  }

  uint8_t HAm_adc_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_adc_pin_mask, &HAm_adc_pin_mask, sizeof(HAm_adc_pin_mask));
  uint8_t HAm_read_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_read_pin_mask, &HAm_read_pin_mask, sizeof(HAm_read_pin_mask));
  uint8_t HAm_pullup_changeable_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_pullup_changeable_pin_mask, &HAm_pullup_changeable_pin_mask, sizeof(HAm_pullup_changeable_pin_mask));

  MYDEBUGF("discover name=%s pin=%d adc=%d digital=%d pullup=%d", mqtt_static_conf.client_id, home_assistant_discover_send, HAm_adc_pin_mask,HAm_read_pin_mask,HAm_pullup_changeable_pin_mask);

  uint8_t run=(home_assistant_discover_send / HOME_ASSISTANT_MQTT_MAX_PINS) << 4 | (home_assistant_discover_send % HOME_ASSISTANT_MQTT_MAX_PINS);
#define ROUND0 ((run & 0xf0) == 0x00)
#define ROUND1 ((run & 0xf0) == 0x10)
#define ROUND2 ((run & 0xf0) == 0x20)

#define pinNr (run & 0x0f)

  if((ROUND0 && ((1 << pinNr & HAm_adc_pin_mask) || (1 << pinNr & HAm_read_pin_mask))) // round 0 + pinNr = adc pin
    || (ROUND1 && ((1 << pinNr) & HAm_pullup_changeable_pin_mask))                     // round 1 + pinNr = pullup changeable switch
	|| (ROUND2 && (pinNr == 0))) {                                                     // round 2 + pinNr0 => status discover

    #define is_adc (ROUND0 && (1 << pinNr & HAm_adc_pin_mask) )
    // round1 und da => kann nur switch sein
    #define is_switch (ROUND1)
	// round2 und da => kann nur status discover sein
    #define is_status (ROUND2)

#define HA_MQTT_DISCOVERY_TOPIC_FORMAT       "homeassistant/sensor/%s/pin%d/config"
#define HA_MQTT_DISCOVERY_TOPIC_FORMATSWITCH "homeassistant/switch/%s/pin%d/config"
#define HA_MQTT_DISCOVERY_TOPIC_FORMATSTATUS "homeassistant/sensor/%s/status/config"
// %s => clientid
//               -  -1 %d => 0 -
// +1 => \0
#define HA_TOPIC_LEN (sizeof(HA_MQTT_DISCOVERY_TOPIC_FORMATSTATUS)-1 -2+strlen(mqtt_static_conf.client_id) +1)
  char topic[HA_TOPIC_LEN];

// device_class=none = default
// Abbrevations: https://www.home-assistant.io/docs/mqtt/discovery/
// json_attr_t brauch ma ned
// #define HA_MQTT_DISCOVERY_MSG_FORMAT1       "{\"~\":\"avr/%s\",\"name\":\"%s pin%d\",\"uniq_id\":\"%s_pin%d\",\"stat_t\":\"~\",\"avty_t\":\"~/LWT\",\"json_attr_t\":\"~\","
#define HA_MQTT_DISCOVERY_MSG_FORMAT1       "{\"~\":\"avr/%s\",\"name\":\"%s pin%d\",\"uniq_id\":\"%s_pin%d\",\"stat_t\":\"~\",\"avty_t\":\"~/LWT\",\"val_tpl\":\"{{ value_json.pin%d }}\","
// ADC braucht unit of measurement damit ein graph angezeigt wird
#define HA_MQTT_DISCOVERY_MSG_FORMAT1ADC                                                                                                                                                  "\"unit_of_meas\":\" \","

#define HA_MQTT_DISCOVERY_MSG_FORMAT1SWITCH "{\"~\":\"avr/%s\",\"name\":\"%s pin%d\",\"uniq_id\":\"%s_pin%d\",\"stat_t\":\"~\",\"avty_t\":\"~/LWT\",\"cmd_t\":\"~/set/pin%d\",\"val_tpl\":\"{{ value_json.pin%d }}\","

// qos => dummy key, default 0
#define HA_MQTT_DISCOVERY_MSG_FORMAT1STATUS "{\"~\":\"avr/%s\",\"name\":\"%s status\",\"qos\":%d,\"uniq_id\":\"%s_status\",\"stat_t\":\"~/LWT\","

#define HA_MQTT_DISCOVERY_MSG_FORMAT2 "\"dev\":{\"ids\":[\"avr_%s\"],\"name\":\"%s\"}}"
// #define HA_MQTT_DISCOVERY_MSG_FORMAT2 "\"val_tpl\":\"{{ value_json.pin%d }}\"}"

// ,\"dev\":{\"ids\":[\"avrnetio_%s\"],\"name\":\"%s\",\"sw\":\"" GIT_VERSION "\"}\ d

// +10 *4 mac address
// -2* id
#define HA_MSG_LEN (sizeof(HA_MQTT_DISCOVERY_MSG_FORMAT1SWITCH)-1 + sizeof(HA_MQTT_DISCOVERY_MSG_FORMAT2)-1 +2*5 - 2*1 +1 )
  char *buf = (char *) malloc(HA_MSG_LEN);
  if(!buf) {
    MYDEBUGF("failed to allocate discovery buffer (%d)", HA_MSG_LEN);
  } else {

    //for(int i=0; i < home_assistant_mqtt_adcs; i++) {   <<= MQTT buffer growing to large if all discovery messages sent at once
      snprintf_P(topic, HA_TOPIC_LEN, (is_switch) ? PSTR(HA_MQTT_DISCOVERY_TOPIC_FORMATSWITCH) : 
	      (is_status ? PSTR(HA_MQTT_DISCOVERY_TOPIC_FORMATSTATUS) : PSTR(HA_MQTT_DISCOVERY_TOPIC_FORMAT)) ,
	    mqtt_static_conf.client_id, pinNr);
      topic[HA_TOPIC_LEN - 1] = '\0';

      int len=snprintf_P(buf, HA_MSG_LEN, (is_switch) ? PSTR(HA_MQTT_DISCOVERY_MSG_FORMAT1SWITCH) : 
	      (is_status ? PSTR(HA_MQTT_DISCOVERY_MSG_FORMAT1STATUS) : PSTR(HA_MQTT_DISCOVERY_MSG_FORMAT1) ) ,
	    mqtt_static_conf.client_id
	   ,mqtt_static_conf.client_id, pinNr
	   ,mqtt_static_conf.client_id, pinNr
	   ,pinNr, pinNr    // last only for switch
      );
	  
      if(is_adc) {
        len+=snprintf_P(buf+len, HA_MSG_LEN-len, PSTR(HA_MQTT_DISCOVERY_MSG_FORMAT1ADC) );
      }
      len+=snprintf_P(buf+len, HA_MSG_LEN-len, PSTR(HA_MQTT_DISCOVERY_MSG_FORMAT2),
	    mqtt_static_conf.client_id
	   ,mqtt_static_conf.client_id
	  );
      MYDEBUGF("sending discover %d to %s", pinNr, topic);
      MYDEBUGF("buf len=%d %s",len,buf);
      buf[HA_MSG_LEN - 1] = '\0';
      if(mqtt_construct_publish_packet(topic, buf, len, /*retain*/ true) == false) {
        MYDEBUGF("connack_c config mqtt failed for pin %d", home_assistant_discover_send%HOME_ASSISTANT_MQTT_MAX_PINS);
      } else {
        home_assistant_discover_send++;
      }
    // }
    free(buf);
  }
  return false;

// subscribe to avr/%s/set (/pin*) if we have a switch
  } else if(ROUND2 && (pinNr==1) && HAm_pullup_changeable_pin_mask) {
    MYDEBUGF("subscribe pin (%d)", home_assistant_discover_send);
    #define HA_MQTT_SUBSCRIBE "avr/%s/set/#"
    char topic[sizeof(HA_MQTT_SUBSCRIBE)-1 -2+strlen(mqtt_static_conf.client_id)];
    snprintf_P(topic, sizeof(topic), PSTR(HA_MQTT_SUBSCRIBE), mqtt_static_conf.client_id);
    MYDEBUGF("subscribe %s", topic);
    if (mqtt_construct_subscribe_packet(topic) == false) {
      MYDEBUGF("...failed");
    } else {
      home_assistant_discover_send++;
    }
  } else {
    MYDEBUGF("no discover for pin %d run 0x%02x", home_assistant_discover_send, run);
    home_assistant_discover_send++;
  }
  return false;
}

/*
  If enabled in menuconfig, this function is periodically called
  change "timer(100,app_sample_periodic)" if needed

  wird alle 10*20ms aufgerufen
*/
static void
home_assistant_mqtt_poll_cb(void)
{
  if(! home_assistant_mqtt_send_discover()) {
    MYDEBUGF("poll_cb: discover not done");
    return;
  }
  if(home_assistant_mqtt_lastAdc == NULL) {
    MYDEBUGF("poll_cb: no lastadc");
    return;
  }
  uint8_t HAm_adc_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_adc_pin_mask, &HAm_adc_pin_mask, sizeof(HAm_adc_pin_mask));

  uint8_t HAm_read_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_read_pin_mask, &HAm_read_pin_mask, sizeof(HAm_read_pin_mask));

// "pin0":"OFF", => 13 + { }  + \0
  int DATA_LENGTH=13*HOME_ASSISTANT_MQTT_MAX_PINS+4;
  char buf[DATA_LENGTH];

#define MAX_DELTA 5

/* Prefix */
#ifdef MQTT_STATIC_CONF
#define MQTT_TOPIC_CLIENT              MQTT_STATIC_CONF_CLIENTID
#else
#define MQTT_TOPIC_CLIENT              CONF_HOSTNAME
#endif

// mac address - last 2 bytes
#define MQTT_SENSOR_PUBLISH_FORMAT   "avr/%s"

// %02x => AA
#define TOPIC_LENGTH               (sizeof(MQTT_SENSOR_PUBLISH_FORMAT)-1 -2+strlen(mqtt_static_conf.client_id) + 1)

  char topic[TOPIC_LENGTH];

  bool force_send=false;
  int adcValues[HOME_ASSISTANT_MQTT_MAX_PINS];

  for(int i=0; i < HOME_ASSISTANT_MQTT_MAX_PINS; i++) {
    if(HAm_adc_pin_mask & 1 << i) {
      //MYDEBUGF("poll_cb adc%d", HOME_ASSISTANT_MQTT_PIN_OFFSET+i);
      adcValues[i] = adc_get(HOME_ASSISTANT_MQTT_PIN_OFFSET+i); // ADC4 ist mit adc1 beschriftet, eigentlich uint16_t 0...1023 aber dann geht das < 0-5 ned gescheit
      if(adcValues[i] > (home_assistant_mqtt_lastAdc[i]+MAX_DELTA) || adcValues[i] < (home_assistant_mqtt_lastAdc[i]-MAX_DELTA) ) {
        MYDEBUGF("changed ADC%d: %d, last=%d", HOME_ASSISTANT_MQTT_PIN_OFFSET+i, adcValues[i], home_assistant_mqtt_lastAdc[i]);
        force_send=true;
      }
    } else if(HAm_read_pin_mask & 1 << i) {
      //MYDEBUGF("poll_cb read%d", HOME_ASSISTANT_MQTT_PIN_OFFSET+i);
	  // FIXME: ATMEGA32 specific code !!!
      adcValues[i] = PINA & ( 1 << (HOME_ASSISTANT_MQTT_PIN_OFFSET+i));
      if(adcValues[i] != home_assistant_mqtt_lastAdc[i] ) {
        MYDEBUGF("changed pin%d: %d, last=%d", HOME_ASSISTANT_MQTT_PIN_OFFSET+i, adcValues[i], home_assistant_mqtt_lastAdc[i]);
        force_send=true;
      }
    } else {
      adcValues[i]=0;
    }
  }

  if(force_send || home_assistant_mqtt_periodicCounter > (60*50)) { // 5 ticks / s
      snprintf_P(topic, TOPIC_LENGTH, PSTR(MQTT_SENSOR_PUBLISH_FORMAT), mqtt_static_conf.client_id);
      topic[TOPIC_LENGTH - 1] = '\0';

      int len=snprintf_P(buf,sizeof(buf),PSTR("{"));
      for(int i=0; i < HOME_ASSISTANT_MQTT_MAX_PINS; i++) {
        if(HAm_adc_pin_mask & 1 << i)
          len += snprintf_P(buf+len, sizeof(buf)-len, PSTR("\"pin%d\":%d,"), i, adcValues[i]);
        if(HAm_read_pin_mask & 1 << i)
          len += snprintf_P(buf+len, sizeof(buf)-len, PSTR("\"pin%d\":%S,"), i, adcValues[i] ? PSTR("\"ON\"") : PSTR("\"OFF\"") );
      }
      if(buf[len-1] == ',') {
        len--;
      }
      len += snprintf_P(buf+len,sizeof(buf)-len, PSTR("}"));

      MYDEBUGF("send mqtt message %s (len=%d)", buf, len);
      mqtt_construct_publish_packet(topic, buf, len, false);
      for(int i=0; i < HOME_ASSISTANT_MQTT_MAX_PINS; i++) { 
        home_assistant_mqtt_lastAdc[i]=adcValues[i];
      }
      home_assistant_mqtt_periodicCounter=0;
    }
  home_assistant_mqtt_periodicCounter++;

  // MYDEBUGF("home_assistant_mqtt_poll_cb done");
}

static void
home_assistant_mqtt_publish_cb(char const *topic, uint16_t topic_length,
                   const void *payload, uint16_t payload_length
                   )
// erst in neueren versionen:                   bool retained)
{
  bool on=memcmp_P(payload, PSTR("ON"),2) == 0;
  // topic is not \0 terminated!
  MYDEBUGF("publish_cb topic:%.20s (%S %d)", topic, on ? PSTR("ON") : PSTR("OFF"), payload_length);
  if(memcmp_P(topic + TOPIC_LENGTH, PSTR("set/pin"),7) == 0) {
    MYDEBUGF("invalid topic");
    return;
  }
  uint8_t pin;
  if((topic[TOPIC_LENGTH + 6] >= '0') && (topic[TOPIC_LENGTH + 6] <= '9')) {
    pin = topic[TOPIC_LENGTH + 6] - '0';
  } else {
    MYDEBUGF("invalid pin # %c", topic[TOPIC_LENGTH + 6]);
	return;
  }
  
  // FIXME: ATMEGA32 specific code !!!
  if(on) {
    PORTA |= ( 1 << (HOME_ASSISTANT_MQTT_PIN_OFFSET+pin));
  } else {
    PORTA &= ~( 1 << (HOME_ASSISTANT_MQTT_PIN_OFFSET+pin));
  }
}

static void
home_assistant_mqtt_connack_cb(void)
{
  MYDEBUGF("connack_c");
  char mqtt_static_conf_last_will_topic[4 + strlen(mqtt_static_conf.client_id) + 4 + 1];
  snprintf_P(mqtt_static_conf_last_will_topic, sizeof(mqtt_static_conf_last_will_topic), PSTR("avr/%s/LWT"), mqtt_static_conf.client_id);
  mqtt_static_conf.will_topic=mqtt_static_conf_last_will_topic;
#define STATUS_ONLINE "online"
  char buf[sizeof(STATUS_ONLINE)];
  strcpy_P(buf, PSTR(STATUS_ONLINE));
  if(mqtt_construct_publish_packet(mqtt_static_conf_last_will_topic, buf, strlen(buf), /*retain*/ true) == false) {
    MYDEBUGF("connack_c config mqtt failed to set device to online");
  }
  MYDEBUGF("connack_c done");
}

/*
  This function will be called on request by menuconfig, if wanted...
  You need to enable ECMD_SUPPORT for this.
  Otherwise you can use this function for anything you like 
int16_t
home_assistant_mqtt_onrequest(char *cmd, char *output, uint16_t len){
  MYDEBUGF ("onrequest");
  // enter your code here

  return ECMD_FINAL_OK;
}
*/

// http://www.ethersex.de/index.php/Own_module
// home_assistant_periodic 1. Parameter = 20ms delay schritte

// -> über uip_tcp_timer => uip_poll => poll callback  ifdef(`conf_HOME_ASSISTANT_MQTT_PERIODIC_AUTOSTART',`timer(5,home_assistant_mqtt_periodic())')
// -> ???  ifdef(`conf_HOME_ASSISTANT_MQTT_INIT_AUTOSTART',`init(home_assistant_mqtt_init)')

static
void home_assistant_mqtt_construct_connect_packet_cb(void) {
  MYDEBUGF("construct_connect_packet_cb");
#define WILL_MESSAGE "offline"
  char will_message[sizeof(WILL_MESSAGE)];
  strcpy_P(will_message, PSTR(WILL_MESSAGE));

  mqtt_static_conf.will_message = will_message;

  // scheinbar macht tasmota auch /LWT und schreibt dann 'Offline' rein
  char mqtt_static_conf_last_will_topic[4 + strlen(mqtt_static_conf.client_id) + 4 + 1];
  snprintf_P(mqtt_static_conf_last_will_topic, sizeof(mqtt_static_conf_last_will_topic), PSTR("avr/%s/LWT"), mqtt_static_conf.client_id);
  mqtt_static_conf.will_topic=mqtt_static_conf_last_will_topic;

  mqtt_construct_connect_packet();

  // cleanup
  mqtt_static_conf.will_message = NULL;
  mqtt_static_conf.will_topic=NULL;
  MYDEBUGF("home_assistant_construct_connect_packet_cb done");
}

static const mqtt_callback_config_t mqtt_callback_config PROGMEM = {
  .construct_connect_packet_callback = home_assistant_mqtt_construct_connect_packet_cb,
  .connack_callback = home_assistant_mqtt_connack_cb,
  .poll_callback = home_assistant_mqtt_poll_cb, // called every 20ms * 10 timer ticks
  .close_callback = NULL,
  .publish_callback = home_assistant_mqtt_publish_cb, // publish request received
};

mqtt_connection_config_t mqtt_static_conf =
{
  .user = NULL,
  .pass = NULL,
//  .will_topic = MQTT_STATIC_CONF_WILL_TOPIC,
  .will_qos = 0,
  .will_retain = 1,
  .will_message = NULL, // GRRR !!! muss im ram sein !!!
  .auto_subscribe_topics = NULL,
  //target_hostname not set, see mqtt_set_static_conf
//  .target_hostname = PSTR(HOME_ASSISTATNT_MQTT_SERVER_HOSTNAME),
  .target_hostname_isP = true
};


void
home_assistant_mqtt_init(void)
{
  MYDEBUGF ("init"); 
  mqtt_register_callback(&mqtt_callback_config);
  uint8_t HAm_pullup_pin_mask=0;
  eeprom_restore(home_assistant_mqtt_pullup_pin_mask, &HAm_pullup_pin_mask, sizeof(HAm_pullup_pin_mask));
  for(int i=0; i < HOME_ASSISTANT_MQTT_MAX_PINS; i++) {
    if((1 << (i+HOME_ASSISTANT_MQTT_PIN_OFFSET)) && HAm_pullup_pin_mask) {
      MYDEBUGF ("setting startup pullup for pin %d",i);
      PORTA |= ( 1 << (HOME_ASSISTANT_MQTT_PIN_OFFSET+i));
    } else {
      // do nothing if no pullup
      // PORTA &= ~( 1 << (HOME_ASSISTANT_MQTT_PIN_OFFSET+pin));
    }
  }

// ========== create config ============

  mqtt_static_conf.target_hostname = PSTR(HOME_ASSISTATNT_MQTT_SERVER_HOSTNAME);

  static char mqtt_static_conf_client_id[2*2+1];
  if(uip_ethaddr.addr[4] == 0 && uip_ethaddr.addr[5] == 0 ) {
    MYDEBUGF("error setting clientid - mac empty");
  }
  snprintf_P(mqtt_static_conf_client_id, sizeof(mqtt_static_conf_client_id), PSTR("%02x%02x"), uip_ethaddr.addr[4], uip_ethaddr.addr[5]);
  mqtt_static_conf.client_id=mqtt_static_conf_client_id;


  mqtt_set_connection_config(&mqtt_static_conf);

}

/*
  -- Ethersex META --
  header(services/home-assistant-mqtt/home-assistant-mqtt.h)
  net_init(home_assistant_mqtt_init)
*/
