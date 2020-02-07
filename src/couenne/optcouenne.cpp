/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optcouenne.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

#include "BonCouenneSetup.hpp"
#include "BonCouenneInterface.hpp"

using namespace Ipopt;

// TODO specify Ipopt and Bonmin options of Couenne as hidden in optcouenne.gms, so they don't show up in docu?

int main(int argc, char** argv)
{
   SmartPtr<OptionsList> options = new OptionsList();
   SmartPtr<Ipopt::Journalist> journalist = new Ipopt::Journalist();
   SmartPtr<Bonmin::RegisteredOptions> regoptions = new Bonmin::RegisteredOptions();
   options->SetJournalist(journalist);
   options->SetRegisteredOptions(GetRawPtr(regoptions));

   Couenne::CouenneSetup::registerAllOptions(regoptions);

   regoptions->AddStringOption1("solvetrace",
      "Name of file for writing solving progress information.",
      "", "*", "");

   regoptions->AddLowerBoundedIntegerOption("solvetracenodefreq",
      "Frequency in number of nodes for writing solving progress information.",
      0, 100, "giving 0 disables writing of N-lines to trace file");

   regoptions->AddLowerBoundedNumberOption("solvetracetimefreq",
      "Frequency in seconds for writing solving progress information.",
      0.0, false, 5.0, "giving 0.0 disables writing of T-lines to trace file");

   regoptions->SetRegisteringCategory("Output", Bonmin::RegisteredOptions::IpoptCategory);
   regoptions->AddStringOption2("print_eval_error",
      "Switch to enable printing information about function evaluation errors into the GAMS listing file.",
      "yes",
      "no", "", "yes", "");

   regoptions->SetRegisteringCategory("Branch-and-bound options", Bonmin::RegisteredOptions::BonminCategory);
   regoptions->AddStringOption2("clocktype",
      "Type of clock to use for time_limit",
      "wall",
      "cpu", "CPU time", "wall", "Wall-clock time",
      "");

   const Bonmin::RegisteredOptions::RegOptionsList& optionlist(regoptions->RegisteredOptionsList());

   // options sorted by category
   std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;

   for( Bonmin::RegisteredOptions::RegOptionsList::const_iterator it(optionlist.begin()); it != optionlist.end(); ++it )
   {
      std::string category(it->second->RegisteringCategory());

      if( category.empty() )
         continue;
      if( regoptions->categoriesInfo(category) == Bonmin::RegisteredOptions::UndocumentedCategory )
         continue;
      else if( regoptions->categoriesInfo(category) == Bonmin::RegisteredOptions::IpoptCategory )
         category = "Ipopt " + category;
      else if( regoptions->categoriesInfo(category) == Bonmin::RegisteredOptions::BonminCategory )
         category = "Bonmin " + category;
      else
         category = " " + category;

      /* Couenne skips */
      if( it->second->Name()=="couenne_check" ||
          it->second->Name()=="opt_window" ||
          it->second->Name()=="save_soltext" ||
          it->second->Name()=="test_mode" ||
          it->second->Name()=="lp_solver" ||
          it->second->Name()=="display_stats" )
         continue;

      /* Bonmin skips */
      if( it->second->Name() == "nlp_solver" ||
          it->second->Name() == "file_solution" ||
          it->second->Name() == "sos_constraints"
        )
         continue;

      /* Ipopt skips */
      if( category == "Ipopt Undocumented" ||
          category == "Ipopt Uncategorized" ||
          category == "Ipopt " ||
          category == "Ipopt Derivative Checker"
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

   GamsOptions::OPTTYPE opttype;
   GamsOptions::OPTVAL defaultval, minval, maxval;
   // bool minval_strict, maxval_strict;
   GamsOptions::ENUMVAL enumval;
   std::string tmpstr;
   std::string longdescr;

   GamsOptions gmsopt("couenne");
   gmsopt.setEolChars("#");

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      gmsopt.setGroup(it_categ->first);

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         // minval_strict = false;
         // maxval_strict = false;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Number:
            {
               opttype = GamsOptions::OPTTYPE_REAL;
               minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
               maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
               //TODO should ask Couenne for value for infinity
               if( minval.realval == -1e+20 )
                  minval.realval = -DBL_MAX;
               if( maxval.realval ==  1e+20 )
                  maxval.realval =  DBL_MAX;
               defaultval.realval = (*it_opt)->DefaultNumber();
               // minval_strict = (*it_opt)->HasLower() ? (*it_opt)->LowerStrict() : false;
               // maxval_strict = (*it_opt)->HasUpper() ? (*it_opt)->UpperStrict() : false;
               break;
            }

            case Ipopt::OT_Integer:
            {
               opttype = GamsOptions::OPTTYPE_INTEGER;
               minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
               maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
               defaultval.intval = (*it_opt)->DefaultInteger();
               break;
            }

            case Ipopt::OT_String:
            {
               defaultval.stringval = strdup((*it_opt)->DefaultString().c_str());

               const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->GetValidStrings());
               if( settings.size() == 1 && settings[0].value_ == "*")
               {
                  opttype = GamsOptions::OPTTYPE_STRING;
               }
               else
               {
                  opttype = GamsOptions::OPTTYPE_ENUM;
                  if( (*it_opt)->Name().find("branch_pt_select_") == 0 )
                  {
                     enumval.clear();
                     for( size_t j = 0; j < settings.size(); ++j )
                        enumval.push_back(std::pair<std::string, std::string>(settings[j].value_, ""));
                  }
                  else if( (*it_opt)->Name() == "linear_solver" )
                  {
                     enumval.clear();
                     enumval.push_back(std::pair<std::string, std::string>("ma27", "use the Harwell routine MA27"));
                     enumval.push_back(std::pair<std::string, std::string>("ma57", "use the Harwell routine MA57"));
                     enumval.push_back(std::pair<std::string, std::string>("ma77", "use the Harwell routine HSL_MA77"));
                     enumval.push_back(std::pair<std::string, std::string>("ma86", "use the Harwell routine HSL_MA86"));
                     enumval.push_back(std::pair<std::string, std::string>("ma97", "use the Harwell routine HSL_MA97"));
                     enumval.push_back(std::pair<std::string, std::string>("pardiso", "use the Pardiso package"));
                     enumval.push_back(std::pair<std::string, std::string>("mumps", "use MUMPS package"));

                     longdescr = "Determines which linear algebra package is to be used for the solution of the augmented linear system (for obtaining the search directions). "
                        "Note, that MA27, MA57, MA86, and MA97 are only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately. "
                        "To use HSL_MA77, a HSL library needs to be provided.";

                     defaultval.stringval = "ma27, if IpoptH licensed, otherwise mumps";
                  }
                  else
                  {
                     if( (*it_opt)->Name() == "linear_system_scaling" )
                     {
                        longdescr = "Determines the method used to compute symmetric scaling factors for the augmented system (see also the \"linear_scaling_on_demand\" option).  This scaling is independent of the NLP problem scaling.  By default, MC19 is only used if MA27 or MA57 are selected as linear solvers. "
                           "Note, that MC19 is only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately.";

                        defaultval.stringval = "mc19, if IpoptH licensed, otherwise none";
                     }

                     enumval.resize(settings.size());
                     for( size_t j = 0; j < settings.size(); ++j )
                        enumval[j] = std::pair<std::string, std::string>(settings[j].value_, settings[j].description_);
                  }
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

         longdescr = (*it_opt)->LongDescription();
         // Couenne options
         if( longdescr.find("cuts are generated every k nodes") != std::string::npos && (*it_opt)->Name() != "2mir_cuts" )
            longdescr = "See option `2mir_cuts` for the meaning of k.";
         else if( (*it_opt)->Name().find("branch_pt_select_") == 0 )
            longdescr = longdescr + "Default is to use the value of `branch_pt_select` (value `common`).";
         else if( (*it_opt)->Name() == "feas_pump_usescip" )
            longdescr = "Note, that SCIP is only available for GAMS users with a SCIP or academic GAMS license.";
         // Bonmin options
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
            defaultval.realval = 0.1;
         else if( (*it_opt)->Name() == "time_limit" )
            defaultval.realval = 1000;
         else if( (*it_opt)->Name() == "problem_print_level" )
            defaultval.intval = Ipopt::J_STRONGWARNING;
         else if( it_categ->first == "Bonmin MILP cutting planes in hybrid algorithm" && (*it_opt)->Name() != "2mir_cuts" )
            longdescr = "See option `2mir_cuts` for a detailed description.";
         else if( (*it_opt)->Name() == "milp_solver" )
            longdescr = "To use Cplex, a valid license is required.";
         else if( (*it_opt)->Name() == "resolve_on_small_infeasibility" )
            longdescr = "";
         // Ipopt options
         else if( (*it_opt)->Name() == "bound_relax_factor" )
            defaultval.realval = 1e-10;
         else if( (*it_opt)->Name() == "ma86_order" )
            defaultval.stringval = "auto";
         else if( (*it_opt)->Name() == "nlp_scaling_method" )
         {
            for( GamsOptions::ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "user-scaling" )
               {
                  enumval.erase(it);
                  break;
               }
         }
         else if( (*it_opt)->Name() == "dependency_detector" )
         {
            for( GamsOptions::ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "wsmp" )
               {
                  enumval.erase(it);
                  break;
               }
         }

         gmsopt.collect((*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, enumval);
      }
   }

   gmsopt.write();
}
