// Copyright (C) GAMS Development and others 2010
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsBonmin.cpp 885 2010-03-28 14:45:46Z stefan $
//
// Author: Stefan Vigerske

#include "GAMSlinksConfig.h"

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

#if COIN_HAS_BONMIN
#include "IpIpoptApplication.hpp"
#endif

#if COIN_HAS_BONMIN
#include "BonBonminSetup.hpp"
#include "BonCbc.hpp"
#endif

#if COIN_HAS_COUENNE
#include "BonCouenneSetup.hpp"
#include "BonCouenneInterface.hpp"
#endif

//#define COIN_HAS_SCIP 1
#if COIN_HAS_SCIP
#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/pub_paramset.h"
#endif

enum OPTTYPE {
	OPTTYPE_BOOL, OPTTYPE_INTEGER, OPTTYPE_REAL, OPTTYPE_CHAR, OPTTYPE_STRING, OPTTYPE_ENUM
};

union OPTVAL {
	bool boolval;
	int intval;
	double realval;
	char charval;
	const char* stringval;
};

typedef std::vector<std::pair<std::string, std::string> > ENUMVAL;


void makeValidLatexString(std::string& str) {
	int numdollar = 0;
	for (size_t i = 0; i < str.length(); ) {
		if (str[i] == '$')
			++numdollar;
		if (str[i] == '_' || str[i] == '^' || str[i] == '#' || str[i] == '&') {
			str.insert(i, "\\");
			i += 2;
		} else if ((str[i] == '<' || str[i] == '>') && !(numdollar%2)) {
			str.insert(i, "$");
			str.insert(i+2, "$");
			i += 2;
		} else if (str[i] == '\n') {
			str[i] = '\\';
			str.insert(i, "\\");
			i += 2;
		}
		++i;
	}
	//TODO could also substitute greek letters
}

std::string makeValidLatexString(const char* str) {
	std::string tmp = str;
	makeValidLatexString(tmp);
	return tmp;
}

std::string makeValidLatexNumber(double value) {
	if (value ==  DBL_MAX)
		return "\\infty";
	if (value == -DBL_MAX)
		return "-\\infty";

  char buffer[256];
  sprintf(buffer, "%g", value);
  std::string str = buffer;

  size_t epos = str.find("e");
  if (epos != std::string::npos) {
  	if (str[epos+1] == '+')
  		str[epos+1] = ' ';
  	if (str[epos+2] == '0')
  		str[epos+2] = ' ';
  	if (epos == 1 && str[0] == '1') // number is 1e...
    	str.replace(0, 2, "10^{");
  	else if (epos == 2 && str[0] == '-' && str[1] == '1')  // number is -1e...
  		str.replace(1, 2, "10^{");
  	else // number is ...e...
  		str.replace(epos, 1, " \\cdot 10^{");
  	str.append("}");
  }
  
  return str;
}


std::string makeValidLatexNumber(int value) {
	if (value ==  INT_MAX)
		return "\\infty";
	if (value == -INT_MAX)
		return "-\\infty";

  char buffer[256];
  sprintf(buffer, "%d", value);
  
  return buffer;
}

int LongintToInt(long int value) {
	//TODO what is INT_MAX for long int ?
	if (value > INT_MAX)
		return INT_MAX;
	if (value < -INT_MAX)
		return -INT_MAX;
//	if (value > INT_MAX) {
//		std::cout << "Warning, chop long int value " << value << " to max integer = " << INT_MAX << std::endl;
//		return INT_MAX;
//	}
//	if (value < -INT_MAX) {
//		std::cout << "Warning, chop long int value " << value << " to min integer = " << -INT_MAX << std::endl;
//		return -INT_MAX;
//	}
	return (int)value;
}

void printOptionCategoryStart(std::ostream& out,
	std::string name) {
	makeValidLatexString(name);
	out << "\\printoptioncategory{" << name << "}" << std::endl;
}

