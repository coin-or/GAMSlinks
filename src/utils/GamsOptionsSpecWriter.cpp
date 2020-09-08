/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file GamsOptionsSpecWriter.cpp
  * @author Stefan Vigerske
  */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

#include <cstdio>
#include <iostream>
#include <fstream>

void GamsOptions::writeGMS(
   bool shortdoc
   )
{
   // process or sort out options for GAMS (TODO do this on a copy of options?)
   // collect values
   std::set<std::string> values;
   for( std::list<GamsOption>::iterator o(options.begin()); o != options.end(); )
   {
      /* ignore options with number in beginning, because the GAMS options object cannot handle them so far */
      //      if( isdigit(name[0]) )
      //      {
      //         std::cerr << "Warning: Ignoring " << solver << " option " << name << " for GAMS options file." << std::endl;
      //         return;
      //      }
      if( o->name.length() > 63 )
      {
         std::cerr << "Skip writing option " << o->name << " because its name is too long for stupid GAMS." << std::endl;
         o = options.erase(o);
         continue;
      }

      if( o->shortdescr.length() >= 255 )
      {
         std::cerr << "Short description of option " << o->name << " too long for stupid GAMS. Moving to long description." << std::endl;
         if( !o->longdescr.empty() )
            o->longdescr = o->shortdescr + " " + o->longdescr;
         else
            o->longdescr = o->shortdescr;
         o->shortdescr.clear();
      }

      if( o->name.find(".") != std::string::npos && !o->longdescr.empty() )
      {
         std::cerr << "Cannot have long description because option " << o->name << " has a dot in the name and GAMS is stupid. Skipping long description." << std::endl;
         o->longdescr.clear();
      }

      /* replace all double quotes by single quotes */
      std::replace(o->shortdescr.begin(), o->shortdescr.end(), '"', '\'');

      switch( o->type )
      {
         case GamsOption::Type::BOOL:
         case GamsOption::Type::INTEGER:
         case GamsOption::Type::REAL:
            break;

         case GamsOption::Type::CHAR:
         {
            std::string str;
            str.push_back(o->defaultval.charval);
            values.insert(tolower(str));
            break;
         }

         case GamsOption::Type::STRING:
         {
            if( o->defaultval.stringval[0] != '\0' )
               values.insert(tolower(o->defaultval.stringval));
            break;
         }
      }

      /* collect enum values for values
       * update enum description
       */
      for( auto& e : o->enumval )
      {
         switch( o->type )
         {
            case GamsOption::Type::BOOL:
               values.insert(std::to_string(e.first.boolval));
               break;

            case GamsOption::Type::INTEGER:
               values.insert(std::to_string(e.first.intval));
               break;

            case GamsOption::Type::REAL:
               values.insert(std::to_string(e.first.realval));
               break;

            case GamsOption::Type::CHAR:
               values.insert(tolower(std::to_string(e.first.charval)));
               break;

            case GamsOption::Type::STRING:
               values.insert(tolower(e.first.stringval));
               break;
         }

         /* replace all double quotes by single quotes */
         std::replace(e.second.begin(), e.second.end(), '"', '\'');
         if( e.second.length() >= 255 )
         {
            size_t pos = e.second.rfind('.');
            if( pos < std::string::npos )
               e.second.erase(pos+1, e.second.length());
            else
               std::cerr << "Could not cut down description of enum value of parameter " << o->name << ": " << e.second << std::endl;
         }
      }

      ++o;
   }

   std::string filename = "opt" + solver + ".gms";
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

   f << "set e / 0*100 " << std::endl;
   for( int i = 0; i <= 100; ++i )
      values.erase(std::to_string(i));
   for( std::set<std::string>::iterator v(values.begin()); v != values.end(); ++v )
      f << "  '" << *v << "'" << std::endl;
   f << "/;" << std::endl;

   f << "set f / def Default, lo Lower Bound, up Upper Bound, ref Reference /;" << std::endl;
   f << "set t / I Integer, R Real, S String, B Binary /;" << std::endl;
   //f << "set m / %system.gamsopt% /;" << std::endl;

   f << "set g Option Groups /" << std::endl;
   for( std::map<std::string, std::string>::iterator g(groups.begin()); g != groups.end(); ++g )
   {
      std::string id(g->first);
      std::replace(id.begin(), id.end(), ' ', '_');
      std::replace(id.begin(), id.end(), '(', '_');
      std::replace(id.begin(), id.end(), ')', '_');
      std::replace(id.begin(), id.end(), '-', '_');
      std::replace(id.begin(), id.end(), '/', '_');
      f << "  gr_" << id << "   '" << (g->second.length() > 0 ? g->second : g->first) << "'" << std::endl;
   }
   f << "/;" << std::endl;

   f << "set o Solver and Link Options with one-line description /" << std::endl;
   for( std::list<GamsOption>::iterator d(options.begin()); d != options.end(); ++d )
      f << "  '" << d->name << "'  \"" << makeValidMarkdownString(d->shortdescr) << '"' << std::endl;
   f << "/;" << std::endl;

   f << "$onembedded" << std::endl;
   f << "set optdata(g,o,t,f) /" << std::endl;
   bool havelongdescr = false;
   for( std::list<GamsOption>::iterator d(options.begin()); d != options.end(); ++d )
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
         case GamsOption::Type::BOOL:
            f << "B.(def " << d->defaultval.boolval;
            break;

         case GamsOption::Type::INTEGER:
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

         case GamsOption::Type::REAL:
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

         case GamsOption::Type::CHAR:
            /* no character type in GAMS option files */
            f << "S.(def '" << d->defaultval.charval << '\'';
            break;

         case GamsOption::Type::STRING:
            f << "S.(def '" << makeValidMarkdownString(d->defaultval.stringval) << '\'';
            break;
      }
      if( d->refval >= -1 )
         f << ", ref " << d->refval;
      f << ")" << std::endl;
   }
   f << "/;" << std::endl;

   f << "set oe(o,e) /" << std::endl;
   for( std::list<GamsOption>::iterator d(options.begin()); d != options.end(); ++d )
   {
      for( GamsOption::EnumVals::iterator e(d->enumval.begin()); e != d->enumval.end(); ++e )
      {
         f << "  '" << d->name << "'.'";
         switch( d->type )
         {
            case GamsOption::Type::BOOL :
               f << e->first.boolval;
               break;
            case GamsOption::Type::INTEGER :
               f << e->first.intval;
               break;
            case GamsOption::Type::REAL :
               f << e->first.realval;
               break;
            case GamsOption::Type::CHAR :
               f << e->first.charval;
               break;
            case GamsOption::Type::STRING :
               f << e->first.stringval;
               break;
         }
         f << '\'';
         if( !e->second.empty() )
            f << "  \"" << makeValidMarkdownString(e->second) << '"';
         f << std::endl;
      }
   }
   f << "/;" << std::endl;

   f << "$onempty" << std::endl;

   f << "set odefault(o) /" << std::endl;
   for( std::list<GamsOption>::iterator d(options.begin()); d != options.end(); ++d )
   {
      if( d->defaultdescr.length() == 0 )
         continue;

      f << "  '" << d->name << "' '" << makeValidMarkdownString(d->defaultdescr) << '\'' << std::endl;
   }
   f << "/;" << std::endl;

   f << "set os(o,*) synonyms  /";
   for( std::list<GamsOption>::iterator d(options.begin()); d != options.end(); ++d )
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
      for( std::list<GamsOption>::iterator d(options.begin()); d != options.end(); ++d )
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

