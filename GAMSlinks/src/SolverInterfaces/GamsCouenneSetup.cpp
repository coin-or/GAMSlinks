// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsBonmin.hpp 652 2009-04-11 17:14:51Z stefan $
//
// this is an adapted version of BonCouenneSetup to use GAMS instead of ASL

#include "BonCouenneSetup.hpp"
#include "BonInitHeuristic.hpp"
#include "BonNlpHeuristic.hpp"
#include "BonCouenneInterface.hpp"

#include "BonGuessHeuristic.hpp"
#include "CbcCompareActual.hpp"

#include "CouenneObject.hpp"
#include "CouenneVarObject.hpp"
#include "CouenneVTObject.hpp"
#include "CouenneChooseVariable.hpp"
#include "CouenneChooseStrong.hpp"
#include "CouenneSolverInterface.hpp"
#include "CouenneCutGenerator.hpp"
#include "BonCouenneInfo.hpp"
#include "BonCbcNode.hpp"

// MILP cuts
#include "CglGomory.hpp"
#include "CglProbing.hpp"
#include "CglKnapsackCover.hpp"
#include "CglOddHole.hpp"
#include "CglClique.hpp"
#include "CglFlowCover.hpp"
#include "CglMixedIntegerRounding2.hpp"
#include "CglTwomir.hpp"
#include "CglPreProcess.hpp"
#include "CglLandP.hpp"
#include "CglRedSplit.hpp"

// to be sure to get (or not get) HAVE_M??? and HAVE_PARDISO defined
#include "IpoptConfig.h"

extern "C" {
#ifndef HAVE_MA27
#define HAVE_HSL_LOADER
#else
# ifndef HAVE_MA28
# define HAVE_HSL_LOADER
# else
#  ifndef HAVE_MA57
#  define HAVE_HSL_LOADER
#  else
#   ifndef HAVE_MC19
#   define HAVE_HSL_LOADER
#   endif
#  endif
# endif
#endif
#ifdef HAVE_HSL_LOADER
#include "HSLLoader.h"
#endif
#ifndef HAVE_PARDISO
#include "PardisoLoader.h"
#endif
}

#include "GamsMINLP.hpp"
#include "GamsMessageHandler.hpp"

// GAMS
#ifdef GAMS_BUILD
#include "gmomcc.h"
#else
#include "gmocc.h"
#endif

using namespace Bonmin;
using namespace Ipopt;


GamsCouenneSetup::~GamsCouenneSetup(){
  //if (CouennePtr_)
  //delete CouennePtr_;
}

bool GamsCouenneSetup::InitializeCouenne(SmartPtr<GamsMINLP> minlp, CouenneProblem* problem) {
	// instead of initializeOptionsAndJournalist we do it our own way, so we can use the SmagJournal
  SmartPtr<OptionsList> options_   = new OptionsList();
	SmartPtr<Journalist> journalist_ = new Journalist();
  SmartPtr<Bonmin::RegisteredOptions> roptions_ = new Bonmin::RegisteredOptions();
 	SmartPtr<Journal> jrnl = new GamsJournal(gmo, "console", J_ITERSUMMARY, J_STRONGWARNING);
	jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!journalist_->AddJournal(jrnl))
		gmoLogStat(gmo, "Failed to register GamsJournal for IPOPT output.");
	options_->SetJournalist(journalist_);
	options_->SetRegisteredOptions(GetRawPtr(roptions_));

	setOptionsAndJournalist(roptions_, options_, journalist_);
  registerOptions();

	// Change some options
	options()->SetNumericValue("bound_relax_factor", 0, true, true);
	options()->SetNumericValue("nlp_lower_bound_inf", gmoMinf(gmo), false, true);
	options()->SetNumericValue("nlp_upper_bound_inf", gmoPinf(gmo), false, true);
	if (gmoUseCutOff(gmo))
		options()->SetNumericValue("bonmin.cutoff", gmoSense(gmo) == Obj_Min ? gmoCutOff(gmo) : -gmoCutOff(gmo), true, true);
	options()->SetNumericValue("bonmin.allowable_gap", gmoOptCA(gmo), true, true);
	options()->SetNumericValue("bonmin.allowable_fraction_gap", gmoOptCR(gmo), true, true);
	if (gmoNodeLim(gmo))
		options()->SetIntegerValue("bonmin.node_limit", gmoNodeLim(gmo), true, true);
	options()->SetNumericValue("bonmin.time_limit", gmoResLim(gmo), true, true);
  /** Change default value for failure behavior so that code doesn't crash when Ipopt does not solve a sub-problem.*/
  options()->SetStringValue("nlp_failure_behavior", "fathom", "bonmin.");

	if (gmoNLM(gmo) == 0  && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp || gmoModelType(gmo) == Proc_miqcp))
		options()->SetStringValue("hessian_constant", "yes", true, true); 
	
	try {
		if (gmoOptFile(gmo)) {
			options()->SetStringValue("print_user_options", "yes", true, true);
			char buffer[1024];
			gmoNameOptFile(gmo, buffer);
			readOptionsFile(buffer);
		}
	} catch (IpoptException error) {
		gmoLogStat(gmo, error.Message().c_str());
	  return 1;
	} catch (std::bad_alloc) {
		gmoLogStat(gmo, "Error: Not enough memory\n");
		return -1;
	} catch (...) {
		gmoLogStat(gmo, "Error: Unknown exception thrown.\n");
		return -1;
	}

	std::string libpath;
