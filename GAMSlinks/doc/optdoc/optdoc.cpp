// Copyright (C) GAMS Development and others 2010-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GAMSlinksConfig.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>

#ifdef COIN_HAS_IPOPT
#include "IpIpoptApplication.hpp"
#endif

#ifdef COIN_HAS_BONMIN
#include "BonBonminSetup.hpp"
#include "BonCbc.hpp"
#endif

#ifdef COIN_HAS_COUENNE
#include "BonCouenneSetup.hpp"
#include "BonCouenneInterface.hpp"
#endif

#ifdef COIN_HAS_SCIP
#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/pub_paramset.h"
#include "GamsScip.hpp"
#endif

enum OPTTYPE
{
   OPTTYPE_BOOL,
   OPTTYPE_INTEGER,
   OPTTYPE_REAL,
   OPTTYPE_CHAR,
   OPTTYPE_STRING,
   OPTTYPE_ENUM
};
typedef enum OPTTYPE OPTTYPE;

union OPTVAL
{
   bool                  boolval;
   int                   intval;
   double                realval;
   char                  charval;
   const char*           stringval;
};

typedef std::vector<std::pair<std::string, std::string> > ENUMVAL;

class GamsOptions
{
private:
   class Data
   {
   public:
      std::string        group;
      std::string        name;
      std::string        shortdescr;
      std::string        longdescr;
      OPTTYPE            type;
      OPTVAL             defaultval;
      OPTVAL             minval;
      OPTVAL             maxval;
      ENUMVAL            enumval;

      Data(
         const std::string& group_,
         const std::string& name_,
         const std::string& shortdescr_,
         const std::string& longdescr_,
         OPTTYPE            type_,
         OPTVAL             defaultval_,
         OPTVAL             minval_,
         OPTVAL             maxval_,
         const ENUMVAL&     enumval_
         )
      : group(group_),
        name(name_),
        shortdescr(shortdescr_),
        longdescr(longdescr_),
        type(type_),
        defaultval(defaultval_),
        minval(minval_),
        maxval(maxval_),
        enumval(enumval_)
      { }
   };

   std::list<Data>       data;
   std::set<std::string> groups;
   std::set<std::string> values;

   std::string           solver;
   std::string           curgroup;
   std::string           separator;
   std::string           stringquote;

   std::string tolower(std::string s)
   {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
   }

public:
   GamsOptions(
      const std::string& solver_
      )
   : solver(solver_), separator(" "), stringquote()
   { }

   void setGroup(
      const std::string& group
      )
   {
      curgroup = group;
      groups.insert(group);
   }
   
   void setSeparator(
      const std::string& sepa
      )
   {
      separator = sepa;
   }

