/**
 * \addtogroup uipopt
 * @{
 */

/**
 * \name Project-specific configuration options
 * @{
 *
 * uIP has a number of configuration options that can be overridden
 * for each project. These are kept in a project-specific uip-conf.h
 * file and all configuration names have the prefix UIP_CONF.
 *
 * Configuration for the Dragon...
 */

/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack
 *
 * $Id: uip-conf.h,v 1.6 2006/06/12 08:00:31 adam Exp $
 */

/**
 * \file
 *         An example uIP configuration file
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef __UIP_CONF_H__
#define __UIP_CONF_H__

/**
 * 8 bit datatype
 *
 * This typedef defines the 8-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef unsigned char u8_t;

/**
 * 16 bit datatype
 *
 * This typedef defines the 16-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef unsigned short u16_t;

/**
 * Statistics datatype
 *
 * This typedef defines the dataype used for keeping statistics in
 * uIP.
 *
 * \hideinitializer
 */
typedef unsigned short uip_stats_t;

/**
 * Maximum number of TCP connections.
 *
 * \hideinitializer
 */
#define UIP_CONF_MAX_CONNECTIONS 4

/**
 * Maximum number of listening TCP ports.
 *
 * \hideinitializer
 */
#define UIP_CONF_MAX_LISTENPORTS 4

/**
 * uIP buffer size.
 *
 * \hideinitializer
 */
#define UIP_CONF_BUFFER_SIZE     1500

/**
 * CPU byte order.
 *
 * \hideinitializer
 */
#define UIP_CONF_BYTE_ORDER      UIP_BIG_ENDIAN

/**
 * Logging on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_LOGGING         1

/**
 * UDP support on or off
 *
 * Only enable UDP support if an app is included that requires it.
 *
 * \hideinitializer
 */
#if defined(APP_SOLARUDP) || defined(APP_WEATHERUDP) || defined(APP_RESOLV) || \
defined(APP_DHCPC) || defined(APP_NTP)

#define UIP_CONF_UDP             1
/**
 * UDP checksums on or off
 *
 * \hideinitializer
 */
#define UIP_CONF_UDP_CHECKSUMS   1

/**
 * Enable UDP broadcast support
 */
#define UIP_CONF_BROADCAST 1

#else
#define UIP_CONF_UDP             0
#endif

/**
 * uIP statistics on or off
 *
 * \hideinitializer
 */
// stats only enabled if the webserver also is
// since in the (current) demo configurations, that's 
// the only thing that makes use of them so is a waste
// of memory otherwise
#ifndef APP_HTTPD
#define UIP_CONF_STATISTICS      0
#else
#define UIP_CONF_STATISTICS      1
#endif

/**
 * Architecture specific overrides
 *
 */
 
// we provide a custom 32 bit addition
#define UIP_ARCH_ADD32 1
// we provide all checksum routines
#define UIP_ARCH_CHKSUM 1

// These two are probably redundant now because UIP_ARCH_CHKSUM
// takes precendence but whatever...
// we provide a custom IP checksum
#define UIP_ARCH_IPCHKSUM 1
// and a custom TCP checksum
#define UIP_ARCH_TCPCHKSUM 1

// This guard stops the following headers being pulled when the
// individual apps themselves are built because they need to correctly define
// the appcall structures in order to operate properly - if not (and the 
// multi-app callchain is in play), they can end up just being the dummy structure
// created by that and then things either don't build or work properly.
// Hence any new apps need to add to this list.. Additionally it also needs
// to be grouped by protocol. 
//
// This is messy and I don't like it but within the confines of how this all
// hangs together, the best I could come up with.

#if !(defined(__HELLO_WORLD_H__) || defined(__SMTP_H__) \
 || defined(__TELNETD_H__) || defined(__WEBCLIENT_H__) \
 || defined(__WEBSERVER_H__))

/* Here we include the header file for the application(s) we use in
   our project. Note that if multiple applications are defined, unless
   the callchain app is used, only (the first listed here per protocol)
   will be serviced. These should also be included with the application
   that has the largest appstate data FIRST (per protocol) to ensure 
   enough space is reserved. */

// TCP based apps
#ifdef APP_WEBCLIENT
#include "webclient.h"
#endif
#ifdef APP_HTTPD
#include "webserver.h"
#endif
#ifdef APP_TELNETD
#include "telnetd.h"
#endif
#ifdef APP_SMTP
#include "smtp.h"
#endif
#ifdef APP_HELLOWORLD
#include "hello-world.h"
#endif

#endif // included from a TCP app

#if !(defined(__DHCPC_H__) || defined(__RESOLV_H__) \
 || defined(SOLAR_UDP_H) || defined(WEATHER_UDP_H) \
 || defined(NTPCLIENT_H))

// UDP based apps
#ifdef APP_DHCPC
#include "dhcpc.h"
#endif
#ifdef APP_RESOLV
#include "resolv.h"
#endif
#ifdef APP_SOLARUDP
#include "solar-udp.h"
#endif
#ifdef APP_WEATHERUDP
#include "weather-udp.h"
#endif
#ifdef APP_NTP
#include "ntpclient.h"
#endif
// this must be the last thing in the list
#ifdef APP_CALLCHAIN
#include "app-callchain.h"
#endif

#else

// The UIP stack requires that at least one TCP app
// be enabled, in a configuration that doesn't want
// one, the easiest way to achieve this is to enable
// the callchain app. This condition ensures that the
// required appstate is then defined when the UDP app(s)
// is being built
#ifdef APP_CALLCHAIN
#include "app-callchain.h"
#undef UIP_UDP_APPCALL
#endif

#endif // included from a UDP app

#endif /* __UIP_CONF_H__ */

/** @} */
/** @} */