#ifdef HAVE_HSL_LOADER
	if (options()->GetStringValue("hsl_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadHSL(libpath.c_str(), buffer, 512) != 0) {
			gmoLogStatPChar(gmo, "Failed to load HSL library at user specified path: ");
			gmoLogStat(gmo, buffer);
		  return 1;
		}
	}
#endif
#ifndef HAVE_PARDISO
	if (options()->GetStringValue("pardiso_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadPardisoLib(libpath.c_str(), buffer, 512) != 0) {
			gmoLogStatPChar(gmo, "Failed to load Pardiso library at user specified path: ");
			gmoLogStat(gmo, buffer);
		  return 1;
		}
	}
#endif

	options()->GetNumericValue("diverging_iterates_tol", minlp->nlp->div_iter_tol, "");
//	// or should we also check the tolerance for acceptable points?
//	bonmin_setup.options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, "");
//	bonmin_setup.options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, "");

	bool hessian_is_approx = false;
	std::string parvalue;
	options()->GetStringValue("hessian_approximation", parvalue, "");
	if (parvalue == "exact") {
		int do2dir, dohess;
		if (gmoHessLoad(gmo, 10, 10, &do2dir, &dohess) || !dohess) { // TODO make 10 (=conopt default for rvhess) a parameter
			gmoLogStat(gmo, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
			options()->SetStringValue("hessian_approximation", "limited-memory");
			hessian_is_approx = true;
	  }
	} else
		hessian_is_approx = true;
	
	if (hessian_is_approx) { // check whether QP strong branching is enabled
		options()->GetStringValue("varselect_stra", parvalue, "bonmin.");
		if (parvalue == "qp-strong-branching") {
			gmoLogStat(gmo, "Error: QP strong branching does not work when the Hessian is approximated. Aborting...");
			return 1;
		}
	}

  gatherParametersValues(options());

  /** Set the output level for the journalist for all Couenne
   categories.  We probably want to make that a bit more flexible
   later. */
  int i;

  options()->GetIntegerValue("boundtightening_print_level", i, "bonmin.");
  journalist()->GetJournal("console")-> SetPrintLevel(J_BOUNDTIGHTENING, (EJournalLevel) i);

  options()->GetIntegerValue("branching_print_level", i, "bonmin.");
  journalist()->GetJournal("console")-> SetPrintLevel(J_BRANCHING, (EJournalLevel) i);

  options()->GetIntegerValue("convexifying_print_level", i, "bonmin.");
  journalist()->GetJournal("console")-> SetPrintLevel(J_CONVEXIFYING, (EJournalLevel) i);

  options()->GetIntegerValue("problem_print_level", i, "bonmin.");
  journalist()->GetJournal("console")-> SetPrintLevel(J_PROBLEM, (EJournalLevel) i);

  options()->GetIntegerValue("nlpheur_print_level", i, "bonmin.");
  journalist()->GetJournal("console")-> SetPrintLevel(J_NLPHEURISTIC, (EJournalLevel) i);

	CouenneInterface* ci = new CouenneInterface();
	ci->initialize(roptions(), options(), journalist(), /* "bonmin.",*/ GetRawPtr(minlp));
	GamsMessageHandler* msghandler = new GamsMessageHandler(gmo);
	ci->passInMessageHandler(msghandler);

	continuousSolver_ = new CouenneSolverInterface;
  nonlinearSolver_ = ci;  
  
  /* Initialize Couenne cut generator.*/
  //int ivalue, num_points;
  //options()->GetEnumValue("convexification_type", ivalue,"bonmin.");
  //options()->GetIntegerValue("convexification_points",num_points,"bonmin.");
  
  CouenneCutGenerator * couenneCg = 
    new CouenneCutGenerator (ci, this, NULL, journalist());

  CouenneProblem * couenneProb = problem;

  Bonmin::BabInfo * extraStuff = new Bonmin::CouenneInfo(0);
  // as per instructions by John Forrest, to get changed bounds
  extraStuff -> setExtraCharacteristics (extraStuff -> extraCharacteristics () | 2);
  continuousSolver_ -> setAuxiliaryInfo (extraStuff);
  delete extraStuff;
  extraStuff = dynamic_cast <Bonmin::BabInfo *> (continuousSolver_ -> getAuxiliaryInfo ());
  
  /* Setup log level*/
  int lpLogLevel;
  options()->GetIntegerValue("lp_log_level",lpLogLevel,"bonmin.");
  continuousSolver_->messageHandler()->setLogLevel(lpLogLevel);

  //////////////////////////////////////////////////////////////

  couenneCg ->Problem()->setMaxCpuTime(getDoubleParameter(BabSetupBase::MaxTime));

  ci->extractLinearRelaxation (*continuousSolver_, *couenneCg);

  // In case there are no discrete variables, we have already a
  // heuristic solution for which create a initialization heuristic
  if (!(extraStuff -> infeasibleNode ()) && ci -> isProvenOptimal () && ci -> haveNlpSolution ()) {

    /// setup initial heuristic (in principle it should only run once...)
    InitHeuristic* initHeuristic = new InitHeuristic(ci -> getObjValue (), ci -> getColSolution (), *couenneProb);
    HeuristicMethod h;
    h.id = "Init Rounding NLP";
    h.heuristic = initHeuristic;
    heuristics_.push_back(h);
  }

  if (extraStuff -> infeasibleNode ()){
    std::cout<<"Initial linear relaxation constructed by Couenne is infeasible, exiting..." <<std::endl;
    return false;
  }

  //continuousSolver_ -> findIntegersAndSOS (false);
  //addSos (); // only adds embedded SOS objects

  // Add Couenne SOS ///////////////////////////////////////////////////////////////

  std::string s;
  int nSOS = 0;

  // allocate sufficient space for both nonlinear variables and SOS's
  OsiObject ** objects;
  options () -> GetStringValue ("enable_sos", s, "couenne.");
  if (s == "yes") {
    objects = new OsiObject* [couenneProb -> nVars ()];
    nSOS = couenneProb -> findSOS (nonlinearSolver (), objects);
    //printf ("==================== found %d SOS\n", nSOS);
    //nonlinearSolver () -> addObjects (nSOS, objects);
    continuousSolver () -> addObjects (nSOS, objects);
    for (int i=0; i<nSOS; i++)
    	delete objects [i];
    delete [] objects;
  }

  //model -> assignSolver (continuousSolver_, true);
  //continuousSolver_ = model -> solver();

  // Add Couenne objects for branching /////////////////////////////////////////////

  options () -> GetStringValue ("branching_object", s, "couenne.");

  enum CouenneObject::branch_obj objType = CouenneObject::VAR_OBJ;

  if      (s == "vt_obj")   objType = CouenneObject::VT_OBJ;
  else if (s == "var_obj")  objType = CouenneObject::VAR_OBJ;
  else if (s == "expr_obj") objType = CouenneObject::EXPR_OBJ;
  else {
    printf ("CouenneSetup: Unknown branching object type\n");
    exit (-1);
  }

  int 
    nobj  = 0, // if no SOS then objects is empty
    nVars = couenneProb -> nVars ();

  objects = new OsiObject* [couenneProb -> nVars ()];

  int contObjPriority = 2000; // default object priority -- it is 1000 for integers and 10 for SOS

  options () -> GetIntegerValue ("cont_var_priority", contObjPriority, "bonmin.");

  for (int i = 0; i < nVars; i++) { // for each variable

    exprVar *var = couenneProb -> Var (i);

    // we only want enabled variables
    if (var -> Multiplicity () <= 0) 
    	continue;

    switch (objType) {
    case CouenneObject::EXPR_OBJ:
    	// if this variable is associated with a nonlinear function
    	if (var -> isInteger () || 
    			(var -> Type  () == AUX) && 
    			(var -> Image () -> Linearity () > LINEAR)) {
    		objects [nobj] = new CouenneObject (couenneProb, var, this, journalist ());
    		objects [nobj++] -> setPriority (contObjPriority);
    		//objects [nobj++] -> setPriority (contObjPriority + var -> rank ());
    	}
    	break;

    case CouenneObject::VAR_OBJ:
    	// branching objects on variables
    	if // comment three lines below for linear variables too
    	(var -> isInteger () || 
    			(couenneProb -> Dependence () [var -> Index ()] . size () > 0)) {  // has indep
    		//|| ((var -> Type () == AUX) &&                                  // or, aux 
    		//    (var -> Image () -> Linearity () > LINEAR))) {              // of nonlinear

    		objects [nobj] = new CouenneVarObject (couenneProb, var, this, journalist ());
    		objects [nobj++] -> setPriority (contObjPriority);
    		//objects [nobj++] -> setPriority (contObjPriority + var -> rank ());
    	}
    	break;

    default:
    case CouenneObject::VT_OBJ:
    	// branching objects on variables
    	if // comment three lines below for linear variables too
    	(var -> isInteger () || 
    			(couenneProb -> Dependence () [var -> Index ()] . size () > 0)) { // has indep
    		//|| ((var -> Type () == AUX) &&                      // or, aux 
    		//(var -> Image () -> Linearity () > LINEAR))) { // of nonlinear
    		objects [nobj] = new CouenneVTObject (couenneProb, var, this, journalist ());
    		objects [nobj++] -> setPriority (contObjPriority);
    		//objects [nobj++] -> setPriority (contObjPriority + var -> rank ());
    	}
    	break;
    }
  }

  // Add objects /////////////////////////////////

  continuousSolver_ -> addObjects (nobj, objects);

  for (int i = 0 ; i < nobj ; i++)
    delete objects [i];

  delete [] objects;

  // Setup Convexifier generators ////////////////////////////////////////////////

  int freq;
  options()->GetIntegerValue("convexification_cuts",freq,"couenne.");
  if (freq != 0) {
    CuttingMethod cg;
    cg.frequency = freq;
    cg.cgl = couenneCg;
    cg.id = "Couenne convexifier cuts";
    cutGenerators().push_back(cg);
    // set cut gen pointer
    dynamic_cast <CouenneSolverInterface *> (continuousSolver_) -> setCutGenPtr (couenneCg);
  }

  // add other cut generators -- test for integer variables first
  if (couenneCg -> Problem () -> nIntVars () > 0)
    addMilpCutGenerators ();

  CouennePtr_ = couenneCg;

  // Setup heuristic to solve nlp problems. /////////////////////////////////

  int doNlpHeurisitic = 0;
  options()->GetEnumValue("local_optimization_heuristic", doNlpHeurisitic, "couenne.");
  if(doNlpHeurisitic)
  {
    int numSolve;
    options()->GetIntegerValue("log_num_local_optimization_per_level",numSolve,"couenne.");
    NlpSolveHeuristic * nlpHeuristic = new NlpSolveHeuristic;
    nlpHeuristic->setNlp(*ci,false);
    nlpHeuristic->setCouenneProblem(couenneProb);
    //nlpHeuristic->setMaxNlpInf(1e-4);
    nlpHeuristic->setMaxNlpInf(maxNlpInf_0);
    nlpHeuristic->setNumberSolvePerLevel(numSolve);
    HeuristicMethod h;
    h.id = "Couenne Rounding NLP";
    h.heuristic = nlpHeuristic;
    heuristics_.push_back(h);
  }

#if 0
  {
    CbcCompareEstimate compare;
    model -> setNodeComparison(compare);
    GuessHeuristic * guessHeu = new GuessHeuristic (*model);
    HeuristicMethod h;
    h.id = "Bonmin Guessing";
    h.heuristic = guessHeu;
    heuristics_.push_back (h);
    //model_.addHeuristic(guessHeu);
    //delete guessHeu;
  }
#endif

  // Add Branching rules ///////////////////////////////////////////////////////

  int varSelection;
  if (!options_->GetEnumValue("variable_selection",varSelection,"bonmin.")) {
    // change the default for Couenne
    varSelection = OSI_SIMPLE;
  }

  switch (varSelection) {
  	case OSI_STRONG: { // strong branching
  		CouenneChooseStrong * chooseVariable = new CouenneChooseStrong(*this, couenneProb, journalist ());
  		chooseVariable->setTrustStrongForSolution(false);
  		chooseVariable->setTrustStrongForBound(false);
  		chooseVariable->setOnlyPseudoWhenTrusted(true);
  		branchingMethod_ = chooseVariable;
  		break;
  	}
  	case OSI_SIMPLE: // default choice
  		branchingMethod_ = new CouenneChooseVariable(continuousSolver_, couenneProb, journalist ());
  		break;
  	default:
  		std::cerr << "Unknown variable_selection for Couenne\n" << std::endl;
  		throw;
  		break;
  }

  int ival;
  if (!options_->GetEnumValue("node_comparison",ival,"bonmin.")) {
    // change default for Couenne
    nodeComparisonMethod_ = bestBound;
  }
  else {
    nodeComparisonMethod_ = NodeComparison(ival);
  }

  if(intParam_[NumCutPasses] < 2)
  	intParam_[NumCutPasses] = 2;
  // Tell Cbc not to check again if a solution returned from
  // heuristic is indeed feasible
  intParam_[BabSetupBase::SpecialOption] = 16 | 4;
	return true;
}
 