   void setStringQuote(
      const std::string& strquote
      )
   {
      stringquote = strquote;
   }

   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      OPTTYPE            type,
      OPTVAL             defaultval,
      OPTVAL             minval,
      OPTVAL             maxval,
      const ENUMVAL&     enumval
   )
   {
      /* ignore options with number in beginning, because the GAMS options object cannot handle them so far */
//      if( isdigit(name[0]) )
//      {
//         std::cerr << "Warning: Ignoring " << solver << " option " << name << " for GAMS options file." << std::endl;
//         return;
//      }

      data.push_back(Data(curgroup, name, shortdescr, longdescr, type, defaultval, minval, maxval, enumval));

      /* replace all double quotes by single quotes */
      std::replace(data.back().shortdescr.begin(), data.back().shortdescr.end(), '"', '\'');

      switch( type )
      {
         case OPTTYPE_BOOL:
         case OPTTYPE_INTEGER:
         case OPTTYPE_REAL:
            break;

         case OPTTYPE_CHAR:
         {
            std::string str;
            str.push_back(defaultval.charval);
            values.insert(tolower(str));
            break;
         }

         case OPTTYPE_STRING:
         {
            if( defaultval.stringval[0] != '\0' )
               values.insert(tolower(defaultval.stringval));
            break;
         }

         case OPTTYPE_ENUM:
         {
            if( defaultval.stringval[0] != '\0' )
               values.insert(tolower(defaultval.stringval));

            for( std::vector<std::pair<std::string, std::string> >::iterator e(data.back().enumval.begin()); e != data.back().enumval.end(); ++e )
            {
               if( defaultval.stringval[0] != '\0' )
                  values.insert(tolower(e->first));

               /* replace all double quotes by single quotes */
               std::replace(e->second.begin(), e->second.end(), '"', '\'');
               if( e->second.length() >= 255 )
               {
                  size_t pos = e->second.rfind('.');
                  if( pos < std::string::npos )
                     e->second.erase(pos+1, e->second.length());
                  else
                     std::cerr << "Could not cut down description of enum value '" << e->first << "' of parameter " << name << ": " << e->second << std::endl;
               }
            }

            break;
         }
      }
   }


   void write()
   {
      std::string filename;

      filename = "opt" + solver + ".gms";
      std::ofstream f(filename.c_str());
      
      f << "$setglobal SEPARATOR \"" << separator << '"' << std::endl;
      if( stringquote == "\"" )
         f << "$setglobal STRINGQUOTE '\"'" << std::endl;
      else
         f << "$setglobal STRINGQUOTE \"" << stringquote << "\"" << std::endl;
      f << "$onempty" << std::endl;

      f << "set e / 1*100 " << std::endl;  // the 1*100 is necessary to get the long description into the html file
      for( std::set<std::string>::iterator v(values.begin()); v != values.end(); ++v )
         f << "  '" << *v << "'" << std::endl;
      f << "/;" << std::endl;

      f << "set f / def Default, lo Lower Bound, up Upper Bound, ref Reference /;" << std::endl;
      f << "set t / I Integer, R Real, S String, B Binary /;" << std::endl;
      //f << "set m / %system.gamsopt% /;" << std::endl;

      f << "set g Option Groups /" << std::endl;
      for( std::set<std::string>::iterator g(groups.begin()); g != groups.end(); ++g )
      {
         std::string id(*g);
         std::replace(id.begin(), id.end(), ' ', '_');
         std::replace(id.begin(), id.end(), '(', '_');
         std::replace(id.begin(), id.end(), ')', '_');
         std::replace(id.begin(), id.end(), '-', '_');
         std::replace(id.begin(), id.end(), '/', '_');
         f << "  gr_" << id << "   '" << *g << "'" << std::endl;
      }
      f << "/;" << std::endl;

      f << "set o Solver and Link Options with one-line description /" << std::endl;
      for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
         f << "  '" << d->name << "'  \"" << d->shortdescr << '"' << std::endl;
      f << "/;" << std::endl;

      f << "$onembedded" << std::endl;
      f << "set optdata(g,o,t,f) /" << std::endl;
      for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
      {
         std::string id(d->group);
         std::replace(id.begin(), id.end(), ' ', '_');
         std::replace(id.begin(), id.end(), '(', '_');
         std::replace(id.begin(), id.end(), ')', '_');
         std::replace(id.begin(), id.end(), '-', '_');
         std::replace(id.begin(), id.end(), '/', '_');
         f << "  gr_" << id << ".'" << d->name << "'.";
         switch( d->type )
         {
            case OPTTYPE_BOOL:
               f << "B.(def " << d->defaultval.boolval << ")";
               break;

            case OPTTYPE_INTEGER:
               f << "I.(def ";
               if( d->defaultval.intval == INT_MAX )
                  f << "maxint";
               else if( d->defaultval.intval == -INT_MAX )
                  f << "minint";
               else
                  f << d->defaultval.intval;
               if( d->minval.intval != -INT_MAX )
                  f << ", lo " << d->minval.intval;
               else
                  f << ", lo minint";
               if( d->maxval.intval !=  INT_MAX )
                  f << ", up " << d->maxval.intval;
               f << ")";
               break;

            case OPTTYPE_REAL:
               f << "R.(def ";
               if( d->defaultval.realval == DBL_MAX )
                  f << "maxdouble";
               else if( d->defaultval.realval == -DBL_MAX )
                  f << "mindouble";
               else
                  f << d->defaultval.realval;
               if( d->minval.realval != -DBL_MAX )
                  f << ", lo " << d->minval.realval;
               else
                  f << ", lo mindouble";
               if( d->maxval.realval !=  DBL_MAX )
                  f << ", up " << d->maxval.realval;
               f << ")";
               break;

            case OPTTYPE_CHAR:
               /* no character type in GAMS option files */
               f << "S.(def '" << d->defaultval.charval << "')";
               break;

            case OPTTYPE_STRING:
            case OPTTYPE_ENUM:
               f << "S.(def '" << d->defaultval.stringval << "')";
               break;
         }
         f << std::endl;
      }
      f << "/;" << std::endl;

      f << "set oe(o,e) /" << std::endl;
      for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
      {
         if( d->type != OPTTYPE_ENUM )
            continue;

         for( std::vector<std::pair<std::string, std::string> >::iterator e(d->enumval.begin()); e != d->enumval.end(); ++e )
            f << "  '" << d->name << "'.'" << e->first << "'  \"" << e->second << '"' << std::endl;
      }
      f << "/;" << std::endl;


      f << "$onempty" << std::endl
        << "set os(o,*) synonyms  / /;" << std::endl
        << "set im immediates recognized  / EolFlag , Message /;" << std::endl   /* ???? */
        << "set strlist(o)        / /;" << std::endl
        << "set immediate(o,im)   / /;" << std::endl
        << "set multilist(o,o)    / /;" << std::endl
        << "set indicator(o)      / /;" << std::endl
        << "set dotoption(o)      / /;" << std::endl
        << "set dropdefaults(o)   / /;" << std::endl
        << "set deprecated(*,*)   / /;" << std::endl
        << "set ooverwrite(o,f)   / /;" << std::endl
        << "set orange(o)         / /;" << std::endl
        << "set odefault(o)       / /;" << std::endl
        << "set oedepr(o,e)       / /;" << std::endl  /* deprecated enum options */
        << "set oep(o)            / /;" << std::endl  /* enum options for documentation only */
        << "set olink(o)          / /;" << std::endl
        //<< "set om(o,m)           / /;" << std::endl  /* option with GAMS synonym and GAMS initialization */
        << "$offempty" << std::endl;


      f << "set orange(o);" << std::endl
        << "orange(o) = (sum(g,optdata(g,o,'R','def')) or sum(g,optdata(g,o,'I','def'))) and sum(oe(o,e),1)=0 "
        << "and sum((g,t),optdata(g,o,t,'up')) and sum((g,t),optdata(g,o,t,'lo')) and not ooverwrite(o,'lo') and not ooverwrite(o,'up');"
        << std::endl;

      f << "optdata(g,o,t,f)$optdata(g,o,t,'def') $= ooverwrite(o,f);" << std::endl;

      f.close();


      filename = "opt" + solver + ".txt_";
      f.open(filename.c_str());
      for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
      {
         f << d->name << std::endl;
         f << d->longdescr << std::endl;
         f << std::endl << std::endl;
      }

      f.close();

      std::string foldcall = "fold -s "+filename+" > opt"+solver+".txt";
      system(foldcall.c_str());
      remove(filename.c_str());
   }

};


using namespace Ipopt;

