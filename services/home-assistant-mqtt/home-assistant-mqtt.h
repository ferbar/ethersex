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

#ifndef HAVE_HOME_ASSISTANT_MQTT_H
#define HAVE_HOME_ASSISTANT_MQTT_H

#include "protocols/mqtt/mqtt.h"

extern int16_t *home_assistant_mqtt_lastAdc;

// int16_t
// home_assistant_mqtt_onrequest(char *cmd, char *output, uint16_t len);

void
home_assistant_mqtt_init(void);

// int16_t home_assistant_mqtt_periodic(void);

// char *home_assistant_mqtt_querystring(uint8_t home_assistant_mqtt_adcs);

#include "config.h"
#ifdef DEBUG_HOME_ASSISTANT_MQTT
# include "core/debug.h"
# define MYDEBUGF(a, ...)  debug_printf("HA mqtt: " a "\n", ## __VA_ARGS__ )
#else
# define MYDEBUGF(...) do { } while(0)
#endif

extern mqtt_connection_config_t mqtt_static_conf;

#endif  /* HAVE_APPSAMPLE_H */