void printOption(std::ostream& out, 
	std::string name,
	std::string shortdescr,
	std::string longdescr,
	OPTTYPE type,
	OPTVAL& def,
	OPTVAL& min,
	bool min_strict,
	OPTVAL& max,
	bool max_strict,
	const ENUMVAL& enumvals = ENUMVAL()) {
	//std::clog << "print SCIP option " << name << "...";
	
	// some preprocessing
	switch (type) {
		case OPTTYPE_INTEGER:
			if (min_strict && min.intval != -INT_MAX) {
				min.intval++;
				min_strict = false;
			}
			if (max_strict && max.intval !=  INT_MAX) {
				max.intval--;
				max_strict = false;
			}
//			if (min.intval == -INT_MAX)
//				min_strict = true;
//			if (max.intval ==  INT_MAX)
//				max_strict = true;
			break;
		default: ;
	}
	
  makeValidLatexString(name);
  makeValidLatexString(shortdescr);
  
	out << "\\printoption{" << name << "}%" << std::endl;
  
  bool have_enum_docu = false;

	switch (type) {
		case OPTTYPE_BOOL:
		  out << "{";
//			if (min.boolval == max.boolval)
//				out << "\\{" << min.boolval << "\\} ";
//			else
//				out << "\\{" << min.boolval << ", " << max.boolval << "\\} ";
		  out << "boolean";
			out << "}%" << std::endl;
			out << "{" << def.boolval << "}%" << std::endl;
			break;

		case OPTTYPE_INTEGER:
			out << "{$";
			if (min.intval > -INT_MAX)
				out << makeValidLatexNumber(min.intval) << "\\leq";
			out << "\\textrm{integer}";
			if (max.intval < INT_MAX)
				out << "\\leq" << makeValidLatexNumber(max.intval);
			out << "$}%" << std::endl;
			out << "{$" << makeValidLatexNumber(def.intval) << "$}%" << std::endl;
			break;
			
		case OPTTYPE_REAL: {
			out << "{$";
			if (min.realval > -DBL_MAX)
				out << makeValidLatexNumber(min.realval) << (min_strict ? "<" : "\\leq");
			out << "\\textrm{real}";
			if (max.realval < DBL_MAX)
				out << (max_strict ? "<" : "\\leq") << makeValidLatexNumber(max.realval);
			out << "$}%" << std::endl;
			out << "{$" << makeValidLatexNumber(def.realval) << "$}%" << std::endl;
			break;
		}
		
		case OPTTYPE_CHAR: {
			out << "{character}%" << std::endl;
			out << "{" << def.charval << "}%" << std::endl;
			break;
		}

		case OPTTYPE_STRING:
		  out << "{string}%" << std::endl;
			out << "{" << makeValidLatexString(def.stringval) << "}%" << std::endl;
			break;
			
		case OPTTYPE_ENUM: {
			std::string tmp;
			out << "{";
			for (ENUMVAL::const_iterator it(enumvals.begin()); it != enumvals.end(); ++it) {
				if (it != enumvals.begin())
					out << ", ";
				tmp = it->first;
				makeValidLatexString(tmp);
				out << tmp;
				if (it->second != "")
					have_enum_docu = true;
			}
			out << "}%" << std::endl;
			out << "{" << makeValidLatexString(def.stringval) << "}%" << std::endl;
			break;
		}
	}

  out << "{" << shortdescr;

  if (longdescr != "") {
    out << "\\\\" << std::endl;
    makeValidLatexString(longdescr);
    out << longdescr;
  }
  out << "}%" << std::endl;

  out << "{";
  if (type == OPTTYPE_ENUM && have_enum_docu) {
  	std::string tmp;
  	out << "\\begin{list}{}{" << std::endl;
  	out << "\\setlength{\\parsep}{0em}" << std::endl;
  	out << "\\setlength{\\leftmargin}{3ex}" << std::endl;
  	out << "\\setlength{\\labelwidth}{1ex}" << std::endl;
  	out << "\\setlength{\\itemindent}{0ex}" << std::endl;
  	out << "\\setlength{\\topsep}{0pt}}" << std::endl;
		for (ENUMVAL::const_iterator it(enumvals.begin()); it != enumvals.end(); ++it) {
			tmp = it->first;
			makeValidLatexString(tmp);
			out << "\\item[\\textit{" << tmp << "}] ";
			
			tmp = it->second;
			makeValidLatexString(tmp);
			out << tmp << std::endl;
		}
  	out << "\\end{list}" << std::endl;
  }
  out << "}";

	out << std::endl << std::endl;
//	std::clog << "done" << std::endl;
}

