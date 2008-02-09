/* Copyright (C) 2008
 All Rights Reserved.
 This code is published under the Common Public License.

 $Id$

 Authors:  Stefan Vigerske
*/

#ifndef PARDISOLOADER_H_
#define PARDISOLOADER_H_

/**
 * @return Zero on success, nonzero on failure.
 */
int LSL_loadPardisoLib(const char* libname, char* msgbuf, int msglen);

/**
 * @return Zero on success, nonzero on failure.
 */
int LSL_unloadPardisoLib();

/**
 * @return Zero if not loaded, nonzero if handle is loaded
 */
int LSL_isPardisoLoaded();

#endif /*PARADISOLOADER_H_*/
