// Copyright (C) GAMS Development and others 2008-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMS2OSIL_HPP_
#define GAMS2OSIL_HPP_

#include "GAMSlinksConfig.h"

class OSInstance;
class OSnLNode;

struct gmoRec;
struct gevRec;

/** Creating a OSInstance from a GAMS model given as GAMS Modeling Object (GMO).
 */
class Gams2OSiL {
private:
	struct gmoRec* gmo;
	struct gevRec* gev;

	OSnLNode* parseGamsInstructions(int codelen, int* opcodes, int* fields, int constantlen, double* constants);

	OSInstance *osinstance;
public:

	Gams2OSiL(struct gmoRec* gmo_);

	~Gams2OSiL();

 	/** Creates an OSInstance from the GAMS smag instance representation
 	 * @return whether the instance is created successfully.
 	 */
	bool createOSInstance();

	/** Gives OSInstance and ownership to calling function.
	 * This object forgets about the created instance.
	 */
	OSInstance* takeOverOSInstance();

	/** Gives OSInstances but keeps ownership.
	 * Destruction will destruct OSInstance.
	 */
	OSInstance* getOSInstance() { return osinstance; }
};


#endif /*GAMS2OSIL_HPP_*/
