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
#ifndef __SW_PORTAB_H__
#define __SW_PORTAB_H__

#if defined(__STDC__) && (__STDC__ == 1)
#define CONST           const
#define PROTO(x)        x
#else   /* __STDC__ */
#define CONST           /* const */
#define PROTO(x)        (/* x */)
#endif

#endif
