// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#ifndef OSRL2SMAG_HPP_
#define OSRL2SMAG_HPP_

#include "GAMSlinksConfig.h"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

//#include "OSInstance.h"
//#include "OSnLNode.h"

#include "OSResult.h"

#include "smag.h"

/*! \class OSrL2Smag
 *  \brief The OSrL2Smag  Class.
 * 
 * @author Stefan Vigerske
 * 
 * \remarks
 * the OSrL2Smag class is used for reading an optimization result
 * and writing it into a gams solution file using the smag i/o.
 * 
 */
class OSrL2Smag {
public:
	/** Constructor */
	OSrL2Smag(smagHandle_t smag_);

	/** Destructor */
	~OSrL2Smag();
	
	void writeSolution(OSResult& osresult);

	void writeSolution(std::string& osrl);

private:
	smagHandle_t smag;
};


#endif /*OSRL2SMAG_HPP_*/
