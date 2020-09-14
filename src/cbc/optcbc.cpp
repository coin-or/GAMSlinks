/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optcbc.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

#include "CbcOrClpParam.hpp"
#include "OsiClpSolverInterface.hpp"
#include "CbcModel.hpp"
#include "CbcSolver.hpp"

static
bool hasCbcOption(
   std::vector<CbcOrClpParam>& cbcopts,
   const std::string&          namecbc
   )
{
   for( auto& o : cbcopts )
      if( o.name() == namecbc )
         return true;
   return false;
}

static
GamsOption& collectCbcOption(
   GamsOptions&                gmsopt,
   std::vector<CbcOrClpParam>& cbcopts,
   CbcModel&                   cbcmodel,
   const std::string&          namegams,
   const std::string&          namecbc_ = ""
   )
{
   std::string namecbc(namecbc_.empty() ? namegams : namecbc_);

   unsigned int idx;
   for( idx = 0; idx < cbcopts.size(); ++idx )
      if( cbcopts[idx].name() == namecbc )
         break;
   if( idx >= cbcopts.size() )
   {
      std::cerr << "Error: Option " << namecbc << " not known to Cbc." << std::endl;
      exit(1);
   }

   const CbcOrClpParam& cbcopt(cbcopts[idx]);

   GamsOption::Type opttype;
   GamsOption::Value defaultval, minval, maxval;
   GamsOption::EnumVals enumval;
   std::string tmpstr;
   std::string longdescr;

   /*   1 -- 100  double parameters
    * 101 -- 200  integer parameters
    * 201 -- 300  Clp string parameters
    * 301 -- 400  Cbc string parameters
    * 401 -- 500  Clp actions
    * 501 -- 600  Cbc actions
   */
   CbcOrClpParameterType cbcoptnum = cbcopt.type();
   if( cbcoptnum <= 100 )
   {
      opttype = GamsOption::Type::REAL;
      minval = cbcopt.lowerDoubleValue();
      maxval = cbcopt.upperDoubleValue();
      defaultval = cbcopt.doubleParameter(cbcmodel);
   }
   else if( cbcoptnum <= 200 )
   {
      opttype = GamsOption::Type::INTEGER;
      minval = cbcopt.lowerIntValue();
      maxval = cbcopt.upperIntValue();
      defaultval = cbcopt.intParameter(cbcmodel);
   }
   else if( cbcoptnum <= 400 )
   {
      // check whether this might be a bool option
      const std::vector<std::string>& kws(cbcopt.definedKeywords());
      if( kws.size() == 2 &&
         ((kws[0] == "on" && kws[1] == "off") || (kws[1] == "on" && kws[0] == "off")) )
      {
         opttype = GamsOption::Type::BOOL;
         defaultval = cbcopt.currentOption() == "on";
      }
      else
      {
         opttype = GamsOption::Type::STRING;

         std::string def = cbcopt.currentOption();
         // remove '!' and '?' marker from default
         auto newend = std::remove(def.begin(), def.end(), '!');
         newend = std::remove(def.begin(), newend, '?');
         defaultval = std::string(def.begin(), newend);

         for( auto v : cbcopt.definedKeywords() )
         {
            // remove '!' and '?' marker from keyword
            newend = std::remove(v.begin(), v.end(), '!');
            newend = std::remove(v.begin(), newend, '?');
            v = std::string(v.begin(), newend);

            enumval.append(v);

            if( v == "01first" )
               enumval.append("binaryfirst", "This is a deprecated setting. Please use 01first.");
            else if( v == "01last" )
               enumval.append("binarylast", "This is a deprecated setting. Please use 01last.");
         }
      }
   }
   else
   {
      std::cerr << "ERROR: 'action'-option encountered: " << namegams << std::endl;
      exit(1);
   }

   GamsOption& opt(gmsopt.collect(namegams, cbcopt.shortHelp(), cbcopt.longHelp(), opttype, defaultval, minval, maxval, true, true, enumval, "", cbcoptnum));
   if( namegams != namecbc )
      opt.synonyms.insert(namecbc);

   // remove parameter from array, so we can later easily check which options we haven't taken
   cbcopts[idx] = CbcOrClpParam();

   return opt;
}

