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
#include <list>
#include <algorithm>

#include <cstring>
#include <cassert>
#include <climits>
#include <cfloat>

class GamsOptions
{
public:
   typedef enum
   {
      OPTTYPE_BOOL,
      OPTTYPE_INTEGER,
      OPTTYPE_REAL,
      OPTTYPE_CHAR,
      OPTTYPE_STRING,
      OPTTYPE_ENUM
   } OPTTYPE;

   union OPTVAL
   {
      bool                  boolval;
      int                   intval;
      double                realval;
      char                  charval;
      const char*           stringval;
   };

   typedef std::vector<std::pair<std::string, std::string> > ENUMVAL;

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
      int                refval;
      std::set<std::string> synonyms;

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

   std::list<Data>       data;
   std::set<std::string> groups;
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
   : values({"1", "2", "3", "4", "5", "6", "7", "8", "9"}),  // GAMS needs these to enumerate lines in long descr.
     solver(solver_),
     separator(" ")
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

   void setEolChars(
      const std::string& eolchs
      )
   {
      eolchars = eolchs;
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
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   );

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
      collect(name, shortdescr, longdescr,
         OPTTYPE_REAL, OPTVAL({.realval = defaultval}), OPTVAL({.realval = minval}), OPTVAL({.realval = maxval}), ENUMVAL(),
         defaultdescr, refval);
   }

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
      collect(name, shortdescr, longdescr,
         OPTTYPE_INTEGER, OPTVAL({.intval = defaultval}), OPTVAL({.intval = minval}), OPTVAL({.intval = maxval}), ENUMVAL(),
         defaultdescr, refval);
   }

   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      bool               defaultval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   {
      collect(name, shortdescr, longdescr,
         OPTTYPE_BOOL, OPTVAL({.boolval = defaultval}), OPTVAL(), OPTVAL(), ENUMVAL(),
         defaultdescr, refval);
   }

   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      const std::string& defaultval,
      const ENUMVAL&     enumval,
      const std::string& defaultdescr = std::string(),
      int                refval = -2
   )
   {
      const char* def = strdup(defaultval.c_str());
      collect(name, shortdescr, longdescr,
         OPTTYPE_ENUM, OPTVAL({.stringval = def}), OPTVAL(), OPTVAL(), enumval,
         defaultdescr, refval);
   }

   void collect(
      const std::string& name,
      const std::string& shortdescr,
      const std::string& longdescr,
      const std::string& defaultval,
      int                refval = -2
   )
   {
      const char* def = strdup(defaultval.c_str());
      collect(name, shortdescr, longdescr,
         OPTTYPE_STRING, OPTVAL({.stringval = def}), OPTVAL(), OPTVAL(), ENUMVAL(),
         std::string(), refval);
   }

   /// get last added option
   Data& back()
   {
      assert(!data.empty());
      return data.back();
   }

   /// add element to "value" error - for hacks
   void addvalue(
      const std::string& v
      )
   {
      values.insert(v);
   }

   void write(bool shortdoc = false);
};

#endif // GAMSOPTIONSSPECWRITER_HPP_
