/* Copyright (C) GAMS Development and others 2011
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * Author: Stefan Vigerske
 */

#ifndef __GAMSLINKSCONFIG_H__
#define __GAMSLINKSCONFIG_H__

#include "config.h"

#ifndef HAVE_SNPRINTF
#ifdef HAVE__SNPRINTF
#define snprintf _snprintf
#else
/* some snprintf seems to be availabe on Windows, though configure couldn't detect it */
/*#error "Do not have snprintf of _snprintf." */
#endif
#endif

#ifndef HAVE_STRTOK_R
#ifdef HAVE_STRTOK_S
#define strtok_r strtok_s
#else
#ifdef HAVE_STRTOK
#define strtok_r(a,b,c) strtok(a,b)
#else
#error "Do not have strtok_r, strtok_s, or strtok."
#endif
#endif
#endif

#if defined(_WIN32)
# if ! defined(STDCALL)
#  define STDCALL   __stdcall
# endif
# if ! defined(DllExport)
#  define DllExport __declspec( dllexport )
# endif
#elif defined(__GNUC__)
# if ! defined(STDCALL)
#  define STDCALL
# endif
# if ! defined(DllExport)
#  define DllExport __attribute__((__visibility__("default")))
# endif
#else
# if ! defined(STDCALL)
#  define STDCALL
# endif
# if ! defined(DllExport)
#  define DllExport
# endif
#endif

#endif /*__GAMSLINKSCONFIG_H__*/
