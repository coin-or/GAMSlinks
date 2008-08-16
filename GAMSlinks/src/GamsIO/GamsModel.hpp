// Copyright (C) 2006-2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Michael Bussieck, Stefan Vigerske

#ifndef __GAMSMODEL_HPP__
#define __GAMSMODEL_HPP__

#include "GAMSlinksConfig.h"
// from CoinUtils
#include "CoinPragma.hpp"

extern "C" struct dictRec;

/** Representation of a mixed-integer linear GAMS model read in from GAMS iolib.
 * This class is a layer between the GAMS I/O libraries and the GAMS-interfaces to (OSI-compatible) LP and MIP solvers.
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
   * Note, that you need to call readMatrix() before you can access the LP.
   * After constructing a GamsModel you have only access to the options file and can, e.g., change the value for infinity.  
   * @param cntrfile Name of GAMS control file.
   */
  GamsModel(const char *cntrfile);

  /** Destructor.
   */
  ~GamsModel();

	/** Reads the LP from the GAMS matrix file and stores it into the data structures of this class.
	 */
  void readMatrix();

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
	
	/** Value used by GAMS for minus infinity.
	 */ 
	double getMInfinity() const;
	/** Value used by GAMS for plus infinity.
	 */ 
	double getPInfinity() const;
	/** Set values that GAMS should use for minus and plus infinity.
	 */ 
	void setInfinity(const double& SolverMInf, const double& SolverPInf);
	
	/** Tells whether the model has been reformulated by moving the objective equation into the objective and eliminating the objective variable.
	 */ 
	bool isReformulated() const { return isReform_; }
	
	/**@name Accessing the GAMS model.
	 * @{*/
	/** Indicates whether we represent an LP or not.
	 * It is an LP, if there are no discrete columns, no SOS, and no semicontinuous variables.
	 */
	bool isLP() const;
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
  /** The number of semicontinuous variables.
   */
	inline int nSemiContinuous() const { return nSemiCon_; }
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
	/** Indicates whether a variable is discrete (true) or not (false).
	 */
  inline bool   *ColDisc()  { return ColDisc_; }
	/** The special ordered sets indicator.
	 * For each variable, SOSIndicator()[i] tells to which special ordered set from which type it belongs, if any. 
	 * If SOSIndicator()[i] is
	 * - 0, then column i does not belong to any special ordered set.
	 * - >0, then column i belongs to the special ordered sets of type 1 with the number SOSIndicator()[i]. 
	 * - <0, then column i belongs to the special ordered sets of type 2 with the number -SOSIndicator()[i]. 
	 */
  inline int    *SOSIndicator() { return SOSIndicator_; }
  /** Indicates whether a variable is semicontinuous or semiinteger (true) or not (false).
   * To distinguish between semicontinious and semiinteger variables, consult the ColDisc() array.
   * That is, a variable is semiinteger if it is semicontinuous and discrete.    
   */
  inline bool   *ColSemiContinuous() { return ColSemiCon_; } 
	/** Initial values and storage for the solution values of the primal variables.
	 */
  inline double *ColLevel()     { return ColLevel_; }
	/** Initial marginals and storage for the solution values of the dual variables corresponding to the column bound constraints (reduced costs).
	 */
  inline double *ColMargin()    { return ColMargin_; }
	/** Initial basis status and storage for the solution basis status of the columns.
	 */
  inline int    *ColBasis()     { return ColBasis_; }
  /** Branching priorities of columns.
   * In GAMS, you can specify branching priorities with the .prior suffix.
   * As lower the value, as higher is the priority.
   */
  inline double *ColPriority()  { return ColPriority_; }
  /** Scaling parameters for columns.
   * In GAMS, you can specify scaling parameters with the .scale suffix.
   */ 
  inline double *ColScale()     { return ColScale_; }

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
	/** Initial values and storage for the solution levels of the rows (row activities).
	 */
  inline double *RowLevel()     { return RowLevel_; }
	/** Initial marginals and storage for the solution values of the dual variables corresponding to the rows (row prices).
	 */
  inline double *RowMargin()    { return RowMargin_; }
	/** Initial basis status and storage for the solution basis status of the rows.
	 */
  inline int    *RowBasis()     { return RowBasis_; }
  /** Scaling parameters for rows.
   * In GAMS, you can specify scaling parameters with the .scale suffix.
   */
  inline double *RowScale()     { return RowScale_; }

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
  /** Scaling parameter for objective function.
   * Computing from scaling parameter of objective row and objective variable.
   * Default: 1
   */
  inline double ObjScale() const { return ObjScale_; } 

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
	/** Sets the number of nodes used (in a branch-and-bound algorithm).
	 * @param NodesUsed Number of nodes the solver used.
	 */                 
  inline void setNodesUsed(const int NodesUsed) { NodeUsed_ = NodesUsed; }
	/** Sets the objective function value in the solution.
	 * @param ObjVal Value of objective function.
	 */                 
  inline void setObjVal(const double ObjVal) { ObjVal_ = ObjVal; }
	/** Sets an bound (estimate) on the optimal value.
	 * @param ObjBound Value of optimal value estimate.
	 */                 
  inline void setObjBound(const double ObjBound) { ObjBound_ = ObjBound; }
	/** Gives the stored objective function value.
	 */                 
  inline double getObjVal() { return ObjVal_; }
	/** Gives the stored number of seconds used.
	 */                 
  inline double getResUsed() { return ResUsed_; }

	/** Storage for column indicators.
	 * @see VariableStatus
	 */
  inline int    *ColIndicator() { return ColIndicator_; }
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

	/**@name GAMS specific options and parameters
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
	/** GAMS Parameter: A new valid solution must be at least this much better than incumbent integer solution (for MIPs).
	 */
  double getCheat();   
  /** GAMS Parmeter: Implied upper/lower bound on objective function. 
   */
  double getCutOff();
  /** GAMS Option: If set, then the solver should use the scaling parameters in ColScale() and RowScale().
   */
  bool getScaleOption();
  /** GAMS Option: If set, then the solver should use the branching priorities in ColPriority().
   */
  bool getPriorityOption();

	/** The name of the option file.
	 * @return The name of the option file, or NULL if no optionfile should be read.
	 */
	const char* getOptionfile();
	/**@}*/
	
	/** Indicates whether we have column and row names.
	 */
