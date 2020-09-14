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

      // Couenne skips
      if( it->second->Name()=="couenne_check" ||
          it->second->Name()=="opt_window" ||
          it->second->Name()=="save_soltext" ||
          it->second->Name()=="test_mode" ||
          it->second->Name()=="lp_solver" ||
          it->second->Name()=="display_stats" )
         continue;

      // Bonmin skips
      if( it->second->Name() == "nlp_solver" ||
          it->second->Name() == "file_solution" ||
          it->second->Name() == "sos_constraints"
        )
         continue;

      // Ipopt skips
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

   GamsOption::Type opttype;
   GamsOption::Value defaultval, minval, maxval;
   bool minval_strict, maxval_strict;
   GamsOption::EnumVals enumval;
   std::string tmpstr;
   std::string longdescr;
   std::string defaultdescr;

   GamsOptions gmsopt("Couenne");
   gmsopt.setEolChars("#");

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      gmsopt.setGroup(it_categ->first);

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         enumval.clear();
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
               if( settings.size() > 1 || settings[0].value_ != "*")
               {
                  enumval.reserve(settings.size());
                  if( (*it_opt)->Name().find("branch_pt_select_") == 0 )
                  {
                     for( size_t j = 0; j < settings.size(); ++j )
                        enumval.append(settings[j].value_);
                  }
                  else
                  {
                     for( size_t j = 0; j < settings.size(); ++j )
                        enumval.append(settings[j].value_, settings[j].description_);
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
         defaultdescr.clear();

         // Couenne options
         if( longdescr.find("cuts are generated every k nodes") != std::string::npos && (*it_opt)->Name() != "2mir_cuts" )
            longdescr = "See option `2mir_cuts` for the meaning of k.";
         else if( (*it_opt)->Name().find("branch_pt_select_") == 0 )
            longdescr = longdescr + "Default is to use the value of `branch_pt_select` (value `common`).";
         else if( (*it_opt)->Name() == "feas_pump_usescip" )
            longdescr = "Note, that SCIP is only available for GAMS users with a SCIP or academic GAMS license.";
         else if( (*it_opt)->Name() == "problem_print_level" )
            defaultval = Ipopt::J_STRONGWARNING;

         // GAMS overwrites of Bonmin option defaults
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
         {
            defaultval = 1e-4;
            defaultdescr = "GAMS optcr";
         }
         else if( (*it_opt)->Name() == "allowable_gap" )
         {
            defaultval = 0.0;
            defaultdescr = "GAMS optca";
         }
         else if( (*it_opt)->Name() == "time_limit" )
         {
            defaultval = 10000000000.0;
            defaultdescr = "GAMS reslim";
         }
         else if( (*it_opt)->Name() == "node_limit" )
            defaultdescr = "GAMS nodlim";
         else if( (*it_opt)->Name() == "iteration_limit" )
            defaultdescr = "GAMS iterlim";
         else if( (*it_opt)->Name() == "cutoff" )
            defaultdescr = "GAMS cutoff";
         else if( it_categ->first == "Bonmin MILP cutting planes in hybrid algorithm" && (*it_opt)->Name() != "2mir_cuts" )
            longdescr = "See option `2mir_cuts` for a detailed description.";
         else if( (*it_opt)->Name() == "milp_solver" )
            longdescr = "To use Cplex, a valid license is required.";
         else if( (*it_opt)->Name() == "resolve_on_small_infeasibility" )
            longdescr.clear();

         // Ipopt options
         else if( (*it_opt)->Name() == "bound_relax_factor" )
            defaultval = 1e-10;
         else if( (*it_opt)->Name() == "nlp_scaling_method" )
            enumval.drop("user-scaling");
         else if( (*it_opt)->Name() == "linear_solver" )
            enumval.drop("custom");

         gmsopt.collect((*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, !minval_strict, !maxval_strict, enumval, defaultdescr);
      }
   }
   gmsopt.finalize();

   gmsopt.writeMarkdown();
   gmsopt.writeDef();
}
