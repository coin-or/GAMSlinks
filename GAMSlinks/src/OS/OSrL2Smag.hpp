// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: OSrL2Smag.hpp 510 2008-08-16 19:31:27Z stefan $
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

struct smagRec;
class OSResult;

/** Reads an optimization result and writes it into a gams solution file using the SMAG I/O library.
 */
class OSrL2Smag {
public:
	/** Constructor.
	 * @param smag_ SMAG handler.
	 */
	OSrL2Smag(struct smagRec* smag_);

	/** Destructor.
	 */
	~OSrL2Smag() {}
	
	/** Writes a GAMS solution file with the result given as OSResult object.
	 * @param osresult Optimization result as object.
	 */ 
	void writeSolution(OSResult& osresult);

	/** Writes a GAMS solution file with the result given as osrl string.
	 * @param osrl Optimization result as string.
	 */ 
	void writeSolution(std::string& osrl);

private:
	struct smagRec* smag;
};


#endif /*OSRL2SMAG_HPP_*/
