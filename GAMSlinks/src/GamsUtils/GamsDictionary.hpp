// Copyright (C) GAMS Development and others 2008-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSDICTIONARY_HPP_
#define GAMSDICTIONARY_HPP_

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

struct dctRec;
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
	struct dctRec* dict;
	
	bool have_dictread;
	bool dict_is_own;
	
	bool initGCD();
	
	char* constructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices);

public:
	/** Constructor.
	 * @param gmo_ A Gams Modeling object handler, can be NULL and set later by setGMO.
	 * @param opt_ An Gams dictionary file handler. If NULL and setGCD is not called, an own one will be created.
	 */
	GamsDictionary(struct gmoRec* gmo_ = NULL, struct dctRec* dict_ = NULL);
	
	/** Destructor.
	 */
	~GamsDictionary();
	
	/** Sets the GMO handle to use.
	 */
	void setGMO(struct gmoRec* gmo_) { gmo = gmo_; }
	
	/** Sets a GCD handle to use.
	 * If not set, an own gcd handle is created.
	 */
	void setGCD(struct dctRec* gcd_);
	
	/** Reads the GAMS dictionary.
	 * @return True if successful, false otherwise.
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