//	inline bool haveNames() { return dict; }
	
private:
  int nCols_;  // # columns
  int nDCols_; // # discrete columns
  int nSOS1_;  // # SOS type 1
  int nSOS2_;  // # SOS type 2
  int nSemiCon_; // # semicontinuous or semicont. integer columns
  int nRows_;  // # rows
  int nNnz_;   // # non zeros

  double startTime_; // Start time

  double *ColLb_;   // lower bound for columns
  double *ColUb_;   // upper bound for columns
  bool   *ColDisc_; // indicator for discrete columns: discrete (true) or not (false)
  int    *SOSIndicator_;  // indicator for SOS of type 1 and 2
  bool   *ColSemiCon_;  // indicator for semicontinuous columns: semicon. (true) or not (false)
  double *ColLevel_; // values of column variables
  double *ColMargin_; // marginal values = duals
  int    *ColBasis_; // column basis status
  double *ColPriority_; // branching priority
  double *ColScale_; // scaling parameter
  int    *ColIndicator_; // indicator for feasibility

  char   *RowSense_; // sense of row
  double *RowRhs_;   // right hand side
  double *RowScale_; // scaling parameters
  double *RowLevel_; // values of rows = row activity
  double *RowMargin_; // marginal values = duals
  int    *RowBasis_; // Row basis status
  int    *RowIndicator_; // indicator for feasibility

  double ObjSense_; // objective sense 1=min, -1=max
  double *ObjCoef_;  // Dense objective function
  double  ObjRhs_;   // constant in objective function
  double  zCoef_;    // coefficient of objective variable
  double ObjScale_;  // scale value of objective variable
  int     isReform_; // reformulated objective function

  int    *matStart_;  // matrix start of column
  int    *matRowIdx_; // matrix row index
  double *matValue_;  // matrix values
 
  void Allocate();

  double ObjVal_;
  double ObjBound_;
  double ResUsed_;
  int    IterUsed_;
  int    DomUsed_;
  int    NodeUsed_;

  SolverStatus SolverStatus_;
  ModelStatus ModelStatus_;
};

#endif // __GAMSMODEL_HPP__
