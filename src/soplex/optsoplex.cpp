/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optbonmin.cpp
  * @author Stefan Vigerske
 */

#include "GamsOptionsSpecWriter.hpp"
#include "GAMSlinksConfig.h"

#include "soplex.h"
using namespace soplex;

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

int main(int argc, char** argv)
{
   SoPlex soplex;

   GamsOptions::OPTVAL defaultval, minval, maxval;
   GamsOptions::ENUMVAL enumval;
   std::string tmpstr;
   std::string descr;
   std::string defaultdescr;

   GamsOptions gmsopt("soplex");
   gmsopt.setSeparator("=");
   // gmsopt.setStringQuote("\"");
   gmsopt.setGroup("soplex");
   gmsopt.setEolChars("#");

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
         GamsOptions::OPTTYPE_BOOL, defaultval, minval, maxval, enumval, defaultdescr);
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
      if( i == SoPlex::SOLUTION_POLISHING )
         continue;

      // TODO recognize intenums
      defaultval.intval = SoPlex::Settings::intParam.defaultValue[i];
      minval.intval = SoPlex::Settings::intParam.lower[i];
      maxval.intval = SoPlex::Settings::intParam.upper[i];
      defaultdescr = std::string();

      if( i == SoPlex::ITERLIMIT )
         defaultdescr = "GAMS iterlim";
      if( i == SoPlex::TIMER )
         defaultval.intval = SoPlex::TIMER_WALLCLOCK;

      gmsopt.collect(
         std::string("int:") + SoPlex::Settings::intParam.name[i],
         SoPlex::Settings::intParam.description[i], std::string(),
         GamsOptions::OPTTYPE_INTEGER, defaultval, minval, maxval, enumval, defaultdescr);
   }

   for( int i = 0; i < SoPlex::REALPARAM_COUNT; ++i )
   {
      if( i == SoPlex::RATREC_FREQ )
         continue;
      if( i == SoPlex::OBJ_OFFSET )
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
         GamsOptions::OPTTYPE_REAL, defaultval, minval, maxval, enumval, defaultdescr);
   }

   gmsopt.write(true);
}