static
void add01(
   GamsOption::EnumVals& enumval)
{
   enumval.append("0", "Same as off. This is a deprecated setting.");
   enumval.append("1", "Same as on. This is a deprecated setting.");
}

int main(int argc, char** argv)
{
   // to read the defaults for some options, we need a CbcModel; let's even initialize it with an LP
   // calling CbcMain0 with a CbcSolverUsefulData seems to make Cbc store its defaults in the parameters in there
   OsiClpSolverInterface solver;
   CbcModel cbcmodel(solver);
   CbcSolverUsefulData cbcusefuldata;
   CbcMain0(cbcmodel, cbcusefuldata);
   std::vector<CbcOrClpParam>& cbcopts = cbcusefuldata.parameters_;
   GamsOption* opt;

   // collection of GAMS/Cbc parameters
   GamsOptions gmsopt("CBC");

   // General parameters
   gmsopt.setGroup("General Options");

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "reslim", "seconds");
   opt->defaultval.realval = 1e10;
   opt->defaultdescr = "GAMS reslim";
   opt->longdescr.clear();

   GamsOption::EnumVals clocktypes;
   clocktypes.append("cpu", "CPU clock");
   clocktypes.append("wall", "Wall clock");
   gmsopt.collect("clocktype", "type of clock for time measurement", "",
      "wall", clocktypes, "", -1);

   gmsopt.collect("special", "options passed unseen to CBC",
      "This parameter let you specify CBC options which are not supported by the GAMS/CBC interface. "
      "The string value given to this parameter is split up into parts at each space and added to the array of parameters given to CBC (in front of the -solve command). "
      "Hence, you can use it like the command line parameters for the CBC standalone version.",
      "", -1);

   gmsopt.collect("writemps", "create MPS file for problem",
      "Write the problem formulation in MPS format. "
      "The parameter value is the name of the MPS file.",
      "", -1);

   // LP parameters
   gmsopt.setGroup("LP Options");

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "iterlim", "maxIterations");
   opt->longdescr = "For an LP, this is the maximum number of iterations to solve the LP. For a MIP, this option is ignored.";
   opt->defaultval.intval = INT_MAX;
   opt->defaultdescr = "GAMS iterlim";

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "idiotCrash");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "sprintCrash").synonyms.insert("sifting");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "crash");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "factorization");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "denseThreshold");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "smallFactorization");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "sparseFactor");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "biasLU");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "maxFactor");

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "crossover");
   // value "maybe" is only relevant for quadratic: remove value and mention in longdescr
   opt->enumval.drop("maybe");
   opt->longdescr = "Interior point algorithms do not obtain a basic solution. "
      "This option will crossover to a basic solution suitable for ranging or branch and cut.";
   add01(opt->enumval);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "dualPivot").enumval.append("auto", "Same as automatic. This is a deprecated setting.");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "primalPivot").enumval.append("auto", "Same as automatic. This is a deprecated setting.");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "psi");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "perturbation");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "scaling").enumval.append("auto", "Same as automatic. This is a deprecated setting.");

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "presolve");
   opt->enumval.drop("file");
   add01(opt->enumval);
   opt->longdescr = "Presolve analyzes the model to find such things as redundant equations, "
      "equations which fix some variables, equations which can be transformed into bounds, etc. "
      "For the initial solve of any problem this is worth doing unless one knows that it will have no effect. "
      "Option 'on' will normally do 5 passes, while using 'more' will do 10.";

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "passPresolve");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "substitution");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "randomSeedClp", "randomSeed");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "tol_primal", "primalTolerance");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "tol_dual", "dualTolerance");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "tol_presolve", "preTolerance");

   GamsOption::EnumVals startalgs;
   startalgs.append("primal", "Primal Simplex algorithm");
   startalgs.append("dual", "Dual Simplex algorithm");
   startalgs.append("barrier", "Primal-dual predictor-corrector algorithm");
   gmsopt.collect("startalg", "LP solver for root node",
      "Determines the algorithm to use for an LP or the initial LP relaxation if the problem is a MIP.",
      "dual", startalgs, "", -1);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "primalWeight");

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "autoScale");
   opt->shortdescr += " (experimental)";
   opt->longdescr.clear();

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "bscale");

   //TODO collectCbcOption(gmsopt, cbcopts, cbcmodel, "cholesky");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "gamma", "gamma(Delta)").synonyms.clear();  // GAMS options object doesn't like parenthesis in synonym

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "KKT");

   // MIP parameters
   gmsopt.setGroup("MIP Options");

   if( CbcModel::haveMultiThreadSupport() )
   {
      opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "threads");
      opt->defaultdescr = "GAMS threads";
      opt->longdescr.clear();
      opt->defaultval = 1; // somehow I got a -1 from Cbc
      opt->minval = 1;
      opt->maxval = 99;

      GamsOption::EnumVals parallelmodes;
      parallelmodes.append("opportunistic");
      parallelmodes.append("deterministic");
      gmsopt.collect("parallelmode", "whether to run opportunistic or deterministic",
         "Determines whether a parallel MIP search (threads > 1) should be done in a deterministic (i.e., reproducible) way or in a possibly faster but not necessarily reproducible way",
         "deterministic", parallelmodes, "", -1);
   }

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "strategy");

   gmsopt.collect("mipstart", "whether it should be tried to use the initial variable levels as initial MIP solution",
      "This option controls the use of advanced starting values for mixed integer programs. "
      "A setting of 1 indicates that the variable level values should be checked to see if they provide an integer feasible solution before starting optimization.",
      false, "", -1);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "tol_integer", "integerTolerance");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "sollim", "maxSolutions").longdescr.clear();

   gmsopt.collect("dumpsolutions", "name of solutions index gdx file for writing alternate solutions",
      "The name of a solutions index gdx file for writing alternate solutions found by CBC. "
      "The GDX file specified by this option will contain a set called index that contains the names of GDX files with the individual solutions.",
      "", -1);
   gmsopt.collect("dumpsolutionsmerged", "name of gdx file for writing all alternate solutions", "",
      "", -1);

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "maxsol", "maxSavedSolutions");
   opt->defaultval = 100;
   opt->longdescr = "Maximal number of solutions to store during search and to dump into gdx files if dumpsolutions options is set.";

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "strongBranching");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "trustPseudoCosts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "expensiveStrong");

   if( hasCbcOption(cbcopts, "OrbitalBranching") )  // only available if cbc build with numpy
      collectCbcOption(gmsopt, cbcopts, cbcmodel, "OrbitalBranching");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "costStrategy");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "extraVariables");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "multipleRootPasses");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "nodeStrategy");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "infeasibilityWeight");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "preprocess");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "fixOnDj");

   // overwritten by Cbc? collectCbcOption(gmsopt, cbcopts, cbcmodel, "tunePreProcess");

   gmsopt.collect("printfrequency", "frequency of status prints",
      "Controls the number of nodes that are evaluated between status prints.",
      0, 0, INT_MAX, std::string(), -1);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "randomSeedCbc", "randomCbcSeed");

   gmsopt.collect("loglevel", "amount of output printed by CBC", "",
      1, 0, INT_MAX, std::string(), -1);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "increment").defaultdescr = "GAMS cheat";

   gmsopt.collect("solvefinal", "final solve of MIP with fixed discrete variables",
      "Whether the MIP with discrete variables fixed to solution values should be solved after CBC finished.",
      true, "", -1);

   gmsopt.collect("solvetrace", "name of trace file for solving information",
      "Name of file for writing solving progress information during solve.",
      "", -1);
   gmsopt.collect("solvetracenodefreq", "frequency in number of nodes for writing to solve trace file", "",
      100, 0, INT_MAX, std::string(), -1);
   gmsopt.collect("solvetracetimefreq", "frequency in seconds for writing to solve trace file", "",
      5.0, 0.0, DBL_MAX, true, true, std::string(), -1);

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "nodlim", "maxNodes");
   opt->shortdescr = "node limit";
   opt->longdescr = "Maximum number of nodes that are enumerated in the Branch and Bound tree search.";
   opt->defaultdescr = "GAMS nodlim";
   opt->synonyms.insert("nodelim");

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "optca", "allowableGap");
   opt->defaultval.realval = 0.0;
   opt->defaultdescr = "GAMS optca";

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "optcr", "ratioGap");
   opt->defaultval.realval = 1e-4;
   opt->defaultdescr = "GAMS optcr";

   opt = &collectCbcOption(gmsopt, cbcopts, cbcmodel, "cutoff");
   opt->defaultdescr = "GAMS cutoff";
   opt->longdescr = "A valid solution must be at least this much better than last integer solution. "
      "If this option is not set then it CBC will try and work one out. "
      "E.g., if all objective coefficients are multiples of 0.01 and only integer variables have entries in objective then this can be set to 0.01.";

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "cutoffConstraint", "constraintfromCutoff").enumval);

   // allows to turn on CPLEX (licensing...)
   // collectCbcOption(gmsopt, cbcopts, cbcmodel, "depthMiniBab");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "sosPrioritize");


   gmsopt.setGroup("MIP Options for Cutting Plane Generators");
   collectCbcOption(gmsopt, cbcopts, cbcmodel, "cutDepth");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "cut_passes_root", "passCuts").defaultdescr = "20 or 100";

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "cut_passes_tree", "passTreeCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "cut_passes_slow", "slowcutpasses");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "cutLength");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "cuts", "cutsOnOff");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "cliqueCuts");

   // another cut option that actually does the same as -constraint conflict
   gmsopt.collect("conflictcuts", "Conflict Cuts", "Equivalent to setting cutoffconstraint=conflict", false, "", -1);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "flowCoverCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "gomoryCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "gomorycuts2", "GMICuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "knapsackCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "liftAndProjectCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "mirCuts", "mixedIntegerRoundingCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "twoMirCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "probingCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "reduceAndSplitCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "reduceAndSplitCuts2", "reduce2AndSplitCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "residualCapacityCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "zeroHalfCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "lagomoryCuts");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "latwomirCuts");

   gmsopt.setGroup("MIP Options for Primal Heuristics");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "heuristics", "heuristicsOnOff");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "hOptions");

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "combineSolutions").enumval);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "combine2Solutions");

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "Dins").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "DivingRandom", "DivingSome").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "DivingCoefficient").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "DivingFractional").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "DivingGuided").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "DivingLineSearch").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "DivingPseudoCost").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "DivingVectorLength").enumval);

   // overwritten by Cbc? collectCbcOption(gmsopt, cbcopts, cbcmodel, "diveOpt");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "diveSolves");

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump", "feasibilityPump").enumval);

   // overwritten by Cbc?
   collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump_passes", "passFeasibilityPump");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump_artcost", "artificialCost");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump_fracbab", "fractionforBAB");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump_cutoff", "pumpCutoff");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump_increment", "pumpIncrement");

   // overwritten by Cbc? collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump_tune", "pumpTune");
   // collectCbcOption(gmsopt, cbcopts, cbcmodel, "feaspump_moretune", "moreTune");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "greedyHeuristic");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "localTreeSearch");

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "naiveHeuristics").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "pivotAndFix").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "randomizedRounding").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "Rens").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "Rins").enumval);

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "roundingHeuristic").enumval);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "vubheuristic");

   add01(collectCbcOption(gmsopt, cbcopts, cbcmodel, "proximitySearch").enumval);

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "dwHeuristic");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "pivotAndComplement");

   collectCbcOption(gmsopt, cbcopts, cbcmodel, "VndVariableNeighborhoodSearch");

   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen();
#else
   gmsopt.writeMarkdown();
#endif

   // print uncollected Cbc options
   //for( auto& cbcopt : cbcopts )
   //   if( cbcopt.type() < CLP_PARAM_ACTION_DIRECTORY )
   //      printf("%-30s %3d %s\n", cbcopt.name().c_str(), cbcopt.type(), cbcopt.shortHelp().c_str());
}
