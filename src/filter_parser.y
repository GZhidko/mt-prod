/*
 * Sweetspot -- an OSI layer 3 network access controller.
 * Written by Ilya Etingof <ilya@glas.net>, 2006.
 *
 * Partially based on http://chillispot.org.
 * Copyright (C) 2003, 2004, 2005 Mondru AB.
 * The contents of this file may be used under the terms of the GNU
 * General Public License Version 2, provided that the above copyright
 * notice and this permission notice is included in all copies or
 * substantial portions of the software.
 *
 * Packet filter syntax
 * 
 * ruleset = [ rule | [ rule ruleset ]
 * rule = action in-out [ proto ] [ srcdst ] [ flags ] [ redir ] [ rate ]
 * action = "block" | "pass" | "dnat"
 * in-out = "in" | "out"
 * proto = "proto" protocol
 * protocol = string | number
 * srcdst = [ "from" object] [ "to" object ]
 * object = [ network ] [ port-comp | port-range ]
 * flags = "flags" flags-set [ "/" flags-mask ]
 * flags-set = "F" | "S" | "R" | "P" | "A" | "U"
 * flags-mask = "F" | "S" | "R" | "P" | "A" | "U"
 * redir = "redir" "to" host-name "port" number
 * rate = "rate" number
 * network = nummask | host-name [ "mask" ipaddr ]
 * port-comp = "port" compare_eq_ne seq_of_numbers | "port" compare_other number
 * port-range = "port" number range number
 * nummask = host-name [ "/" number ]
 * host-name = ipaddr
 * ipaddr  = number "." number "." number "." number
 */

%{
  #ifdef HAVE_CONFIG_H
  #include "config.h"
  #endif
  #include <stdio.h>
  #include <string.h>
  #include <netdb.h>
  #include <errno.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include "filter.h"
  #include "pretty.h"
  #include "log.h"
  int yylex (void);
  void yyerror (char const *);
  typedef struct yy_buffer_state * YY_BUFFER_STATE;

  YY_BUFFER_STATE yy_scan_string (const char * yystr );
  void yy_delete_buffer (YY_BUFFER_STATE  b );

#define YYERROR_VERBOSE 0  /* Compatibility hack for old Bison */

#define YYDEBUG 1

  static sw_filter_rule_t *ruleset_in, *ruleset_out, *rule;
  static sw_filter_target_t *target = NULL;
  static char tcp_flags;
%}

%start ruleset

%union
{
  char *string;
  int number;
}

%type  <string> STRING
%type  <number> NUMBER


%type  <number> portnum compare_eq_ne compare_other hostname ipv4mask

%token STRING NUMBER

%token SW_YY_BLOCK SW_YY_PASS SW_YY_DNAT SW_YY_SHAPE SW_YY_RATE SW_YY_IN
%token SW_YY_OUT SW_YY_FROM SW_YY_TO SW_YY_PORT SW_YY_EQ SW_YY_NE SW_YY_LT
%token SW_YY_LE SW_YY_GT SW_YY_GE SW_YY_MASK SW_YY_PROTO SW_YY_FLAGS
%token SW_YY_REDIR SW_YY_ERROR

%%

ruleset     :
            | {
  rule = (sw_filter_rule_t *)malloc(sizeof(sw_filter_rule_t));
  if (!rule) {
    sw_log("%s:%d malloc failed: %s", __FILE__, __LINE__, strerror(errno));
    return -1;
  }
  target = NULL;
  memset(rule, 0, sizeof(sw_filter_rule_t));
} rule ruleset
;

rule        :  inrule {
  sw_filter_rule_t *prev = ruleset_in;
  if (prev) {
    while(prev->next) prev = prev->next;
    rule->num = prev->num+1;
    prev->next = rule;
  } else {
    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d inbound ruleset created", __FILE__, __LINE__);
    ruleset_in = rule;
  }
  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d inbound rule #%d parsed", __FILE__, __LINE__, rule->num);
}
            |  outrule {
  sw_filter_rule_t *prev = ruleset_out;
  if (prev) {
    while(prev->next) prev = prev->next;
    rule->num = prev->num+1;
    prev->next = rule;
  } else {
    if (sw_debug_flags & SW_DEBUG_FILTER)
      sw_log("%s:%d outbound ruleset created", __FILE__, __LINE__);
    ruleset_out = rule;
  }
  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d outbound rule #%d parsed", __FILE__, __LINE__, rule->num);
}
;