static
void makeValidLatexString(
   std::string&          str
)
{
   int numdollar = 0;
   for( size_t i = 0; i < str.length(); )
   {
      if( str[i] == '$' )
         ++numdollar;
      if( str[i] == '_' || str[i] == '^' || str[i] == '#' || str[i] == '&' )
      {
         str.insert(i, "\\");
         i += 2;
      }
      else if( (str[i] == '<' || str[i] == '>' || str[i] == '|') && (numdollar % 2 == 0) )
      {
         str.insert(i, "$");
         str.insert(i+2, "$");
         i += 2;
      }
      else if( str[i] == '\n' )
      {
         str[i] = '\\';
         str.insert(i, "\\");
         i += 2;
      }
      ++i;
   }
   //TODO could also substitute greek letters
}

static
std::string makeValidLatexString(
   const char*           str
)
{
   std::string tmp = str;
   makeValidLatexString(tmp);

   return tmp;
}

static
std::string makeValidLatexNumber(
   double                value
)
{
   if( value ==  DBL_MAX )
      return "\\infty";
   if( value == -DBL_MAX )
      return "-\\infty";

   char buffer[256];
   sprintf(buffer, "%g", value);
   std::string str = buffer;

   size_t epos = str.find("e");
   if( epos != std::string::npos )
   {
      if( str[epos+1] == '+' )
         str[epos+1] = ' ';
      if( str[epos+2] == '0' )
         str[epos+2] = ' ';
      if( epos == 1 && str[0] == '1' ) // number is 1e...
         str.replace(0, 2, "10^{");
      else if( epos == 2 && str[0] == '-' && str[1] == '1' )  // number is -1e...
         str.replace(1, 2, "10^{");
      else // number is ...e...
         str.replace(epos, 1, " \\cdot 10^{");
      str.append("}");
   }

   return str;
}

static
std::string makeValidLatexNumber(
   int                   value
)
{
   if( value ==  INT_MAX )
      return "\\infty";
   if( value == -INT_MAX )
      return "-\\infty";

   char buffer[256];
   sprintf(buffer, "%d", value);

   return buffer;
}

