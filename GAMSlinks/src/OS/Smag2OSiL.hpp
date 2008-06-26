// Copyright (C) 2007-2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef SMAG2OSIL_HPP_
#define SMAG2OSIL_HPP_

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

class OSInstance;
class OSnLNode;

struct smagRec;

/*! \class Smag2OSiL
 *  \brief The Smag2OSiL  Class.
 * 
 * @author Stefan Vigerske
 * 
 * \remarks
 * the Smag2OSiL class is used for reading an instance
 * in GAMS Smag format and creating an OSInstance object in OSiL format
 * 
 */
class Smag2OSiL
{
public:
	/** Constructor */
	Smag2OSiL(struct smagRec* smag_);

	/** Destructor */
	~Smag2OSiL();
	
 	/**
 	 * create an OSInstance from the GAMS smag instance representation
 	 * 
 	 * @return whether the instance is created successfully. 
 	 */
	bool createOSInstance();

	/** osinstance is a pointer to the OSInstance object that gets
	 * created from the instance represented in SMAG format
	 */
	OSInstance *osinstance;
	
private:
	struct smagRec* smag;

	OSnLNode* parseGamsInstructions(unsigned int* instr, int num_instr, double* constants);
	
	bool setupQuadraticTerms();
	
	bool setupTimeDomain();
};

#endif /*SMAG2OSIL_HPP_*/