#if COIN_HAS_IPOPT
void printIpoptOptions() {
	Ipopt::SmartPtr<Ipopt::IpoptApplication> ipopt = new Ipopt::IpoptApplication();

 	// if we have linear rows and a quadratic objective, then the hessian of the Lag.func. is constant, and Ipopt can make use of this
// 	if (gmoNLM(gmo) == 0 && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp))
// 		ipopt->Options()->SetStringValue("hessian_constant", "yes", true, true);
// 	if (gmoSense(gmo) == Obj_Max)
// 		ipopt->Options()->SetNumericValue("obj_scaling_factor", -1., true, true);

	ipopt->RegOptions()->SetRegisteringCategory("Linear Solver");
	// add option to specify path to hsl library
	ipopt->RegOptions()->AddStringOption1("hsl_library", // name
			"path and filename of HSL library for dynamic load",  // short description
			"", // default value
			"*", // setting1
			"path (incl. filename) of HSL library", // description1
			"Specify the path to a library that contains HSL routines and can be load via dynamic linking. "
			"Note, that you still need to specify to use the corresponding routines (ma27, ...) by setting the corresponding options, e.g., ``linear_solver''."
	);
	// add option to specify path to pardiso library
	ipopt->RegOptions()->AddStringOption1("pardiso_library", // name
			"path and filename of PARDISO library for dynamic load",  // short description
			"", // default value
			"*", // setting1
			"path (incl. filename) of Pardiso library", // description1
			"Specify the path to a PARDISO library that and can be load via dynamic linking. "
			"Note, that you still need to specify to use pardiso as linear_solver."
	);
	

  const Ipopt::RegisteredOptions::RegOptionsList& optionlist(ipopt->RegOptions()->RegisteredOptionsList());
    
  // options sorted by category
  std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;
  
  for (Ipopt::RegisteredOptions::RegOptionsList::const_iterator it(optionlist.begin()); it!=optionlist.end(); ++it) {
  	std::string category(it->second->RegisteringCategory());
  	
  	if (category == "Undocumented" ||
  			category == "Uncategorized" ||
  			category == "" ||
  			category == "Derivative Checker")
  		continue;
  	
  	if (it->second->Name() == "hessian_constant" ||
  			it->second->Name() == "obj_scaling_factor" ||
  			it->second->Name() == "file_print_level" ||
  			it->second->Name() == "option_file_name" ||
  			it->second->Name() == "output_file" ||
  			it->second->Name() == "print_options_documentation" ||
  			it->second->Name() == "print_user_options" ||
  			it->second->Name() == "nlp_lower_bound_inf" ||
  			it->second->Name() == "nlp_upper_bound_inf" ||
  			it->second->Name() == "skip_finalize_solution_call")
  		continue;

		opts[category].push_back(it->second);
  }
  
  OPTTYPE opttype;
  OPTVAL defaultval, minval, maxval;
  bool minval_strict, maxval_strict;
  ENUMVAL enumval;
  std::string tmpstr;
  std::string longdescr;
  
  std::ofstream optfile("optipopt_a.tex"); 

  for (std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ!=opts.end(); ++it_categ) {
  	printOptionCategoryStart(optfile, it_categ->first);
  	
  	for (std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt!=it_categ->second.end(); ++it_opt) {
  		longdescr = (*it_opt)->LongDescription();
  		minval_strict = false;
  		maxval_strict = false;
  		switch ((*it_opt)->Type()) {
  			case Ipopt::OT_Number:
  				opttype = OPTTYPE_REAL;
  				minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
  				maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
  				//TODO should ask Bonmin for value for infinity
  				if (minval.realval == -1e+20)
  					minval.realval = -DBL_MAX;
  				if (maxval.realval ==  1e+20)
  					maxval.realval =  DBL_MAX;
  				defaultval.realval = (*it_opt)->DefaultNumber();
  		  	minval_strict = (*it_opt)->LowerStrict();
  		  	maxval_strict = (*it_opt)->UpperStrict();
  				break;
  				
  			case Ipopt::OT_Integer:
  				opttype = OPTTYPE_INTEGER;
  				minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
  				maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
  				defaultval.intval = (*it_opt)->DefaultInteger();
  				break;
  				
  			case Ipopt::OT_String: {
  				tmpstr = (*it_opt)->DefaultString();
  				defaultval.stringval = tmpstr.c_str();

  				const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->StringSettings());
  				if( settings.size() == 1 && settings[0].value_ == "*") {
  					opttype = OPTTYPE_STRING;
  					
  				} else {
  					opttype = OPTTYPE_ENUM;
  					if ((*it_opt)->Name() == "linear_solver") {
  						enumval.clear();
  						for (size_t j = 0; j < settings.size(); ++j) {
  							if (settings[j].value_ == "ma27" ||
  									settings[j].value_ == "ma57" ||
  									settings[j].value_ == "pardiso" ||
  									settings[j].value_ == "mumps") {
  								enumval.push_back(std::pair<std::string, std::string>(settings[j].value_, settings[j].description_));
  							}
  						}
  						longdescr = "Determines which linear algebra package is to be used for the solution of the augmented linear system (for obtaining the search directions). "
  							"Note, that in order to use MA27, MA57, or Pardiso, a library with HSL or Pardiso code need to be provided (see Section 5.2 and options \"hsl_library\" and \"pardiso_library\").";

  					} else {
  						enumval.resize(settings.size());
  						size_t offset = 0;
  						for (size_t j = 0; j < settings.size(); ++j) {
  							enumval[j-offset] = std::pair<std::string, std::string>(settings[j].value_, settings[j].description_);
  						}
  						if (offset)
  							enumval.resize(settings.size()-offset);
  					}
  				}

  				break;
  			}

  			case Ipopt::OT_Unknown:
  				std::cerr << "Skip option " << (*it_opt)->Name() << " of unknown type." << std::endl;
  				continue;
  		}

  		if ((*it_opt)->Name() == "bound_relax_factor")
  			defaultval.realval = 1e-10;
  		else if ((*it_opt)->Name() == "max_iter")
  			defaultval.intval = INT_MAX;
  		else if ((*it_opt)->Name() == "max_cpu_time")
  			defaultval.realval = 1000;
  		else if ((*it_opt)->Name() == "mu_strategy")
  			defaultval.stringval = "adaptive";

  		printOption(optfile, (*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
  				opttype, defaultval, minval, minval_strict, maxval, maxval_strict, enumval);
  	}
  }
  optfile.close();
}
#endif