void GamsOptions::writeMarkdown()
{
   std::string filename = "opt" + solver + ".md";
   std::cout << "Writing " << filename << std::endl;
   std::ofstream f(filename.c_str());

   f << "# " << solver << " options" << std::endl << std::endl;

   for( auto& group : groups )
   {
      f << "## " << (group.second.empty() ? group.first : group.second) << std::endl << std::endl;

      for( auto& opt : options )
      {
         if( opt.group != group.first )
            continue;

         f << "- **" << opt.name << "**: " << makeValidMarkdownString(opt.shortdescr) << std::endl;
         if( !opt.longdescr.empty() )
            f << std::endl << "    " << makeValidMarkdownString(opt.longdescr) << std::endl;
         f << std::endl;
         if( opt.enumval.empty() )
         {
            switch( opt.type )
            {
               case GamsOption::Type::BOOL :
               {
                  f << "    Possible values: boolean  " << std::endl;
                  if( opt.defaultdescr.empty() )
                     f << "    Default: " << opt.defaultval.boolval << "  " << std::endl;
                  break;
               }
               case GamsOption::Type::INTEGER :
               {
                  if( opt.minval.intval == -INT_MAX )
                  {
                     if( opt.maxval.intval == INT_MAX )
                        f << "    Possible values: integer  " << std::endl;
                     else
                        f << "    Possible values: integer <= " << opt.maxval.intval << "  " << std::endl;
                  }
                  else
                  {
                     if( opt.maxval.intval == INT_MAX )
                        f << "    Possible values: integer >= " << opt.minval.intval << "  " << std::endl;
                     else
                        f << "    Possible values: " << opt.minval.intval << " <= integer <= " << opt.maxval.intval << "  " << std::endl;
                  }
                  if( opt.defaultdescr.empty() )
                  {
                     if( opt.defaultval.intval == -INT_MAX )
                        f << "    Default: -infinity";
                     else if( opt.defaultval.intval == INT_MAX )
                        f << "    Default: infinity";
                     else
                        f << "    Default: " << opt.defaultval.intval;
                     f << "  " << std::endl;
                  }
                  break;
               }
               case GamsOption::Type::REAL :
               {
                  if( opt.minval.realval == -DBL_MAX )
                  {
                     if( opt.maxval.realval == DBL_MAX )
                        f << "    Possible values: real  " << std::endl;
                     else
                        f << "    Possible values: real <= " << opt.maxval.realval << "  " << std::endl;
                  }
                  else
                  {
                     if( opt.maxval.realval == DBL_MAX )
                        f << "    Possible values: real >= " << opt.minval.realval << "  " << std::endl;
                     else
                        f << "    Possible values: " << opt.minval.realval << " <= real <= " << opt.maxval.realval << "  " << std::endl;
                  }
                  if( opt.defaultdescr.empty() )
                  {
                     if( opt.defaultval.realval == -DBL_MAX )
                        f << "    Default: -infinity";
                     else if( opt.defaultval.realval == DBL_MAX )
                        f << "    Default: infinity";
                     else
                        f << "    Default: " << opt.defaultval.realval;
                     f << "  " << std::endl;
                  }
                  break;
               }
               case GamsOption::Type::CHAR :
               {
                  f << "    Possible values: character  " << std::endl;
                  if( opt.defaultdescr.empty() )
                     f << "    Default: " << opt.defaultval.charval << "  " << std::endl;
                  break;
               }
               case GamsOption::Type::STRING :
               {
                  f << "    Possible values: string  " << std::endl;
                  if( opt.defaultdescr.empty() )
                  {
                     f << "    Default: ";
                     if( opt.defaultval.stringval[0] != '\0' )
                        f << makeValidMarkdownString(opt.defaultval.stringval);
                     else
                        f << "_empty_";
                     f << "  " << std::endl;
                  }
                  break;
               }
            }
         }
         else
         {
            f << "    Possible values:" << std::endl << std::endl;
            for( auto& e : opt.enumval )
            {
               bool isdefault;
               f << "    - ";
               switch( opt.type )
               {
                  case GamsOption::Type::BOOL :
                     f << e.first.boolval;
                     isdefault = e.first.boolval == opt.defaultval.boolval;
                     break;
                  case GamsOption::Type::INTEGER :
                     if( e.first.intval == -INT_MAX )
                        f << "-infinity";
                     else if( e.first.intval == INT_MAX )
                        f << "infinity";
                     else
                        f << e.first.intval;
                     isdefault = e.first.intval == opt.defaultval.intval;
                     break;
                  case GamsOption::Type::REAL :
                     if( e.first.realval == -DBL_MAX )
                        f << "-infinity";
                     else if( e.first.realval == DBL_MAX )
                        f << "infinity";
                     else
                        f << e.first.realval;
                     isdefault = e.first.realval == opt.defaultval.realval;
                     break;
                  case GamsOption::Type::CHAR :
                     f << e.first.charval;
                     isdefault = e.first.charval == opt.defaultval.charval;
                     break;
                  case GamsOption::Type::STRING :
                     f << makeValidMarkdownString(e.first.stringval);
                     isdefault = strcmp(e.first.stringval, opt.defaultval.stringval) == 0;
                     break;
               }
               if( isdefault && opt.defaultdescr.empty() )
                  f << " (default)";
               if( !e.second.empty() )
                  f << ": " << makeValidMarkdownString(e.second);
               f << std::endl;
            }
         }
         if( !opt.defaultdescr.empty() )
         {
            f << "    Default: " << opt.defaultdescr << "  " << std::endl;
         }
         if( !opt.synonyms.empty() )
         {
            f << "    Synonyms:";
            for( auto& syn : opt.synonyms )
            {
               f << ' ' << syn;
            }
            f << std::endl;
         }
         f << std::endl;
      }
   }

   f.close();
}