void GamsCouenneSetup::registerOptions(){
	registerAllOptions(roptions());
}

void GamsCouenneSetup::registerAllOptions(Ipopt::SmartPtr<Bonmin::RegisteredOptions> roptions){
  BabSetupBase::registerAllOptions(roptions);
  BonCbcFullNodeInfo::registerOptions(roptions);
  CouenneCutGenerator::registerOptions (roptions);

  roptions->AddBoundedIntegerOption(
    "branching_print_level",
    "Output level for braching code in Couenne",
    -2, J_LAST_LEVEL-1, J_NONE,
    "");
  roptions->AddBoundedIntegerOption(
    "boundtightening_print_level",
    "Output level for bound tightening code in Couenne",
    -2, J_LAST_LEVEL-1, J_NONE,
    "");
  roptions->AddBoundedIntegerOption(
    "convexifying_print_level",
    "Output level for convexifying code in Couenne",
    -2, J_LAST_LEVEL-1, J_NONE,
    "");
  roptions->AddBoundedIntegerOption(
    "problem_print_level",
    "Output level for problem manipulation code in Couenne",
    -2, J_LAST_LEVEL-1, J_WARNING,
    "");
  roptions->AddBoundedIntegerOption(
    "nlpheur_print_level",
    "Output level for NLP heuristic in Couenne",
    -2, J_LAST_LEVEL-1, J_WARNING,
    "");

	roptions->SetRegisteringCategory("Linear Solver", Bonmin::RegisteredOptions::IpoptCategory);
#ifdef HAVE_HSL_LOADER
	// add option to specify path to hsl library
  roptions->AddStringOption1("hsl_library", // name
			"path and filename of HSL library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"path (incl. filename) of HSL library", // description1
			"Specify the path to a library that contains HSL routines and can be load via dynamic linking. "
			"Note, that you still need to specify to use the corresponding routines (ma27, ...) by setting the corresponding options, e.g., ``linear_solver''."
	);
#endif
#ifndef HAVE_PARDISO
	// add option to specify path to pardiso library
  roptions->AddStringOption1("pardiso_library", // name
			"path and filename of Pardiso library for dynamic load",  // short description
			"", // default value 
			"*", // setting1
			"path (incl. filename) of Pardiso library", // description1
			"Specify the path to a Pardiso library that and can be load via dynamic linking. "
			"Note, that you still need to specify to pardiso as linear_solver."
	);
#endif

  // copied from BonminSetup::registerMilpCutGenerators(), in
  // BonBonminSetup.cpp

  struct cutOption_ {

    const char *cgname;
    int         defaultFreq;

  } cutOption [] = {
    {(const char *) "Gomory_cuts",             0},
    {(const char *) "probing_cuts",            0},
    {(const char *) "cover_cuts",              0},
    {(const char *) "mir_cuts",                0},
    {(const char *) "2mir_cuts",               0},
    {(const char *) "flow_covers_cuts",        0},
    {(const char *) "lift_and_project_cuts",   0},
    {(const char *) "reduce_split_cuts",       0},
    {(const char *) "clique_cuts",             0},
    {NULL, 0}};

  for (int i=0; cutOption [i].cgname; i++) {

    char descr [150];

    sprintf (descr, "Frequency k (in terms of nodes) for generating %s cuts in branch-and-cut.",
       cutOption [i].cgname);

    roptions -> AddLowerBoundedIntegerOption(cutOption [i].cgname, 
    		descr,
    		-100, cutOption [i].defaultFreq,
    		"If k > 0, cuts are generated every k nodes, "
    		"if -99 < k < 0 cuts are generated every -k nodes but "
    		"Cbc may decide to stop generating cuts, if not enough are generated at the root node, "
    		"if k=-99 generate cuts only at the root node, if k=0 or 100 do not generate cuts.");

    roptions->setOptionExtraInfo (cutOption [i].cgname, 5);
  }
}

