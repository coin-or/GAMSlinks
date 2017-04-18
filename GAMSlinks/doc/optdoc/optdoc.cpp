// Copyright (C) GAMS Development and others 2010 and later
// All Rights Reserved.
// This code is distributed under the terms of the Eclipse Public License.
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

#ifdef COIN_HAS_OSISPX
#include "soplex.h"
using namespace soplex;
#endif

extern "C" const char* SCIPgetBuildFlags();
extern "C" const char* SCIPgetBuildFlags() { return ""; }

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
      std::string        defaultdescr;
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
         const std::string& defaultdescr_,
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
        defaultdescr(defaultdescr_),
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

   void replaceAll(std::string& str, const std::string& from, const std::string& to) {
       if(from.empty())
           return;
       size_t start_pos = 0;
       while((start_pos = str.find(from, start_pos)) != std::string::npos) {
           str.replace(start_pos, from.length(), to);
           start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
       }
   }
   
   std::string makeValidMarkdownString(const std::string& s)
   {
      std::string r(s);
      replaceAll(r, "<=", "&le;");
      replaceAll(r, ">=", "&ge;");
      replaceAll(r, "<", "&lt;");
      replaceAll(r, ">", "&gt;");
      replaceAll(r, "|", "\\|");
      replaceAll(r, "$", "\\f$");
      return r;
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
      const ENUMVAL&     enumval,
      const std::string& defaultdescr = std::string()
   )
   {
      /* ignore options with number in beginning, because the GAMS options object cannot handle them so far */
//      if( isdigit(name[0]) )
//      {
//         std::cerr << "Warning: Ignoring " << solver << " option " << name << " for GAMS options file." << std::endl;
//         return;
//      }

      data.push_back(Data(curgroup, name, shortdescr, longdescr, defaultdescr, type, defaultval, minval, maxval, enumval));

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
   
   void write(bool shortdoc = false)
   {
      std::string filename;

      filename = "opt" + solver + ".gms";
      std::ofstream f(filename.c_str());
      
      f << "$setglobal SEPARATOR \"" << separator << '"' << std::endl;
      if( stringquote == "\"" )
         f << "$setglobal STRINGQUOTE '\"'" << std::endl;
      else
         f << "$setglobal STRINGQUOTE \"" << stringquote << "\"" << std::endl;
      if( shortdoc )
         f << "$setglobal SHORTDOCONLY" << std::endl;
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
         f << "  '" << d->name << "'  \"" << makeValidMarkdownString(d->shortdescr) << '"' << std::endl;
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
               if( d->minval.intval == -INT_MAX )
                  f << ", lo minint";
               else if( d->minval.intval != 0 )
                  f << ", lo " << d->minval.intval;
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
               if( d->minval.realval == -DBL_MAX )
                  f << ", lo mindouble";
               else if( d->minval.realval != 0 )
                  f << ", lo " << d->minval.realval;
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
               f << "S.(def '" << makeValidMarkdownString(d->defaultval.stringval) << "')";
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
            f << "  '" << d->name << "'.'" << e->first << "'  \"" << makeValidMarkdownString(e->second) << '"' << std::endl;
      }
      f << "/;" << std::endl;

      f << "$onempty" << std::endl;

      f << "set odefault(o) /" << std::endl;
      for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
      {
         if( d->defaultdescr.length() == 0 )
            continue;

         f << "  '" << d->name << "' \"" << makeValidMarkdownString(d->defaultdescr) << '"' << std::endl;
      }
      f << "/;" << std::endl;

      f << "set os(o,*) synonyms  / /;" << std::endl
        << "set im immediates recognized  / EolFlag , Message /;" << std::endl   /* ???? */
        << "set strlist(o)        / /;" << std::endl
        << "set immediate(o,im)   / /;" << std::endl
        << "set multilist(o,o)    / /;" << std::endl
        << "set indicator(o)      / /;" << std::endl
        << "set dotoption(o)      / /;" << std::endl
        << "set dropdefaults(o)   / /;" << std::endl
        << "set deprecated(*,*)   / /;" << std::endl
        << "set oedepr(o,e)       / /;" << std::endl  /* deprecated enum options */
        << "set oep(o)            / /;" << std::endl  /* enum options for documentation only */
        << "set olink(o)          / /;" << std::endl
        //<< "set om(o,m)           / /;" << std::endl  /* option with GAMS synonym and GAMS initialization */
        << "$offempty" << std::endl;

      f.close();


      filename = "opt" + solver + ".txt_";
      f.open(filename.c_str());
      for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
      {
         if( d->longdescr.length() == 0 )
            continue;
         f << d->name << std::endl;
         f << makeValidMarkdownString(d->longdescr) << std::endl;
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
   bool                  isSCIP = false,
   std::string           defaultdescr = std::string()
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
         if( defaultdescr.length() > 0 )
            out << "{" << defaultdescr << "}%" << std::endl;
         else if( isSCIP )
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
         if( defaultdescr.length() > 0 )
            out << "{" << defaultdescr << "}%" << std::endl;
         else
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
         if( defaultdescr.length() > 0 )
            out << "{" << defaultdescr << "}%" << std::endl;
         else
            out << "{$" << makeValidLatexNumber(def.realval) << "$}%" << std::endl;
         break;
      }

      case OPTTYPE_CHAR:
      {
         out << "{character}%" << std::endl;
         if( defaultdescr.length() > 0 )
            out << "{" << defaultdescr << "}%" << std::endl;
         else
            out << "{" << def.charval << "}%" << std::endl;
         break;
      }

      case OPTTYPE_STRING:
      {
         out << "{string}%" << std::endl;
         if( defaultdescr.length() > 0 )
            out << "{" << defaultdescr << "}%" << std::endl;
         else
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
         if( defaultdescr.length() > 0 )
            out << "{" << defaultdescr << "}%" << std::endl;
         else
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
          it->second->Name() == "num_linear_variables" ||
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

// TODO specify Ipopt options of Bonmin as hidden in optbonmin.gms, so they don't show up in docu?
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
          it->second->Name() == "num_linear_variables" ||
          it->second->Name() == "skip_finalize_solution_call" ||
          it->second->Name() == "warm_start_entire_iterate" ||
          it->second->Name() == "warm_start_same_structure"
        )
         continue;


      opts[category].push_back(it->second);
   }

   // print options tables
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
               defaultval.append(makeValidLatexNumber((*it_opt)->DefaultInteger()));
               defaultval.append("\\f$");
               break;
            }

            case Ipopt::OT_Number:
            {
               typestring = "\\f$\\mathbb{Q}\\f$";
               defaultval = "\\f$";
               defaultval.append(makeValidLatexNumber((*it_opt)->DefaultNumber()));
               defaultval.append("\\f$");
               break;
            }

            case Ipopt::OT_String:
            {
               typestring = "string";
               defaultval = "``";
               defaultval.append((*it_opt)->DefaultString());
               defaultval.append("``");
               break;
            }

            case Ipopt::OT_Unknown: ;
         }

         if( (*it_opt)->Name() == "nlp_log_at_root" )
            defaultval = makeValidLatexNumber(Ipopt::J_ITERSUMMARY);
         else if( (*it_opt)->Name() == "allowable_gap" )
            defaultval = "GAMS ``optca``";
         else if( (*it_opt)->Name() == "allowable_fraction_gap" )
            defaultval = "GAMS ``optcr``";
         else if( (*it_opt)->Name() == "node_limit" )
            defaultval = "GAMS ``nodlim``";
         else if( (*it_opt)->Name() == "time_limit" )
            defaultval = "GAMS ``reslim``";
         else if( (*it_opt)->Name() == "iteration_limit" )
            defaultval = "GAMS ``iterlim``";
         else if( (*it_opt)->Name() == "cutoff" )
            defaultval = "GAMS ``cutoff``";

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
            longdescr = "See option `2mir_cuts` for a detailed description.";
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
          it->second->Name() == "num_linear_variables" ||
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
            longdescr = "See option `2mir_cuts` for the meaning of k.";
         else if( (*it_opt)->Name().find("branch_pt_select_") == 0 )
            longdescr = longdescr + "Default is to use the value of `branch_pt_select` (value `common`).";
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
   categname["history"] = "History";
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
   std::string defaultdescr;
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

      if( category == "compression" ||  //FIXME should be here?
          category == "reading" ||
          category == "reoptimization" ||
          category == "pricing" ||
          category == "nlp" ||
          category == "nlpi" ||
          category == "visual" ||
          category == "write"
        )
         continue;

      if( category == "gams" )
         category = " gams";

      paramsort[category].push_back(param);
   }

   for( std::map<std::string, std::list<SCIP_PARAM*> >::iterator it_categ(paramsort.begin()); it_categ != paramsort.end(); ++it_categ )
   {
      if( !categname.count(it_categ->first) )
      {
         std::cerr << "Error: Do not have name for SCIP option category " << it_categ->first << std::endl;
         return;
      }
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
         defaultdescr = std::string();

         if( strcmp(SCIPparamGetName(param), "limits/time") == 0 )
         {
            defaultval.realval = 1000.0;
            defaultdescr = "GAMS reslim";
         }
         else if( strcmp(SCIPparamGetName(param), "limits/gap") == 0 )
         {
            defaultval.realval = 0.1;
            defaultdescr = "GAMS optcr";
         }
         else if( strcmp(SCIPparamGetName(param), "limits/absgap") == 0 )
         {
            defaultval.realval = 0.0;
            defaultdescr = "GAMS optca";
         }
         else if( strcmp(SCIPparamGetName(param), "limits/memory") == 0 )
            defaultdescr = "GAMS workspace";
         else if( strcmp(SCIPparamGetName(param), "limits/nodes") == 0 )
            defaultdescr = "GAMS nodlim, if set, otherwise -1";
         else if( strcmp(SCIPparamGetName(param), "lp/solver") == 0 )
         {
            defaultdescr = "cplex, if licensed, otherwise soplex";
            descr = "LP solver to use (clp, cplex, soplex, soplex2)";
         }
         else if( strcmp(SCIPparamGetName(param), "lp/threads") == 0 )
            defaultdescr = "GAMS threads";
         else if( strcmp(SCIPparamGetName(param), "misc/printreason") == 0 )
            defaultval.boolval = false;
         else if( strcmp(SCIPparamGetName(param), "display/lpavgiterations/active") == 0 )
            defaultdescr = "1 (0 for Windows without IDE)";
         else if( strcmp(SCIPparamGetName(param), "display/maxdepth/active") == 0 )
            defaultdescr = "1 (0 for Windows without IDE)";
         else if( strcmp(SCIPparamGetName(param), "display/nexternbranchcands/active") == 0 )
            defaultdescr = "1 (2 for nonlinear instances)";
         else if( strcmp(SCIPparamGetName(param), "display/nfrac/active") == 0 )
            defaultdescr = "1 (2 if discrete variables)";
         else if( strcmp(SCIPparamGetName(param), "display/time/active") == 0 )
            defaultdescr = "1 (2 for Windows without IDE)";
         else if( strcmp(SCIPparamGetName(param), "display/width") == 0 )
            defaultdescr = "139 (80 for Windows without IDE)";

         if( !hadadvanced && SCIPparamIsAdvanced(param) )
         {
            printOptionCategoryStart(optfile, categname[it_categ->first] + " (advanced options)");
            hadadvanced = true;
         }
         printOption(optfile, SCIPparamGetName(param), descr, "",
            opttype, defaultval, minval, false, maxval, false, enumval, true, defaultdescr);

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
            opttype, defaultval, minval, maxval, enumval, defaultdescr);
      }
   }

   optfile.close();
   gmsopt.write(true);

   /* branching rules */
   {
      std::ofstream outfile("branchrules.md");
      outfile << "| branching rule | priority | maxdepth | maxbounddist | description |" << std::endl;
      outfile << "|:------------- -|---------:|---------:|-------------:|:------------|" << std::endl;

      int nbranchrules = SCIPgetNBranchrules(scip);
      SCIP_BRANCHRULE** sorted;
      SCIP_CALL_ABORT( SCIPduplicateBufferArray(scip, &sorted, SCIPgetBranchrules(scip), nbranchrules) );
      SCIPsortPtr((void**)sorted, SCIPbranchruleComp, nbranchrules);
      for( int i = 0; i < nbranchrules; ++i )
      {
         outfile << "| \\ref SCIP_gr_branching_" << SCIPbranchruleGetName(sorted[i]) << " \"" << SCIPbranchruleGetName(sorted[i]) << '"';
         outfile << " | " << SCIPbranchruleGetPriority(sorted[i]);
         outfile << " | " << SCIPbranchruleGetMaxdepth(sorted[i]);
         outfile << " | " << 100.0 * SCIPbranchruleGetMaxbounddist(sorted[i]);
         outfile << " | " << SCIPbranchruleGetDesc(sorted[i]);
         outfile << " |" << std::endl;
      }
      SCIPfreeBufferArray(scip, &sorted);
   }
   
   /* conflict handler */
   {
      SCIP_CONFLICTHDLR** conflicthdlrs = SCIPgetConflicthdlrs(scip);
      SCIP_CONFLICTHDLR** sorted;
      int nconflicthdlrs = SCIPgetNConflicthdlrs(scip);
      SCIP_CALL_ABORT( SCIPduplicateBufferArray(scip, &sorted, conflicthdlrs, nconflicthdlrs) );
      SCIPsortPtr((void**)sorted, SCIPconflicthdlrComp, nconflicthdlrs);

      std::ofstream outfile("conflicthdlrs.md");
      outfile << "| conflict handler | priority | description |" << std::endl;
      outfile << "|:-----------------|---------:|:------------|" << std::endl;

      for( int i = 0; i < nconflicthdlrs; ++i )
      {
         outfile << "| \\ref SCIP_gr_conflict_" << SCIPconflicthdlrGetName(sorted[i]) << " \"" << SCIPconflicthdlrGetName(sorted[i]) << '"';
         outfile << " | " << SCIPconflicthdlrGetPriority(sorted[i]);
         outfile << " | " << SCIPconflicthdlrGetDesc(sorted[i]);
         outfile << " |" << std::endl;
      }
      SCIPfreeBufferArray(scip, &sorted);
   }

   /* constraint handler */
   {
      std::ofstream outfile("conshdlrs.md");
      outfile << "| constraint handler | checkprio | enfoprio | sepaprio | sepafreq | propfreq | eagerfreq | presolvetimings | description |" << std::endl;
      outfile << "|:-------------------|----------:|---------:|---------:|---------:|---------:|----------:|:---------------:|:------------|" << std::endl;
      SCIP_CONSHDLR** conshdlrs = SCIPgetConshdlrs(scip);
      for( int i = 0; i < SCIPgetNConshdlrs(scip); ++i )
      {
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "conjunction") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "countsols") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "cumulative") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "disjunction") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "linking") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "or") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "orbitope") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "pseudoboolean") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "superindicator") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "xor") == 0 )
            continue;

         outfile << "| \\ref SCIP_gr_constraints_" << SCIPconshdlrGetName(conshdlrs[i]) << " \"" << SCIPconshdlrGetName(conshdlrs[i]) << '"';
         outfile << " | " << SCIPconshdlrGetCheckPriority(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetEnfoPriority(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetSepaPriority(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetSepaFreq(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetPropFreq(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetEagerFreq(conshdlrs[i]);
         if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) == SCIP_PRESOLTIMING_ALWAYS )
            outfile << " | always";
         else
         {
            outfile << " |";
            if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) & SCIP_PRESOLTIMING_FAST )
               outfile << " fast";
            if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) & SCIP_PRESOLTIMING_MEDIUM )
               outfile << " medium";
            if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) & SCIP_PRESOLTIMING_EXHAUSTIVE )
               outfile << " exhaustive";
         }
         outfile << " | " << SCIPconshdlrGetDesc(conshdlrs[i]);
         outfile << " |" << std::endl;
      }
   }
   
   /* display columns */
   {
      SCIP_DISP** disps = SCIPgetDisps(scip);
      int ndisps = SCIPgetNDisps(scip);
      
      std::ofstream outfile("disps.md");
      outfile << "| display column | header | position | width | priority | status | description |" << std::endl;
      outfile << "|:---------------|:-------|---------:|------:|---------:|-------:|:------------|" << std::endl;
      for( int i = 0; i < ndisps; ++i )
      {
         outfile << "| \\ref SCIP_gr_display_" << SCIPdispGetName(disps[i]) << " \"" << SCIPdispGetName(disps[i]) << '"';
         outfile << " | " << SCIPdispGetHeader(disps[i]);
         outfile << " | " << SCIPdispGetPosition(disps[i]);
         outfile << " | " << SCIPdispGetWidth(disps[i]);
         outfile << " | " << SCIPdispGetPriority(disps[i]);
         switch( SCIPdispGetStatus(disps[i]) )
         {
         case SCIP_DISPSTATUS_OFF:
            outfile << " | off";
            break;
         case SCIP_DISPSTATUS_AUTO:
            outfile << " | auto";
            break;
         case SCIP_DISPSTATUS_ON:
            outfile << " | on";
            break;
         default:
            outfile << " | ?";
            break;
         }
         outfile << " | ";
         
         /* escape "|" */
         char* descr = strdup(SCIPdispGetDesc(disps[i]));
         char* desc = descr;
         char* rest = desc;
         while( (rest = strstr(desc, "|")) != NULL )
         {
            *rest = '\0';
            outfile << desc << "\\|";
            desc = rest+1;
         }
         outfile << desc;
         free(descr);
         
         outfile << " |" << std::endl;
      }
   }

   /* primal heuristics */
   {
      std::ofstream outfile("heurs.md");
      outfile << "| primal heuristic | char | priority | freq | freqoffset | description |" << std::endl;
      outfile << "|:-----------------|:----:|---------:|-----:|-----------:|:------------|" << std::endl;
      
      int nheurs = SCIPgetNHeurs(scip);
      SCIP_HEUR** heurs = SCIPgetHeurs(scip);
      for( int h = 0; h < nheurs; ++h )
      {
         outfile << "| \\ref SCIP_gr_heuristics_" << SCIPheurGetName(heurs[h]) << " \"" << SCIPheurGetName(heurs[h]) << '"';
         outfile << " | " << SCIPheurGetDispchar(heurs[h]);
         outfile << " | " << SCIPheurGetPriority(heurs[h]);
         outfile << " | " << SCIPheurGetFreq(heurs[h]);
         outfile << " | " << SCIPheurGetFreqofs(heurs[h]);
         outfile << " | " << SCIPheurGetDesc(heurs[h]);
         outfile << " |" << std::endl;
      }
   }

   /* node selectors */
   {
      SCIP_NODESEL** nodesels;
      int nnodesels;
      int i;

      nodesels = SCIPgetNodesels(scip);
      nnodesels = SCIPgetNNodesels(scip);

      std::ofstream outfile("nodesels.md");
      outfile << "| node selector | standard priority | memsave priority | description |" << std::endl;
      outfile << "|:--------------|------------------:|-----------------:|:------------|" << std::endl;
      for( i = 0; i < nnodesels; ++i )
      {
         outfile << "| \\ref SCIP_gr_nodeselection_" << SCIPnodeselGetName(nodesels[i]) << " \"" << SCIPnodeselGetName(nodesels[i]) << '"';
         outfile << " | " << SCIPnodeselGetStdPriority(nodesels[i]);
         outfile << " | " << SCIPnodeselGetMemsavePriority(nodesels[i]);
         outfile << " | " << SCIPnodeselGetDesc(nodesels[i]);
         outfile << " |" << std::endl;
      }
   }

   /* presolvers */
   {
      SCIP_PRESOL** presols;
      int npresols;
      int i;

      presols = SCIPgetPresols(scip);
      npresols = SCIPgetNPresols(scip);

      std::ofstream outfile("presols.md");
      outfile << "| presolver | priority | timing | maxrounds | description |" << std::endl;
      outfile << "|:----------|---------:|:------:|----------:|:------------|" << std::endl;
      for( i = 0; i < npresols; ++i )
      {
         outfile << "| \\ref SCIP_gr_presolving_" << SCIPpresolGetName(presols[i]) << " \"" << SCIPpresolGetName(presols[i]) << '"';
         outfile << " | " << SCIPpresolGetPriority(presols[i]);
         if( SCIPpresolGetTiming(presols[i]) == SCIP_PRESOLTIMING_ALWAYS )
            outfile << " | always";
         else
         {
            outfile << " |";
            if( SCIPpresolGetTiming(presols[i]) & SCIP_PRESOLTIMING_FAST )
               outfile << " fast";
            if( SCIPpresolGetTiming(presols[i]) & SCIP_PRESOLTIMING_MEDIUM )
               outfile << " medium";
            if( SCIPpresolGetTiming(presols[i]) & SCIP_PRESOLTIMING_EXHAUSTIVE )
               outfile << " exhaustive";
         }
         outfile << " | " << SCIPpresolGetMaxrounds(presols[i]);
         outfile << " | " << SCIPpresolGetDesc(presols[i]);
         outfile << " |" << std::endl;
      }
   }

   /* propagators */
   {
      SCIP_PROP** props;
      int nprops;
      int i;

      props = SCIPgetProps(scip);
      nprops = SCIPgetNProps(scip);

      std::ofstream outfile("props.md");
      outfile << "| propagator | propprio | freq | presolveprio | presolvetiming | description |" << std::endl;
      outfile << "|:-----------|---------:|-----:|-------------:|:--------------:|:------------|" << std::endl;
      for( i = 0; i < nprops; ++i )
      {
         outfile << "| \\ref SCIP_gr_propagating_" << SCIPpropGetName(props[i]) << " \"" << SCIPpropGetName(props[i]) << '"';
         outfile << " | " << SCIPpropGetPriority(props[i]) << (SCIPpropIsDelayed(props[i]) ? 'd' : ' ');
         outfile << " | " << SCIPpropGetFreq(props[i]);
         outfile << " | " << SCIPpropGetPresolPriority(props[i]);
         if( SCIPpropGetPresolTiming(props[i]) == SCIP_PRESOLTIMING_ALWAYS )
            outfile << " | always";
         else
         {
            outfile << " |";
            if( SCIPpropGetPresolTiming(props[i]) & SCIP_PRESOLTIMING_FAST )
               outfile << " fast";
            if( SCIPpropGetPresolTiming(props[i]) & SCIP_PRESOLTIMING_MEDIUM )
               outfile << " medium";
            if( SCIPpropGetPresolTiming(props[i]) & SCIP_PRESOLTIMING_EXHAUSTIVE )
               outfile << " exhaustive";
         }
         outfile << " | " << SCIPpropGetDesc(props[i]);
         outfile << " |" << std::endl;
      }
   }

   /* separators */
   {
      SCIP_SEPA** sepas;
      int nsepas;
      int i;

      sepas = SCIPgetSepas(scip);
      nsepas = SCIPgetNSepas(scip);

      std::ofstream outfile("sepas.md");
      outfile << "| separator | priority | freq | bounddist | description |" << std::endl;
      outfile << "|:----------|---------:|-----:|----------:|:------------|" << std::endl;
      for( i = 0; i < nsepas; ++i )
      {
         outfile << "| \\ref SCIP_gr_separating_" << SCIPsepaGetName(sepas[i]) << " \"" << SCIPsepaGetName(sepas[i]) << '"';
         outfile << " | " <<  SCIPsepaGetPriority(sepas[i]) << (SCIPsepaIsDelayed(sepas[i]) ? 'd' : ' ');
         outfile << " | " << SCIPsepaGetFreq(sepas[i]);
         outfile << " | " << SCIPsepaGetMaxbounddist(sepas[i]);
         outfile << " | " << SCIPsepaGetDesc(sepas[i]);
         outfile << " |" << std::endl;
      }
   }

   delete gamsscip;
}
#endif

