// Copyright (C) 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSHANDLER_HPP_
#define GAMSHANDLER_HPP_

#include "GAMSlinksConfig.h"

/** Forwards requests for output and point transformations to a GAMS I/O library.
 * The use of this class is to provide a uniformed interface to some operations for different GAMS I/O libraries (iolib and smag).
 * It is not meant to become a layer that allows an exchangable use of smag and iolib.  
 */
class GamsHandler {
	
public:
  /** Distinguishing between message types.
   */
  enum PrintMask {
  	LogMask = 0x1,
  	StatusMask = 0x2,
  	AllMask = LogMask|StatusMask,
  	LastPrintMask
  };
	
	
public:
	GamsHandler() { }
	
	virtual ~GamsHandler() { }
	
	/** Prints the given message into the GAMS output channels (log and/or status file).
	 */ 
	virtual void print(PrintMask mask, const char* msg) const=0;
	
	/** Prints the given message plus an extra newline into the GAMS output channels (log and/or status file).
	 */ 
	virtual void println(PrintMask mask, const char* msg) const=0;
	
	/** Flushes the buffer of a GAMS output channel.
	 */ 
	virtual void flush(PrintMask mask=AllMask) const=0;

	/** Translates a given point as seem by the GamsModel or smag user into the original gams space.
	 * These are different because GamsModel and SMAG reformulate the objective function, if possible.
	 */
	virtual void translateToGamsSpaceX(const double* x_, double objval_, double* x) const=0;
	/** Translates a given lower bound as seem by the GamsModel or smag user into the original gams space.
	 */
	virtual void translateToGamsSpaceLB(const double* lb_, double* lb) const=0;
	/** Translates a given upper bound as seem by the GamsModel or smag user into the original gams space.
	 */
	virtual void translateToGamsSpaceUB(const double* ub_, double* ub) const=0;
	/** Translates a given point from the original gams space into what the user sees in a GamsModel or smag.
	 */
	virtual void translateFromGamsSpaceX(const double* x_, double* x) const=0;
	
	/** Translates given column indices from the original gams space into what the user sees in a GamsModel or smag.
	 * Here, input and output are allowed to be the same array.
	 * @param indices_ Array of column indices of length nr (input).
	 * @param indices Array of length nr to store column indices (output).
	 * @param nr Length of arrays.
	 * @return False if there was an error, e.g., a column index is given that was reformulated out by GamsModel or smag (e.g., objective variable). True otherwise.
	 */
	virtual bool translateFromGamsSpaceCol(const int* indices_, int* indices, int nr) const=0;
	/** Translates given column index into the original gams space.
	 * @Return -1 if failure, the column index in gams space otherwise.
	 */
	virtual int translateToGamsSpaceCol(int colindex) const=0;
	/** Translates given row index into the original gams space.
	 * @Return -1 if failure, the row index in gams space otherwise.
	 */
	virtual int translateToGamsSpaceRow(int rowindex) const=0;
	
	
	/** GAMS value for minus infinity.
	 */
	virtual double getMInfinity() const=0;
	/** GAMS value for plus infinity.
	 */
	virtual double getPInfinity() const=0;
	
	/** Objective sense: +1 for min, -1 for max.
	 */
	virtual int getObjSense() const=0;
	
	/** The number of columns in the original gams space.
	 */
	virtual int getColCountGams() const=0;
	/** The index of the objective variable in the original gams space.
	 */
	virtual int getObjVariable() const=0;
	
	/** Path to GAMS system.
	 */
	virtual const char* getSystemDir() const=0;
	
	virtual bool isDictionaryWritten() const=0;
	virtual const char* dictionaryFile() const=0;
	virtual int dictionaryVersion() const=0;
}; // class GamsHandler


#endif /*GAMSHANDLER_HPP_*/