#if COIN_HAS_BONMIN
void printBonminOptions() {
	Bonmin::BonminSetup bonmin_setup;

  SmartPtr<Bonmin::OptionsList> options = new Bonmin::OptionsList();
	SmartPtr<Ipopt::Journalist> journalist = new Ipopt::Journalist();
  SmartPtr<Bonmin::RegisteredOptions> regoptions = new Bonmin::RegisteredOptions();
	options->SetJournalist(journalist);
	options->SetRegisteredOptions(GetRawPtr(regoptions));

	bonmin_setup.setOptionsAndJournalist(regoptions, options, journalist);
  bonmin_setup.registerOptions();

  const Bonmin::RegisteredOptions::RegOptionsList& optionlist(regoptions->RegisteredOptionsList());
    
  // options sorted by category
  std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;
  
  for (Bonmin::RegisteredOptions::RegOptionsList::const_iterator it(optionlist.begin()); it!=optionlist.end(); ++it) {
  	std::string category(it->second->RegisteringCategory());

  	if (category.empty()) continue;
  	// skip ipopt options 
  	if (regoptions->categoriesInfo(category)==Bonmin::RegisteredOptions::IpoptCategory) continue;
  	if (regoptions->categoriesInfo(category)==Bonmin::RegisteredOptions::UndocumentedCategory) continue;

		if (it->second->Name()=="nlp_solver" ||
				it->second->Name()=="file_solution" ||
				it->second->Name()=="sos_constraints" ||
				it->second->Name()=="cplex_number_threads")
			continue;

  	if (category=="Bonmin ecp based strong branching")
  		category="ECP based strong branching";
  	if (category=="MILP cutting planes in hybrid")
  		category+=" algorithm (B-Hyb)";
  	if (category=="Nlp solution robustness")
  		category="NLP solution robustness";
  	if (category=="Nlp solve options in B-Hyb")
  		category="NLP solves in hybrid algorithm (B-Hyb)";
  	if (category=="Options for MILP subsolver in OA decomposition" || category=="Options for OA decomposition")
  		category="Outer Approximation Decomposition (B-OA)";
  	if (category=="Options for ecp cuts generation")
  		category="ECP cuts generation";
  	if (category=="Options for non-convex problems")
  		category="Nonconvex problems";
  	if (category=="Output and log-level options")
  		category="Output";
  	if (category=="nlp interface option")
  		category="NLP interface";
  	if (category=="Options for MILP solver")
  		category="MILP Solver";
  	if (category=="Options for feasibility checker using OA cuts")
  		category="Feasibility checker using OA cuts";
  	if (category=="Options for feasibility pump")
  		category="MINLP Heuristics";
  	if (category=="Test Heuristics") {
  		category="MINLP Heuristics";
  		continue;
  	}
  	if (category=="Undocumented MINLP Heuristics") {
  		category="MINLP Heuristics";
  		continue;
  	}
  	
		if (it->second->Name()=="oa_cuts_log_level" || 
				it->second->Name()=="nlp_log_level" || 
				it->second->Name()=="milp_log_level" ||
				it->second->Name()=="oa_log_level" ||
				it->second->Name()=="oa_log_frequency" ||
				it->second->Name()=="fp_log_level" ||
				it->second->Name()=="fp_log_frequency")
			category="Output";
		
		opts[category].push_back(it->second);
  }
  
  // print options table
  std::ofstream tabfile("optbonmin_s.tex");
  //Print table header
  tabfile<<"\\topcaption{\\label{tab:bonminoptions} "<<std::endl
    <<"List of options and compatibility with the different algorithms."<<std::endl
    <<"}"<<std::endl;
  tabfile<<"\\tablehead{\\hline "<<std::endl
    <<"Option & type &  default & {\\tt B-BB} & {\\tt B-OA} & {\\tt B-QG} & {\\tt B-Hyb} & {\\tt B-Ecp} & {\\tt B-iFP} & {\\tt Cbc\\_Par} \\\\"<<std::endl
    <<"\\hline}"<<std::endl;
  tabfile<<"\\tabletail{\\hline \\multicolumn{10}{|r|}{continued on next page}\\\\"
    <<"\\hline}"<<std::endl; 
  tabfile << "{\\small" << std::endl;
  tabfile << "\\begin{xtabular}{@{}|l|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{}}" << std::endl;

  for (std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ!=opts.end(); ++it_categ) {
  	if (it_categ != opts.begin())
  		tabfile <<"\\hline"<<std::endl;
    tabfile <<"\\multicolumn{1}{|c}{} & \\multicolumn{9}{l|}{"
      <<makeValidLatexString(it_categ->first.c_str())<<"}\\\\"<<std::endl
      <<"\\hline"<<std::endl;
    
    for (std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt!=it_categ->second.end(); ++it_opt) {
  		tabfile << makeValidLatexString((*it_opt)->Name().c_str()) << " & ";
     
  		std::string typestring;
  		std::string defaultval;
  		switch ((*it_opt)->Type()) {
  			case Ipopt::OT_Integer:
  				typestring = "$\\mathbb{Z}$";
  				defaultval = "$";
  				defaultval.append(makeValidLatexNumber((*it_opt)->DefaultInteger()));
  				defaultval.append("$");
  				break;
  			case Ipopt::OT_Number:
  				typestring = "$\\mathbb{Q}$";
  				defaultval = "$";
  				defaultval.append(makeValidLatexNumber((*it_opt)->DefaultNumber()));
  				defaultval.append("$");
  				break;
  			case Ipopt::OT_String:
  				typestring = "string";
  				defaultval = (*it_opt)->DefaultString();
  				makeValidLatexString(defaultval);
  				break;
  			case Ipopt::OT_Unknown: ;
  		}
  		
  		if ((*it_opt)->Name() == "nlp_log_at_root")
  			defaultval = makeValidLatexNumber(Ipopt::J_ITERSUMMARY);
  		else if ((*it_opt)->Name() == "allowable_gap")
  			defaultval = "\\MYGAMS \\texttt{optca}";
  		else if ((*it_opt)->Name() == "allowable_fraction_gap")
  			defaultval = "\\MYGAMS \\texttt{optcr}";
  		else if ((*it_opt)->Name() == "node_limit")
  			defaultval = "\\MYGAMS \\texttt{nodlim}";
  		else if ((*it_opt)->Name() == "time_limit")
  			defaultval = "\\MYGAMS \\texttt{reslim}";
  		else if ((*it_opt)->Name() == "cutoff")
  			defaultval = "\\MYGAMS \\texttt{cutoff}";
  		  		
  		tabfile << typestring << " & ";
  		tabfile << defaultval << " & ";
  		tabfile <<( (regoptions->isValidForBBB((*it_opt)->Name()))? "$\\checkmark$" : "--" )
       <<"& "<<( (regoptions->isValidForBOA((*it_opt)->Name()))? "$\\checkmark$" : "--" )
       <<"& "<<( (regoptions->isValidForBQG((*it_opt)->Name()))? "$\\checkmark$" : "--" )
       <<"& "<<( (regoptions->isValidForHybrid((*it_opt)->Name()))? "$\\checkmark$" : "--" )
       <<"& "<<( (regoptions->isValidForBEcp((*it_opt)->Name()))? "$\\checkmark$" : "--" )
       <<"& "<<( (regoptions->isValidForBiFP((*it_opt)->Name()))? "$\\checkmark$" : "--" )
       <<"& "<<( (regoptions->isValidForCbc((*it_opt)->Name()))? "$\\checkmark$" : "--" )
       <<"\\\\"<<std::endl;
    }
  }
  //Print table end
  tabfile <<"\\hline"<<std::endl
  	<<"\\end{xtabular}"<<std::endl;
  tabfile <<"}"<<std::endl;
  tabfile.close();
  
  OPTTYPE opttype;
  OPTVAL defaultval, minval, maxval;
  bool minval_strict, maxval_strict;
  ENUMVAL enumval;
  std::string tmpstr;
  
  std::ofstream optfile("optbonmin_a.tex"); 

  for (std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ!=opts.end(); ++it_categ) {
  	printOptionCategoryStart(optfile, it_categ->first);
  	
  	for (std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt!=it_categ->second.end(); ++it_opt) {
  		minval_strict = false;
  		maxval_strict = false;
  		switch ((*it_opt)->Type()) {
  			case Ipopt::OT_Number:
  				opttype = OPTTYPE_REAL;
  				minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
  				maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
  				//TODO should ask Bonmin for value for infinity
  				if (minval.realval == -1e+20)
  					minval.realval = -DBL_MAX;
  				if (maxval.realval ==  1e+20)
  					maxval.realval =  DBL_MAX;
  				defaultval.realval = (*it_opt)->DefaultNumber();
  		  	minval_strict = (*it_opt)->LowerStrict();
  		  	maxval_strict = (*it_opt)->UpperStrict();
  				break;
  				
  			case Ipopt::OT_Integer:
  				opttype = OPTTYPE_INTEGER;
  				minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
  				maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
  				defaultval.intval = (*it_opt)->DefaultInteger();
  				break;
  				
  			case Ipopt::OT_String: {
  				tmpstr = (*it_opt)->DefaultString();
  				defaultval.stringval = tmpstr.c_str();

  				const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->StringSettings());
  				if( settings.size() == 1 && settings[0].value_ == "*") {
  					opttype = OPTTYPE_STRING;
  					
  				} else {
  					opttype = OPTTYPE_ENUM;
  					enumval.resize(settings.size());
  					for( size_t j = 0; j < settings.size(); ++j ) {
  						if( settings[j].value_ == "Cplex" ) {
  							enumval.resize(j);
  							break;
  						}
  						enumval[j] = std::pair<std::string, std::string>(settings[j].value_, settings[j].description_);
  					}
  				}

  				break;
  			}

  			case Ipopt::OT_Unknown:
  				std::cerr << "Skip option " << (*it_opt)->Name() << " of unknown type." << std::endl;
  				continue;
  		}
  		
  		if ((*it_opt)->Name() == "nlp_log_at_root")
  			defaultval.intval = Ipopt::J_ITERSUMMARY;
  		else if ((*it_opt)->Name() == "allowable_fraction_gap")
  			defaultval.realval = 0.1;
  		else if ((*it_opt)->Name() == "time_limit")
  			defaultval.realval = 1000;

  		printOption(optfile, (*it_opt)->Name(), (*it_opt)->ShortDescription(), (*it_opt)->LongDescription(),
  				opttype, defaultval, minval, minval_strict, maxval, maxval_strict, enumval);
  	}
  }
  optfile.close();
}
#endif

