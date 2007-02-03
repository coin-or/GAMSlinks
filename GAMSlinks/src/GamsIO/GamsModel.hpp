// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Michael Bussieck, Stefan Vigerske

#ifndef __GAMSMODEL_HPP__
#define __GAMSMODEL_HPP__

#include "GAMSlinksConfig.h"

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

extern "C" struct dictRec;
extern "C" struct optRec; 

/** Representation of a mixed-integer linear GAMS model.
 * This class is a layer between the GAMS I/O libraries and the GAMS-interfaces to COIN-OR LP and MIP solvers.
 */ 
class GamsModel  {
public:

	/** Basis status indicator used in GAMS.
	 */ 
  enum  BasisStatus { 
    NonBasicLower = 0,
    NonBasicUpper,
    Basic,
    SuperBasic,
    LastBasisStatus 
  };
  
	/** Column status indicator used in GAMS.
	 */ 
  enum VariableStatus {
		VarNormal = 0,
		VarNonOptimal,
		VarInfeasible,
		VarUnbounded,
		VarLastStatus  	
  };

	/** Row status indicator used in GAMS.
	 */ 
  enum RowStatus {
		RowNormal = 0,
		RowNonOptimal,
		RowInfeasible,
		RowUnbounded,
		RowLastStatus  	
  };
  
	/** GAMS Solver status codes.
	 */ 
  enum  SolverStatus { 
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

	/** GAMS Model status codes.
	 */ 
  enum  ModelStatus { 
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
  
  /** Distinguishing between message types.
   */
  enum PrintMask {
  	LogMask = 0x1,
  	StatusMask = 0x2,
  	AllMask = LogMask|StatusMask,
  	LastPrintMask
  };
  

  /** Constructor.
   * @param cntrfile Name of GAMS control file.
   * @param SolverMInf If not zero, then the solver value for minus infinity. 
   * @param SolverPInf If not zero, then the solver value for plus infinity. 
   */
  GamsModel(const char *cntrfile, const double SolverMInf=0, const double SolverPInf=0);

  /** Destructor.
   */
  ~GamsModel();

	/**@name Output
	 *@{*/
	/** Log- and Statusfile print function.
	 * If the PrintMask is set to LogMask, the message appears in the logfile,
	 * if the PrintMask is set to StatusMask, the message appears in the statusfile,
	 * and if the message is set to AllMask, the message appears in both log- and statusfile.
	 * @param mask Tell where the message should be printed to.
	 * @param msg The message to print. It has to be terminated by a zero.
	 */
  void PrintOut(PrintMask mask, const char *msg);
  
  /** If SysOut is set, the solver should give plenty off output.
   */
  int getSysOut(); 
  /**@}*/

	/**@name Timing.
	 * @{*/  
  /** Starts the GAMS timer.
   */
  void TimerStart();
  /** The number of seconds since the GAMS timer was started.
   */
  double SecondsSinceStart();
  /**@}*/

	/** Name of GAMS System directory.
	 * @return Name of GAMS System directory, or NULL if not available.
	 */
	const char* getSystemDir();
	
	/**@name Accessing the GAMS model.
	 * @{*/
	/** The number of columns in the model.
	 */
  inline int nCols() const  { return nCols_; }
	/** The number of discrete columns in the model.
	 */
  inline int nDCols() const { return nDCols_; }
	/** The number of special ordered sets of type 1 in the model.
	 */
  inline int nSOS1() const  { return nSOS1_; }
	/** The number of special ordered sets of type 2 in the model
	 */
  inline int nSOS2() const  { return nSOS2_; }
	/** The number of rows in the model.
	 */
  inline int nRows() const  { return nRows_; }
	/** The number of nonzeros in the matrix of the model.
	 */
  inline int nNnz()  const  { return nNnz_; }

