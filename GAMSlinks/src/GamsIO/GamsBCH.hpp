// Copyright (C) GAMS Development 2007-2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#ifndef GAMSBCH_HPP_
#define GAMSBCH_HPP_

#include "GAMSlinksConfig.h"
// from CoinUtils
#include "CoinPragma.hpp"

#include <vector>

#include "GamsHandler.hpp"
#include "GamsOptions.hpp"
#include "GamsDictionary.hpp"

/** Interface to GAMS BCH facility.
 * To use this interface, it is assumed that you have initialized the GDX I/O library (use gdxGetReady or gdxCreate).
 */
class GamsBCH {
public:
	/** Structure to store a Cut generated by a Gams BCH cut generator.
	 */
	class Cut {
	public:
		double lb;
		double ub;
		int nnz;
		int* indices;
		double* coeff;
		
		/** Constructor.
		 * Initializes lb and ub to 0! indices and coeff are set to NULL, nnz to -1.
		 */
		Cut();
		~Cut();
	};
	
private:
	GamsHandler& gams;

	struct dictRec* dict;
	
	/* storage of size iolib.ncols for node relaxation solution */
  double    *node_x;
	/* storage of size iolib.ncols for node lower bound */
  double    *node_lb;
	/* storage of size iolib.ncols for node upper bound */
  double    *node_ub;

  bool      have_incumbent;
  bool      new_incumbent;
  double    incumbent_value; /* objective function value of incumbent */
	/* storage of size iolib.ncols for incumbent solution */
  double    *incumbent;
	/* storage of size iolib.ncols for global lower bounds */
  double    *global_lb;
	/* storage of size iolib.ncols for global upper bounds */
  double    *global_ub;
  
  bool       userkeep;       /* indicator for running gamskeep */

  int       heurfreq;       /* heuristic frequency */
  int       heurinterval;
  int       heurmult;
  int       heurfirst;      /* heuristic at first nodes */
  bool      heurnewint;     /* heuristic if new incumbent */
  int       heurobjfirst;   /* heurisitc at first nodes if node obj is good */
  char      heurcall[1024];      /* command line minus gams for the heuristic */

  char      cutcall[1024];       /* command line minus gams for the cut generation */
  int       cutfreq;        /* cut frequency */
  int       cutinterval;
  int       cutmult;
  int       cutfirst;       /* cuts at first nodes */
  bool      cutnewint;      /* cuts if new incumbent */

  char      incbcall[1024];      /* command line minus gams for the incumbent checking callback */
  char      incbicall[1024];     /* command line minus gams for the incumbent reporting callback */

  char      userjobid[1024];     /* jobid added to gdxname, gdxnameinc, usergdxin, adds --userjobid to the calls and o and lf */

  char      gdxname[1024];       /* GDX file name for solution read by the GAMS models */
  char      gdxnameinc[1024];    /* GDX file name for incumbent read by the GAMS models */
  char      usergdxin[1024];     /* GDX file name for reading stuff back */
	
	int       ncalls; /* number of BCH calls performed */
	int       ncuts;  /* number of cuts received */
	int       nsols;  /* number of primal solutions received */
	
	int       loglevel; /* amount of output printed by GamsBCH class */

	void init();
	
	/** Writes the incumbent to the file specified in gdxnameinc and calls the incumbent reporting callback, if specified.
	 * Does nothing if there is no new incumbent.
	 * @return >0 on success, <0 on failure: -1 if writing the incumbent failed, -2 if the incumbent reporter failed, -3 if the incumbent checker failed; 1 if the incumbent is accepted, 2 if the incumbent is rejected 
	 */
	int reportIncumbent();
public:
	GamsBCH(GamsHandler& gams_, GamsDictionary& gamsdict, GamsOptions& opt);
	GamsBCH(GamsHandler& gams_, GamsDictionary& gamsdict);
	
	void setupParameters(GamsOptions& opt);