#if COIN_HAS_COUENNE
void printCouenneOptions() {
  SmartPtr<Bonmin::OptionsList> options = new Bonmin::OptionsList();
	SmartPtr<Ipopt::Journalist> journalist = new Ipopt::Journalist();
  SmartPtr<Bonmin::RegisteredOptions> regoptions = new Bonmin::RegisteredOptions();
	options->SetJournalist(journalist);
	options->SetRegisteredOptions(GetRawPtr(regoptions));

	Bonmin::CouenneSetup::registerAllOptions(regoptions);

  const Bonmin::RegisteredOptions::RegOptionsList& optionlist(regoptions->RegisteredOptionsList());
    
  // options sorted by category
  std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;
  
  for (Bonmin::RegisteredOptions::RegOptionsList::const_iterator it(optionlist.begin()); it!=optionlist.end(); ++it) {
  	std::string category(it->second->RegisteringCategory());

  	if (category.empty()) continue;
  	// skip ipopt options 
  	if (regoptions->categoriesInfo(category)==Bonmin::RegisteredOptions::IpoptCategory) continue;
  	if (regoptions->categoriesInfo(category)==Bonmin::RegisteredOptions::BonminCategory) continue;
  	if (regoptions->categoriesInfo(category)==Bonmin::RegisteredOptions::UndocumentedCategory) continue;

		if (it->second->Name()=="lp_solver" ||
				it->second->Name()=="couenne_check" ||
				it->second->Name()=="test_mode")
			continue;

		opts[category].push_back(it->second);
  }
  
  OPTTYPE opttype;
  OPTVAL defaultval, minval, maxval;
  bool minval_strict, maxval_strict;
  ENUMVAL enumval;
  std::string tmpstr;
  
  std::ofstream optfile("optcouenne_a.tex"); 

  for (std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ!=opts.end(); ++it_categ) {
//  	printOptionCategoryStart(optfile, it_categ->first);
  	
  	for (std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt!=it_categ->second.end(); ++it_opt) {
  		minval_strict = false;
  		maxval_strict = false;
  		switch ((*it_opt)->Type()) {
  			case Ipopt::OT_Number:
  				opttype = OPTTYPE_REAL;
  				minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
  				maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
  				//TODO should ask Couenne for value for infinity
  				if (minval.realval == -1e+20)
  					minval.realval = -DBL_MAX;
  				if (maxval.realval ==  1e+20)
  					maxval.realval =  DBL_MAX;
  				defaultval.realval = (*it_opt)->DefaultNumber();
  		  	minval_strict = (*it_opt)->LowerStrict();
  		  	maxval_strict = (*it_opt)->UpperStrict();
  				break;
  				
  			case Ipopt::OT_Integer:
  				opttype = OPTTYPE_INTEGER;
  				minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
  				maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
  				defaultval.intval = (*it_opt)->DefaultInteger();
  				break;
  				
  			case Ipopt::OT_String: {
  				tmpstr = (*it_opt)->DefaultString();
  				defaultval.stringval = tmpstr.c_str();

  				const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->StringSettings());
  				if( settings.size() == 1 && settings[0].value_ == "*") {
  					opttype = OPTTYPE_STRING;
  					
  				} else {
  					opttype = OPTTYPE_ENUM;
  					enumval.resize(settings.size());
  					for( size_t j = 0; j < settings.size(); ++j )
  						enumval[j] = std::pair<std::string, std::string>(settings[j].value_, settings[j].description_);
  				}

  				break;
  			}

  			case Ipopt::OT_Unknown:
  				std::cerr << "Skip option " << (*it_opt)->Name() << " of unknown type." << std::endl;
  				continue;
  		}
  		
//  		if ((*it_opt)->Name() == "nlp_log_at_root")
//  			defaultval.intval = Ipopt::J_ITERSUMMARY;
//  		else if ((*it_opt)->Name() == "allowable_fraction_gap")
//  			defaultval.realval = 0.1;
//  		else if ((*it_opt)->Name() == "time_limit")
//  			defaultval.realval = 1000;

  		// 	options->SetIntegerValue("bonmin.problem_print_level", J_STRONGWARNING, true, true); /* otherwise Couenne prints the problem to stdout */ 
  		//	if (havetinyconst)
  		//    options->SetStringValue("delete_redundant", "no", "couenne.");

  		printOption(optfile, (*it_opt)->Name(), (*it_opt)->ShortDescription(), (*it_opt)->LongDescription(),
  				opttype, defaultval, minval, minval_strict, maxval, maxval_strict, enumval);
  	}
  }
  optfile.close();
}
#endif