static
int ScipLongintToInt(
   SCIP_Longint              value
)
{
   if( value >=  SCIP_LONGINT_MAX )
      return INT_MAX;
   if( value <= -SCIP_LONGINT_MAX )
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

static
void printOptionCategoryStart(
   std::ostream&         out,
   std::string           name
)
{
   makeValidLatexString(name);
   name = name.substr(name.find_first_not_of(" "));
   out << "\\printoptioncategory{" << name << "}" << std::endl;
}

static
void printOption(
   std::ostream&         out,
   std::string           name,
   std::string           shortdescr,
   std::string           longdescr,
   OPTTYPE               type,
   OPTVAL&               def,
   OPTVAL&               min,
   bool                  min_strict,
   OPTVAL&               max,
   bool                  max_strict,
   const ENUMVAL&        enumvals = ENUMVAL(),
   bool                  isSCIP = false
)
{
   //std::clog << "print SCIP option " << name << "...";

   // some preprocessing
   switch( type )
   {
      case OPTTYPE_INTEGER:
         if( min_strict && min.intval != -INT_MAX )
         {
            min.intval++;
            min_strict = false;
         }
         if( max_strict && max.intval !=  INT_MAX )
         {
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

   switch( type )
   {
      case OPTTYPE_BOOL:
      {
         out << "{";
         //			if (min.boolval == max.boolval)
         //				out << "\\{" << min.boolval << "\\} ";
         //			else
         //				out << "\\{" << min.boolval << ", " << max.boolval << "\\} ";
         out << "boolean";
         out << "}%" << std::endl;
         if( isSCIP )
            out << "{" << (def.boolval ? "TRUE" : "FALSE") << "}%" << std::endl;
         else
            out << "{" << def.boolval << "}%" << std::endl;
         break;
      }

      case OPTTYPE_INTEGER:
      {
         out << "{$";
         if( min.intval > -INT_MAX )
            out << makeValidLatexNumber(min.intval) << "\\leq";
         out << "\\textrm{integer}";
         if( max.intval < INT_MAX )
            out << "\\leq" << makeValidLatexNumber(max.intval);
         out << "$}%" << std::endl;
         out << "{$" << makeValidLatexNumber(def.intval) << "$}%" << std::endl;
         break;
      }

      case OPTTYPE_REAL:
      {
         out << "{$";
         if( min.realval > -DBL_MAX )
            out << makeValidLatexNumber(min.realval) << (min_strict ? "<" : "\\leq");
         out << "\\textrm{real}";
         if( max.realval < DBL_MAX )
            out << (max_strict ? "<" : "\\leq") << makeValidLatexNumber(max.realval);
         out << "$}%" << std::endl;
         out << "{$" << makeValidLatexNumber(def.realval) << "$}%" << std::endl;
         break;
      }

      case OPTTYPE_CHAR:
      {
         out << "{character}%" << std::endl;
         out << "{" << def.charval << "}%" << std::endl;
         break;
      }

      case OPTTYPE_STRING:
      {
         out << "{string}%" << std::endl;
         out << "{" << makeValidLatexString(def.stringval) << "}%" << std::endl;
         break;
      }

      case OPTTYPE_ENUM:
      {
         std::string tmp;
         out << "{\\ttfamily ";
         for( ENUMVAL::const_iterator it(enumvals.begin()); it != enumvals.end(); ++it )
         {
            if( it != enumvals.begin() )
               out << ", ";
            tmp = it->first;
            makeValidLatexString(tmp);
            out << tmp;
            if( it->second != "" )
               have_enum_docu = true;
         }
         out << "}%" << std::endl;
         out << "{" << makeValidLatexString(def.stringval) << "}%" << std::endl;
         break;
      }
   }

   out << "{" << shortdescr;

   if( longdescr != "" )
   {
      out << "\\\\" << std::endl;
      makeValidLatexString(longdescr);
      out << longdescr;
   }
   out << "}%" << std::endl;

   out << "{";
   if( type == OPTTYPE_ENUM && have_enum_docu )
   {
      std::string tmp;
      out << "\\begin{list}{}{" << std::endl;
      out << "\\setlength{\\parsep}{0em}" << std::endl;
      out << "\\setlength{\\leftmargin}{5ex}" << std::endl;
      out << "\\setlength{\\labelwidth}{2ex}" << std::endl;
      out << "\\setlength{\\itemindent}{0ex}" << std::endl;
      out << "\\setlength{\\topsep}{0pt}}" << std::endl;
      for( ENUMVAL::const_iterator it(enumvals.begin()); it != enumvals.end(); ++it )
      {
         tmp = it->first;
         makeValidLatexString(tmp);
         out << "\\item[\\texttt{" << tmp << "}] ";

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

#ifdef COIN_HAS_IPOPT
static
void printIpoptOptions()
{
   Ipopt::SmartPtr<Ipopt::IpoptApplication> ipopt = new Ipopt::IpoptApplication();

   // if we have linear rows and a quadratic objective, then the hessian of the Lag.func. is constant, and Ipopt can make use of this
   // 	if (gmoNLM(gmo) == 0 && (gmoModelType(gmo) == Proc_qcp || gmoModelType(gmo) == Proc_rmiqcp))
   // 		ipopt->Options()->SetStringValue("hessian_constant", "yes", true, true);
   // 	if (gmoSense(gmo) == Obj_Max)
   // 		ipopt->Options()->SetNumericValue("obj_scaling_factor", -1., true, true);


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
          it->second->Name() == "skip_finalize_solution_call" ||
          it->second->Name() == "warm_start_entire_iterate" ||
          it->second->Name() == "warm_start_same_structure"
        )
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
   GamsOptions gmsopt("ipopt");

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      printOptionCategoryStart(optfile, it_categ->first);
      gmsopt.setGroup(it_categ->first);

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         longdescr = (*it_opt)->LongDescription();
         minval_strict = false;
         maxval_strict = false;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Number:
            {
               opttype = OPTTYPE_REAL;
               minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
               maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
               //TODO should ask Ipopt for value for infinity
               if( minval.realval == -1e+20 )
                  minval.realval = -DBL_MAX;
               if( maxval.realval ==  1e+20 )
                  maxval.realval =  DBL_MAX;
               defaultval.realval = (*it_opt)->DefaultNumber();
               minval_strict = (*it_opt)->HasLower() ? (*it_opt)->LowerStrict() : false;
               maxval_strict = (*it_opt)->HasUpper() ? (*it_opt)->UpperStrict() : false;
               break;
            }

            case Ipopt::OT_Integer:
            {
               opttype = OPTTYPE_INTEGER;
               minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
               maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
               defaultval.intval = (*it_opt)->DefaultInteger();
               break;
            }

            case Ipopt::OT_String:
            {
               defaultval.stringval = (*it_opt)->DefaultString().c_str();

               const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->GetValidStrings());
               if( settings.size() == 1 && settings[0].value_ == "*")
               {
                  opttype = OPTTYPE_STRING;
               }
               else
               {
                  opttype = OPTTYPE_ENUM;
                  if( (*it_opt)->Name() == "linear_solver" )
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
                        "If no GAMS/IpoptH license is available, the default linear solver is MUMPS. "
                        "Pardiso is only available on Linux and Windows systems. "
                        "For using Pardiso on non-Linux/Windows systems or MA77, a Pardiso or HSL library need to be provided.";

                     defaultval.stringval = "ma27";
                  }
                  else
                  {
                     if( (*it_opt)->Name() == "linear_system_scaling" )
                        longdescr = "Determines the method used to compute symmetric scaling factors for the augmented system (see also the \"linear_scaling_on_demand\" option).  This scaling is independent of the NLP problem scaling.  By default, MC19 is only used if MA27 or MA57 are selected as linear solvers. "
                           "Note, that MC19 is only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately. "
                           "If no commerical GAMS/IpoptH license is available, the default scaling method is slack-based.";

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

         if( (*it_opt)->Name() == "bound_relax_factor" )
            defaultval.realval = 1e-10;
         else if( (*it_opt)->Name() == "max_iter" )
            defaultval.intval = INT_MAX;
         else if( (*it_opt)->Name() == "max_cpu_time" )
            defaultval.realval = 1000;
         else if( (*it_opt)->Name() == "mu_strategy" )
            defaultval.stringval = "adaptive";
         else if( (*it_opt)->Name() == "ma86_order" )
            defaultval.stringval = "auto";
         else if( (*it_opt)->Name() == "nlp_scaling_method" )
         {
            for( ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "user-scaling" )
               {
                  enumval.erase(it);
                  break;
               }
         }
         else if( (*it_opt)->Name() == "dependency_detector" )
         {
            for( ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "wsmp" )
               {
                  enumval.erase(it);
                  break;
               }
         }


         printOption(optfile, (*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, minval_strict, maxval, maxval_strict, enumval);

         gmsopt.collect((*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, enumval);
      }
   }

   optfile.close();
   gmsopt.write();
}
#endif

#if COIN_HAS_BONMIN
static
void printBonminOptions()
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

   const Bonmin::RegisteredOptions::RegOptionsList& optionlist(regoptions->RegisteredOptionsList());

   // options sorted by category
   std::map<std::string, std::list<SmartPtr<RegisteredOption> > > opts;
   GamsOptions gmsopt("bonmin");

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
          it->second->Name() == "skip_finalize_solution_call" ||
          it->second->Name() == "warm_start_entire_iterate" ||
          it->second->Name() == "warm_start_same_structure"
        )
         continue;


      opts[category].push_back(it->second);
   }

   // print options table
   std::ofstream tabfile("optbonmin_s.tex");
   //Print table header
   tabfile << "\\topcaption{\\label{tab:bonminoptions} " << std::endl
      << "List of options and compatibility with the different algorithms." << std::endl
      << "}" << std::endl;
   tabfile << "\\tablehead{\\hline " << std::endl
      << "Option & type &  default & {\\tt B-BB} & {\\tt B-OA} & {\\tt B-QG} & {\\tt B-Hyb} & {\\tt B-Ecp} & {\\tt B-iFP} & {\\tt Cbc\\_Par} \\\\" << std::endl
      << "\\hline}" << std::endl;
   tabfile << "\\tabletail{\\hline \\multicolumn{10}{|r|}{continued on next page}\\\\"
      << "\\hline}" << std::endl;
   tabfile << "{\\small" << std::endl;
   tabfile << "\\begin{xtabular}{@{}|l|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{\\;}r@{\\;}|@{}}" << std::endl;

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      if( it_categ->first.find("Ipopt") == 0 )
         continue;

      if( it_categ != opts.begin() )
         tabfile << "\\hline" << std::endl;
      tabfile << "\\multicolumn{1}{|c}{} & \\multicolumn{9}{l|}{"
         << makeValidLatexString(it_categ->first.c_str()) << "}\\\\" << std::endl
         << "\\hline" << std::endl;

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         tabfile << makeValidLatexString((*it_opt)->Name().c_str()) << " & ";

         std::string typestring;
         std::string defaultval;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Integer:
            {
               typestring = "$\\mathbb{Z}$";
               defaultval = "$";
               defaultval.append(makeValidLatexNumber((*it_opt)->DefaultInteger()));
               defaultval.append("$");
               break;
            }

            case Ipopt::OT_Number:
            {
               typestring = "$\\mathbb{Q}$";
               defaultval = "$";
               defaultval.append(makeValidLatexNumber((*it_opt)->DefaultNumber()));
               defaultval.append("$");
               break;
            }

            case Ipopt::OT_String:
            {
               typestring = "string";
               defaultval = (*it_opt)->DefaultString();
               makeValidLatexString(defaultval);
               break;
            }

            case Ipopt::OT_Unknown: ;
         }

         if( (*it_opt)->Name() == "nlp_log_at_root" )
            defaultval = makeValidLatexNumber(Ipopt::J_ITERSUMMARY);
         else if( (*it_opt)->Name() == "allowable_gap" )
            defaultval = "\\GAMS \\texttt{optca}";
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
            defaultval = "\\GAMS \\texttt{optcr}";
         else if( (*it_opt)->Name() == "node_limit" )
            defaultval = "\\GAMS \\texttt{nodlim}";
         else if( (*it_opt)->Name() == "time_limit" )
            defaultval = "\\GAMS \\texttt{reslim}";
         else if( (*it_opt)->Name() == "iteration_limit" )
            defaultval = "\\GAMS \\texttt{iterlim}";
         else if( (*it_opt)->Name() == "cutoff" )
            defaultval = "\\GAMS \\texttt{cutoff}";

         tabfile << typestring << " & ";
         tabfile << defaultval << " & ";
         tabfile  << ( (regoptions->isValidForBBB((*it_opt)->Name()))    ? "$\\checkmark$" : "--" )
          << "& " << ( (regoptions->isValidForBOA((*it_opt)->Name()))    ? "$\\checkmark$" : "--" )
          << "& " << ( (regoptions->isValidForBQG((*it_opt)->Name()))    ? "$\\checkmark$" : "--" )
          << "& " << ( (regoptions->isValidForHybrid((*it_opt)->Name())) ? "$\\checkmark$" : "--" )
          << "& " << ( (regoptions->isValidForBEcp((*it_opt)->Name()))   ? "$\\checkmark$" : "--" )
          << "& " << ( (regoptions->isValidForBiFP((*it_opt)->Name()))   ? "$\\checkmark$" : "--" )
          << "& " << ( (regoptions->isValidForCbc((*it_opt)->Name()))    ? "$\\checkmark$" : "--" )
          << "\\\\" << std::endl;
      }
   }
   //Print table end
   tabfile << "\\hline" << std::endl
      << "\\end{xtabular}" << std::endl;
   tabfile << "}" << std::endl;
   tabfile.close();

   OPTTYPE opttype;
   OPTVAL defaultval, minval, maxval;
   bool minval_strict, maxval_strict;
   ENUMVAL enumval;
   std::string tmpstr;
   std::string longdescr;

   std::ofstream optfile("optbonmin_a.tex");

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      if( it_categ->first.find("Ipopt") != 0 )
         printOptionCategoryStart(optfile, it_categ->first);
      gmsopt.setGroup(it_categ->first);

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         minval_strict = false;
         maxval_strict = false;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Number:
            {
               opttype = OPTTYPE_REAL;
               minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
               maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
               //TODO should ask Bonmin for value for infinity
               if( minval.realval == -1e+20 )
                  minval.realval = -DBL_MAX;
               if( maxval.realval ==  1e+20 )
                  maxval.realval =  DBL_MAX;
               defaultval.realval = (*it_opt)->DefaultNumber();
               minval_strict = (*it_opt)->HasLower() ? (*it_opt)->LowerStrict() : false;
               maxval_strict = (*it_opt)->HasUpper() ? (*it_opt)->UpperStrict() : false;
               break;
            }

            case Ipopt::OT_Integer:
            {
               opttype = OPTTYPE_INTEGER;
               minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
               maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
               defaultval.intval = (*it_opt)->DefaultInteger();
               break;
            }

            case Ipopt::OT_String:
            {
               tmpstr = (*it_opt)->DefaultString();
               defaultval.stringval = tmpstr.c_str();

               const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->GetValidStrings());
               if( settings.size() == 1 && settings[0].value_ == "*")
               {
                  opttype = OPTTYPE_STRING;
               }
               else
               {
                  opttype = OPTTYPE_ENUM;
                  if( (*it_opt)->Name() == "linear_solver" )
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
                        "If no GAMS/IpoptH license is available, the default linear solver is MUMPS. "
                        "Pardiso is only available on Linux and Windows systems. "
                        "For using Pardiso on non-Linux/Windows systems or MA77, a Pardiso or HSL library need to be provided.";

                     defaultval.stringval = "ma27";
                  }
                  else
                  {
                     if( (*it_opt)->Name() == "linear_system_scaling" )
                        longdescr = "Determines the method used to compute symmetric scaling factors for the augmented system (see also the \"linear_scaling_on_demand\" option).  This scaling is independent of the NLP problem scaling.  By default, MC19 is only used if MA27 or MA57 are selected as linear solvers. "
                           "Note, that MC19 is only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately. "
                           "If no commerical GAMS/IpoptH license is available, the default scaling method is slack-based.";

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

         // Bonmin options
         if( (*it_opt)->Name() == "nlp_log_at_root" )
            defaultval.intval = Ipopt::J_ITERSUMMARY;
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
            defaultval.realval = 0.1;
         else if( (*it_opt)->Name() == "time_limit" )
            defaultval.realval = 1000;
         else if( it_categ->first == " MILP cutting planes in hybrid algorithm" && (*it_opt)->Name() != "2mir_cuts" )
            longdescr = "See option \\texttt{2mir_cuts} for a detailed description.";
         else if( (*it_opt)->Name() == "milp_solver" )
            longdescr = "To use Cplex, a valid license is required.";
         else if( (*it_opt)->Name() == "resolve_on_small_infeasibility" )
            longdescr = "";
         // Ipopt options
         else if( (*it_opt)->Name() == "bound_relax_factor" )
            defaultval.realval = 1e-10;
         else if( (*it_opt)->Name() == "mu_strategy" )
            defaultval.stringval = "adaptive";
         else if( (*it_opt)->Name() == "mu_oracle" )
            defaultval.stringval = "probing";
         else if( (*it_opt)->Name() == "gamma_phi" )
            defaultval.realval = 1e-8;
         else if( (*it_opt)->Name() == "gamma_theta" )
            defaultval.realval = 1e-4;
         else if( (*it_opt)->Name() == "required_infeasibility_reduction" )
            defaultval.realval = 0.1;
         else if( (*it_opt)->Name() == "expect_infeasible_problem" )
            defaultval.stringval = "yes";
         else if( (*it_opt)->Name() == "warm_start_init_point" )
            defaultval.stringval = "yes";
         else if( (*it_opt)->Name() == "print_level" )
            defaultval.intval = 0;
         else if( (*it_opt)->Name() == "print_frequency_time" )
            defaultval.realval = 0.5;
         else if( (*it_opt)->Name() == "ma86_order" )
            defaultval.stringval = "auto";
         else if( (*it_opt)->Name() == "nlp_scaling_method" )
         {
            for( ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "user-scaling" )
               {
                  enumval.erase(it);
                  break;
               }
         }
         else if( (*it_opt)->Name() == "dependency_detector" )
         {
            for( ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "wsmp" )
               {
                  enumval.erase(it);
                  break;
               }
         }

         if( it_categ->first.find("Ipopt") != 0 )
            printOption(optfile, (*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
               opttype, defaultval, minval, minval_strict, maxval, maxval_strict, enumval);

         gmsopt.collect((*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, enumval);
      }
   }

   optfile.close();
   gmsopt.write();
}
#endif

#if COIN_HAS_COUENNE
static
void printCouenneOptions()
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
          it->second->Name()=="test_mode" )
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
          it->second->Name() == "skip_finalize_solution_call" ||
          it->second->Name() == "warm_start_entire_iterate" ||
          it->second->Name() == "warm_start_same_structure"
        )
         continue;

      opts[category].push_back(it->second);
   }

   OPTTYPE opttype;
   OPTVAL defaultval, minval, maxval;
   bool minval_strict, maxval_strict;
   ENUMVAL enumval;
   std::string tmpstr;
   std::string longdescr;

   GamsOptions gmsopt("couenne");

   std::ofstream optfile("optcouenne_a.tex");

   for( std::map<std::string, std::list<SmartPtr<RegisteredOption> > >::iterator it_categ(opts.begin()); it_categ != opts.end(); ++it_categ )
   {
      // printOptionCategoryStart(optfile, it_categ->first);
      gmsopt.setGroup(it_categ->first);

      for( std::list<SmartPtr<RegisteredOption> >::iterator it_opt(it_categ->second.begin()); it_opt != it_categ->second.end(); ++it_opt )
      {
         minval_strict = false;
         maxval_strict = false;
         switch( (*it_opt)->Type() )
         {
            case Ipopt::OT_Number:
            {
               opttype = OPTTYPE_REAL;
               minval.realval = (*it_opt)->HasLower() ? (*it_opt)->LowerNumber() : -DBL_MAX;
               maxval.realval = (*it_opt)->HasUpper() ? (*it_opt)->UpperNumber() :  DBL_MAX;
               //TODO should ask Couenne for value for infinity
               if( minval.realval == -1e+20 )
                  minval.realval = -DBL_MAX;
               if( maxval.realval ==  1e+20 )
                  maxval.realval =  DBL_MAX;
               defaultval.realval = (*it_opt)->DefaultNumber();
               minval_strict = (*it_opt)->HasLower() ? (*it_opt)->LowerStrict() : false;
               maxval_strict = (*it_opt)->HasUpper() ? (*it_opt)->UpperStrict() : false;
               break;
            }

            case Ipopt::OT_Integer:
            {
               opttype = OPTTYPE_INTEGER;
               minval.intval = (*it_opt)->HasLower() ? (*it_opt)->LowerInteger() : -INT_MAX;
               maxval.intval = (*it_opt)->HasUpper() ? (*it_opt)->UpperInteger() :  INT_MAX;
               defaultval.intval = (*it_opt)->DefaultInteger();
               break;
            }

            case Ipopt::OT_String:
            {
               tmpstr = (*it_opt)->DefaultString();
               defaultval.stringval = tmpstr.c_str();

               const std::vector<Ipopt::RegisteredOption::string_entry>& settings((*it_opt)->GetValidStrings());
               if( settings.size() == 1 && settings[0].value_ == "*")
               {
                  opttype = OPTTYPE_STRING;
               }
               else
               {
                  opttype = OPTTYPE_ENUM;
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
                        "If no GAMS/IpoptH license is available, the default linear solver is MUMPS. "
                        "Pardiso is only available on Linux and Windows systems. "
                        "For using Pardiso on non-Linux/Windows systems or MA77, a Pardiso or HSL library need to be provided.";

                     defaultval.stringval = "ma27";
                  }
                  else
                  {
                     if( (*it_opt)->Name() == "linear_system_scaling" )
                        longdescr = "Determines the method used to compute symmetric scaling factors for the augmented system (see also the \"linear_scaling_on_demand\" option).  This scaling is independent of the NLP problem scaling.  By default, MC19 is only used if MA27 or MA57 are selected as linear solvers. "
                           "Note, that MC19 is only available with a commercially supported GAMS/IpoptH license, or when the user provides a library with HSL code separately. "
                           "If no commerical GAMS/IpoptH license is available, the default scaling method is slack-based.";

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
            longdescr = "See option \\texttt{2mir_cuts} for the meaning of k.";
         else if( (*it_opt)->Name().find("branch_pt_select_") == 0 )
            longdescr = longdescr + "Default is to use the value of \\texttt{branch_pt_select} (value \\texttt{common}).";
         else if( (*it_opt)->Name() == "feas_pump_usescip" )
            longdescr = "Note, that SCIP is only available for GAMS users with an academic GAMS license.";
         // Bonmin options
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
            defaultval.realval = 0.1;
         else if( (*it_opt)->Name() == "time_limit" )
            defaultval.realval = 1000;
         else if( (*it_opt)->Name() == "problem_print_level" )
            defaultval.intval = Ipopt::J_STRONGWARNING;
         else if( it_categ->first == "Bonmin MILP cutting planes in hybrid algorithm" && (*it_opt)->Name() != "2mir_cuts" )
            longdescr = "See option \\texttt{2mir_cuts} for a detailed description.";
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
            for( ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "user-scaling" )
               {
                  enumval.erase(it);
                  break;
               }
         }
         else if( (*it_opt)->Name() == "dependency_detector" )
         {
            for( ENUMVAL::iterator it(enumval.begin()); it != enumval.end(); ++it )
               if( it->first == "wsmp" )
               {
                  enumval.erase(it);
                  break;
               }
         }

         if( it_categ->first.find("Ipopt") != 0 && it_categ->first.find("Bonmin") != 0 )
            printOption(optfile, (*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
               opttype, defaultval, minval, minval_strict, maxval, maxval_strict, enumval);

         gmsopt.collect((*it_opt)->Name(), (*it_opt)->ShortDescription(), longdescr,
            opttype, defaultval, minval, maxval, enumval);
      }
   }
   optfile.close();
   gmsopt.write();
}
#endif

#if COIN_HAS_SCIP
bool ScipParamCompare(SCIP_PARAM* a, SCIP_PARAM* b)
{
   /* move advanced parameters to end */
   if( SCIPparamIsAdvanced(a) == SCIPparamIsAdvanced(b) )
   {
#if 0
      int nslasha = 0;
      for( const char* c = SCIPparamGetName(a); *c ; ++c )
         if( *c == '/' )
            ++nslasha;
      int nslashb = 0;
      for( const char* c = SCIPparamGetName(b); *c ; ++c )
         if( *c == '/' )
            ++nslashb;

      if( nslasha == nslashb )
         return strcasecmp(SCIPparamGetName(a), SCIPparamGetName(b)) < 0;
      return nslasha < nslashb;
#else
      return strcasecmp(SCIPparamGetName(a), SCIPparamGetName(b)) < 0;
#endif
   }
   return SCIPparamIsAdvanced(b);
}

void printSCIPOptions()
{
   GamsScip* gamsscip;
   SCIP* scip;

   gamsscip = new GamsScip();
   gamsscip->setupSCIP();
   scip = gamsscip->scip;

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
   categname["nlp"] = "Nonlinear Programming Relaxation";
   categname["nlpi"] = "Nonlinear Programming Solver interfaces";
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
   std::string descr;
   std::string longdescr;
   std::string category;

   std::ofstream optfile("optscip_a.tex");
   GamsOptions gmsopt("scip");
   gmsopt.setSeparator("=");
   gmsopt.setStringQuote("\"");

   for( int i = 0; i < nparams; ++i )
   {
      param = params[i];
      const char* paramname = SCIPparamGetName(param);

      if( strcmp(paramname, "numerics/infinity") == 0 )
         continue;

      if( strcmp(paramname, "display/lpcond") == 0 )
         continue;

      if( strstr(paramname, "constraints/conjunction")   == paramname ||
          strstr(paramname, "constraints/countsols")     == paramname ||
          strstr(paramname, "constraints/cumulative")    == paramname ||
          strstr(paramname, "constraints/disjunction")   == paramname ||
          strstr(paramname, "constraints/linking")       == paramname ||
          strstr(paramname, "constraints/or")            == paramname ||
          strstr(paramname, "constraints/orbitope")      == paramname ||
          strstr(paramname, "constraints/pseudoboolean") == paramname ||
          strstr(paramname, "constraints/superindicator")== paramname ||
          strstr(paramname, "constraints/xor")           == paramname
         )
         continue;

      const char* catend = strchr(paramname, '/');
      if( catend != NULL )
         category = std::string(paramname, 0, catend - paramname);
      else
         category = "";

      if( category == "reading" ||
          category == "pricing" ||
          category == "nlp" ||
          category == "nlpi" ||
          category == "vbc"
        )
         continue;

      if( category == "gams" )
         category = " gams";

      paramsort[category].push_back(param);
   }

   for( std::map<std::string, std::list<SCIP_PARAM*> >::iterator it_categ(paramsort.begin()); it_categ != paramsort.end(); ++it_categ )
   {
      if( !categname.count(it_categ->first) )
         std::cerr << "Do not have name for SCIP option category " << it_categ->first << std::endl;
      printOptionCategoryStart(optfile, categname[it_categ->first]);

      it_categ->second.sort(ScipParamCompare);
      bool hadadvanced = false;
      for( std::list<SCIP_PARAM*>::iterator it_param(it_categ->second.begin()); it_param != it_categ->second.end(); ++it_param)
      {
         param = *it_param;
         switch( SCIPparamGetType(param) )
         {
            case SCIP_PARAMTYPE_BOOL:
            {
               opttype = OPTTYPE_BOOL;
               minval.boolval = 0;
               maxval.boolval = 1;
               defaultval.boolval = SCIPparamGetBoolDefault(param);
               break;
            }

            case SCIP_PARAMTYPE_INT:
            {
               opttype = OPTTYPE_INTEGER;
               minval.intval = SCIPparamGetIntMin(param);
               maxval.intval = SCIPparamGetIntMax(param);
               defaultval.intval = SCIPparamGetIntDefault(param);
               break;
            }

            case SCIP_PARAMTYPE_LONGINT:
            {
               opttype = OPTTYPE_INTEGER;
               minval.intval = ScipLongintToInt(SCIPparamGetLongintMin(param));
               maxval.intval = ScipLongintToInt(SCIPparamGetLongintMax(param));
               defaultval.intval = ScipLongintToInt(SCIPparamGetLongintDefault(param));
               break;
            }

            case SCIP_PARAMTYPE_REAL:
            {
               opttype = OPTTYPE_REAL;
               minval.realval = SCIPparamGetRealMin(param);
               maxval.realval = SCIPparamGetRealMax(param);
               defaultval.realval = SCIPparamGetRealDefault(param);
               if( SCIPisInfinity(scip, -minval.realval) )
                  minval.realval = -DBL_MAX;
               if( SCIPisInfinity(scip,  maxval.realval) )
                  maxval.realval =  DBL_MAX;
               if( SCIPisInfinity(scip, ABS(defaultval.realval)) )
                  defaultval.realval = (defaultval.realval < 0 ? -1.0 : 1.0) * DBL_MAX;
               break;
            }

            case SCIP_PARAMTYPE_CHAR:
               opttype = OPTTYPE_CHAR;
               defaultval.charval = SCIPparamGetCharDefault(param);
               break;

            case SCIP_PARAMTYPE_STRING:
               opttype = OPTTYPE_STRING;
               defaultval.stringval = SCIPparamGetStringDefault(param);
               break;

            default:
               std::cerr << "Skip option " << SCIPparamGetName(param) << " of unknown type." << std::endl;
               continue;

         }
         descr = SCIPparamGetDesc(param);

         if( strcmp(SCIPparamGetName(param), "limits/time") == 0 )
            defaultval.realval = 1000.0;
         else if( strcmp(SCIPparamGetName(param), "limits/gap") == 0 )
            defaultval.realval = 0.1;
         else if( strcmp(SCIPparamGetName(param), "limits/absgap") == 0 )
            defaultval.realval = 0.0;
         else if( strcmp(SCIPparamGetName(param), "lp/solver") == 0 )
         {
            defaultval.stringval = "cplex, if licensed, otherwise soplex";
            descr = "LP solver to use (clp, cplex, soplex)";
         }

         if( !hadadvanced && SCIPparamIsAdvanced(param) )
         {
            printOptionCategoryStart(optfile, categname[it_categ->first] + " (advanced options)");
            hadadvanced = true;
         }
         printOption(optfile, SCIPparamGetName(param), descr, "",
            opttype, defaultval, minval, false, maxval, false, enumval, true);

         longdescr = "";
         std::string::size_type nlpos = descr.find("\n");
         if( nlpos != std::string::npos )
         {
            longdescr = std::string(descr, nlpos+1);
            descr = std::string(descr, 0, nlpos);
         }

         tmpstr = SCIPparamGetName(param);

         char* lastslash = strrchr(const_cast<char*>(SCIPparamGetName(param)), '/');
         if( lastslash == NULL )
         {
            category = "";
         }
         else
         {
            category = std::string(SCIPparamGetName(param), 0, lastslash - SCIPparamGetName(param));
            if( category.find("gams") == 0 )
               category = " " + category;
         }
         gmsopt.setGroup(category);

         gmsopt.collect(SCIPparamGetName(param), descr, longdescr,
            opttype, defaultval, minval, maxval, enumval);
      }
   }

   optfile.close();
   gmsopt.write();

   delete gamsscip;
}
#endif

int main(int argc, char** argv)
{
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