	void set_userjobid(const char* userjobid); // should be called before other parameter set methods
	void set_usergdxname(char* usergdxname, const char* usergdxprefix = NULL);
	void set_usergdxnameinc(char* usergdxnameinc, const char* usergdxprefix = NULL);
	void set_usergdxin(char* usergdxin_, const char* usergdxprefix = NULL);
	void set_userkeep(bool userkeep_) { userkeep = userkeep_; }

	void set_usercutcall(const char* usercutcall);
	void set_usercutfreq(int usercutfreq) { cutfreq=usercutfreq; }
	void set_usercutinterval(int usercutinterval) { cutinterval=usercutinterval; }
	void set_usercutmult(int usercutmult) { cutmult=usercutmult; }
	void set_usercutfirst(int usercutfirst) { cutfirst=usercutfirst; }
	void set_usercutnewint(bool usercutnewint) { cutnewint=usercutnewint; }

	void set_userheurcall(const char* userheurcall);
	void set_userheurfreq(int userheurfreq) { heurfreq=userheurfreq; }
	void set_userheurinterval(int userheurinterval) { heurinterval=userheurinterval; }
	void set_userheurmult(int userheurmult) { heurmult=userheurmult; }
	void set_userheurfirst(int userheurfirst) { heurfirst=userheurfirst; }
	void set_userheurnewint(bool userheurnewint) { heurnewint=userheurnewint; }
	void set_userheurobjfirst(int userheurobjfirst) { heurobjfirst=userheurobjfirst; }
	
	void set_userincbcall(const char* userincbcall);
	void set_userincbicall(const char* userincbicall);

	const char* get_usercutcall() { return cutcall; }
	bool get_usercutnewint() const { return cutnewint; }

	const char* get_userheurcall() { return heurcall; }
	bool get_userheurnewint() const { return heurnewint; }
	
	/** Level of output created by GamsBCH handler.
	 * @param loglevel_ 0 turns off all output, 1 prints a status line for each BCH call, 2 might print more.
	 */
	void setLogLevel(int loglevel_) { loglevel=loglevel_; }
	
	/** Accumulated number of BCH calls.
	 */
	int getNumCalls() const { return ncalls; }
	/** Accumulated number of created cuts.
	 */
	int getNumCuts() const { return ncuts; }
	/** Accumulated number of solutions.
	 */
	int getNumSols() const { return nsols; }

	void printParameters() const;
	
	~GamsBCH();
	
	/** You should call this method after the constructor.
	 */
	void setGlobalBounds(const double* lb_, const double* ub_);

	/** Sets the solution and bounds of the current node.
	 * Should be called before generateCuts() and runHeuristics().
	 */
	void setNodeSolution(const double* x_, double objval_, const double* lb_, const double* ub_);

	/** Informs BCH about the current incumbent.
	 * Should be called before generateCuts() and runHeuristics().
	 * Checks whether the objective value changed, and does nothing if its the same.
	 * If it does and an incumbent reporter or checking program is specified, calls reportIncumbent()
	 * @return True if there is no incumbent checker or if the incumbent checker accepts the solution. False if an incumbent checker rejects the solution.
	 */
	bool setIncumbentSolution(const double* x_, double objval_);
	
	/** Returns true if generateCuts should be called, otherwise false.
	 * You should call this method before generateCuts().
	 */
	bool doCuts();
	
	/** Calls the GAMS cut generator.
	 * Note, that you need to set the solution of the current nodes relaxation with.
	 * Note, that you should call this method only if doCuts() returned true.
	 * @return True, if everything worked fine (even though we might not have found cuts), false if something went wrong.
	 */
	bool generateCuts(std::vector<Cut>& cuts);
	
	/** Returns true if runHeuristic should be called, otherwise false.
	 * You should call this method before doHeuristic().
	 */
	bool doHeuristic(double bestbnd, double curobj);

	/** Calls the GAMS heuristic.
	 * @param x Space to store a solution.
	 * @param objvalue A double to store objective value of new solution.
	 * @return True, if a point has been found, false otherwise.  
	 */
	bool runHeuristic(double* x, double& objvalue);
};

#endif /*GAMSBCH_HPP_*/
