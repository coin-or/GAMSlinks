/** Copyright (C) GAMS Development and others 2009-2019
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

   highsopt.records.push_back(
      new OptionRecordBool("sensitivity",
         "Whether to run sensitivity analysis after solving an LP with a simplex method",
         false, &dummy, false) );

   highsopt.records.push_back(
      new OptionRecordBool("mipstart",
         "Whether to pass initial level values as starting point to MIP solver",
         false, &dummy, false) );

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
            else if( hopt->name == "mipstart" )
               longdescr = "If the solution is not feasible, HiGHS will solve the LP obtained from fixing all discrete variables to their initial level values.";

            gmsopt.collect(hopt->name, hopt->description, longdescr,
               static_cast<OptionRecordBool*>(hopt)->default_value,
               defaultdescr).advanced = hopt->advanced;
            break;
         }

         case HighsOptionType::kInt :
         {
            int defaultval = translateHighsInfinity(static_cast<OptionRecordInt*>(hopt)->default_value);

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
            else if( hopt->name == "mip_max_nodes" )
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOnodlim \"GAMS nodlim\", if > 0, &infin; otherwise";
#else
               defaultdescr = "GAMS nodlim, if > 0, &infin; otherwise";
#endif
            else if( hopt->name == "threads" )
            {
               defaultval = 1;
#ifdef GAMS_BUILD
               defaultdescr = "\\ref GAMSAOthreads \"GAMS threads\"";
#else
               defaultdescr = "GAMS threads";
#endif
            }

            gmsopt.collect(hopt->name, hopt->description, std::string(),
               defaultval,
               translateHighsInfinity(static_cast<OptionRecordInt*>(hopt)->lower_bound),
               translateHighsInfinity(static_cast<OptionRecordInt*>(hopt)->upper_bound),
               defaultdescr).advanced = hopt->advanced;
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

            gmsopt.collect(hopt->name, hopt->description, std::string(),
               defaultval,
               translateHighsInfinity(static_cast<OptionRecordDouble*>(hopt)->lower_bound),
               translateHighsInfinity(static_cast<OptionRecordDouble*>(hopt)->upper_bound),
               true, true,
               defaultdescr).advanced = hopt->advanced;
            break;
         }

         case HighsOptionType::kString :
            if( hopt->name == "write_model_file" )
               defaultdescr = "&lt;inputname&gt;.lp";
            else if( hopt->name == "solution_file" )
               defaultdescr = "&lt;inputname&gt;.sol";
            else if( hopt->name == "solver" ) // for ignore option solver for MIP, since it switches to solving the LP relaxation
               hopt->description = "LP algorithm to run: \"simplex\", \"choose\", or \"ipm\"; ignored for MIP";

            gmsopt.collect(hopt->name, hopt->description, std::string(),
               static_cast<OptionRecordString*>(hopt)->default_value,
               defaultdescr).advanced = hopt->advanced;
            break;
      }
   }

   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen(true);
#else
   gmsopt.writeMarkdown();
#endif
}
