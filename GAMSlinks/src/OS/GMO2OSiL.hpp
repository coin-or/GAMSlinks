// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GMO2OSIL_HPP_
#define GMO2OSIL_HPP_

#include "GAMSlinksConfig.h"


class OSInstance;
class OSnLNode;

struct gmoRec;

/** Creating a OSInstance from a GAMS model given as GAMS Modeling Object (GMO).
 */
class GMO2OSiL {
public:
	/** Constructor.
	 * @param gmo_ A GMO handler.
	 */
	GMO2OSiL(struct gmoRec* gmo_);

	/** Destructor.
	 */
	~GMO2OSiL();
	
 	/** Creates an OSInstance from the GAMS smag instance representation
 	 * @return whether the instance is created successfully. 
 	 */
	bool createOSInstance();

	/** osinstance is a pointer to the OSInstance object that gets created from the instance represented in SMAG format.
	 */
	OSInstance *osinstance;
	
	/** If you set this flag to true, then the instructions in SMAG are not touched by parseGamsInstructions().
	 * Reordering is then done on a copy of the instruction list.
	 * The default is currently true, because the gradient evaluation by G2D seem to be failing on a reordered instructions list.
	 */
//	bool keep_original_instr;
	
private:
	struct gmoRec* gmo;

	OSnLNode* parseGamsInstructions(int codelen, int* opcodes, int* fields, int constantlen, double* constants);
	
//	bool setupQuadraticTerms();
//	
//	bool setupTimeDomain();
};


#endif /*GMO2OSIL_HPP_*/
