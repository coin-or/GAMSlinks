// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// this is an adapted version of BonCouenneSetup to use GAMS instead of ASL

#include "GamsCouenneSetup.hpp"

// GAMS
#ifdef GAMS_BUILD
#include "gmomcc.h"
#else
#include "gmocc.h"
#endif
// gmo might include windows.h
#ifdef CONST
#undef CONST
#endif

#include "BonInitHeuristic.hpp"
#include "BonNlpHeuristic.hpp"
#include "BonCouenneInterface.hpp"

#include "BonGuessHeuristic.hpp"
#include "CbcCompareActual.hpp"

#include "CouenneProblem.hpp"
#include "CouenneObject.hpp"
#include "CouenneVarObject.hpp"
#include "CouenneVTObject.hpp"
#include "CouenneChooseVariable.hpp"
#include "CouenneChooseStrong.hpp"
#include "CouenneSolverInterface.hpp"
#include "CouenneCutGenerator.hpp"
#include "BonCouenneInfo.hpp"
#include "BonCbcNode.hpp"

#include "CouenneTypes.hpp"
#include "exprClone.hpp"
#include "exprGroup.hpp"
#include "exprAbs.hpp"
#include "exprConst.hpp"
#include "exprCos.hpp"
#include "exprDiv.hpp"
#include "exprExp.hpp"
#include "exprInv.hpp"
#include "exprLog.hpp"
#include "exprMax.hpp"
#include "exprMin.hpp"
#include "exprMul.hpp"
#include "exprOpp.hpp"
#include "exprPow.hpp"
#include "exprSin.hpp"
#include "exprSub.hpp"
#include "exprSum.hpp"
#include "exprVar.hpp"

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
#include "GamsJournal.hpp"
#include "GamsMessageHandler.hpp"
#include "GamsNLinstr.h"

using namespace Bonmin;
using namespace Ipopt;
using namespace std;


GamsCouenneSetup::~GamsCouenneSetup(){
  //if (CouennePtr_)
  //delete CouennePtr_;
}

bool GamsCouenneSetup::InitializeCouenne(SmartPtr<GamsMINLP> minlp) {
	SmartPtr<Journalist> journalist_ = new Journalist();
 	SmartPtr<Journal> jrnl = new GamsJournal(gmo, "console", J_ITERSUMMARY, J_STRONGWARNING);
	jrnl->SetPrintLevel(J_DBG, J_NONE);
	if (!journalist_->AddJournal(jrnl))
		gmoLogStat(gmo, "Failed to register GamsJournal for IPOPT output.");
	
	// instead of initializeOptionsAndJournalist we do it our own way, so we can use the SmagJournal
  SmartPtr<OptionsList> options_   = new OptionsList();
  SmartPtr<Bonmin::RegisteredOptions> roptions_ = new Bonmin::RegisteredOptions();
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
  options()->SetStringValue("delete_redundant", "no", "couenne.");

	if (gmoNLM(gmo) == 0  && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp || gmoModelType(gmo) == Proc_miqcp))
		options()->SetStringValue("hessian_constant", "yes", true, true); 
	
	try {
		if (gmoOptFile(gmo)) {
			options()->SetStringValue("print_user_options", "yes", true, true);
			char buffer[1024];
			gmoNameOptFile(gmo, buffer);
			BabSetupBase::readOptionsFile(buffer);
		}
	} catch (IpoptException error) {
		gmoLogStat(gmo, error.Message().c_str());
	  return false;
	} catch (std::bad_alloc) {
		gmoLogStat(gmo, "Error: Not enough memory\n");
		return false;
	} catch (...) {
		gmoLogStat(gmo, "Error: Unknown exception thrown.\n");
		return false;
	}

	std::string libpath;
#ifdef HAVE_HSL_LOADER
	if (options()->GetStringValue("hsl_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadHSL(libpath.c_str(), buffer, 512) != 0) {
			gmoLogStatPChar(gmo, "Failed to load HSL library at user specified path: ");
			gmoLogStat(gmo, buffer);
		  return false;
		}
	}
#endif
#ifndef HAVE_PARDISO
	if (options()->GetStringValue("pardiso_library", libpath, "")) {
		char buffer[512];
		if (LSL_loadPardisoLib(libpath.c_str(), buffer, 512) != 0) {
			gmoLogStatPChar(gmo, "Failed to load Pardiso library at user specified path: ");
			gmoLogStat(gmo, buffer);
		  return false;
		}
	}
