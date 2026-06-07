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
 */
#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
/* Trick md5.h to use prototypes in declarations */
#if defined(__STDC__) && (__STDC__ == 1) && !defined(PROTOTYPES)
#define PROTOTYPES 1
#endif
#include "md5.h"
#include "uammsg.h"
#include "log.h"
#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int encrypt_message(char *plaintext, int plain_len,
                           char *cyphertext, int cypher_len) {
  if (plain_len >= cypher_len) {
    sw_log("%s:%d plaintext overflow", __FILE__, __LINE__);
    return -1;
  }
  memset(cyphertext, 0, sizeof(cyphertext));
  memcpy(cyphertext, plaintext, plain_len);
  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d encrypted UAM msg: %d octets", __FILE__, __LINE__, 
           plain_len);
  return plain_len;
}

static int decrypt_message(char *cyphertext, int cypher_len,
                           char *plaintext, int plain_len) {
  if (cypher_len >= plain_len) {
    sw_log("%s:%d cyphertext overflow", __FILE__, __LINE__);
    return -1;
  }
  memset(plaintext, 0, sizeof(plaintext));
  memcpy(plaintext, cyphertext, cypher_len);
  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d plain UAM msg: %d octets", __FILE__, __LINE__, cypher_len);
  return cypher_len;
}

/*
** UAM protocol grammar:
**
** Version 1 (legacy)
**
** Query:
**
** msg ::= event sep ip sep arg1 sep arg2 sep ... argN
** event ::= "UP" | "DN" | "ST" | "LI" | "CN"
** sep ::= '\0'
** ip ::= quad-dot-string
** arg1 .. argN ::= <string>
**
** Response:
** msg ::= status sep ip sep arg1 sep arg2 sep ... argN
** status ::= "OK" | "ER"
** sep ::= '\0'
** ip ::= quad-dot-string
** arg1 .. argN ::= <string>
**
** Version 2
**
** Query:
**
** msg ::= serial stateid event sep ip sep arg1 sep arg2 sep ... argN
** serial ::= <int>
** stateid ::= <int>
** event ::= "UP" | "DN" | "ST" | "LI" | "CN"
** sep ::= '\0'
** ip ::= quad-dot-string
** arg1 .. argN ::= <string>
**
** Response:
** msg ::= serial stateid status sep ip sep arg1 sep arg2 sep ... argN
** status ::= "OK" | "ER"
** sep ::= '\0'
** ip ::= quad-dot-string
** arg1 .. argN ::= <string>
**
*/

int sw_uam_build_msg(char **argv, char **cyphertext) {
  static __thread char __cyphertext[SW_UAM_MSG_SIZE];
  char plaintext[SW_UAM_MSG_SIZE], *p;
  int l, rc, idx;
  
  memset(plaintext, 0, sizeof(plaintext));

  l = 0;
  p = plaintext;

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d encoding UAM message", __FILE__, __LINE__);

  for(idx=0; argv[idx]; idx++) {
    if (idx == SW_UAM_ARG_MAX-1) {
      sw_log("%s:%d UAM argv overflown", __FILE__, __LINE__);
      return -1;
    }
    l += strlen(argv[idx])+1;
    if (l > sizeof(plaintext)-1) {
      sw_log("%s:%d item \"%s\" at #%d does not fit %s proto message", 
             __FILE__, __LINE__, argv[idx], idx, argv[0]);
      return -1;
    }
    strcpy(p, argv[idx]);
    if (sw_debug_flags & SW_DEBUG_UAM)
      sw_log("%s:%d UAM arg #%d: %s", __FILE__, __LINE__, idx, p);
    p += strlen(p)+1;
  }

  rc = encrypt_message(plaintext, p-plaintext,
                       __cyphertext, sizeof(__cyphertext));
  if (rc == -1)
    return -1;

  *cyphertext = __cyphertext;
  
  return rc;
}

int sw_uam_parse_msg(char **argv, char *cyphertext, int cyberlen) {
  static __thread char plaintext[SW_UAM_MSG_SIZE];
  char *p;
  int rc, idx;
  
  rc = decrypt_message(cyphertext, cyberlen, plaintext, sizeof(plaintext)-1);
  if (rc == -1)
    return -1;

  if (sw_debug_flags & SW_DEBUG_UAM)
    sw_log("%s:%d decoding UAM message", __FILE__, __LINE__);

  p = plaintext;

  for(idx=0; p-plaintext < rc; idx++) {
    if (idx == SW_UAM_ARG_MAX-1) {
      sw_log("%s:%d UAM argv overflown", __FILE__, __LINE__);
      return -1;
    }
    argv[idx] = p;
    if (sw_debug_flags & SW_DEBUG_UAM)
      sw_log("%s:%d UAM arg #%d: %s", __FILE__, __LINE__, idx, p);
    p = p + strlen(p)+1;
  }

  for(;idx<SW_UAM_ARG_MAX; idx++)
    argv[idx] = NULL; /* end-of-vector */

  return 0;
}
