/* Copyright (C) 2008 GAMS Development and others
 All Rights Reserved.
 This code is published under the Common Public License.

 $Id$

 Author: Stefan Vigerske
*/

/* bch.o uses a reference to stricmp which is defined in iolib.
 * If we use smag and do not link to iolib, we define our own stricmp function.
 * Either we use strcasecmp, if available, or we use gcdstricmp from GAMS dictread.o 
 */
#ifndef HAVE_STRICMP
#ifdef HAVE_STRCASECMP
#include <strings.h>
int stricmp(const char* s1, const char* s2) {
	return strcasecmp(s1, s2);
}
#else
int gcdstricmp(const char* s1, const char* s2);
int stricmp(const char* s1, const char* s2) {
	return gcdstricmp(s1, s2);
}
#endif
#endif