inrule      :  action markin rulemain redir rate
            |  action markin rulemain redir
            |  action markin rulemain rate
            |  action markin rulemain
            |  action markin redir rate
            |  action markin redir
            |  action markin rate
            |  action markin
;

outrule     :  action markout rulemain rate
            |  action markout rulemain
            |  action markout rate
            |  action markout
;

markin      :  SW_YY_IN { rule->dir = SW_FILTER_DIR_IN; }
;

markout     :  SW_YY_OUT { rule->dir = SW_FILTER_DIR_OUT; }
;

rulemain    :  proto srcdst flags
            |  proto srcdst
            |  proto flags
            |  proto
            |  srcdst
;

action      :  SW_YY_BLOCK { rule->action = SW_FILTER_ACTION_BLOCK; }
            |  SW_YY_PASS { rule->action = SW_FILTER_ACTION_PASS; }
            |  SW_YY_DNAT { rule->action = SW_FILTER_ACTION_DNAT; }
            |  SW_YY_SHAPE SW_YY_RATE NUMBER { 
                 rule->action = SW_FILTER_ACTION_SHAPE;
                 rule->shape.rate = $3;
                }
;

proto       :  SW_YY_PROTO NUMBER { 
               rule->proto = $2;
            }
            |  SW_YY_PROTO STRING {
               struct protoent *pe;
               pe = getprotobyname($2);
               if (!pe) {
                 sw_log("%s:%d unknown protocol %s", __FILE__, __LINE__, $2);
               }
               rule->proto = pe->p_proto;
               free($2);
            }
;

srcdst      :  from to
            |  from
            |  to
;

from        :  SW_YY_FROM { target = &rule->src; } object
;

to          :  SW_YY_TO { target = &rule->dst; } object
;

flags       :  SW_YY_FLAGS flagset
            |  SW_YY_FLAGS flagset '/' flagmask
;

flagset     :  { tcp_flags = 0; } flag {
  if (!rule->proto) {
    rule->proto = IPPROTO_TCP;
    sw_log("%s:%d assuming protocol %s at rule %d",
           __FILE__, __LINE__, sw_pretty_proto(rule->proto), rule->num);
  }
  rule->flags.set = tcp_flags;
}
;

flagmask    :  { tcp_flags = 0; } flag {
  rule->flags.mask = tcp_flags;
}
;

flag        :  STRING {
  char *p = $1;
  while(*p) {
    switch(*p++) {
    case 'F':
      tcp_flags |= SW_FILTER_FLAGS_FIN;
      break;
    case 'S':
      tcp_flags |= SW_FILTER_FLAGS_SYN;
      break;
    case 'R':
      tcp_flags |= SW_FILTER_FLAGS_RST;
      break;
    case 'P':
      tcp_flags |= SW_FILTER_FLAGS_PUSH;
      break;
    case 'A':
      tcp_flags |= SW_FILTER_FLAGS_ACK;
      break;
    case 'U':
      tcp_flags |= SW_FILTER_FLAGS_URG;
      break;
    default:
      sw_log("%s:%d invalid tcp flag %c at rule %d",
             __FILE__, __LINE__, *p, rule->num);
      free($1);
      return -1;
    }
  }
  free($1);
}
;

redir       :  SW_YY_REDIR SW_YY_TO hostname SW_YY_PORT portnum {
  rule->dnat.ip = $3;
  rule->dnat.port = $5;
}
;

rate       :  SW_YY_RATE NUMBER {
  rule->action |= SW_FILTER_ACTION_SHAPE;
  rule->shape.rate = $2;
}
;

object      :  network port
            |  network
            |  port
;

port        :  portcomp
            |  portrange
;


