/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file GamsOptionsSpecWriter.hpp
  * @author Stefan Vigerske
 */

#ifndef GAMSOPTIONSSPECWRITER_HPP_
#define GAMSOPTIONSSPECWRITER_HPP_

#include <string>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <algorithm>

#include <cstring>
#include <cassert>
#include <climits>
#include <cfloat>

class GamsOption
{
public:
   typedef enum
   {
      OPTTYPE_BOOL,
      OPTTYPE_INTEGER,
      OPTTYPE_REAL,
      OPTTYPE_CHAR,
      OPTTYPE_STRING
   } OPTTYPE;

   union OPTVAL
   {
      bool                  boolval;
      int                   intval;
      double                realval;
      char                  charval;
      const char*           stringval;
   };

   typedef std::vector<std::pair<OPTVAL, std::string> > ENUMVAL;

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
   int                refval;
   std::set<std::string> synonyms;

   GamsOption(
      const std::string& group_,
      const std::string& name_,
      const std::string& shortdescr_,
      const std::string& longdescr_,
      const std::string& defaultdescr_,
      OPTTYPE            type_,
      OPTVAL             defaultval_,
      OPTVAL             minval_,
      OPTVAL             maxval_,
      const ENUMVAL&     enumval_,
      int                refval_
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
     enumval(enumval_),
     refval(refval_)
   { }
};


class GamsOptions
{
private:
   std::list<GamsOption> options;
   std::map<std::string, std::string> groups;
   std::set<std::string> values;

   std::string           solver;
   std::string           curgroup;
   std::string           separator;
   std::string           stringquote;
   std::string           eolchars;

   static
   std::string tolower(std::string s)
   {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
   }

   static
   void replaceAll(std::string& str, const std::string& from, const std::string& to) {
       if(from.empty())
           return;
       size_t start_pos = 0;
       while((start_pos = str.find(from, start_pos)) != std::string::npos) {
           str.replace(start_pos, from.length(), to);
           start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
       }
   }

public:
   static
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

   GamsOptions(
      const std::string& solver_
      )
   : values(),
     solver(solver_),
     separator(" ")
   { }

   void setGroup(
      const std::string& group,
      const std::string& description = ""
      )
   {
      curgroup = group;
      groups[group] = description;
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

   void setEolChars(
      const std::string& eolchs
      )
   {
      eolchars = eolchs;
   }

   void collect(
      const std::string& name,
      std::string        shortdescr,
      std::string        longdescr,
      GamsOption::OPTTYPE type,
      GamsOption::OPTVAL defaultval,
      GamsOption::OPTVAL minval,
      GamsOption::OPTVAL maxval,
      const GamsOption::ENUMVAL& enumval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   );

   /** add real-type option */
   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      double             defaultval,
      double             minval,
      double             maxval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   {
      GamsOption::OPTVAL defaultval_;
      defaultval_.realval = defaultval;

      GamsOption::OPTVAL minval_;
      minval_.realval = minval;

      GamsOption::OPTVAL maxval_;
      maxval_.realval = maxval;

      collect(name, shortdescr, longdescr,
         GamsOption::OPTTYPE_REAL,
         defaultval_,
         minval_, maxval_,
         GamsOption::ENUMVAL(),
         defaultdescr, refval);
   }

   /** add integer-type option */
   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      int                defaultval,
      int                minval,
      int                maxval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   {
      GamsOption::OPTVAL defaultval_;
      defaultval_.intval = defaultval;

      GamsOption::OPTVAL minval_;
      minval_.intval = minval;

      GamsOption::OPTVAL maxval_;
      maxval_.intval = maxval;

      collect(name, shortdescr, longdescr,
         GamsOption::OPTTYPE_INTEGER,
         defaultval_,
         minval_, maxval_,
         GamsOption::ENUMVAL(),
         defaultdescr, refval);
   }

   /** add enumerated integer-type option */
   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      int                defaultval,
      const GamsOption::ENUMVAL& enumval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   {
      GamsOption::OPTVAL minval;
      GamsOption::OPTVAL maxval;
      minval.intval =  INT_MAX;
      maxval.intval = -INT_MAX;
      for( GamsOption::ENUMVAL::const_iterator e(enumval.begin()); e != enumval.end(); ++e )
      {
         minval.intval = std::min(e->first.intval, minval.intval);
         maxval.intval = std::max(e->first.intval, maxval.intval);
      }

      GamsOption::OPTVAL defaultval_;
      defaultval_.intval = defaultval;

      collect(name, shortdescr, longdescr,
         GamsOption::OPTTYPE_INTEGER,
         defaultval_,
         minval, maxval,
         enumval, defaultdescr, refval);
   }

   /** add bool-type option */
   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      bool               defaultval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   {
      GamsOption::OPTVAL defaultval_;
      defaultval_.boolval = defaultval;

      collect(name, shortdescr, longdescr,
         GamsOption::OPTTYPE_BOOL,
         defaultval_,
         GamsOption::OPTVAL(), GamsOption::OPTVAL(),
         GamsOption::ENUMVAL(),
         defaultdescr, refval);
   }

   /** add string-type option */
   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      const std::string& defaultval,
      int                refval = -2
   )
   {
      GamsOption::OPTVAL defaultval_;
      defaultval_.stringval = strdup(defaultval.c_str());

      collect(name, shortdescr, longdescr,
         GamsOption::OPTTYPE_STRING,
         defaultval_,
         GamsOption::OPTVAL(), GamsOption::OPTVAL(),
         GamsOption::ENUMVAL(),
         std::string(), refval);
   }

   /** add enumerated string-type option */
   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      const std::string& defaultval,
      const GamsOption::ENUMVAL& enumval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   {
      GamsOption::OPTVAL defaultval_;
      defaultval_.stringval = strdup(defaultval.c_str());

      collect(name, shortdescr, longdescr,
         GamsOption::OPTTYPE_STRING,
         defaultval_,
         GamsOption::OPTVAL(), GamsOption::OPTVAL(),
         enumval, defaultdescr, refval);
   }

   /// get last added option
   GamsOption& back()
   {
      assert(!options.empty());
      return options.back();
   }

   /// add element to "values" set - for hacks
   void addvalue(
      const std::string& v
      )
   {
      values.insert(v);
   }

   void write(bool shortdoc = false);
};

#endif // GAMSOPTIONSSPECWRITER_HPP_
