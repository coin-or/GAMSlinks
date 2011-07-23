// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author:  Stefan Vigerske

#ifndef GAMSOPTIONS_HPP_
#define GAMSOPTIONS_HPP_

#include "GAMSlinksConfig.h"

#include <cstdlib>

struct gmoRec;
struct gevRec;
struct optRec;

/** Wrapper class for GAMS option file handler.
 * Provides methods to simplify reading and setting of options.
 */
class GamsOptions {
private:
	struct gmoRec* gmo;
	struct gevRec* gev;
	struct optRec* optionshandle; // handle for options

	bool opt_is_own;

	bool initOpt(const char* solvername);

public:
	/** Constructor for GamsOptions class.
	 * Initialization of options handle.
	 * Reading of the file "<systemdir>/opt<solvername>.def" to learn which options are supported.
	 * @param gmo_ A Gams Modeling Object to get access to the system directory name and other stuff. Can be NULL and set later by setGMO.
	 * @param opt_ A Gams Optionfile handler. If NULL and setOpt is not called, an own one might be created.
	 */
	GamsOptions(struct gmoRec* gmo_ = NULL, struct optRec* opt_ = NULL);

	/** Destructor.
	 */
	~GamsOptions();

	void setGMO(struct gmoRec* gmo_);
	void setOpt(struct optRec* opt_);

	/** Reads an options file.
	 * @param solvername The name of your solver.
	 * @param optfilename Giving NULL for optfilename will read nothing and returns true.
	 */
	bool readOptionsFile(const char* solvername, const char* optfilename);

	/** Checks whether an option exists.
	 * @return True, if the option exists, i.e., defined in the options definition file. False otherwise.
	 */
	bool isKnown(const char* optname);
	/** Checks whether the user specified some option.
	 * @param optname The name of the option.
	 * @return True, if the option had been specified in the option file.
	 */
	bool isDefined(const char *optname);
//	bool optDefinedRecent(const char *optname);

	/** Gets the value of a boolean option.
	 * @param optname The name of the option.
	 */
	inline bool getBool(const char* optname) { return getInteger(optname); }
	/** Gets the value of an integer option.
	 * @param optname The name of the option.
	 */
	int getInteger(const char *optname);
	/** Gets the value of a real (double) option.
	 * @param optname The name of the option.
	 */
	double getDouble(const char *optname);
	/** Gets the value of a string option.
	 * @param optname The name of the option.
	 * @param buffer A buffer where the value can be stored (it should be large enough).
	 */
	char* getString(const char *optname, char *buffer);

	/** Sets the value of a boolean option.
	 * @param optname The name of the option.
	 * @param bval The value to set.
	 */
	inline void setBool(const char *optname, bool bval) { setInteger(optname, bval ? 1 : 0); }
	/** Sets the value of an integer option.
	 * @param optname The name of the option.
	 * @param ival The value to set.
	 */
	void setInteger(const char *optname, int ival);
	/** Sets the value of a double option.
	 * @param optname The name of the option.
	 * @param dval The value to set.
	 */
	void setDouble(const char *optname, double dval);
	/** Sets the value of a string option.
	 * @param optname The name of the option.
	 * @param sval The value to set.
	 */
	void setString(const char *optname, const char *sval);
};

#endif /*GAMSOPTIONS_HPP_*/
