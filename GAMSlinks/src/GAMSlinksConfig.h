/* Copyright (C) GAMS Development and others 2011
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * Author: Stefan Vigerske
 */

#ifndef __GAMSLINKSCONFIG_H__
#define __GAMSLINKSCONFIG_H__

#ifndef HAVE_CONFIG_H
#error "HAVE_CONFIG_H not defined. Need configuration header files."
#endif

#ifdef GAMSLINKS_BUILD
#include "config.h"
#else
#include "config_gamslinks.h"
#endif

#endif /*__GAMSLINKSCONFIG_H__*/