#endif

	options()->GetNumericValue("diverging_iterates_tol", minlp->nlp->div_iter_tol, "");
//	// or should we also check the tolerance for acceptable points?
//	bonmin_setup.options()->GetNumericValue("tol", mysmagminlp->scaled_conviol_tol, "");
//	bonmin_setup.options()->GetNumericValue("constr_viol_tol", mysmagminlp->unscaled_conviol_tol, "");

	std::string s;
	options()->GetStringValue("hessian_approximation", s, "");
	if (s == "exact") {
		int do2dir, dohess;
		if (gmoHessLoad(gmo, 10, 10, &do2dir, &dohess) || !dohess) { // TODO make 10 (=conopt default for rvhess) a parameter
			gmoLogStat(gmo, "Failed to initialize Hessian structure. We continue with a limited-memory Hessian approximation!");
			options()->SetStringValue("hessian_approximation", "limited-memory");
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
  continuousSolver_->passInMessageHandler(msghandler);

  nonlinearSolver_ = ci;
  
  /* Initialize Couenne cut generator.*/
  //int ivalue, num_points;
  //options()->GetEnumValue("convexification_type", ivalue,"bonmin.");
  //options()->GetIntegerValue("convexification_points",num_points,"bonmin.");
  
  // CouenneCutGenerator creates an empty problem here, and there seem to be no way to set the problem afterwards
  // so we pick the empty problem up and fill it with the gams model 
  CouenneCutGenerator* couenneCg = new CouenneCutGenerator (ci, this, NULL, journalist());
  
  CouenneProblem * couenneProb = couenneCg->Problem();
  if (!setupProblem(couenneProb)) {
  	gmoLogStat(gmo, "Error setting up problem for Couenne.\n");
  	return false;
  }

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

  couenneProb->setMaxCpuTime(getDoubleParameter(BabSetupBase::MaxTime));
  
  ci->extractLinearRelaxation(*continuousSolver_, *couenneCg);

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

  if (extraStuff->infeasibleNode()) {
  	gmoLogStat(gmo, "Initial linear relaxation constructed by Couenne is infeasible.");
		gmoSolveStatSet(gmo, SolveStat_Normal);
		gmoModelStatSet(gmo, ModelStat_InfeasibleNoSolution);
    return false;
  }

  //continuousSolver_ -> findIntegersAndSOS (false);
  //addSos (); // only adds embedded SOS objects

  // Add Couenne SOS ///////////////////////////////////////////////////////////////

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
    			((var -> Type  () == AUX) && (var -> Image () -> Linearity () > LINEAR))) {
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
  if (couenneProb -> nIntVars () > 0)
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
    -2, J_LAST_LEVEL-1, J_STRONGWARNING,
    "");
  roptions->AddBoundedIntegerOption(
    "nlpheur_print_level",
    "Output level for NLP heuristic in Couenne",
    -2, J_LAST_LEVEL-1, J_STRONGWARNING,
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
//TODO    cg.cgl->passInMessageHandler(...);
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


bool GamsCouenneSetup::setupProblem(CouenneProblem* prob) {
	prob->AuxSet() = new std::set<exprAux*, compExpr>;
	
	//add variables
	for (int i = 0; i < gmoN(gmo); ++i) {
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
				prob->addVariable(false, prob->domain());
				break;
			case var_B:
			case var_I:
				prob->addVariable(true, prob->domain());
				break;
			case var_S1:
			case var_S2:
		  	//TODO prob->addVariable(false, prob->domain());
		  	gmoLogStat(gmo, "Special ordered sets not supported by Gams/Couenne link yet.");
		  	return false;
			case var_SC:
			case var_SI:
		  	gmoLogStat(gmo, "Semicontinuous and semiinteger variables not supported by Couenne.");
		  	return false;
			default:
		  	gmoLogStat(gmo, "Unknown variable type.");
		  	return false;
		}
	}
	
	// add variable bounds and initial values
	CouNumber* x_ = new CouNumber[gmoN(gmo)];
	CouNumber* lb = new CouNumber[gmoN(gmo)];
	CouNumber* ub = new CouNumber[gmoN(gmo)];
	
	gmoGetVarL(gmo, x_);
	gmoGetVarLower(gmo, lb);
	gmoGetVarUpper(gmo, ub);
	
	prob->domain()->push(gmoN(gmo), x_, lb, ub);
	
	delete[] x_;
	delete[] lb;
	delete[] ub;
	

	int* opcodes = new int[gmoMaxSingleFNL(gmo)+1];
	int* fields  = new int[gmoMaxSingleFNL(gmo)+1];
	int constantlen = gmoNLConst(gmo);
	double* constants = (double*)gmoPPool(gmo);
	int codelen;

	exprGroup::lincoeff lin;
	expression *body = NULL;

	// add objective function: first linear part, then nonlinear
	double isMin = (gmoSense(gmo) == Obj_Min) ? 1 : -1;
	
	lin.reserve(gmoObjNZ(gmo) - gmoObjNLNZ(gmo));
	
	int* colidx = new int[gmoObjNZ(gmo)];
	double* val = new double[gmoObjNZ(gmo)];
	int* nlflag = new int[gmoObjNZ(gmo)];
	int* dummy  = new int[gmoObjNZ(gmo)];
	
	if (gmoObjNZ(gmo)) nlflag[0] = 0; // workaround for gmo bug
	gmoGetObjSparse(gmo, colidx, val, nlflag, dummy, dummy);
	for (int i = 0; i < gmoObjNZ(gmo); ++i) {
		if (nlflag[i])
			continue;
		lin.push_back(pair<exprVar*, CouNumber>(prob->Var(colidx[i]), isMin*val[i]));
	}
  assert(lin.size() == gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

	delete[] colidx;
	delete[] val;
	delete[] nlflag;
	delete[] dummy;
		
	if (gmoObjNLNZ(gmo)) {
		gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);
		
		expression** nl = new expression*[1];
		nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
		if (!nl[0])
			return false;

		double objjacval = isMin*gmoObjJacVal(gmo);
		//std::clog << "obj jac val: " << objjacval << std::endl;
		if (objjacval == 1.) { // scale by -1/objjacval = negate
			nl[0] = new exprOpp(nl[0]);
		} else if (objjacval != -1.) { // scale by -1/objjacval
			nl[0] = new exprMul(nl[0], new exprConst(-1/objjacval));
		}
		
		body = new exprGroup(isMin*gmoObjConst(gmo), lin, nl, 1);
	} else {
		body = new exprGroup(isMin*gmoObjConst(gmo), lin, NULL, 0);			
	}
	
	prob->addObjective(body, "min");
	
	int nz = gmoNZ(gmo);
	double* values  = new double[nz];
	int* rowstarts  = new int[gmoM(gmo)+1];
	int* colindexes = new int[nz];
	int* nlflags    = new int[nz];
	
	gmoGetMatrixRow(gmo, rowstarts, colindexes, values, nlflags);
	rowstarts[gmoM(gmo)] = nz;

	for (int i = 0; i < gmoM(gmo); ++i) {
		lin.clear();
		lin.reserve(rowstarts[i+1] - rowstarts[i]);
		for (int j = rowstarts[i]; j < rowstarts[i+1]; ++j) {
			if (nlflags[j])
				continue;
			lin.push_back(pair<exprVar*, CouNumber>(prob->Var(colindexes[j]), values[j]));
		}
		if (gmoNLfunc(gmo, i)) {
			gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
			expression** nl = new expression*[1];
			nl[0] = parseGamsInstructions(prob, codelen, opcodes, fields, constantlen, constants);
			if (!nl[0])
				return false;
			body = new exprGroup(0., lin, nl, 1);
		} else {
			body = new exprGroup(0., lin, NULL, 0);			
		}
		
		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E:
				prob->addEQConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_L:
				prob->addLEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_G:
				prob->addGEConstraint(body, new exprConst(gmoGetRhsOne(gmo, i)));
				break;
			case equ_N:
				// TODO I doubt that adding a RNG constraint with -infty/infty bounds would work here
				gmoLogStat(gmo, "Free constraints not supported by Gams/Couenne link yet.");
				break;
		}
	}
	
	delete[] opcodes;
	delete[] fields;
	delete[] values;
	delete[] rowstarts;
	delete[] colindexes;
	delete[] nlflags;
	

	// the rest is copied from the CouenneProblem constructor
	
	double now = CoinCpuTime();

  // link initial variables to problem's domain
  for (std::vector<exprVar*>::iterator it = prob->Variables().begin(); it != prob->Variables().end(); ++it)
    (*it)->linkDomain(prob->domain());

  if (prob->Jnlst()->ProduceOutput(Ipopt::J_SUMMARY, J_PROBLEM))
    prob->print();

  // save -- for statistic purposes (and used indeed only there) -- number of original
  // constraints. Some of them will be deleted as definition of
  // auxiliary variables.
//TODO  nOrigCons_    = constraints_.size();
//TODO  nOrigIntVars_ = nIntVars ();

//  prob->Jnlst()->Printf(Ipopt::J_SUMMARY, J_PROBLEM,
//		  "Problem size before reformulation: %d variables (%d integer), %d constraints.\n",
//		  prob->nOrigVars(), prob->nOrigIntVars(), prob->nOrigCons());

  std::string s;
//TODO useQuadratic_ is protected...
//    options() -> GetStringValue ("use_quadratic", s, "couenne."); 
//    useQuadratic_ = (s == "yes");

  // reformulation
  prob->standardize();

  // clear all spurious variables pointers not referring to the variables_ vector
  // prob->realign();
  // the following three loops are copied from the protected function CouenneProblem::realign():
  // link variables to problem's domain
  for (std::vector <exprVar *>::iterator i = prob->Variables().begin(); i != prob->Variables().end(); ++i) {
    (*i)->linkDomain (prob->domain());
    (*i)->realign (prob);
    if ((*i)->Type() == AUX)
      (*i)->Image()->realign(prob);
  }
  // link variables to problem's domain
  for (int i = 0; i < prob->nObjs(); ++i) 
    prob->Obj(i)->Body()->realign(prob);

  // link variables to problem's domain
  for (int i = 0; i < prob->nCons(); ++i) 
    prob->Con(i)->Body()->realign(prob);

  // fill dependence_ structure
  //  prob->fillDependence(this);
  std::vector<std::set<int> >& dependence_(*const_cast<std::vector<std::set<int> >*>(&prob->Dependence()));
  std::vector<CouenneObject>& objects_(*const_cast<std::vector<CouenneObject>*>(&prob->Objects()));
  // the following code is copied from the protected function CouenneProblem::fillDependence():
  for (int i = prob->nVars(); i--; )
    dependence_.push_back(std::set<int>());

  // empty object to fill space for linear-defined auxiliaries and for
  // originals
  CouenneObject nullObject;

  for (std::vector <exprVar *>::iterator i = prob->Variables().begin(); i != prob->Variables().end(); ++i) {
    if (((*i) -> Type () == AUX)                           // consider aux's only
    		&& ((*i) -> Image () -> Linearity () > LINEAR)) {  // and nonlinear
      // add object for this variable
      objects_.push_back(CouenneObject(prob, *i, this, journalist()));

      std::set<int> deplist;

      // fill the set of independent variables on which the expression
      // associated with *i depends; if empty (should not happen...), skip
      if ((*i)->Image()->DepList(deplist, STOP_AT_AUX) == 0)
      	continue;

      // build dependence set for this variable
      for (std::set<int>::iterator j = deplist.begin (); j != deplist.end (); ++j) {
      	std::set <int> &obj = dependence_[*j];
      	int ind = (*i)->Index();
      	if (obj.find(ind) == obj.end())
      		obj.insert(ind);
      }
    } else objects_.push_back (nullObject); // null object for original and linear auxiliaries
  }

  // quadratic handling
  prob->fillQuadIndices();

  if ((now = (CoinCpuTime () - now)) > 10.)
    prob->Jnlst()->Printf(Ipopt::J_WARNING, J_PROBLEM, "Couenne: reformulation time %.3fs\n", now);

  // give a value to all auxiliary variables
  prob->initAuxs();

  int nActualVars = 0;

//TODO no one seem to need use nIntVars(), so we just skip it and hide it with local variable
  // later, the "int" need to be removed
  int nIntVars_ = 0;

  // check how many integer variables we have now (including aux)
  for (int i = 0; i < prob->nVars(); ++i)
    if (prob->Variables()[i]->Multiplicity() > 0) {
      nActualVars++;
      if (prob->Variables()[i]->isDefinedInteger())
      	nIntVars_++;
    }

  prob->Jnlst()->Printf(Ipopt::J_SUMMARY, J_PROBLEM, "Problem size after reformulation: %d variables (%d integer), %d constraints.\n", nActualVars, prob->nIntVars(), prob->nCons());

//TODO
//  options() -> GetStringValue ("feasibility_bt",  s, "couenne."); doFBBT_ = (s == "yes");
//  options() -> GetStringValue ("optimality_bt",   s, "couenne."); doOBBT_ = (s == "yes");
//  options() -> GetStringValue ("aggressive_fbbt", s, "couenne."); doABT_  = (s == "yes");
//  options() -> GetIntegerValue ("log_num_obbt_per_level", logObbtLev_, "couenne.");
//  options() -> GetIntegerValue ("log_num_abt_per_level",  logAbtLev_,  "couenne.");

  CouNumber 
    art_cutoff =  COIN_DBL_MAX,
    art_lower  = -COIN_DBL_MAX;

//TODO options() -> GetNumericValue ("feas_tolerance",  feas_tolerance_, "couenne.");
  //options() -> GetNumericValue ("opt_window",      opt_window_,     "couenne.");
  options() -> GetNumericValue ("art_cutoff",      art_cutoff,      "couenne.");
  options() -> GetNumericValue ("art_lower",       art_lower,       "couenne.");

  if (art_cutoff <  1.e50)
  	prob->setCutOff(art_cutoff);
  if (art_lower  > -1.e50) {
    int indobj = prob->Obj(0)->Body()->Index();
    if (indobj >= 0)
    	prob->domain()->lb(indobj) = art_lower;
  }

  // check if optimal solution is available (for debug purposes)
  prob->readOptimum();

  if (prob->Jnlst()->ProduceOutput(Ipopt::J_DETAILED, J_PROBLEM))
    prob->print();

  prob->createUnusedOriginals();

	return true;
}

