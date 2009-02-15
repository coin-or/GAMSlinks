// Copyright (C) 2008-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSDICTIONARY_HPP_
#define GAMSDICTIONARY_HPP_

#include "GAMSlinksConfig.h"

struct gcdRec;
struct gmoRec;

class GamsBCH;
class GamsGDX;

/** Wrapper class for GAMS dictionary handler.
 * Provides some methods to simplify reading of column, row, and objective names.
 */
class GamsDictionary {
	friend class GamsBCH;
	friend class GamsGDX;
private:
	struct gmoRec* gmo;
	struct gcdRec* dict;
	
	bool have_dictread;
	
	char* constructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices);

public:
	/** Constructor.
	 * @param gams_ A GamsHandler that gives access to the GAMS dictionary file.
	 */
	GamsDictionary(struct gmoRec* gmo_);
	
	/** Destructor.
	 */
	~GamsDictionary();
	
	/** Reads the GAMS dictionary.
	 * @return True if successfull, false otherwise.
	 */
	bool readDictionary();
	
	/** Indicates whether a dictionary has been successfully read.
	 */
	bool haveNames() { return dict; }
	
	/** The name of a column.
	    @param colnr column index w.r.t. gmo model
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getColName(int colnr, char *buffer, int bufLen);

	/** The name of a column in the original model space.
	    @param colnr column index w.r.t. model space
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	 */
	char* getModelColName(int colnr, char *buffer, int bufLen);

	/** The name of a row.
	    @param rownr row index w.r.t. gmo model
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getRowName(int rownr, char *buffer, int bufLen);

	/** The name of a row in the original model space.
	    @param rownr row index w.r.t. model space
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getModelRowName(int rownr, char *buffer, int bufLen);

	/** The name of the objective.
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getObjName(char* buffer, int bufLen);
	
	/** The descriptive text of a column.
	    @param colnr col index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getColText(int colnr, char* buffer, int bufLen);
	
	/** The descriptive text of a row.
	    @param rownr row index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getRowText(int rownr, char* buffer, int bufLen);

	/** The descriptive text of the objective.
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char* getObjText(char* buffer, int bufLen);

}; // class GamsDictionary

#endif /*GAMSDICTIONARY_HPP_*/
