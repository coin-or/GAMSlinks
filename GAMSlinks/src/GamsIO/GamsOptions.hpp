// Copyright (C) GAMS Development 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsOptions.hpp 510 2008-08-16 19:31:27Z stefan $
//
// Author:  Stefan Vigerske

#ifndef GAMSOPTIONS_HPP_
#define GAMSOPTIONS_HPP_

#include "GAMSlinksConfig.h"
// from CoinUtils
#include "CoinPragma.hpp"

#include "GamsHandler.hpp"

extern "C" struct optRec; 

/** Class to handle the access to a GAMS options file.
 */
class GamsOptions {
private:
	GamsHandler& gams;

	struct optRec* optionshandle; // handle for options

public:
	/** Constructor for GamsOptions class.
	 * Initialization of options handle.
	 * Reading of the file "<systemdir>/opt<solvername>.def" to learn which options are supported.
	 * @param gams_ A GAMS handler to get access to the system directory name and other stuff.
	 * @param solvername The name of your solver.
	 */
	GamsOptions(GamsHandler& gams_, const char* solvername);
	
	/** Destructor.
	 */
	~GamsOptions();
	
	/** Reads an options file.
	 * @param optfilename Giving NULL for optfilename will read nothing and returns true.
	 */
	bool readOptionsFile(const char* optfilename); 

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
