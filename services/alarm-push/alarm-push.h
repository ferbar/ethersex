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

#ifndef HAVE_ALARM_PUSH_H
#define HAVE_ALARM_PUSH_H

extern uint16_t *alarm_push_lastAdc;

int16_t
alarm_push_onrequest(char *cmd, char *output, uint16_t len);

int16_t
alarm_push_init(void);

int16_t
alarm_push_periodic(void);

char *alarm_push_querystring(uint8_t alarm_push_adcs);

#include "config.h"
#ifdef DEBUG_ALARM_PUSH
# include "core/debug.h"
# define MYDEBUGF(a...)  debug_printf("alarm-push: " a)
#else
# define MYDEBUGF(a...)
#endif

#endif  /* HAVE_APPSAMPLE_H */