expression* GamsCouenneSetup::parseGamsInstructions(CouenneProblem* prob, int codelen, int* opcodes, int* fields, int constantlen, double* constants) {
	const bool debugoutput = false;

	list<expression*> stack;
	
	int nargs = -1;

	for (int pos = 0; pos < codelen; ++pos)
	{	
		GamsOpCode opcode = (GamsOpCode)opcodes[pos];
		int address = fields[pos]-1;

		if (debugoutput) std::clog << '\t' << GamsOpCodeName[opcode] << ": ";
		
		expression* exp = NULL;
		
		switch(opcode) {
			case nlNoOp : { // no operation
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushV : { // push variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "push variable " << address << std::endl;
				exp = new exprClone(prob->Variables()[address]);
			} break;
			case nlPushI : { // push constant
				if (debugoutput) std::clog << "push constant " << constants[address] << std::endl;
				exp = new exprConst(constants[address]);
			} break;
			case nlStore: { // store row
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlAdd : { // add
				if (debugoutput) std::clog << "add" << std::endl;
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				exp = new exprSum(term1, term2);
			} break;
			case nlAddV: { // add variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "add variable " << address << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				exp = new exprSum(term1, term2);
			} break;
			case nlAddI: { // add immediate
				if (debugoutput) std::clog << "add constant " << constants[address] << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprSum(term1, term2);
			} break;
			case nlSub: { // minus
				if (debugoutput) std::clog << "minus" << std::endl;
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				exp = new exprSub(term2, term1);
			} break;
			case nlSubV: { // subtract variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "substract variable " << address << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				exp = new exprSub(term1, term2);
			} break;
			case nlSubI: { // subtract immediate
				if (debugoutput) std::clog << "substract constant " << constants[address] << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprSub(term1, term2);
			} break;
			case nlMul: { // multiply
				if (debugoutput) std::clog << "multiply" << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				exp = new exprMul(term1, term2);
			} break;
			case nlMulV: { // multiply variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "multiply variable " << address << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				exp = new exprMul(term1, term2);
			} break;
			case nlMulI: { // multiply immediate
				if (debugoutput) std::clog << "multiply constant " << constants[address] << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprMul(term1, term2);
			} break;
			case nlDiv: { // divide
				if (debugoutput) std::clog << "divide" << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = stack.back(); stack.pop_back();
				if (term2->Type() == CONST)
					exp = new exprMul(term2, new exprInv(term1));
				else
					exp = new exprDiv(term2, term1);
			} break;
			case nlDivV: { // divide variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "divide variable " << address << std::endl;
				
				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprClone(prob->Variables()[address]);
				if (term1->Type() == CONST)
					exp = new exprMul(term1, new exprInv(term2));
				else
					exp = new exprDiv(term1, term2);
			} break;
			case nlDivI: { // divide immediate
				if (debugoutput) std::clog << "divide constant " << constants[address] << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				expression* term2 = new exprConst(constants[address]);
				exp = new exprDiv(term1, term2);
			} break;
			case nlUMin: { // unary minus
				if (debugoutput) std::clog << "negate" << std::endl;
				
				expression* term = stack.back(); stack.pop_back();
				exp = new exprOpp(term);
			} break;
			case nlUMinV: { // unary minus variable
				address = gmoGetjSolver(gmo, address);
				if (debugoutput) std::clog << "push negated variable " << address << std::endl;

				exp = new exprOpp(new exprClone(prob->Variables()[address]));
			} break;
			case nlCallArg1 :
			case nlCallArg2 :
			case nlCallArgN : {
				if (debugoutput) std::clog << "call function ";
				GamsFuncCode func = GamsFuncCode(address+1); // here the shift by one was not a good idea
				
				switch (func) {
					case fnmin : {
						if (debugoutput) std::clog << "min" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						exp = new exprMin(term1, term2);
					} break;
					case fnmax : {
						if (debugoutput) std::clog << "max" << std::endl;

						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						exp = new exprMax(term1, term2);
					} break;
					case fnsqr : {
						if (debugoutput) std::clog << "square" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprPow(term, new exprConst(2.));
					} break;
					case fnexp:
					case fnslexp:
					case fnsqexp: {
						if (debugoutput) std::clog << "exp" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprExp(term);
					} break;
					case fnlog : {
						if (debugoutput) std::clog << "ln" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprLog(term);
					} break;
					case fnlog10:
					case fnsllog10:
					case fnsqlog10: {
						if (debugoutput) std::clog << "log10 = ln * 1/ln(10)" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprMul(new exprLog(term), new exprConst(1./log(10.)));
					} break;
					case fnlog2 : {
						if (debugoutput) std::clog << "log2 = ln * 1/ln(2)" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprMul(new exprLog(term), new exprConst(1./log(2.)));
					} break;
					case fnsqrt: {
						if (debugoutput) std::clog << "sqrt" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprPow(term, new exprConst(.5));
					} break;
					case fnabs: {
						if (debugoutput) std::clog << "abs" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprAbs(term);
					} break;
					case fncos: {
						if (debugoutput) std::clog << "cos" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprCos(term);
					} break;
					case fnsin: {
						if (debugoutput) std::clog << "sin" << std::endl;

						expression* term = stack.back(); stack.pop_back();
						exp = new exprSin(term);
					} break;
					case fnpower:
					case fnrpower: { // x ^ y
						if (debugoutput) std::clog << "power" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						if (term1->Type() == CONST)
							exp = new exprPow(term2, term1);
						else
							exp = new exprExp(new exprMul(new exprLog(term2), term1));
					} break;
					case fncvpower: { // constant ^ x
						if (debugoutput) std::clog << "power" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();

						assert(term2->Type() == CONST);
						exp = new exprExp(new exprMul(new exprConst(log(((exprConst*)term2)->Value())), term1));
						delete term2;
					} break;
					case fnvcpower: { // x ^ constant
						if (debugoutput) std::clog << "power" << std::endl;
						
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						assert(term1->Type() == CONST);
						exp = new exprPow(term2, term1);
					} break;
					case fnpi: {
						if (debugoutput) std::clog << "pi" << std::endl;
						//TODO
						assert(false);
					} break;
					case fndiv:
					case fndiv0: {
						expression* term1 = stack.back(); stack.pop_back();
						expression* term2 = stack.back(); stack.pop_back();
						if (term2->Type() == CONST)
							exp = new exprMul(term2, new exprInv(term1));
						else
							exp = new exprDiv(term2, term1);
					} break;
					case fnslrec: // 1/x
					case fnsqrec: { // 1/x
						if (debugoutput) std::clog << "divide" << std::endl;
						
						expression* term = stack.back(); stack.pop_back();
						exp = new exprInv(term);
					} break;
			    case fnpoly: { /* simple polynomial */
						if (debugoutput) std::clog << "polynom of degree " << nargs-1 << std::endl;
						assert(nargs >= 0);
						switch (nargs) {
							case 0:
								delete stack.back(); stack.pop_back(); // delete variable of polynom
								exp = new exprConst(0.);
								break;
							case 1: // "constant" polynom
								exp = stack.back(); stack.pop_back();
								delete stack.back(); stack.pop_back(); // delete variable of polynom
								break;
							default: { // polynom is at least linear
								std::vector<expression*> coeff(nargs);
								while (nargs) {
									assert(!stack.empty());
									coeff[nargs-1] = stack.back();
									stack.pop_back();
									--nargs;
								}
								assert(!stack.empty());
								expression* var = stack.back(); stack.pop_back();
								expression** monoms = new expression*[coeff.size()];
								monoms[0] = coeff[0];
								monoms[1] = new exprMul(coeff[1], var);
								for (size_t i = 2; i < coeff.size(); ++i)
									monoms[i] = new exprMul(coeff[i], new exprPow(new exprClone(var), new exprConst(i)));
								exp = new exprSum(monoms, coeff.size());
							}
						}
						nargs = -1;
			    } break;
					case fnceil: case fnfloor: case fnround:
					case fnmod: case fntrunc: case fnsign:
					case fnarctan: case fnerrf: case fndunfm:
					case fndnorm: case fnerror: case fnfrac: case fnerrorl:
			    case fnfact /* factorial */: 
			    case fnunfmi /* uniform random number */:
			    case fnncpf /* fischer: sqrt(x1^2+x2^2+2*x3) */:
			    case fnncpcm /* chen-mangasarian: x1-x3*ln(1+exp((x1-x2)/x3))*/:
			    case fnentropy /* x*ln(x) */: case fnsigmoid /* 1/(1+exp(-x)) */:
			    case fnboolnot: case fnbooland:
			    case fnboolor: case fnboolxor: case fnboolimp:
			    case fnbooleqv: case fnrelopeq: case fnrelopgt:
			    case fnrelopge: case fnreloplt: case fnrelople:
			    case fnrelopne: case fnifthen:
			    case fnedist /* euclidian distance */:
			    case fncentropy /* x*ln((x+d)/(y+d))*/:
			    case fngamma: case fnloggamma: case fnbeta:
			    case fnlogbeta: case fngammareg: case fnbetareg:
			    case fnsinh: case fncosh: case fntanh:
			    case fnsignpower /* sign(x)*abs(x)^c */:
			    case fnncpvusin /* veelken-ulbrich */:
			    case fnncpvupow /* veelken-ulbrich */:
			    case fnbinomial:
			    case fntan: case fnarccos:
			    case fnarcsin: case fnarctan2 /* arctan(x2/x1) */:
					default : {
						char buffer[256];
						sprintf(buffer, "Gams function code %d not supported.", func);
						gmoLogStat(gmo, buffer);
						return NULL;
					}
				}
			} break;
			case nlMulIAdd: {
				if (debugoutput) std::clog << "multiply constant " << constants[address] << " and add " << std::endl;

				expression* term1 = stack.back(); stack.pop_back();
				term1 = new exprMul(term1, new exprConst(constants[address]));
				expression* term2 = stack.back(); stack.pop_back();
				
				exp = new exprSum(term1, term2);				
			} break;
			case nlFuncArgN : {
				nargs = address;
				if (debugoutput) std::clog << nargs << " arguments" << std::endl;
			} break;
			case nlArg: {
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlHeader: { // header
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushZero: {
				if (debugoutput) std::clog << "push constant zero" << std::endl;
				exp = new exprConst(0.);
			} break;
			case nlStoreS: { // store scaled row
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			// the following three should have been taken out by reorderInstr above; the remaining ones seem to be unused by now
			case nlPushS: // duplicate value from address levels down on top of stack
			case nlPopup: // duplicate value from this level to at address levels down and pop entries in between
			case nlSwap: // swap two positions on top of stack
			case nlAddL: // add local
			case nlSubL: // subtract local
			case nlMulL: // multiply local
			case nlDivL: // divide local
			case nlPushL: // push local
			case nlPopL: // pop local
			case nlPopDeriv: // pop derivative
			case nlUMinL: // push umin local
			case nlPopDerivS: // store scaled gradient
			case nlEquScale: // equation scale
			case nlEnd: // end of instruction list
			default: {
				char buffer[256];
				sprintf(buffer, "Gams instruction %d not supported.", opcode);
				gmoLogStat(gmo, buffer);
				return NULL;
			}
		}
		
		if (exp)
			stack.push_back(exp);
	}

	assert(stack.size() == 1);	
	return stack.back();
}
