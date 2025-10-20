/** Copyright (C) GAMS Development and others
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optsoplex.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

#include "Highs.h"

static
double translateHighsInfinity(
   double val
   )
{
   if( highs_isInfinity(val) )
      return  DBL_MAX;
   if( highs_isInfinity(-val) )
      return -DBL_MAX;
   return val;
}

static
int translateHighsInfinity(
   HighsInt val
   )
{
   if( val == kHighsIInf )
      return  INT_MAX;
   if( val == -kHighsIInf )
      return -INT_MAX;
   return val;
}

int main(int argc, char** argv)
{
   HighsOptions highsopt;
   bool dummy;
   std::string dummys;
   int dummyi;
   double dummyd;

   highsopt.records.push_back(
      new OptionRecordBool("sensitivity",
         "Whether to run sensitivity analysis after solving an LP with a simplex method",
         false, &dummy, false) );

   highsopt.records.push_back(
      new OptionRecordBool("illconditioning",
         "Whether to run ill conditioning analysis after solving an LP with a simplex method",
         false, &dummy, false) );

   highsopt.records.push_back(
      new OptionRecordBool("illconditioning_constraint",
         "Whether to run ill conditioning on constraint view (alternative is variable view)",
         false, &dummy, false) );

   highsopt.records.push_back(
      new OptionRecordInt("illconditioning_method",
         "Method to use for ill conditioning analysis, i.e., auxiliary problem to be solved",
         false, &dummyi, 0, 0, 1) );

   highsopt.records.push_back(
      new OptionRecordDouble("illconditioning_bound",
         "Bound on ill conditioning when using ill conditioning analysis method 1",
         false, &dummyd, 0.0, 1e-4, DBL_MAX) );

   highsopt.records.push_back(
      new OptionRecordInt("mipstart",
         "Whether and how to pass initial level values as starting point to MIP solver",
         false, &dummyi, 0, 2, 4) );

   highsopt.records.push_back(
      new OptionRecordString("solvetrace",
         "Name of file for writing solving progress information during MIP solve",
         false, &dummys, "") );

   highsopt.records.push_back(
      new OptionRecordInt("solvetracenodefreq",
         "Frequency in number of nodes for writing to solve trace file",
         false, &dummyi, 0, 100, INT_MAX) );

   highsopt.records.push_back(
      new OptionRecordDouble("solvetracetimefreq",
         "Frequency in seconds for writing to solve trace file",
         false, &dummyd, 0.0, 5.0, DBL_MAX) );

   highsopt.records.push_back(
      new OptionRecordInt("iis",
         "whether to compute an irreducible infeasible subset of an LP",
         false, &dummyi, 0, 0, 2) );

#ifdef GAMS_BUILD
   highsopt.records.push_back(
      new OptionRecordBool("pdlp_gpu",
         "Whether to attempt using an NVIDIA GPU when solver=pdlp.",
         false, &dummy, false) );
#endif

   GamsOptions gmsopt("HiGHS");
   gmsopt.setSeparator("=");
   gmsopt.setGroup("highs");
   gmsopt.setEolChars("#");

   for( OptionRecord* hopt : highsopt.records )
   {
      std::string defaultdescr;
      assert(hopt != NULL);

      if( hopt->name == "log_file" )
         continue;
      if( hopt->name == "log_to_console" )
         continue;
      if( hopt->name == "log_githash" )
         continue;
      if( hopt->name == "write_presolved_model_file" )  // not processed by HiGHS lib
         continue;
      if( hopt->name == "write_presolved_model_to_file" || hopt->name == "write_model_to_file" || hopt->name == "write_solution_to_file") // deprecated and without function
         continue;
      if( hopt->name == "read_basis_file" || hopt->name == "read_solution_file" )
         continue;
      if( hopt->name == "ranging" )
         continue;
      if( hopt->name == "keep_n_rows" )
         continue;
      if( hopt->name == "mps_parser_type_free" )
         continue;
      if( hopt->name == "glpsol_cost_row_location" )
         continue;
      if( hopt->name == "solve_relaxation" )  // user could just change MIP to RMIP
         continue;
      if( hopt->name == "blend_multi_objectives" )  // GAMS has only at most one objective functions
         continue;
      if( hopt->name == "use_warm_start" )  // better set bratio=1 or mipstart=0 to disable passing basis/sol to highs
         continue;
      if( hopt->name.find("mip_improving_solution") == 0 )
         continue;
      // HiGHS does not undo the scaling if applying this user scaling
      if( hopt->name == "user_bound_scale" || hopt->name == "user_cost_scale" )
         continue;
      // QP solver not made available in GAMS
      if( hopt->name.find("qp_") == 0 )
         continue;

      GamsOption* opt = NULL;
      switch( hopt->type )
      {
         case HighsOptionType::kBool :
         {
            std::string longdescr;
            if( hopt->name == "allow_unbounded_or_infeasible" )
               hopt->description = "whether to spend extra effort to distinguish unboundedness and infeasibility if necessary";
            else if( hopt->name == "output_flag" )
#ifdef GAMS_BUILD
               defaultdescr = "0, if \\ref GAMSAOlogoption \"GAMS logoption\" = 0, otherwise 1";
#else
               defaultdescr = "0, if GAMS logoption = 0, otherwise 1";
#endif
#ifdef GAMS_BUILD
            else if( hopt->name == "pdlp_gpu" )
               longdescr = "If GPU is chosen, then CUDA libraries and drivers need to be installed and be part of the library search path. Option is ignored on macOS.";
#endif

            opt = &gmsopt.collect(hopt->name, hopt->description, longdescr,
               static_cast<OptionRecordBool*>(hopt)->default_value,
               defaultdescr);
            break;
         }

         case HighsOptionType::kInt :
         {
            int lowerbound = translateHighsInfinity(static_cast<OptionRecordInt*>(hopt)->lower_bound);
            int upperbound = translateHighsInfinity(static_cast<OptionRecordInt*>(hopt)->upper_bound);
            int defaultval = translateHighsInfinity(static_cast<OptionRecordInt*>(hopt)->default_value);
            std::string longdescr;

            if( hopt->name == "ipm_iteration_limit" )
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOiterlim \"GAMS iterlim\"";
#else
               defaultdescr = "GAMS iterlim";
#endif
            else if( hopt->name == "simplex_iteration_limit" )
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOiterlim \"GAMS iterlim\"";
#else
               defaultdescr = "GAMS iterlim";
#endif
            else if( hopt->name == "pdlp_iteration_limit" )
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOiterlim \"GAMS iterlim\"";
#else
               defaultdescr = "GAMS iterlim";
#endif
            else if( hopt->name == "mip_max_nodes" )
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOnodlim \"GAMS nodlim\", if > 0, &infin; otherwise";
#else
               defaultdescr = "GAMS nodlim, if > 0, &infin; otherwise";
#endif
            else if( hopt->name == "threads" )
            {
               defaultval = 0;
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOthreads \"GAMS threads\"";
#else
               defaultdescr = "GAMS threads";
#endif
            }
            else if( hopt->name == "mipstart" )
               longdescr = "See section \\ref HIGHS_PARTIALSOL for details.";
            else if( hopt->name == "iis" )
               longdescr = "Set to 1 to compute IIS after solve if infeasible, or to 2 to compute IIS without solving original problem.";
            else if( hopt->name == "pdlp_e_restart_method" )
            {
               // the possible choice 2=CPU has not been implemented in cuPDLP-C so far
               // make default depend on pdlp_gpu
               upperbound = 1;
               hopt->description = "Restart method for PDLP solver: 0 => none; 1 => GPU";
               defaultval = 0;
               defaultdescr = "1 if GPU is used, 0 otherwise";
            }

            opt = &gmsopt.collect(hopt->name, hopt->description, longdescr,
               defaultval,
               lowerbound,
               upperbound,
               defaultdescr);
            break;
         }

         case HighsOptionType::kDouble :
         {
            double defaultval = translateHighsInfinity(static_cast<OptionRecordDouble*>(hopt)->default_value);

            if( hopt->name == "mip_abs_gap" )
            {
               defaultval = 0.0;
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOoptca \"GAMS optca\"";
#else
               defaultdescr = "GAMS optca";
#endif
            }
            else if( hopt->name == "mip_rel_gap" )
            {
               defaultval = 1e-4;
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOoptcr \"GAMS optcr\"";
#else
               defaultdescr = "GAMS optcr";
#endif
            }
            else if( hopt->name == "objective_bound" )
            {
               defaultval = 1e-4;
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOcutoff \"GAMS cutoff\"";
#else
               defaultdescr = "GAMS cutoff";
#endif
            }
            else if( hopt->name == "time_limit" )
            {
               defaultval = 10000.0;
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOreslim \"GAMS reslim\"";
#else
               defaultdescr = "GAMS reslim";
#endif
            }

            opt = &gmsopt.collect(hopt->name, hopt->description, std::string(),
               defaultval,
               translateHighsInfinity(static_cast<OptionRecordDouble*>(hopt)->lower_bound),
               translateHighsInfinity(static_cast<OptionRecordDouble*>(hopt)->upper_bound),
               true, true,
               defaultdescr);
            break;
         }

         case HighsOptionType::kString :
            if( hopt->name == "solver" ) // we ignore option solver for MIP, since it switches to solving the LP relaxation
               hopt->description = "LP algorithm to run: \"simplex\", \"choose\", \"ipm\", or \"pdlp\"; ignored for MIP";

            opt = &gmsopt.collect(hopt->name, hopt->description, std::string(),
               static_cast<OptionRecordString*>(hopt)->default_value,
               defaultdescr);
            break;
      }

      if( opt != NULL )
         opt->advanced = hopt->advanced;

      // make ill condition analyzer hidden for now
      if( opt != NULL && hopt->name.find("illconditioning") == 0 )
         opt->hidden = true;
   }

   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen(true);
#else
   gmsopt.writeMarkdown();
#endif
}
