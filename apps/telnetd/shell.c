 /*
 * Copyright (c) 2003, Adam Dunkels.
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: shell.c,v 1.1 2006/06/07 09:43:54 adam Exp $
 *
 */

#include "shell.h"
#ifndef _CMOC_VERSION_
#include <string.h>
#else
#include <cmoc.h>
#endif
#ifdef APP_SOLARUDP
#include "solar-udp.h"
#endif
#ifdef APP_WEATHERUDP
#include "weather-udp.h"
#endif

struct ptentry {
  const char *commandstr;
  void (* pfunc)(char *str);
};

#define SHELL_PROMPT "uIP 1.0> "

/*---------------------------------------------------------------------------*/
static void
parse(register char *str, struct ptentry *t)
{
  struct ptentry *p;
  for(p = t; p->commandstr != NULL; ++p) {
    if(strncmp(p->commandstr, str, strlen(p->commandstr)) == 0) {
      break;
    }
  }

  p->pfunc(str);
}

/*---------------------------------------------------------------------------*/
static void
help(char *str)
{
  shell_output("Available commands:", "");
#if UIP_STATISTICS  
  shell_output("stats   - show network statistics", "");
  shell_output("conn    - show TCP connections", "");
#endif
#ifdef APP_SOLARUDP
  shell_output("solar   - show solar metrics", "");
#endif
#ifdef APP_WEATHERUDP
  shell_output("weather - show weather data", "");
#endif
  shell_output("help, ? - show help", "");
  shell_output("exit    - exit shell", "");
}
/*---------------------------------------------------------------------------*/
static void
unknown(char *str)
{
  if(strlen(str) > 0) {
    shell_output("Unknown command: ", str);
  }
}

#if defined(APP_SOLARUDP) || defined(APP_WEATHERUDP)
static void shell_output_string1(const char *str)
{
  shell_output(str,"") ;
}
#endif

#ifdef APP_SOLARUDP
static void solar(char *str)
{
  output_solar_metrics(shell_output_string1);
}
#endif

#ifdef APP_WEATHERUDP
static void weather(char *str)
{
  output_weather_data(shell_output_string1);
}
#endif

/*---------------------------------------------------------------------------*/
static struct ptentry parsetab[] =
  {
#if UIP_STATISTICS  
   {"stats", help},
   {"conn", help},
#endif
#ifdef APP_SOLARUDP
   {"solar", solar},
#endif
#ifdef APP_WEATHERUDP
   {"weather", weather},
#endif
   {"help", help},
   {"exit", shell_quit},
   {"?", help},

   /* Default action */
   {NULL, unknown}};
/*---------------------------------------------------------------------------*/
void
shell_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
shell_start(void)
{
  shell_output("uIP command shell", "");
  shell_output("Type '?' and return for help", "");
  shell_prompt(SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
void
shell_input(char *cmd)
{
  parse(cmd, parsetab);
  shell_prompt(SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
