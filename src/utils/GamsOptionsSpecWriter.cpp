/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file GamsOptionsSpecWriter.cpp
  * @author Stefan Vigerske
  */

#include "GamsOptionsSpecWriter.hpp"
#include "GAMSlinksConfig.h"

#include <cstdio>
#include <iostream>
#include <fstream>

void GamsOptions::collect(
   const std::string& name,
   const std::string& shortdescr,
   const std::string& longdescr,
   OPTTYPE            type,
   OPTVAL             defaultval,
   OPTVAL             minval,
   OPTVAL             maxval,
   const ENUMVAL&     enumval,
   const std::string& defaultdescr,
   int                refval
)
{
   /* ignore options with number in beginning, because the GAMS options object cannot handle them so far */
   //      if( isdigit(name[0]) )
   //      {
   //         std::cerr << "Warning: Ignoring " << solver << " option " << name << " for GAMS options file." << std::endl;
   //         return;
   //      }

   data.push_back(Data(curgroup, name, shortdescr, longdescr, defaultdescr, type, defaultval, minval, maxval, enumval, refval));

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

void GamsOptions::write(bool shortdoc)
{
   std::string filename;

   filename = "opt" + solver + ".gms";
   std::cout << "Writing " << filename << std::endl;
   std::ofstream f(filename.c_str());

   f << "$setglobal SEPARATOR \"" << separator << '"' << std::endl;
   if( stringquote == "\"" )
      f << "$setglobal STRINGQUOTE '\"'" << std::endl;
   else
      f << "$setglobal STRINGQUOTE \"" << stringquote << "\"" << std::endl;
   if( !eolchars.empty() )
      f << "$setglobal EOLCHAR \"" << eolchars << "\"" << std::endl;
   if( shortdoc )
      f << "$setglobal SHORTDOCONLY" << std::endl;
   f << "$onempty" << std::endl;

   f << "set e / " << std::endl;
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
   bool havelongdescr = false;
   for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
   {
      if( d->longdescr.length() > 0 )
         havelongdescr = true;

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
            f << "B.(def " << d->defaultval.boolval;
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
            break;

         case OPTTYPE_CHAR:
            /* no character type in GAMS option files */
            f << "S.(def '" << d->defaultval.charval << '\'';
            break;

         case OPTTYPE_STRING:
         case OPTTYPE_ENUM:
            f << "S.(def '" << makeValidMarkdownString(d->defaultval.stringval) << '\'';
            break;
      }
      if( d->refval >= -1 )
         f << ", ref " << d->refval;
      f << ")" << std::endl;
   }
   f << "/;" << std::endl;

   f << "set oe(o,e) /" << std::endl;
   for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
   {
      if( d->type != OPTTYPE_ENUM )
         continue;

      for( std::vector<std::pair<std::string, std::string> >::iterator e(d->enumval.begin()); e != d->enumval.end(); ++e )
      {
         f << "  '" << d->name << "'.'" << e->first << '\'';
         if( !e->second.empty() )
            f << "  \"" << makeValidMarkdownString(e->second) << '"';
         f << std::endl;
      }
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

   f << "set os(o,*) synonyms  /";
   for( std::list<Data>::iterator d(data.begin()); d != data.end(); ++d )
      if( !d->synonyms.empty() )
         for( std::set<std::string>::const_iterator s(d->synonyms.begin()); s != d->synonyms.end(); ++s )
            f << std::endl << "  '" << d->name << "'.'" << *s << '\'';
   f << " /;" << std::endl
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

   if( havelongdescr )
   {
      filename = "opt" + solver + ".txt_";
      std::cout << "Writing " << filename << std::endl;
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
      std::cout << "Folding " << filename << " to opt" << solver << ".txt" << std::endl;
      system(foldcall.c_str());
      remove(filename.c_str());
   }
}
