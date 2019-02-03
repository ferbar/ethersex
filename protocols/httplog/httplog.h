/*
 * Copyright (c) 2009 by Christian Dietrich <stettberger@dokucode.de>
 * Copyright (c) 2010 by Justin Otherguy <justin@justinotherguy.org>
 * Copyright (c) 2012 by Erik Kunze <ethersex@erik-kunze.de>
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

#ifndef HAVE_HTTPLOG_H
#define HAVE_HTTPLOG_H

#include "protocols/uip/uip.h"

uint8_t httplog(const char*, ...);
uint8_t httplog_P(const char*, ...);

extern uip_conn_t *httpConn;
extern char *httplog_tmp_buf;
extern char httplog_state;
extern char httplog_state2;
#define HTTPLOG_STATE_RESOLVE 1
#define HTTPLOG_STATE_DNS_QUERY 2
#define HTTPLOG_STATE_DNS_QUERY_DONE 3
#define HTTPLOG_STATE_CONNECT_ERROR 4
#define HTTPLOG_STATE_CONNECTED 5
#define HTTPLOG_STATE_ACK 6
#define HTTPLOG_STATE_DISCONNECTED 7

#endif  /* HAVE_HTTPLOG_H */
