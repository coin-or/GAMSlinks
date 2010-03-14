// Copyright (C) GAMS Development and others 2008-2010
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSOSXL_HPP_
#define GAMSOSXL_HPP_

#include "GAMSlinksConfig.h"

#include <string>

class OSInstance;
class OSnLNode;
class OSResult;

struct gmoRec;
struct gevRec;

/** Converting between GAMS Modeling Object (GMO) and OS entities (OSiL, OSrL).
 */
class GamsOSxL {
private:
	struct gmoRec* gmo;
	struct gevRec* gev;

	bool gmo_is_our;

	OSnLNode* parseGamsInstructions(int codelen, int* opcodes, int* fields, int constantlen, double* constants);

	OSInstance *osinstance;
public:

	/** Constructor.
	 * @param gmo_ A Gams Modeling Object, or NULL if set later.
	 */
	GamsOSxL(struct gmoRec* gmo_);

	/** Constructor.
	 * @param datfile Name of a file containing a compiled GAMS model.
	 */
	GamsOSxL(const char* datfile);

	~GamsOSxL();

	/** Sets Gams Modeling Object.
	 * Can only be used if not GMO has been set already.
	 */
	void setGMO(struct gmoRec* gmo_);

	/** Initializes GMO from a file containing a compiled GAMS model
	 */
	bool initGMO(const char* datfile);

 	/** Creates an OSInstance from the GAMS modeling object
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

  /** Writes a solution into a GMO with the result given as OSResult object.
   * @param osresult Optimization result as object.
   */
  void writeSolution(OSResult& osresult);

  /** Writes a solution into a GMO with the result given as osrl string.
   * @param osrl Optimization result as string.
   */
  void writeSolution(std::string& osrl);
};


#endif /*GAMSOSXL_HPP_*/
