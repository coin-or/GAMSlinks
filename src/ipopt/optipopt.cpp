/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optipopt.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"
#include "GamsIpopt.hpp"

#include "gclgms.h"

#include "IpIpoptApplication.hpp"
using namespace Ipopt;

int main(int argc, char** argv)
{
   GamsIpopt gamsipopt;
   try
   {
      gamsipopt.setupIpopt();
   }
   catch( const std::exception& e )
   {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
   }
   SmartPtr<IpoptApplication> ipopt = gamsipopt.ipopt;

   ipopt->Options()->SetIntegerValue("max_iter", ITERLIM_INFINITY, true, true);
   ipopt->Options()->SetNumericValue("max_wall_time", RESLIM_INFINITY, true, true);

   GamsOption::Type opttype;
   GamsOption::Value defaultval, minval, maxval;
   bool minval_strict, maxval_strict;
   GamsOption::EnumVals enumval;
   std::string tmpstr;
   std::string longdescr;
   std::string defaultdescr;

   GamsOptions gmsopt("Ipopt");
   gmsopt.setEolChars("#");

   // get option categories
   Ipopt::RegisteredOptions::RegCategoriesByPriority categs;
   ipopt->RegOptions()->RegisteredCategoriesByPriority(categs);

   for( auto& categ : categs )
   {
      // skip categories of undocumented options
      if( categ->Priority() < 0 )
         continue;

      if( categ->Name() == "Derivative Checker" )
         continue;

      if( categ->RegisteredOptions().empty() )
         continue;

      // passing in negative priority to get groups with high priority first in output
      gmsopt.setGroup(categ->Name(), "", "", -categ->Priority());

      for( auto& opt : categ->RegisteredOptions() )
      {
         if( opt->Name() == "hessian_constant" ||
            opt->Name() == "obj_scaling_factor" ||
            opt->Name() == "file_print_level" ||
            opt->Name() == "option_file_name" ||
            opt->Name() == "output_file" ||
            opt->Name() == "print_options_documentation" ||
            opt->Name() == "print_user_options" ||
            opt->Name() == "nlp_lower_bound_inf" ||
            opt->Name() == "nlp_upper_bound_inf" ||
            opt->Name() == "num_linear_variables" ||
            opt->Name() == "skip_finalize_solution_call" ||
            opt->Name() == "warm_start_entire_iterate" ||
            opt->Name() == "warm_start_same_structure"
         )
            continue;

         enumval.clear();
         longdescr = opt->LongDescription();
         defaultdescr.clear();
         minval_strict = false;
         maxval_strict = false;
         switch( opt->Type() )
         {
            case Ipopt::OT_Number:
            {
               opttype = GamsOption::Type::REAL;
               minval = opt->HasLower() ? opt->LowerNumber() : -DBL_MAX;
               maxval = opt->HasUpper() ? opt->UpperNumber() :  DBL_MAX;
               ipopt->Options()->GetNumericValue(opt->Name(), defaultval.realval, "");
               minval_strict = opt->HasLower() ? opt->LowerStrict() : false;
               maxval_strict = opt->HasUpper() ? opt->UpperStrict() : false;
               break;
            }

            case Ipopt::OT_Integer:
            {
               opttype = GamsOption::Type::INTEGER;
               minval = opt->HasLower() ? opt->LowerInteger() : -INT_MAX;
               maxval = opt->HasUpper() ? opt->UpperInteger() :  INT_MAX;
               ipopt->Options()->GetIntegerValue(opt->Name(), defaultval.intval, "");
               break;
            }

            case Ipopt::OT_String:
            {
               opttype = GamsOption::Type::STRING;
               ipopt->Options()->GetStringValue(opt->Name(), tmpstr, "");
               defaultval = tmpstr;

               const std::vector<Ipopt::RegisteredOption::string_entry>& settings(opt->GetValidStrings());
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
               std::cerr << "Skip option " << opt->Name() << " of unknown type." << std::endl;
               continue;
            }
         }

         if( opt->Name() == "max_iter" )
            defaultdescr = "GAMS iterlim";
         else if( opt->Name() == "max_wall_time" )
            defaultdescr = "GAMS reslim";
         else if( opt->Name() == "nlp_scaling_method" )
         {
#ifdef GAMS_BUILD
            for( auto& e : enumval )
               if( e.first == "equilibration-based" )
                  e.second = "scale the problem so that first derivatives are of order 1 at random points (GAMS/Ipopt: requires user-provided library with HSL routine MC19)";

#endif
            enumval.drop("user-scaling");
            defaultdescr = std::string(defaultval.stringval) + " if GAMS scaleopt is not set, otherwise none";
         }
         else if( opt->Name() == "linear_solver" )
         {
#ifdef GAMS_BUILD
            longdescr += " "
               "Note, that MA27, MA57, MA86, and MA97 are included with a commercially supported GAMS/IpoptH license only. "
               "To use MA27, MA57, MA86, or MA97 with GAMS/Ipopt, or to use HSL_MA77, a HSL library needs to be provided by the user. "
               "To use Pardiso from pardiso-project.org, a Pardiso library needs to be provided by the user. "
               "**ATTENTION**: Before Ipopt 3.14 (GAMS 36), value ***pardiso*** specified to use Pardiso from Intel MKL. "
               "With GAMS 36, this value has been renamed to ***pardisomkl***.";

            defaultdescr = "ma27, if IpoptH, otherwise mumps";

            for( auto& e : enumval )
            {
               if( e.first == "ma27" )
                  e.second = "IpoptH: use the Harwell routine MA27; Ipopt: load the Harwell routine MA27 from user-provided library";
               else if( e.first == "ma57" )
                  e.second = "IpoptH: use the Harwell routine MA57; Ipopt: load the Harwell routine MA57 from user-provided library";
               else if( e.first == "ma77" )
                  e.second = "load the Harwell routine HSL_MA77 from user-provided library";
               else if( e.first == "ma86" )
                  e.second = "IpoptH: use the Harwell routine HSL_MA86; Ipopt: load the Harwell routine HSL_MA86 from user-provided library";
               else if( e.first == "ma97" )
                  e.second = "IpoptH: use the Harwell routine HSL_MA97; Ipopt: load the Harwell routine HSL_MA97 from user-provided library";
            }
#endif
            enumval.drop("custom");
         }
#ifdef GAMS_BUILD
         else if( opt->Name() == "linear_system_scaling" )
         {
            longdescr += " "
               "Note, that MC19 is included with a commercially supported GAMS/IpoptH license only. "
               "To use MC19 with GAMS/Ipopt, a HSL library needs to be provided by the user.";

            for( auto& e : enumval )
               if( e.first == "mc19" )
                  e.second = "IpoptH: use the Harwell routine MC19; Ipopt: load the Harwell routine MC19 from user-provided library";

            defaultdescr = "mc19, if IpoptH, otherwise none";
         }
#endif
         else if( opt->Name() == "hsllib" )
         {
            defaultdescr = "libhsl.so (Linux), libhsl.dylib (macOS), libhsl.dll (Windows)";
         }
         else if( opt->Name() == "pardisolib" )
         {
            defaultdescr = "libpardiso.so (Linux), libpardiso.dylib (macOS), libpardiso.dll (Windows)";
         }
         else if( opt->Name() == "warm_start_init_point" )
         {
            defaultdescr = "yes, if run on modified model instance (e.g., from GUSS), otherwise no";
         }

         GamsOption& gopt = gmsopt.collect(opt->Name(), opt->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, !minval_strict, !maxval_strict, enumval, defaultdescr);
         gopt.advanced = opt->Advanced();
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
