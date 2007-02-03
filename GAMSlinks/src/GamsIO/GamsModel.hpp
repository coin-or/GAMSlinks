// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author:  Michael Bussieck

#ifndef __GAMSMODEL_HPP__
#define __GAMSMODEL_HPP__

#include "GAMSlinksConfig.h"

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#define LOGMASK    0x1
#define STATUSMASK 0x2
#define ALLMASK    LOGMASK|STATUSMASK

extern "C" struct dictRec;
extern "C" struct optRec; 

class GamsModel  {

public:

  enum  GamsModelBasisStatus { 
    NonBasicLower = 0,
    NonBasicUpper,
    Basic,
    SuperBasic,
    LastBasisStatus 
  };

  enum  GamsModelSolverStatus { 
    SolverStatusNotSet = 0,
    NormalCompletion = 1,
    IterationInterrupt,
    ResourceInterrupt,
    TerminatedBySolver,
    EvaluationErrorLimit,
    CapabilityProblems,
    LicensingProblems,
    UserInterrupt,
    ErrorSetupFailure,
    ErrorSolverFailure,
    ErrorInternalSolverError,
    SolveProcessingSkipped,
    ErrorSystemFailure,
    LastSolverStatus 
  };

  enum  GamsModelModelStatus { 
    ModelStatusNotSet = 0,
    Optimal,
    LocallyOptimal,
    Unbounded,
    Infeasible,
    LocallyInfeasible,
    IntermediateInfeasible,
    IntermediateNonoptimal,
    IntegerSolution,
    IntermediateNonInteger,
    IntegerInfeasible,
    LicensingProblemNoSolution,
    ErrorUnknown,
    ErrorNoSolution,
    NoSolutionReturned,
    SolvedUnique,
    Solved,
    SolvedSingular,
    UnboundedNoSolution,
    InfeasibleNoSolution,
    LastModelStatus 
  };

  /** Constructor.
   * @param cntrfile Name of GAMS control file.
   * @param SolverMInf If not zero, then the solver value for minus infinity. 
   * @param SolverMInf If not zero, then the solver value for plus infinity. 
   */
  GamsModel(const char *cntrfile, const double SolverMInf=0, const double SolverPInf=0);

  /** Destructor */
  ~GamsModel();

  void PrintOut(int mask, const char *msg); // Log and Statusfile print function
  void TimerStart();  // Start GAMS Timer
  double SecondsSinceStart();

	const char* getOptionfile(); // Name of option file, or NULL if no optionfile
	const char* getSystemDir(); // Name of GAMS System directory, or NULL if not available
  double getResLim();  // Time limit for model run
  int    getIterLim(); // Iteration limit for model run
  int    getNodeLim(); // Node limit for model run
  int    getGamsInteger(int i);  // integer1-5
  int    getGamsSwitch(int i, int j);  // integer1-5 bit 0-15
  int    getSysOut();  // indicator for plenty off output

  double getOptCA();   // allowed absolute difference between incumbent and best bound
  double getOptCR();   // allowed absolute difference between incumbent and best bound

  double getCutOff();  // implied upper/lower bound on objective
  int getModelStatus();

  void setStatus(const GamsModelSolverStatus SolverStatus, 
                 const GamsModelModelStatus ModelStatus);
  inline void setIterUsed(const int IterUsed) { IterUsed_ = IterUsed; }
  inline void setResUsed(const double ResUsed) { ResUsed_ = ResUsed; }
  inline void setObjVal(const double ObjVal) { ObjVal_ = ObjVal; }
  inline double getObjVal() { return ObjVal_; }
  inline double getResUsed() { return ResUsed_; }

  inline int nCols() const  { return nCols_; }   // # columns
  inline int nDCols() const { return nDCols_; }  // # discrete columns
  inline int nSOS1() const  { return nSOS1_; }   // # SOS of type 1
  inline int nSOS2() const  { return nSOS2_; }   // # SOS of type 2
  inline int nRows() const  { return nRows_; }   // # rows
  inline int nNnz()  const  { return nNnz_; }    // # non zeros

  inline double *ColLb()    { return ColLb_; }   // lower bound for columns
  inline double *ColUb()    { return ColUb_; }   // upper bound for columns
  inline int    *ColDisc()  { return ColDisc_; } // discrete columns
  inline int    *SOSIndicator() { return SOSIndicator_; } // indicator for SOS of type 1 and 2 
  inline int    *ColStat()  { return ColStat_; } // Column basis status

