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
   CoinParamVec&        paramvec,
   const std::string&   namecbc
   )
{
   for( auto& o : paramvec )
      if( o->name() == namecbc )
         return true;
   return false;
}

static
GamsOption& collectOption(
   GamsOptions&                gmsopt,
   CoinParamVec&               paramvec,
   const std::string&          namegams,
   const std::string&          namecbc_ = ""
   )
{
   std::string namecbc(namecbc_.empty() ? namegams : namecbc_);

   unsigned int idx;
   for( idx = 0; idx < paramvec.size(); ++idx )
      if( paramvec[idx] != NULL && paramvec[idx]->name() == namecbc )
         break;
   if( idx >= paramvec.size() )
   {
      std::cerr << "Error: Option " << namecbc << " not known to Cbc." << std::endl;
      exit(1);
      //return *new GamsOption(namecbc_, "MISSING", "MISSING", false);
   }

   const CoinParam& cbcopt(*paramvec[idx]);

   GamsOption::Type opttype;
   GamsOption::Value defaultval, minval, maxval;
   GamsOption::EnumVals enumval;
   std::string tmpstr;
   std::string longdescr;

   switch( cbcopt.type() )
   {
      case CoinParam::paramDbl:
      {
         opttype = GamsOption::Type::REAL;
         minval = cbcopt.lowerDblVal();
         maxval = cbcopt.upperDblVal();
         defaultval = cbcopt.dblVal();
         break;
      }

      case CoinParam::paramInt:
      {
         opttype = GamsOption::Type::INTEGER;
         minval = cbcopt.lowerIntVal();
         maxval = cbcopt.upperIntVal();
         defaultval = cbcopt.intVal();
         break;
      }

      // TODO case CoinParam::paramStr:

      case CoinParam::paramKwd:
      {
         // check whether this might be a bool option
         std::vector<std::string> kws(cbcopt.definedKwdsSorted());
         if( kws.size() == 2 && ((kws[0] == "on" && kws[1] == "off") || (kws[0] == "off" && kws[1] == "on")) )
         {
            opttype = GamsOption::Type::BOOL;
            cbcopt.getVal(tmpstr);
            defaultval = tmpstr == "on";
         }
         else
         {
            opttype = GamsOption::Type::STRING;

            std::string def;
            cbcopt.getVal(def);
            // remove '!' and '?' marker from default
            auto newend = std::remove(def.begin(), def.end(), '!');
            newend = std::remove(def.begin(), newend, '?');
            defaultval = std::string(def.begin(), newend);

            for( const auto& kwd : kws )
            {
               // remove '!' and '?' marker from keyword
               std::string v = kwd;
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
         break;
      }
      default:
      {
         std::cerr << "ERROR: 'action'-option encountered: " << namegams << std::endl;
         exit(1);
         //return *new GamsOption(namecbc_, "ACTIONOPT", "ACTIONOPT not expected", false);
      }
   }

   GamsOption& opt(gmsopt.collect(namegams, cbcopt.shortHelp(), cbcopt.longHelp(), opttype, defaultval, minval, maxval, true, true, enumval, "", idx));
   if( namegams != namecbc )
      opt.synonyms[namecbc];

   // remove parameter from array, so we can later easily check which options we haven't taken
   paramvec[idx] = NULL;

   return opt;
}

static
GamsOption& collectCbcOption(
   GamsOptions&                gmsopt,
   CbcParameters&              cbcparams,
   const std::string&          namegams,
   const std::string&          namecbc = ""
   )
{
   GamsOption& o = collectOption(gmsopt, cbcparams.paramVec(), namegams, namecbc);
   // to be able to distinguish between Clp and Cbc options in the Cbc link, we add 1000000 to the refval for Clp options
   // so Cbc options must get a refval below 1000000
   assert(o.refval < 1000000);

   return o;
}

static
GamsOption& collectClpOption(
   GamsOptions&                gmsopt,
   CbcParameters&              cbcparams,
   const std::string&          namegams,
   const std::string&          namecbc = ""
   )
{
   GamsOption& o = collectOption(gmsopt, cbcparams.clpParamVec(), namegams, namecbc);
   // to be able to distinguish between Clp and Cbc options in the Cbc link, we add 1000000 to the refval for Clp options
   o.refval += 1000000;

   return o;
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
   CbcParameters cbcparams;
   CbcMain0(cbcmodel, cbcparams);
   GamsOption* opt;

   // collection of GAMS/Cbc parameters
   GamsOptions gmsopt("CBC");

   // General parameters
   gmsopt.setGroup("General Options");

   opt = &collectClpOption(gmsopt, cbcparams, "reslim", "seconds");
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

   opt = &collectClpOption(gmsopt, cbcparams, "iterlim", "maxIterations");
   opt->longdescr = "For an LP, this is the maximum number of iterations to solve the LP. For a MIP, this option is ignored.";
   opt->defaultval.intval = INT_MAX;
   opt->defaultdescr = "GAMS iterlim";

   collectClpOption(gmsopt, cbcparams, "idiotCrash");

   collectClpOption(gmsopt, cbcparams, "sprintCrash").synonyms["sifting"];

   collectClpOption(gmsopt, cbcparams, "crash");

   collectClpOption(gmsopt, cbcparams, "factorization");

   collectClpOption(gmsopt, cbcparams, "denseThreshold");

   collectClpOption(gmsopt, cbcparams, "smallFactorization");

   collectClpOption(gmsopt, cbcparams, "sparseFactor");

   // FIXME default value not set on Clp/Cbc side
   // collectClpOption(gmsopt, cbcparams, "biasLU");

   collectClpOption(gmsopt, cbcparams, "maxFactor");

   opt = &collectClpOption(gmsopt, cbcparams, "crossover");
   // value "maybe" is only relevant for quadratic: remove value and mention in longdescr
   opt->enumval.drop("maybe");
   opt->longdescr = "Interior point algorithms do not obtain a basic solution. "
      "This option will crossover to a basic solution suitable for ranging or branch and cut.";
   add01(opt->enumval);

   collectClpOption(gmsopt, cbcparams, "dualPivot").enumval.append("auto", "Same as automatic. This is a deprecated setting.");

   collectClpOption(gmsopt, cbcparams, "primalPivot").enumval.append("auto", "Same as automatic. This is a deprecated setting.");

   collectClpOption(gmsopt, cbcparams, "psi");

   collectClpOption(gmsopt, cbcparams, "perturbation");

   collectClpOption(gmsopt, cbcparams, "scaling").enumval.append("auto", "Same as automatic. This is a deprecated setting.");

   opt = &collectClpOption(gmsopt, cbcparams, "presolve");
   opt->enumval.drop("file");
   add01(opt->enumval);
   opt->longdescr = "Presolve analyzes the model to find such things as redundant equations, "
      "equations which fix some variables, equations which can be transformed into bounds, etc. "
      "For the initial solve of any problem this is worth doing unless one knows that it will have no effect. "
      "Option 'on' will normally do 5 passes, while using 'more' will do 10.";

   collectClpOption(gmsopt, cbcparams, "passPresolve");

   collectClpOption(gmsopt, cbcparams, "substitution");

   collectClpOption(gmsopt, cbcparams, "randomSeedClp", "randomSeed");

   collectClpOption(gmsopt, cbcparams, "tol_primal", "primalTolerance");

   collectClpOption(gmsopt, cbcparams, "tol_dual", "dualTolerance");

   collectClpOption(gmsopt, cbcparams, "tol_presolve", "preTolerance");

   GamsOption::EnumVals startalgs;
   startalgs.append("primal", "Primal Simplex algorithm");
   startalgs.append("dual", "Dual Simplex algorithm");
   startalgs.append("barrier", "Primal-dual predictor-corrector algorithm");
   gmsopt.collect("startalg", "LP solver for root node",
      "Determines the algorithm to use for an LP or the initial LP relaxation if the problem is a MIP.",
      "dual", startalgs, "", -1);

   collectClpOption(gmsopt, cbcparams, "primalWeight");

   opt = &collectClpOption(gmsopt, cbcparams, "autoScale");
   opt->shortdescr += " (experimental)";
   opt->longdescr.clear();

   collectClpOption(gmsopt, cbcparams, "bscale");

   //TODO collectClpOption(gmsopt, cbcparams, "cholesky");

   // FIXME default value not set on Clp/Cbc side
   // collectClpOption(gmsopt, cbcparams, "gamma", "gamma(Delta)").synonyms.clear();  // GAMS options object doesn't like parenthesis in synonym

   collectClpOption(gmsopt, cbcparams, "KKT");

   // MIP parameters
   gmsopt.setGroup("MIP Options");

   if( CbcModel::haveMultiThreadSupport() )
   {
      opt = &collectCbcOption(gmsopt, cbcparams, "threads");
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

   collectCbcOption(gmsopt, cbcparams, "strategy");

   gmsopt.collect("mipstart", "whether it should be tried to use the initial variable levels as initial MIP solution",
      "This option controls the use of advanced starting values for mixed integer programs. "
      "A setting of 1 indicates that the variable level values should be checked to see if they provide an integer feasible solution before starting optimization.",
      false, "", -1);

   collectCbcOption(gmsopt, cbcparams, "tol_integer", "integerTolerance");

   collectCbcOption(gmsopt, cbcparams, "sollim", "maxSolutions").longdescr.clear();

   gmsopt.collect("dumpsolutions", "name of solutions index gdx file for writing alternate solutions",
      "The name of a solutions index gdx file for writing alternate solutions found by CBC. "
      "The GDX file specified by this option will contain a set called index that contains the names of GDX files with the individual solutions.",
      "", -1);
   gmsopt.collect("dumpsolutionsmerged", "name of gdx file for writing all alternate solutions", "",
      "", -1);

   opt = &collectCbcOption(gmsopt, cbcparams, "maxsol", "maxSavedSolutions");
   opt->defaultval = 100;
   opt->longdescr = "Maximal number of solutions to store during search and to dump into gdx files if dumpsolutions options is set.";

   collectCbcOption(gmsopt, cbcparams, "strongBranching");

   collectCbcOption(gmsopt, cbcparams, "trustPseudocosts");

   collectCbcOption(gmsopt, cbcparams, "expensiveStrong");

   if( hasCbcOption(cbcparams.paramVec(), "OrbitalBranching") )  // only available if cbc build with numpy
      collectCbcOption(gmsopt, cbcparams, "OrbitalBranching");

   // FIXME option seems to have vanished
   //collectCbcOption(gmsopt, paramvec, "costStrategy");

   collectCbcOption(gmsopt, cbcparams, "extraVariables");

   collectCbcOption(gmsopt, cbcparams, "multipleRootPasses");

   collectCbcOption(gmsopt, cbcparams, "nodeStrategy");

   collectCbcOption(gmsopt, cbcparams, "infeasibilityWeight");

   collectCbcOption(gmsopt, cbcparams, "preprocess");

   collectCbcOption(gmsopt, cbcparams, "fixOnDj");

   // overwritten by Cbc? collectCbcOption(gmsopt, paramvec, "tunePreProcess");

   gmsopt.collect("printfrequency", "frequency of status prints",
      "Controls the number of nodes that are evaluated between status prints.",
      0, 0, INT_MAX, std::string(), -1);

   collectCbcOption(gmsopt, cbcparams, "randomSeedCbc", "randomCbcSeed");

   gmsopt.collect("loglevel", "amount of output printed by CBC", "",
      1, 0, INT_MAX, std::string(), -1);

   collectCbcOption(gmsopt, cbcparams, "increment").defaultdescr = "GAMS cheat";

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

   opt = &collectCbcOption(gmsopt, cbcparams, "nodlim", "maxNodes");
   opt->shortdescr = "node limit";
   opt->longdescr = "Maximum number of nodes that are enumerated in the Branch and Bound tree search.";
   opt->defaultdescr = "GAMS nodlim";
   opt->synonyms["nodelim"];

   opt = &collectCbcOption(gmsopt, cbcparams, "optca", "allowableGap");
   opt->defaultval.realval = 0.0;
   opt->defaultdescr = "GAMS optca";

   opt = &collectCbcOption(gmsopt, cbcparams, "optcr", "ratioGap");
   opt->defaultval.realval = 1e-4;
   opt->defaultdescr = "GAMS optcr";

   opt = &collectCbcOption(gmsopt, cbcparams, "cutoff");
   opt->defaultdescr = "GAMS cutoff";
   opt->longdescr = "All solutions must have a better objective value than the value of this option. "
      "CBC also updates this value whenever it obtains a solution to the value of the objective function of the solution minus the cutoff increment.";

   add01(collectCbcOption(gmsopt, cbcparams, "cutoffConstraint", "constraintfromCutoff").enumval);

   // allows to turn on CPLEX (licensing...)
   // collectCbcOption(gmsopt, paramvec, "depthMiniBab");

   collectCbcOption(gmsopt, cbcparams, "sosPrioritize");


   gmsopt.setGroup("MIP Options for Cutting Plane Generators");
   collectCbcOption(gmsopt, cbcparams, "cutDepth");

   collectCbcOption(gmsopt, cbcparams, "cut_passes_root", "passCuts").defaultdescr = "20 or 100";

   collectCbcOption(gmsopt, cbcparams, "cut_passes_tree", "passTreeCuts");

   collectCbcOption(gmsopt, cbcparams, "cut_passes_slow", "slowcutpasses");

   collectCbcOption(gmsopt, cbcparams, "cutLength");

   collectCbcOption(gmsopt, cbcparams, "cuts", "cutsOnOff");

   collectCbcOption(gmsopt, cbcparams, "cliqueCuts");

   // another cut option that actually does the same as -constraint conflict
   gmsopt.collect("conflictcuts", "Conflict Cuts", "Equivalent to setting cutoffconstraint=conflict", false, "", -1);

   collectCbcOption(gmsopt, cbcparams, "flowCoverCuts");

   collectCbcOption(gmsopt, cbcparams, "gomoryCuts");

   collectCbcOption(gmsopt, cbcparams, "gomorycuts2", "GMICuts");

   collectCbcOption(gmsopt, cbcparams, "knapsackCuts");

   collectCbcOption(gmsopt, cbcparams, "liftAndProjectCuts");

   collectCbcOption(gmsopt, cbcparams, "mirCuts", "mixedIntegerRoundingCuts");

   collectCbcOption(gmsopt, cbcparams, "twoMirCuts");

   collectCbcOption(gmsopt, cbcparams, "probingCuts");

   collectCbcOption(gmsopt, cbcparams, "reduceAndSplitCuts");

   collectCbcOption(gmsopt, cbcparams, "reduceAndSplitCuts2", "reduce2AndSplitCuts");

   collectCbcOption(gmsopt, cbcparams, "residualCapacityCuts");

   collectCbcOption(gmsopt, cbcparams, "zeroHalfCuts");

   collectCbcOption(gmsopt, cbcparams, "lagomoryCuts");

   collectCbcOption(gmsopt, cbcparams, "latwomirCuts");

   gmsopt.setGroup("MIP Options for Primal Heuristics");

   collectCbcOption(gmsopt, cbcparams, "heuristics", "heuristicsOnOff");

   collectCbcOption(gmsopt, cbcparams, "hOptions");

   add01(collectCbcOption(gmsopt, cbcparams, "combineSolutions").enumval);

   collectCbcOption(gmsopt, cbcparams, "combine2Solutions");

   add01(collectCbcOption(gmsopt, cbcparams, "Dins").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "DivingRandom", "DivingSome").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "DivingCoefficient").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "DivingFractional").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "DivingGuided").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "DivingLineSearch").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "DivingPseudocost").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "DivingVectorLength").enumval);

   // overwritten by Cbc? collectCbcOption(gmsopt, cbcparams, "diveOpt");

   collectCbcOption(gmsopt, cbcparams, "diveSolves");

   add01(collectCbcOption(gmsopt, cbcparams, "feaspump", "feasibilityPump").enumval);

   // overwritten by Cbc?
   collectCbcOption(gmsopt, cbcparams, "feaspump_passes", "passFeasibilityPump");

   collectCbcOption(gmsopt, cbcparams, "feaspump_artcost", "artificialCost");

   collectCbcOption(gmsopt, cbcparams, "feaspump_fracbab", "fractionforBAB");

   collectCbcOption(gmsopt, cbcparams, "feaspump_cutoff", "pumpCutoff");

   collectCbcOption(gmsopt, cbcparams, "feaspump_increment", "pumpIncrement");

   // overwritten by Cbc? collectCbcOption(gmsopt, paramvec, "feaspump_tune", "pumpTune");
   // collectCbcOption(gmsopt, paramvec, "feaspump_moretune", "moreTune");

   collectCbcOption(gmsopt, cbcparams, "greedyHeuristic");

   collectCbcOption(gmsopt, cbcparams, "localTreeSearch");

   add01(collectCbcOption(gmsopt, cbcparams, "naiveHeuristics").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "pivotAndFix").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "randomizedRounding").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "Rens").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "Rins").enumval);

   add01(collectCbcOption(gmsopt, cbcparams, "roundingHeuristic").enumval);

   collectCbcOption(gmsopt, cbcparams, "vubheuristic");

   add01(collectCbcOption(gmsopt, cbcparams, "proximitySearch").enumval);

   collectCbcOption(gmsopt, cbcparams, "dwHeuristic");

   // FIXME option seems to have vanished
   // collectCbcOption(gmsopt, paramvec, "pivotAndComplement");

   collectCbcOption(gmsopt, cbcparams, "VndVariableNeighborhoodSearch");

   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen();
#else
   gmsopt.writeMarkdown();
#endif

   // print uncollected Cbc options
//   printf("Uncollected Cbc options:\n");
//   for( auto& cbcopt : cbcparams.paramVec() )
//      if( cbcopt != NULL && cbcopt->type() != CoinParam::paramAct && cbcopt->type() != CoinParam::paramInvalid )
//         printf("%-30s %3d %s\n", cbcopt->name().c_str(), cbcopt->type(), cbcopt->shortHelp().c_str());
   // print uncollected Clp options
//    printf("Uncollected Clp options:\n");
//    for( auto& cbcopt : cbcparams.clpParamVec() )
//       if( cbcopt != NULL && cbcopt->type() != CoinParam::paramAct && cbcopt->type() != CoinParam::paramInvalid )
//          printf("%-30s %3d %s\n", cbcopt->name().c_str(), cbcopt->type(), cbcopt->shortHelp().c_str());
}
