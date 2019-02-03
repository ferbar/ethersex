/*
 * Copyright (c) 2009 by Christian Dietrich <stettberger@dokucode.de>
 * Copyright (c) 2010 by Justin Otherguy <justin@justinotherguy.org>
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

// httplog_ecmd.c
//
// this is a literal copy of twitter_ecmd.c with "twitter" replaced by "httplog"

#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "httplog.h"

#include "protocols/ecmd/ecmd-base.h"
#include "core/debug.h"
#include "protocols/uip/parse.h"

#ifdef HTTPLOG_UUID_EEPROM
#include "core/eeprom.h"
#endif

int16_t parse_cmd_ht (char *cmd, char *output, uint16_t len) 
{
  if (httplog(cmd)) 
    return ECMD_FINAL_OK;
  return ECMD_FINAL(snprintf_P(output, len, PSTR("sending failed")));
}

int16_t parse_cmd_ht_status (char *cmd, char *output, uint16_t len) 
{
  debug_printf("parse_cmd_ht_status [%s]\n", cmd);
  // http://www.ethersex.de/index.php/Development/ECMD
  if (httpConn && cmd[0] != 23) {
    cmd[0] = 23;
	#define IP_LEN 16
    // char *ip_buffer=(char*) malloc(IP_LEN);
    char ip_buffer[IP_LEN];
	// if(ip_buffer) {
      print_ipaddr(&httpConn->ripaddr, ip_buffer, IP_LEN);
	  debug_printf("parse_cmd_ht_status da\n");
      debug_printf("parse_cmd_ht_status remote ip:%s len=%d\n",ip_buffer,len);
      // default buffer len:40bytes
	  int l=snprintf_P(output, len, PSTR("con s:%d:%d, f:%d t:%d %u -> %s:%u"),
          httplog_state,httplog_state2, httpConn->tcpstateflags, httpConn->timer, ntohs(httpConn->lport), ip_buffer, ntohs(httpConn->rport));
      debug_printf("parse_cmd_ht_status return len:%d [%s]\n", l, output);
      // free(ip_buffer);
      return ECMD_AGAIN(l);
	/*
    } else {
      debug_printf("parse_cmd_ht_status ERROR ALLOC\n");
    }
	*/
//    return ECMD_AGAIN(snprintf_P(output, len, PSTR("con f:%d t:%d %u -> %s:%u"),
//      httpConn->tcpstateflags, httpConn->timer, ntohs(httpConn->lport), ip_buffer, ntohs(httpConn->rport)));
  }
  debug_printf("parse_cmd_ht_status AGAIN\n");
#warning fixme: Das braucht global ram:
  return ECMD_FINAL(snprintf_P(output, len, PSTR("buffer: %p=[%s]"),
      httplog_tmp_buf, httplog_tmp_buf ? httplog_tmp_buf : ""));
  debug_printf("parse_cmd_ht_status buffer full\n");
  return ECMD_FINAL(snprintf_P(output, len, PSTR("no connection")));
}

#ifdef HTTPLOG_UUID_EEPROM
int16_t parse_cmd_ht_uuid(char *cmd, char *output, uint16_t len)
{
    debug_printf("parse_cmd_ht_uuid() called with string %s\n", cmd);

    while (*cmd == ' ')
        cmd++;

    if (*cmd != '\0') {
        if(strlen(cmd) < sizeof(CONF_HTTPLOG_UUID)) {
            eeprom_save(httplog_uuid, cmd, sizeof(CONF_HTTPLOG_UUID));
            eeprom_update_chksum();
            return ECMD_FINAL_OK;
        } else {
            return ECMD_FINAL(snprintf_P(output, len, PSTR("new uuid too long (%s)"), cmd));
        }
    } else {
        char uuid[sizeof(CONF_HTTPLOG_UUID)+1];

        eeprom_restore(httplog_uuid, uuid, sizeof(uuid));

        return ECMD_FINAL(snprintf_P(output, len, PSTR("uuid:%s"), uuid));
    }
}
#endif

/*
  -- Ethersex META --
  block([[Httplog]])
  ecmd_feature(ht, "ht ",MESSAGE,Send MESSAGE to compiled in httplog service)
  ecmd_feature(ht_status, "ht-status",MESSAGE,print status)
  ecmd_feature(ht_uuid, "ht-uuid",UUID,Display/set current uuid)
*/