  inline char   *RowSense() { return RowSense_; } // sense of row
  inline double *RowRhs()   { return RowRhs_; }   // rhs
  inline int    *RowStat()  { return RowStat_; }  // Row basis status

  inline double *ObjCoef()  { return ObjCoef_; } // Dense objective function
  inline double ObjRhs()  { return ObjRhs_; }    // constant term in the objective
  inline double ObjSense() const { return ObjSense_; } // 1=min, -1=max

  inline int    *matStart()  { return matStart_; }  // matrix start of column
  inline int    *matRowIdx() { return matRowIdx_; } // matrix row index
  inline double *matValue()  { return matValue_; }  // matrix values
  int matSqueezeZeros();

  inline double *ColLevel()     { return ColLevel_; }
  inline double *ColMargin()    { return ColMargin_; }
  inline int    *ColBasis()     { return ColBasis_; }
  inline int    *ColIndicator() { return ColIndicator_; }
	/** get name of a column
	    @param colnr column index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char          *ColName(int colnr, char *buffer, int bufLen);

  inline double *RowLevel()     { return RowLevel_; }
  inline double *RowMargin()    { return RowMargin_; }
  inline int    *RowBasis()     { return RowBasis_; }
  inline int    *RowIndicator() { return RowIndicator_; }
	/** get name of a row
	    @param rownr row index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char          *RowName(int rownr, char *buffer, int bufLen);
	
  void setSolution(const double *ColLevel=0, const double *ColMargin=0, 
                   const int *ColBasis=0, const int *ColIndicator=0,
                   const double *RowLevel=0, const double *RowMargin=0, 
                   const int *RowBasis=0, const int *RowIndicator=0);

	bool ReadOptionsDefinitions(const char* solvername); 
	bool ReadOptionsFile(); 

	bool optDefined(const char *optname);
	bool optDefinedRecent(const char *optname);
	
	inline bool optGetBool(const char* optname) { return optGetInteger(optname); }
	int optGetInteger(const char *optname);
	double optGetDouble(const char *optname);
	char* optGetString(const char *optname, char *sval);
	
	inline void optSetBool(const char *optname, bool bval) { optSetInteger(optname, bval ? 1 : 0); }
	void optSetInteger(const char *optname, int ival);
	void optSetDouble(const char *optname, double dval);
	void optSetString(const char *optname, char *sval);

private:
  int nCols_;  // # columns
  int nDCols_; // # discrete columns
  int nSOS1_;  // # SOS type 1
  int nSOS2_;  // # SOS type 2
  int nRows_;  // # rows
  int nNnz_;   // # non zeros

  double startTime_; // Start time

  double *ColLb_;   // lower bound for columns
  double *ColUb_;   // upper bound for columns
  int    *ColDisc_; // list 0..nDCols_-1 for column indicies of discrete columns
  int    *ColStat_; // Column basis status
  int    *SOSIndicator_;  // indicator for SOS of type 1 and 2

  char   *RowSense_; // sense of row
  double *RowRhs_;   // rhs
  int    *RowStat_;  // Row basis status

  double ObjSense_; // objective sense 1=min, -1=max
  double *ObjCoef_;  // Dense objective function
  double  ObjRhs_;   // constant in objective function
  double  zCoef_;    // coefficient of objective variable
  int     isReform_; // reformulated objective function

  int    *matStart_;  // matrix start of column
  int    *matRowIdx_; // matrix row index
  double *matValue_;  // matrix values

  double *RowLevel_;
  double *RowMargin_;
  int    *RowBasis_;
  int    *RowIndicator_;

  double *ColLevel_;
  double *ColMargin_;
  int    *ColBasis_;
  int    *ColIndicator_;
  
  void Allocate();
  void ReadMatrix();

  double ObjVal_;
  double ResUsed_;
  int    IterUsed_;
  int    DomUsed_;

  GamsModelSolverStatus SolverStatus_;
  GamsModelModelStatus ModelStatus_;

  struct dictRec* dict; // handle for dictionary  
	char* ConstructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices);

	struct optRec* optionshandle; // handle for options
};

#endif // __GAMSMODEL_HPP__