/** Add milp cut generators according to options.*/
void GamsCouenneSetup::addMilpCutGenerators () {

  enum extraInfo_ {CUTINFO_NONE, CUTINFO_MIG, CUTINFO_PROBING, CUTINFO_CLIQUE};

  struct cutInfo {

    const char      *optname;
    CglCutGenerator *cglptr;
    const char      *cglId;
    enum extraInfo_  extraInfo;

  } cutList [] = {
    {(const char*)"Gomory_cuts",new CglGomory,      (const char*)"Mixed Integer Gomory",CUTINFO_MIG},
    {(const char*)"probing_cuts",new CglProbing,        (const char*) "Probing", CUTINFO_PROBING},
    {(const char*)"mir_cuts",new CglMixedIntegerRounding2, (const char*) "Mixed Integer Rounding", 
                                                                                      CUTINFO_NONE},
    {(const char*)"2mir_cuts",    new CglTwomir,         (const char*) "2-MIR",    CUTINFO_NONE},
    {(const char*)"cover_cuts",   new CglKnapsackCover,  (const char*) "Cover",    CUTINFO_NONE},
    {(const char*)"clique_cuts",  new CglClique,         (const char*) "Clique",   CUTINFO_CLIQUE},
    {(const char*)"lift_and_project_cuts",new CglLandP,(const char*)"Lift and Project",CUTINFO_NONE},
    {(const char*)"reduce_split_cuts",new CglRedSplit,(const char*) "Reduce and Split",CUTINFO_NONE},
    {(const char*)"flow_covers_cuts",new CglFlowCover,(const char*) "Flow cover cuts", CUTINFO_NONE},
    {NULL, NULL, NULL, CUTINFO_NONE}};

  int freq;

  for (int i=0; cutList [i]. optname; i++) {

    options_ -> GetIntegerValue (std::string (cutList [i]. optname), freq, "bonmin.");

    if (!freq) {
    	delete cutList [i].cglptr;
    	continue;
    }

    CuttingMethod cg;
    cg.frequency = freq;
    cg.cgl       = cutList [i].cglptr;
    cg.id        = std::string (cutList [i]. cglId);
    cutGenerators_.push_back (cg);

    // options for particular cases
    switch (cutList [i].extraInfo) {

    case CUTINFO_MIG: {
    	CglGomory *gc = dynamic_cast <CglGomory *> (cutList [i].cglptr);

    	if (!gc) break;

    	gc -> setLimitAtRoot(512);
    	gc -> setLimit(50);
    }
    break;

    case CUTINFO_PROBING: {
    	CglProbing *pc = dynamic_cast <CglProbing *> (cutList [i].cglptr);

    	if (!pc) break;

    	pc->setUsingObjective(1);
    	pc->setMaxPass(3);
    	pc->setMaxPassRoot(3);
    	// Number of unsatisfied variables to look at
    	pc->setMaxProbe(10);
    	pc->setMaxProbeRoot(50);
    	// How far to follow the consequences
    	pc->setMaxLook(10);
    	pc->setMaxLookRoot(50);
    	pc->setMaxLookRoot(10);
    	// Only look at rows with fewer than this number of elements
    	pc->setMaxElements(200);
    	pc->setRowCuts(3);
    }
    break;

    case CUTINFO_CLIQUE: {
    	CglClique *clique = dynamic_cast <CglClique *> (cutList [i].cglptr);

    	if (!clique) break;

    	clique -> setStarCliqueReport(false);
    	clique -> setRowCliqueReport(false);
    	clique -> setMinViolation(0.1);
    }
    break;

//case CUTINFO_NONE:
    default:
    	break;
    }
  }
}
