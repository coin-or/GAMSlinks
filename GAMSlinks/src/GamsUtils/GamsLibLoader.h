/* Copyright (C) 2008-2009 GAMS Development and others
 All Rights Reserved.
 This code is published under the Common Public License.

 $Id$

 Author: Stefan Vigerske
*/

#ifndef GAMSLIBLOADER_H_
#define GAMSLIBLOADER_H_

#include "GAMSlinksConfig.h"

#ifdef HAVE_WINDOWS_H
# include <windows.h>
  typedef HINSTANCE soHandle_t;
#else
# ifdef HAVE_DLFCN_H
#  include <unistd.h>
#  include <dlfcn.h>
  typedef void *soHandle_t;
# else
#  define ERROR_LOADLIB
  typedef void *soHandle_t;
# endif
#endif

/** Loads a dynamically linked library.
 * @param libname The name of the library to load.
 * @param msgbuf A buffer to store an error message.
 * @param msglen Length of the message buffer.
 * @return Shared library handle, or NULL if failure.
 */
soHandle_t GamsLL_loadLib(const char* libname, char* msgbuf, int msglen);

/** Unloads a shared library.
 * @param libhandle Handle of shared library to unload.
 * @return Zero on success, nonzero on failure.
 */
int GamsLL_unloadLib(soHandle_t libhandle);

#ifndef GAMSLL_SKIPLOADSYM
typedef void (*voidfun)(void);
voidfun GamsLL_loadSym(soHandle_t h, const char *symName, char *msgBuf, int msgLen);
#endif

#endif /*LIBRARYHANDLER_H_*/