#if COIN_HAS_SCIP
void printSCIPOptions() {
	SCIP* scip;
	
	SCIP_CALL_ABORT( SCIPcreate(&scip) );
	SCIP_CALL_ABORT( SCIPincludeDefaultPlugins(scip) );

	SCIP_CALL_ABORT( SCIPaddBoolParam(scip, "gams/names",            "This option causes GAMS names for the variables and equations to be loaded into SCIP. "
			"These names will then be used for error messages, log entries, and so forth. "
			"Turning names off may help if memory is very tight.",
			NULL, FALSE, FALSE, NULL, NULL) );
	/* SCIP_CALL( SCIPaddBoolParam(scip, "gams/solvefinal",       "whether the problem should be solved with fixed discrete variables to get dual values", NULL, FALSE, TRUE,  NULL, NULL) ); */
	SCIP_CALL_ABORT( SCIPaddBoolParam(scip, "gams/mipstart",         "This option controls the use of advanced starting values for mixed integer programs. "
			"A setting of TRUE indicates that the variable level values should be checked to see if they provide an integer feasible solution before starting optimization.",
			NULL, FALSE, TRUE,  NULL, NULL) );
	SCIP_CALL_ABORT( SCIPaddBoolParam(scip, "gams/print_statistics", "This option controls the printing of solve statistics after a MIP solve. "
			"Turning on this option indicates that statistics like the number of generated cuts of each type or the calls of heuristics are printed after the MIP solve.",
			NULL, FALSE, FALSE, NULL, NULL) );
	SCIP_CALL_ABORT( SCIPaddBoolParam(scip, "gams/interactive",      "whether a SCIP shell should be opened instead of issuing a solve command (this option is not available in demo mode)",
			NULL, FALSE, FALSE, NULL, NULL) );

	static std::map<std::string, std::string> categname;
	categname[" gams"] = "GAMS interface specific options";
	categname["branching"] = "Branching";
	categname["conflict"] = "Conflict analysis";
	categname["constraints"] = "Constraints";
	categname["display"] = "Output";
	categname["heuristics"] = "Heuristics";
	categname["limits"] = "Limits";
	categname["lp"] = "LP";
	categname["memory"] = "Memory";
	categname["misc"] = "Micellaneous";
	categname["nodeselection"] = "Node Selection";
	categname["numerics"] = "Tolerances";
	categname["presolving"] = "Presolving";
	categname["propagating"] = "Domain Propagation";
	categname["separating"] = "Separation";
	categname["timing"] = "Timing";
	
  SCIP_PARAM** params = SCIPgetParams(scip);
  int nparams = SCIPgetNParams(scip);
  SCIP_PARAM* param;
  
  std::map<std::string, std::list<SCIP_PARAM*> > paramsort;

  OPTTYPE opttype;
  OPTVAL defaultval, minval, maxval;
  ENUMVAL enumval;
  std::string tmpstr;
  
  std::ofstream optfile("optscip_a.tex"); 
  
  for (int i = 0; i < nparams; ++i) {
  	param = params[i];
  	if (SCIPparamIsAdvanced(param))
  		continue;
  	const char* paramname = SCIPparamGetName(param);
  	
  	if (strcmp(paramname, "numerics/infinity") == 0)
  		continue;
  	
  	char* catend = strchr(paramname, '/');
  	std::string category;
  	if (catend != NULL)
  		category = std::string(paramname, 0, catend - paramname);
  	else
  		category = "";

  	if (category == "reading" ||
  			category == "pricing" ||
  			category == "vbc")
  		continue;

  	if (category == "gams")
  		category = " gams";
  	
  	paramsort[category].push_back(param);
  }
  
  for (std::map<std::string, std::list<SCIP_PARAM*> >::iterator it_categ(paramsort.begin()); it_categ != paramsort.end(); ++it_categ ) {
  	if (!categname.count(it_categ->first))
  		std::cerr << "Do not have name for SCIP option category " << it_categ->first << std::endl;
  	printOptionCategoryStart(optfile, categname[it_categ->first]);

  	for( std::list<SCIP_PARAM*>::iterator it_param(it_categ->second.begin()); it_param != it_categ->second.end(); ++it_param) {
  		param = *it_param;
  		switch( SCIPparamGetType(param) ) {
  			case SCIP_PARAMTYPE_BOOL:
  				opttype = OPTTYPE_BOOL;
  				minval.boolval = 0;
  				maxval.boolval = 1;
  				defaultval.boolval = SCIPparamGetBoolDefault(param);
  				break;

  			case SCIP_PARAMTYPE_INT:
  				opttype = OPTTYPE_INTEGER;
  				minval.intval = SCIPparamGetIntMin(param);
  				maxval.intval = SCIPparamGetIntMax(param);
  				defaultval.intval = SCIPparamGetIntDefault(param);
  				break;

  			case SCIP_PARAMTYPE_LONGINT:
  				opttype = OPTTYPE_INTEGER;
  				minval.intval = LongintToInt(SCIPparamGetLongintMin(param));
  				maxval.intval = LongintToInt(SCIPparamGetLongintMax(param));
  				defaultval.intval = LongintToInt(SCIPparamGetLongintDefault(param));
  				break;

  			case SCIP_PARAMTYPE_REAL:
  				opttype = OPTTYPE_REAL;
  				minval.realval = SCIPparamGetRealMin(param);
  				maxval.realval = SCIPparamGetRealMax(param);
  				defaultval.realval = SCIPparamGetRealDefault(param);
  				if (SCIPisInfinity(scip, -minval.realval))
  					minval.realval = -DBL_MAX;
  				if (SCIPisInfinity(scip,  maxval.realval))
  					maxval.realval =  DBL_MAX;
  				if (SCIPisInfinity(scip, ABS(defaultval.realval)))
  					defaultval.realval = (defaultval.realval < 0 ? -1.0 : 1.0) * DBL_MAX;
  				break;

  			case SCIP_PARAMTYPE_CHAR:
  				opttype = OPTTYPE_CHAR;
  				defaultval.charval = SCIPparamGetCharDefault(param);
  				break;

  			case SCIP_PARAMTYPE_STRING:
  				opttype = OPTTYPE_STRING;
  				defaultval.stringval = SCIPparamGetStringDefault(param);
  				break;
  		}

  		printOption(optfile, SCIPparamGetName(param), SCIPparamGetDesc(param), SCIPparamIsAdvanced(param) ? "This is an advanced option." : "",
  				opttype, defaultval, minval, false, maxval, false, enumval);
  	}
  }
  
  optfile.close();
}
#endif

int main(int argc, char** argv) {

#if COIN_HAS_IPOPT
	printIpoptOptions();
#endif

#if COIN_HAS_BONMIN
	printBonminOptions();
#endif

#if COIN_HAS_COUENNE
	printCouenneOptions();
#endif

#if COIN_HAS_SCIP
	printSCIPOptions();
#endif
	
}
