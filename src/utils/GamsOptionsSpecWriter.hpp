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
   enum Type
   {
      BOOL,
      INTEGER,
      REAL,
      CHAR,
      STRING
   };

   union Value
   {
      bool                  boolval;
      int                   intval;
      double                realval;
      char                  charval;
      const char*           stringval;
   };

   static
   Value makeValue(
      bool val
      )
   {
      Value v;
      v.boolval = val;
      return v;
   }

   static
   Value makeValue(
      int val
      )
   {
      Value v;
      v.intval = val;
      return v;
   }

   static
   Value makeValue(
      double val
      )
   {
      Value v;
      v.realval = val;
      return v;
   }

   static
   Value makeValue(
      char val
      )
   {
      Value v;
      v.charval = val;
      return v;
   }

   static
   Value makeValue(
      const char* val
      )
   {
      Value v;
      v.stringval = val;
      return v;
   }

   static
   Value makeValue(
      const std::string& val
      )
   {
      Value v;
      v.stringval = strdup(val.c_str());
      return v;
   }

   class EnumVals : public std::vector<std::pair<GamsOption::Value, std::string> >
   {
   public:
      void append(
         const std::string& key,
         const std::string& descr = ""
         )
      {
         GamsOption::Value key_;
         key_.stringval = strdup(key.c_str());

         emplace_back(key_, descr);
      }

      void append(
         const char*        key,
         const std::string& descr = ""
         )
      {
         GamsOption::Value key_;
         key_.stringval = strdup(key);

         emplace_back(key_, descr);
      }

   };

   std::string        group;
   std::string        name;
   std::string        shortdescr;
   std::string        longdescr;
   std::string        defaultdescr;
   Type               type;
   Value              defaultval;
   Value              minval;
   Value              maxval;
   EnumVals           enumval;
   int                refval;
   std::set<std::string> synonyms;

   /// constructor for general option
   GamsOption(
      const std::string& name_,
      const std::string& shortdescr_,
      const std::string& longdescr_,
      Type               type_,
      Value              defaultval_,
      Value              minval_,
      Value              maxval_,
      const EnumVals&    enumval_,
      const std::string& defaultdescr_ = std::string(),
      int                refval_ = -2
   )
   : name(name_),
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

   /// constructor for real-type option
   GamsOption(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      double             defaultval,
      double             minval,
      double             maxval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   : GamsOption(name, shortdescr, longdescr,
      GamsOption::Type::REAL,
      makeValue(defaultval),  makeValue(minval),  makeValue(maxval),
      GamsOption::EnumVals(),
      defaultdescr, refval)
   { }

   /// constructor for integer-type option
   GamsOption(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      int                defaultval,
      int                minval,
      int                maxval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   : GamsOption(name, shortdescr, longdescr,
      GamsOption::Type::INTEGER,
      makeValue(defaultval), makeValue(minval), makeValue(maxval),
      GamsOption::EnumVals(),
      defaultdescr, refval)
   { }

   /// constructor for enumerated integer-type option
   GamsOption(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      int                defaultval,
      const GamsOption::EnumVals& enumval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   : GamsOption(name, shortdescr, longdescr,
      GamsOption::Type::INTEGER,
      makeValue(defaultval), makeValue(-INT_MAX), makeValue(INT_MAX),
      enumval, defaultdescr, refval)
   {
      int min, max;
      for( GamsOption::EnumVals::const_iterator e(enumval.begin()); e != enumval.end(); ++e )
      {
         min = std::min(e->first.intval, min);
         max = std::max(e->first.intval, max);
      }
      minval.intval = min;
      maxval.intval = max;
   }

   /// constructor for bool-type option
   GamsOption(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      bool               defaultval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   : GamsOption(name, shortdescr, longdescr,
      GamsOption::Type::BOOL,
      makeValue(defaultval), GamsOption::Value(), GamsOption::Value(),
      GamsOption::EnumVals(),
      defaultdescr, refval)
   { }

   /// constructor for string-type option
   GamsOption(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      const std::string& defaultval,
      int                refval = -2
   )
   : GamsOption(name, shortdescr, longdescr,
      GamsOption::Type::STRING,
      makeValue(defaultval), GamsOption::Value(), GamsOption::Value(),
      GamsOption::EnumVals(),
      std::string(), refval)
   { }

   /// constructor for enumerated string-type option
   GamsOption(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      const std::string& defaultval,
      const GamsOption::EnumVals& enumval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   : GamsOption(name, shortdescr, longdescr,
         GamsOption::Type::STRING,
         makeValue(defaultval), GamsOption::Value(), GamsOption::Value(),
         enumval, defaultdescr, refval)
   { }

};


class GamsOptions
{
private:
   std::list<GamsOption> options;
   std::map<std::string, std::string> groups;

   std::string           solver;
   std::string           curgroup;
   std::string           separator;
   std::string           stringquote;
   std::string           eolchars;

   static
   std::string tolower(
      std::string s
      )
   {
      std::transform(s.begin(), s.end(), s.begin(), ::tolower);
      return s;
   }

   static
   void replaceAll(
      std::string&       str,
      const std::string& from,
      const std::string& to
   )
   {
       if(from.empty())
           return;
       size_t start_pos = 0;
       while((start_pos = str.find(from, start_pos)) != std::string::npos)
       {
           str.replace(start_pos, from.length(), to);
           start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
       }
   }

public:
   static
   std::string makeValidMarkdownString(
      const std::string& s
   )
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
   : solver(solver_),
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

   /// append option to options list, assign it to current group
   template<class... Args>
   GamsOption& collect(
      Args&&... args)
   {
      options.emplace_back(std::forward<Args>(args)...);
      options.back().group = curgroup;
      return options.back();
   }

   /// write options in .gms + .txt for GAMS mkopt scripts
   void writeGMS(bool shortdoc = false);
};

#endif // GAMSOPTIONSSPECWRITER_HPP_