	/** The column lower bounds.
	 */
  inline double *ColLb()    { return ColLb_; }
	/** The column upper bounds.
	 */
  inline double *ColUb()    { return ColUb_; }
	/** The indices of the discrete variables.
	 */
  inline int    *ColDisc()  { return ColDisc_; }
	/** The special ordered sets indicator.
	 * For each variable, SOSIndicator()[i] tells to which special ordered set from which type it belongs, if any. 
	 * If SOSIndicator()[i] is
	 * - 0, then column i does not belong to any special ordered set.
	 * - >0, then column i belongs to the special ordered sets of type 1 with the number SOSIndicator()[i]. 
	 * - <0, then column i belongs to the special ordered sets of type 2 with the number -SOSIndicator()[i]. 
	 */
  inline int    *SOSIndicator() { return SOSIndicator_; }
	/** The name of a column.
	    @param colnr column index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char          *ColName(int colnr, char *buffer, int bufLen);

	/** The sense of the rows.
	 * RowSense[i] is
	 * - 'E' if row i is an equality.
	 * - 'L' if row i is of the form row <= bound.
	 * - 'G' if row i is of the form row >= bound.
	 * - 'N' if row i has no bounds.
	 * Ranges for rows are not supported by the GAMS language.  
	 */
  inline char   *RowSense() { return RowSense_; }
	/** The right-hand-side of the rows.
	 */
  inline double *RowRhs()   { return RowRhs_; }
	/** The name of a row.
	    @param rownr row index
			@param buffer a buffer for writing the name in
			@param bufLen length of the buffer
      @return buffer on success, NULL on failure
	*/
	char          *RowName(int rownr, char *buffer, int bufLen);

	/** The coefficients of the objective function (dense format).
	 */
  inline double *ObjCoef()  { return ObjCoef_; }
  /** A constant term in the objective function.
   */
  inline double ObjConstant()  { return ObjRhs_; }
  /** The sense of the objective function or optimization, respectively.
   * @return 1 for minimization, -1 for maximization.
   */
  inline double ObjSense() const { return ObjSense_; } 

	/** The column starts for the problem matrix.
	 */
  inline int    *matStart()  { return matStart_; }
  /** The row indices for the problem matrix.
   */
  inline int    *matRowIdx() { return matRowIdx_; }
  /** The values for the problem matrix.
   */
  inline double *matValue()  { return matValue_; }
  /** Removes zero entries in the problem matrix.
   * In some cases, it might happen that the sparse problem matrix as given by GAMS still contains a few zero elements.
   * This method checks the matrix and shifts elements front to eleminate nonzeros.
   * @return The number of removed zeros.
   */
  int matSqueezeZeros();
	/*@}*/

	/**@name Setting solution information. 
	 * @{*/
	/** The current GAMS model status.
	 */ 
  ModelStatus getModelStatus();
	/** The current GAMS solver status.
	 */ 
  SolverStatus getSolverStatus();

	/** Sets the GAMS solver and model status.
	 * @param newSolverStatus The new solver status.
	 * @param newModelStatus The new model status.
	 */
  void setStatus(const SolverStatus& newSolverStatus, 
                 const ModelStatus& newModelStatus);

	/** Sets the number of iterations used.
	 * @param IterUsed Number of iterations the solver used.
	 */                 
  inline void setIterUsed(const int IterUsed) { IterUsed_ = IterUsed; }
	/** Sets the number of seconds used.
	 * @param ResUsed Number of seconds the solver used.
	 */                 
  inline void setResUsed(const double ResUsed) { ResUsed_ = ResUsed; }
	/** Sets the objective function value.
	 * @param ObjVal Value of objective function.
	 */                 
  inline void setObjVal(const double ObjVal) { ObjVal_ = ObjVal; }
	/** Gives the stored objective function value.
	 */                 
  inline double getObjVal() { return ObjVal_; }
	/** Gives the stored number of seconds used.
	 */                 
  inline double getResUsed() { return ResUsed_; }

	/** Storage for the solution values of the primal variables.
	 */
  inline double *ColLevel()     { return ColLevel_; }
	/** Storage for the solution values of the dual variables corresponding to the column bound constraints (reduced costs).
	 */
  inline double *ColMargin()    { return ColMargin_; }
	/** Storage for the solution basis status of the columns.
	 */
  inline int    *ColBasis()     { return ColBasis_; }
	/** Storage for column indicators.
	 * @see VariableStatus
	 */
  inline int    *ColIndicator() { return ColIndicator_; }

	/** Storage for the solution levels of the rows (row activities).
	 */
  inline double *RowLevel()     { return RowLevel_; }
	/** Storage for the solution values of the dual variables corresponding to the rows (row prices).
	 */
  inline double *RowMargin()    { return RowMargin_; }
	/** Storage for the solution basis status of the rows.
	 */
  inline int    *RowBasis()     { return RowBasis_; }
	/** Storage for row indicators.
	 * @see RowStatus
	 */
  inline int    *RowIndicator() { return RowIndicator_; }