#if COIN_HAS_OSISPX
static
double translateSoplexInfinity(
   double val
   )
{
   if( val >=  1e100 )
      return  DBL_MAX;
   if( val <= -1e100 )
      return -DBL_MAX;
   return val;
}

void printSoPlexOptions()
{
   SoPlex soplex;

   OPTVAL defaultval, minval, maxval;
   ENUMVAL enumval;
   std::string tmpstr;
   std::string descr;
   std::string defaultdescr;

   GamsOptions gmsopt("soplex");
   gmsopt.setSeparator("=");
   // gmsopt.setStringQuote("\"");
   gmsopt.setGroup("soplex");

   for( int i = 0; i < SoPlex::BOOLPARAM_COUNT; ++i )
   {
      if( i == SoPlex::EQTRANS )
         continue;
      if( i == SoPlex::RATFAC )
         continue;
      if( i == SoPlex::RATREC )
         continue;

      defaultval.boolval = SoPlex::Settings::boolParam.defaultValue[i];
      minval.boolval = 0;
      maxval.boolval = 1;
      defaultdescr = std::string();

      gmsopt.collect(
         std::string("bool:") + SoPlex::Settings::boolParam.name[i],
         SoPlex::Settings::boolParam.description[i], std::string(),
         OPTTYPE_BOOL, defaultval, minval, maxval, enumval, defaultdescr);
   }

   for( int i = 0; i < SoPlex::INTPARAM_COUNT; ++i )
   {
      if( i == SoPlex::OBJSENSE )
         continue;
      if( i == SoPlex::SYNCMODE )
         continue;
      if( i == SoPlex::READMODE )
         continue;
      if( i == SoPlex::SOLVEMODE )
         continue;
      if( i == SoPlex::CHECKMODE )
         continue;
      if( i == SoPlex::RATFAC_MINSTALLS )
         continue;

      // TODO recognize intenums
      defaultval.intval = SoPlex::Settings::intParam.defaultValue[i];
      minval.intval = SoPlex::Settings::intParam.lower[i];
      maxval.intval = SoPlex::Settings::intParam.upper[i];
      defaultdescr = std::string();

      if( i == SoPlex::ITERLIMIT )
         defaultdescr = "GAMS iterlim";

      gmsopt.collect(
         std::string("int:") + SoPlex::Settings::intParam.name[i],
         SoPlex::Settings::intParam.description[i], std::string(),
         OPTTYPE_INTEGER, defaultval, minval, maxval, enumval, defaultdescr);
   }

   for( int i = 0; i < SoPlex::REALPARAM_COUNT; ++i )
   {
      if( i == SoPlex::RATREC_FREQ )
         continue;

      defaultval.realval = translateSoplexInfinity(SoPlex::Settings::realParam.defaultValue[i]);
      minval.realval = translateSoplexInfinity(SoPlex::Settings::realParam.lower[i]);
      maxval.realval = translateSoplexInfinity(SoPlex::Settings::realParam.upper[i]);
      defaultdescr = std::string();

      if( i == SoPlex::TIMELIMIT )
         defaultdescr = "GAMS reslim";

      gmsopt.collect(
         std::string("real:") + SoPlex::Settings::realParam.name[i],
         SoPlex::Settings::realParam.description[i], std::string(),
         OPTTYPE_REAL, defaultval, minval, maxval, enumval, defaultdescr);
   }

   gmsopt.write(true);
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

#if COIN_HAS_OSISPX
   printSoPlexOptions();
#endif
}