portcomp    :  SW_YY_PORT compare_eq_ne portnums {
  if (!rule->proto) {
    sw_log("%s:%d ambiguous protocol at rule %d", 
           __FILE__, __LINE__, rule->num);
    // Assuming $3 is the semantic value of portnums

  }
  target->op = $2;
}
            | SW_YY_PORT compare_other singleport {
  if (!rule->proto) {
    sw_log("%s:%d ambiguous protocol at rule %d",
           __FILE__, __LINE__, rule->num);
    return -1;
  }
  target->op = $2;
}
;

portrange   :  SW_YY_PORT portnum ':' portnum {
  if (!rule->proto) {
    sw_log("%s:%d ambiguous protocol at rule %d",
           __FILE__, __LINE__, rule->num);
    return -1;
  }
  target->op = SW_FILTER_TARGET_RG;
  target->port_lo = $2;
  target->port_hi = $4;
}
;

portnums    :  singleport
            |  portnum ',' singleport {
  target->ports[target->port_cnt++] = (uint16_t) $1;
  }
            |  portnums ',' portnum {
  if (target->port_cnt < SW_FILTER_PORTS_MAX) {
    target->ports[target->port_cnt++] = (uint16_t) $3; 
  } else {
    sw_log("%s:%d rule %d port set too large (max %d)",
            __FILE__, __LINE__, rule->num, SW_FILTER_PORTS_MAX);
  }
}
;

singleport     :  portnum {
  target->port_cnt = 1;
  target->ports[0] = (uint16_t) $1;
} 
;

portnum     :  NUMBER {
  if ($1 > 65535) {       /* Unsigned */
    sw_log("%s:%d invalid port number %d at rule %d", 
           __FILE__, __LINE__, $1, rule->num);
    return -1;
  }
  $$ = $1;
}
;
compare_eq_ne :  SW_YY_EQ { $$ = SW_FILTER_TARGET_EQ; }
              |  SW_YY_NE { $$ = SW_FILTER_TARGET_NE; }
;

compare_other :  SW_YY_LT { $$ = SW_FILTER_TARGET_LT; }
              |  SW_YY_LE { $$ = SW_FILTER_TARGET_LE; }
              |  SW_YY_GT { $$ = SW_FILTER_TARGET_GT; }
              |  SW_YY_GE { $$ = SW_FILTER_TARGET_GE; }
;

network     :  hostname {
              target->ip = $1;
              target->mask = 0xffffffffl; 
}
            |  hostname ipv4mask {
  target->ip = $1;
  target->mask = $2;
}
;

hostname    :  STRING { 
              $$ = ntohl(inet_addr($1));
              free($1);
}
;

ipv4mask    :  SW_YY_MASK STRING { 
              $$ = ntohl(inet_addr($2));
              free($2);
}
            |  '/' NUMBER {
              if ($2 < 0 || $2 > 32) {
                sw_log("%s:%d invalid CIDR %d at rule %d",
                       __FILE__, __LINE__, $2, rule->num);
                return -1;
              }
              $$ = 0xffffffff << (32-$2);
}
;


%%

void yyerror(char const *msg) {
  sw_log("%s:%d %s", __FILE__, __LINE__, msg);
}

int sw_filter_parse_ruleset(sw_filter_rule_t **rs_in, 
                            sw_filter_rule_t **rs_out,
                            char *text) {
  YY_BUFFER_STATE b = 0;

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d scanning ruleset", __FILE__, __LINE__);

  b = yy_scan_string(text);
  if (!b) {
    sw_log("%s:%d yy_scan_buffer() failed", __FILE__, __LINE__);
    return -1;
  }

  yydebug = 0; /* raise this to see AST build */

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d parsing ruleset", __FILE__, __LINE__);

  ruleset_in = ruleset_out = NULL;

  if (yyparse()) {
    yy_delete_buffer(b);
    return -1;
  }

  yy_delete_buffer(b);

  if (sw_debug_flags & SW_DEBUG_FILTER)
    sw_log("%s:%d ruleset finished", __FILE__, __LINE__);

  *rs_in = ruleset_in;
  *rs_out = ruleset_out;

  return 0;
}