	/** Sets the solution information and writes the solution file.
	 * If the model status is Optimal or IntegerSolution,
	 * then for those arrays that are given, the corresponding array in the GamsModel is overwritten
	 * and a solution file is written.
	 */	
  void setSolution(const double *ColLevel=0, const double *ColMargin=0, 
                   const int *ColBasis=0, const int *ColIndicator=0,
                   const double *RowLevel=0, const double *RowMargin=0, 
                   const int *RowBasis=0, const int *RowIndicator=0);
  /**@}*/

	/**@name Options and parameters
	 * @{*/
	/** GAMS Parameter: Time limit for model run in seconds.
	 */
  double getResLim();
	/** GAMS Parameter: Iteration limit for model run.
	 */
  int    getIterLim(); // Iteration limit for model run
	/** GAMS Parameter: Node limit for model run if using a branch-and-bound algorithm.
	 */
  int    getNodeLim();
  /** GAMS Parameter: Integers 1 to 5.
   * In a GAMS model, 5 integer values can be specified, e.g. model.integer1=value;.
   * @param nr Which integer you want to have, it should be between 0 and 4.
   * @return The value of integer number nr.
   */
  int    getGamsInteger(int nr);
  /** GAMS Parameter: Integers 1 to 5, bitwise.
   * In a GAMS model, 5 integer values can be specified, e.g. model.integer1=value;.
   * @param intnr Which integer you want to have. intnr should be between 0 and 4.
   * @param bitnr Which bit of integer intnr you want to have. bitnr should be between 0 and 15.
   * @return Bit number bitnr of integer number intnr.
   */
  int    getGamsInteger(int intnr, int bitnr);
	/** GAMS Parameter: Allowed absolute difference between incumbent solution and best bound (for MIPs).
	 */
  double getOptCA();   
	/** GAMS Parameter: Allowed relative difference between incumbent solution and best bound (for MIPs).
	 */
  double getOptCR();   
  /** GAMS Parmeter: Implied upper/lower bound on objective function. 
   */
  double getCutOff();

	/** The name of the option file.
	 * @return The name of the option file, or NULL if no optionfile should be read.
	 */
	const char* getOptionfile();

	/** Initialization of optionfile reading.
	 * Initialization of options handle.
	 * Reading of the file "<GamsSystemDir>/opt<solvername>.def" to learn which options are supported.
	 * @param solvername The name of your solver.
	 */
	bool ReadOptionsDefinitions(const char* solvername); 
	/** Reads the options file, if desired.
	 */
	bool ReadOptionsFile(); 

	/** Check whether the user specified some option.
	 * @param optname The name of the option.
	 * @return True, if the option had been specified in the option file.
	 */
	bool optDefined(const char *optname);
//	bool optDefinedRecent(const char *optname);
	
	/** Gets the value of a boolean option.
	 * @param optname The name of the option.
	 */
	inline bool optGetBool(const char* optname) { return optGetInteger(optname); }
	/** Gets the value of an integer option.
	 * @param optname The name of the option.
	 */
	int optGetInteger(const char *optname);
	/** Gets the value of a real (double) option.
	 * @param optname The name of the option.
	 */
	double optGetDouble(const char *optname);
	/** Gets the value of a string option.
	 * @param optname The name of the option.
	 * @param buffer A buffer where the value can be stored (it should be large enough).
	 */
	char* optGetString(const char *optname, char *buffer);
	
	/** Sets the value of a boolean option.
	 * @param optname The name of the option.
	 * @param bval The value to set.
	 */
	inline void optSetBool(const char *optname, bool bval) { optSetInteger(optname, bval ? 1 : 0); }
	/** Sets the value of an integer option.
	 * @param optname The name of the option.
	 * @param ival The value to set.
	 */
	void optSetInteger(const char *optname, int ival);
	/** Sets the value of a double option.
	 * @param optname The name of the option.
	 * @param dval The value to set.
	 */
	void optSetDouble(const char *optname, double dval);
	/** Sets the value of a string option.
	 * @param optname The name of the option.
	 * @param sval The value to set.
	 */
	void optSetString(const char *optname, char *sval);
	/**@}*/
	
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

  SolverStatus SolverStatus_;
  ModelStatus ModelStatus_;

  struct dictRec* dict; // handle for dictionary  
	char* ConstructName(char* buffer, int bufLen, int lSym, int* uelIndices, int nIndices);

	struct optRec* optionshandle; // handle for options
};

#endif // __GAMSMODEL_HPP__
