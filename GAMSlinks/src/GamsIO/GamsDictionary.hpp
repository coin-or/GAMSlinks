// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSDICTIONARY_HPP_
#define GAMSDICTIONARY_HPP_

#include "GAMSlinksConfig.h"

#include "GamsHandler.hpp"

extern "C" struct dictRec;

class GamsBCH;

/** Class to provide access to a GAMS dictionary.
 */
class GamsDictionary {
	friend class GamsBCH;
private:
	GamsHandler& gams;
	
	struct dictRec* dict;
	
	char* constructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices);

public:
	GamsDictionary(GamsHandler& gams_);
	
	~GamsDictionary();
	
	/** Reads the GAMS dictionary.
	 * @return True if successfull, false otherwise.
	 */
	bool readDictionary();
	
	/** Indicates whether a dictionary has been successfully read.
	 */
	bool haveNames() { return dict; }

	/** The name of a column.
	    @param colnr column index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getColName(int colnr, char *buffer, int bufLen);

	/** The name of a row.
	    @param rownr row index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getRowName(int rownr, char *buffer, int bufLen);

}; // class GamsDictionary

#endif /*GAMSDICTIONARY_HPP_*/
