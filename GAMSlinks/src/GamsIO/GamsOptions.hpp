// Copyright (C) GAMS Development 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author:  Stefan Vigerske

#ifndef GAMSOPTIONS_HPP_
#define GAMSOPTIONS_HPP_

#include "GAMSlinksConfig.h"
// from CoinUtils
#include "CoinPragma.hpp"

extern "C" struct optRec; 

/** Class to handle the access to a GAMS options file.
 */
class GamsOptions {
private:
	struct optRec* optionshandle; // handle for options

public:
	/** Constructor for GamsOptions class.
	 * Initialization of options handle.
	 * Reading of the file "<systemdir>/opt<solvername>.def" to learn which options are supported.
	 * @param systemdir The name of the GAMS system directory (where the opt...def-file is located).
	 * @param solvername The name of your solver.
	 */
	GamsOptions(const char* systemdir, const char* solvername);
	
	~GamsOptions();
	
	/** Reads an options file.
	 * @param optfilename Giving NULL for optfilename will read nothing and returns true.
	 */
	bool readOptionsFile(const char* optfilename); 

	/** Check whether the user specified some option.
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
