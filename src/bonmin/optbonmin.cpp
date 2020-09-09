/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optbonmin.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

#include "BonBonminSetup.hpp"
#include "BonCbc.hpp"

using namespace Ipopt;

// TODO specify Ipopt options of Bonmin as hidden in optbonmin.gms, so they don't show up in docu?

int main(int argc, char** argv)
{
   Bonmin::BonminSetup bonmin_setup;

   SmartPtr<OptionsList> options = new OptionsList();
   SmartPtr<Ipopt::Journalist> journalist = new Ipopt::Journalist();
   SmartPtr<Bonmin::RegisteredOptions> regoptions = new Bonmin::RegisteredOptions();
   options->SetJournalist(journalist);
   options->SetRegisteredOptions(GetRawPtr(regoptions));

   bonmin_setup.setOptionsAndJournalist(regoptions, options, journalist);
   bonmin_setup.registerOptions();

   bonmin_setup.roptions()->SetRegisteringCategory("Output", Bonmin::RegisteredOptions::IpoptCategory);
   bonmin_setup.roptions()->AddStringOption2("print_eval_error",
      "Switch to enable printing information about function evaluation errors into the GAMS listing file.",
      "yes",
      "no", "", "yes", "");

   bonmin_setup.roptions()->SetRegisteringCategory("Output and Loglevel", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup.roptions()->AddStringOption2("print_funceval_statistics",
      "Switch to enable printing statistics on number of evaluations of GAMS functions/gradients/Hessian.",
      "no",
      "no", "", "yes", "");

   bonmin_setup.roptions()->AddStringOption1("solvetrace",
      "Name of file for writing solving progress information.",
      "", "*", "");

   bonmin_setup.roptions()->AddLowerBoundedIntegerOption("solvetracenodefreq",
      "Frequency in number of nodes for writing solving progress information.",
      0, 100, "giving 0 disables writing of N-lines to trace file");

   bonmin_setup.roptions()->AddLowerBoundedNumberOption("solvetracetimefreq",
      "Frequency in seconds for writing solving progress information.",
      0.0, false, 5.0, "giving 0.0 disables writing of T-lines to trace file");

   bonmin_setup.roptions()->SetRegisteringCategory("NLP interface", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup.roptions()->AddStringOption2("solvefinal",
      "Switch to disable solving MINLP with discrete variables fixed to solution values after solve.",
      "yes",
      "no", "", "yes", "",
      "If enabled, then the dual values from the resolved NLP are made available in GAMS.");

   bonmin_setup.roptions()->SetRegisteringCategory("Branch-and-bound options", Bonmin::RegisteredOptions::BonminCategory);
   bonmin_setup.roptions()->AddStringOption2("clocktype",
      "Type of clock to use for time_limit",
      "wall",
      "cpu", "CPU time", "wall", "Wall-clock time",
      "");

   const Bonmin::RegisteredOptions::RegOptionsList& optionlist(regoptions->RegisteredOptionsList());

   // options sorted by category
   std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;
   GamsOptions gmsopt("Bonmin");
   gmsopt.setEolChars("#");

   for( Bonmin::RegisteredOptions::RegOptionsList::const_iterator it(optionlist.begin()); it != optionlist.end(); ++it )
   {
      std::string category(it->second->RegisteringCategory());

      if( category.empty() )
         continue;
      if( regoptions->categoriesInfo(category) == Bonmin::RegisteredOptions::UndocumentedCategory )
         continue;
      if( regoptions->categoriesInfo(category) == Bonmin::RegisteredOptions::IpoptCategory )
         category = "Ipopt " + category;
      else
         category = " " + category;

      if( it->second->Name() == "nlp_solver" ||
          it->second->Name() == "file_solution" ||
          it->second->Name() == "sos_constraints"
        )
         continue;


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

   // print options tables
   std::cout << "Writing optbonmin_t.md" << std::endl;
   std::ofstream tabfile("optbonmin_t.md");
   tabfile << "<!-- This file is autogenerated by optdoc.cpp in COIN-OR/GAMSlinks -->" << std::endl;
   std::string tablehead = "| Option | Type | Default | B-BB | B-OA | B-QG | B-Hyb | B-Ecp | B-iFP | Cbc_Par |\n";
   tablehead.append(       "|:-------|:-----|--------:|:----:|:----:|:----:|:-----:|:-----:|:-----:|:-------:|");

   int categcount = 0;
   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ, ++categcount )
   {
      tabfile << std::endl;
      tabfile << "\\subsection BONMINopt_" << categcount << ' ' << it_categ->first << std::endl << std::endl;
      if( it_categ->first.find("Ipopt") == 0 )
         tabfile << "| Option | Type | Default |" << std::endl
                 << "|:-------|:-----|--------:|" << std::endl;
      else
         tabfile << tablehead << std::endl;

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         tabfile << "| \\anchor BONMIN" << (*it_opt)->Name() << "SHORTDOC ";
         tabfile << "\\ref BONMIN" << (*it_opt)->Name() << " \"" << (*it_opt)->Name() << "\" | ";

         std::string typestring;
         std::string defaultval;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Integer:
            {
               typestring = "\\f$\\mathbb{Z}\\f$";
               defaultval = "\\f$";
               defaultval.append(GamsOptions::makeValidLatexNumber((*it_opt)->DefaultInteger()));
               defaultval.append("\\f$");
               break;
            }

            case Ipopt::OT_Number:
            {
               typestring = "\\f$\\mathbb{Q}\\f$";
               defaultval = "\\f$";
               defaultval.append(GamsOptions::makeValidLatexNumber((*it_opt)->DefaultNumber()));
               defaultval.append("\\f$");
               break;
            }

            case Ipopt::OT_String:
            {
               typestring = "string";
               defaultval = "``";
               defaultval.append((*it_opt)->DefaultString());
               defaultval.append("``");

               if( (*it_opt)->Name() == "linear_solver" )
                  defaultval = "``ma27``, if BonminH, otherwise ``mumps``";
               else if( (*it_opt)->Name() == "linear_system_scaling" )
                  defaultval = "``mc19``, if BonminH, otherwise ``none``";

               break;
            }

            case Ipopt::OT_Unknown: ;
         }

         if( (*it_opt)->Name() == "nlp_log_at_root" )
            defaultval = GamsOptions::makeValidLatexNumber(Ipopt::J_ITERSUMMARY);
         else if( (*it_opt)->Name() == "allowable_gap" )
            defaultval = "\\ref GAMSAOoptca \"GAMS optca\"";
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
            defaultval = "\\ref GAMSAOoptcr \"GAMS optcr\"";
         else if( (*it_opt)->Name() == "node_limit" )
            defaultval = "\\ref GAMSAOnodlim \"GAMS nodlim\"";
         else if( (*it_opt)->Name() == "time_limit" )
            defaultval = "\\ref GAMSAOreslim \"GAMS reslim\"";
         else if( (*it_opt)->Name() == "iteration_limit" )
            defaultval = "\\ref GAMSAOiterlim \"GAMS iterlim\"";
         else if( (*it_opt)->Name() == "cutoff" )
            defaultval = "\\ref GAMSAOcutoff \"GAMS cutoff\"";
         else if( (*it_opt)->Name() == "number_cpx_threads" )
            defaultval = "\\ref GAMSAOthreads \"GAMS threads\"";

         tabfile << typestring << " | ";
         tabfile << defaultval;
         if( it_categ->first.find("Ipopt") != 0 )
            tabfile
             << " | " << ( (regoptions->isValidForBBB((*it_opt)->Name()))    ? "\\f$\\surd\\f$" : "--" )
             << " | " << ( (regoptions->isValidForBOA((*it_opt)->Name()))    ? "\\f$\\surd\\f$" : "--" )
             << " | " << ( (regoptions->isValidForBQG((*it_opt)->Name()))    ? "\\f$\\surd\\f$" : "--" )
             << " | " << ( (regoptions->isValidForHybrid((*it_opt)->Name())) ? "\\f$\\surd\\f$" : "--" )
             << " | " << ( (regoptions->isValidForBEcp((*it_opt)->Name()))   ? "\\f$\\surd\\f$" : "--" )
             << " | " << ( (regoptions->isValidForBiFP((*it_opt)->Name()))   ? "\\f$\\surd\\f$" : "--" )
             << " | " << ( (regoptions->isValidForCbc((*it_opt)->Name()))    ? "\\f$\\surd\\f$" : "--" );
         tabfile << " |" << std::endl;
      }
   }
   tabfile.close();

   GamsOption::Type opttype;
   GamsOption::Value defaultval, minval, maxval;
   // bool minval_strict, maxval_strict;
   GamsOption::EnumVals enumval;
   std::string tmpstr;
   std::string longdescr;
   std::string defaultdescr;

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      gmsopt.setGroup(it_categ->first);

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         enumval.clear();
         // minval_strict = false;
         // maxval_strict = false;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Number:
            {
               opttype = GamsOption::Type::REAL;
               minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
               maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
               //TODO should ask Bonmin for value for infinity
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
               opttype = GamsOption::Type::INTEGER;
               minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
               maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
               defaultval.intval = (*it_opt)->DefaultInteger();
               break;
            }

            case Ipopt::OT_String:
            {
               opttype = GamsOption::Type::STRING;
               defaultval = GamsOption::makeValue((*it_opt)->DefaultString());

               const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->GetValidStrings());
               if( settings.size() > 1 || settings[0].value_ != "*")
               {
                  opttype = GamsOption::Type::STRING;
                  if( (*it_opt)->Name() == "linear_solver" )
                  {
                     enumval.append("ma27", "use the Harwell routine MA27");
                     enumval.append("ma57", "use the Harwell routine MA57");
                     enumval.append("ma77", "use the Harwell routine HSL_MA77");
                     enumval.append("ma86", "use the Harwell routine HSL_MA86");
                     enumval.append("ma97", "use the Harwell routine HSL_MA97");
                     enumval.append("pardiso", "use the Pardiso package");
                     enumval.append("mumps", "use MUMPS package");

                     longdescr = "Determines which linear algebra package is to be used for the solution of the augmented linear system (for obtaining the search directions). "
                        "Note, that MA27, MA57, MA86, and MA97 are only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately. "
                        "To use HSL_MA77, a HSL library needs to be provided.";

                     defaultval = GamsOption::makeValue("ma27, if BonminH, otherwise mumps");
                  }
                  else
                  {
                     if( (*it_opt)->Name() == "linear_system_scaling" )
                     {
                        longdescr = "Determines the method used to compute symmetric scaling factors for the augmented system (see also the \"linear_scaling_on_demand\" option).  This scaling is independent of the NLP problem scaling.  By default, MC19 is only used if MA27 or MA57 are selected as linear solvers. "
                           "Note, that MC19 is only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately.";

                        defaultval = GamsOption::makeValue("mc19, if BonminH, otherwise none");
                     }

                     enumval.reserve(settings.size());
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

         // GAMS overwrites of Bonmin option defaults
         if( (*it_opt)->Name() == "nlp_log_at_root" )
            defaultval.intval = Ipopt::J_ITERSUMMARY;
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
         {
            defaultval.realval = 0.1;
            defaultdescr = "GAMS optcr";
         }
         else if( (*it_opt)->Name() == "allowable_gap" )
         {
            defaultval.realval = 0.0;
            defaultdescr = "GAMS optca";
         }
         else if( (*it_opt)->Name() == "time_limit" )
         {
            defaultval.realval = 1000;
            defaultdescr = "GAMS reslim";
         }
         else if( (*it_opt)->Name() == "node_limit" )
            defaultdescr = "GAMS nodlim";
         else if( (*it_opt)->Name() == "iteration_limit" )
            defaultdescr = "GAMS iterlim";
         else if( (*it_opt)->Name() == "cutoff" )
            defaultdescr = "GAMS cutoff";
         else if( it_categ->first == " MILP cutting planes in hybrid algorithm" && (*it_opt)->Name() != "2mir_cuts" )
            longdescr = "See option `2mir_cuts` for a detailed description.";
         else if( (*it_opt)->Name() == "milp_solver" )
            longdescr = "To use Cplex, a valid license is required.";
         else if( (*it_opt)->Name() == "resolve_on_small_infeasibility" )
            longdescr = "";
         else if( (*it_opt)->Name() == "number_cpx_threads" )
         {
            defaultval.intval = 1;
            defaultdescr = "GAMS threads";
         }
         // Ipopt options
         else if( (*it_opt)->Name() == "bound_relax_factor" )
            defaultval.realval = 1e-10;
         else if( (*it_opt)->Name() == "mu_strategy" )
            defaultval = GamsOption::makeValue("adaptive");
         else if( (*it_opt)->Name() == "mu_oracle" )
            defaultval = GamsOption::makeValue("probing");
         else if( (*it_opt)->Name() == "gamma_phi" )
            defaultval.realval = 1e-8;
         else if( (*it_opt)->Name() == "gamma_theta" )
            defaultval.realval = 1e-4;
         else if( (*it_opt)->Name() == "required_infeasibility_reduction" )
            defaultval.realval = 0.1;
         else if( (*it_opt)->Name() == "expect_infeasible_problem" )
            defaultval = GamsOption::makeValue("yes");
         else if( (*it_opt)->Name() == "warm_start_init_point" )
            defaultval = GamsOption::makeValue("yes");
         else if( (*it_opt)->Name() == "print_level" )
            defaultval.intval = 0;
         else if( (*it_opt)->Name() == "print_frequency_time" )
            defaultval.realval = 0.5;
         else if( (*it_opt)->Name() == "ma86_order" )
            defaultval = GamsOption::makeValue("auto");
         else if( (*it_opt)->Name() == "nlp_scaling_method" )
         {
            for( GamsOption::EnumVals::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( strcmp(it->first.stringval, "user-scaling") == 0 )
               {
                  enumval.erase(it);
                  break;
               }
         }
         else if( (*it_opt)->Name() == "dependency_detector" )
         {
            for( GamsOption::EnumVals::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( strcmp(it->first.stringval, "wsmp") == 0 )
               {
                  enumval.erase(it);
                  break;
               }
         }

         gmsopt.collect((*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, enumval, defaultdescr);
      }
   }

   gmsopt.writeDef();
   gmsopt.writeGMS();
}
