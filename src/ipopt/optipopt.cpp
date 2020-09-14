/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optipopt.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

#include "IpIpoptApplication.hpp"
using namespace Ipopt;

int main(int argc, char** argv)
{
   Ipopt::SmartPtr<Ipopt::IpoptApplication> ipopt = new Ipopt::IpoptApplication();

   ipopt->RegOptions()->SetRegisteringCategory("Output");
   ipopt->RegOptions()->AddStringOption2("print_eval_error",
      "Switch to enable printing information about function evaluation errors into the GAMS listing file.",
      "yes",
      "no", "", "yes", "");
   ipopt->RegOptions()->AddStringOption2("report_mininfeas_solution",
      "Switch to report intermediate solution with minimal constraint violation to GAMS if the final solution is not feasible.",
      "no",
      "no", "", "yes", "",
      "This option allows to obtain the most feasible solution found by Ipopt during the iteration process, if it stops at a (locally) infeasible solution, due to a limit (time, iterations, ...), or with a failure in the restoration phase.");

   const Ipopt::RegisteredOptions::RegOptionsList& optionlist(ipopt->RegOptions()->RegisteredOptionsList());

   // options sorted by category
   std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;

   for( Ipopt::RegisteredOptions::RegOptionsList::const_iterator it(optionlist.begin()); it != optionlist.end(); ++it )
   {
      std::string category(it->second->RegisteringCategory());

      if( category == "Undocumented" ||
          category == "Uncategorized" ||
          category == "" ||
          category == "Derivative Checker"
        )
         continue;

      if( it->second->Name() == "hessian_constant" ||
          it->second->Name() == "obj_scaling_factor" ||
          it->second->Name() == "file_print_level" ||
          it->second->Name() == "option_file_name" ||
          it->second->Name() == "output_file" ||
          it->second->Name() == "print_options_documentation" ||
          it->second->Name() == "print_user_options" ||
          it->second->Name() == "nlp_lower_bound_inf" ||
          it->second->Name() == "nlp_upper_bound_inf" ||
          it->second->Name() == "num_linear_variables" ||
          it->second->Name() == "skip_finalize_solution_call" ||
          it->second->Name() == "warm_start_entire_iterate" ||
          it->second->Name() == "warm_start_same_structure"
        )
         continue;

      opts[category].push_back(it->second);
   }

   GamsOption::Type opttype;
   GamsOption::Value defaultval, minval, maxval;
   bool minval_strict, maxval_strict;
   GamsOption::EnumVals enumval;
   std::string tmpstr;
   std::string longdescr;
   std::string defaultdescr;

   GamsOptions gmsopt("Ipopt");
   gmsopt.setEolChars("#");

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      gmsopt.setGroup(it_categ->first);

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         enumval.clear();
         longdescr = (*it_opt)->LongDescription();
         defaultdescr.clear();
         minval_strict = false;
         maxval_strict = false;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Number:
            {
               opttype = GamsOption::Type::REAL;
               minval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
               maxval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
               defaultval = (*it_opt)->DefaultNumber();
               minval_strict = (*it_opt)->HasLower() ? (*it_opt)->LowerStrict() : false;
               maxval_strict = (*it_opt)->HasUpper() ? (*it_opt)->UpperStrict() : false;
               break;
            }

            case Ipopt::OT_Integer:
            {
               opttype = GamsOption::Type::INTEGER;
               minval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
               maxval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
               defaultval = (*it_opt)->DefaultInteger();
               break;
            }

            case Ipopt::OT_String:
            {
               opttype = GamsOption::Type::STRING;
               defaultval = (*it_opt)->DefaultString();

               const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->GetValidStrings());
               if( settings.size() > 1 || settings[0].value_ != "*" )
               {
                  enumval.reserve(settings.size());
                  for( size_t j = 0; j < settings.size(); ++j )
                     enumval.append(settings[j].value_, settings[j].description_);
               }

               break;
            }

            default:
            case Ipopt::OT_Unknown:
            {
               std::cerr << "Skip option " << (*it_opt)->Name() << " of unknown type." << std::endl;
               continue;
            }
         }

         if( (*it_opt)->Name() == "bound_relax_factor" )
            defaultval = 1e-10;
         else if( (*it_opt)->Name() == "acceptable_iter" )
            defaultval = 0;
         else if( (*it_opt)->Name() == "max_iter" )
         {
            defaultval = INT_MAX;
            defaultdescr = "GAMS iterlim";
         }
         else if( (*it_opt)->Name() == "max_cpu_time" )
         {
            defaultval = 10000000000.0;
            defaultdescr = "GAMS reslim";
         }
         else if( (*it_opt)->Name() == "mu_strategy" )
            defaultval = "adaptive";
#ifdef GAMS_BUILD
         else if( (*it_opt)->Name() == "ma86_order" )
            defaultval = "auto";
#endif
         else if( (*it_opt)->Name() == "nlp_scaling_method" )
            enumval.drop("user-scaling");
         else if( (*it_opt)->Name() == "linear_solver" )
         {
#ifdef GAMS_BUILD
            longdescr = "Determines which linear algebra package is to be used for the solution of the augmented linear system (for obtaining the search directions). "
               "Note, that MA27, MA57, MA86, and MA97 are only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately. "
               "To use HSL_MA77, a HSL library needs to be provided.";

            defaultdescr = "ma27, if IpoptH, otherwise mumps";

            enumval.drop("wsmp");
#endif
            enumval.drop("custom");
         }
#ifdef GAMS_BUILD
         else if( (*it_opt)->Name() == "dependency_detector" )
            enumval.drop("wsmp");
         else if( (*it_opt)->Name() == "linear_system_scaling" )
         {
            longdescr = "Determines the method used to compute symmetric scaling factors for the augmented system (see also the \"linear_scaling_on_demand\" option).  This scaling is independent of the NLP problem scaling.  By default, MC19 is only used if MA27 or MA57 are selected as linear solvers. "
               "Note, that MC19 is only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately.";

            defaultdescr = "mc19, if IpoptH, otherwise none";
         }
#endif

         gmsopt.collect((*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, !minval_strict, !maxval_strict, enumval, defaultdescr);
      }
   }
   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen();
#else
   gmsopt.writeMarkdown();
#endif
}
