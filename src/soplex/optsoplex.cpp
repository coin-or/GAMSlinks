/** Copyright (C) GAMS Development and others 2009-2019
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optsoplex.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

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

   GamsOptions gmsopt("SoPlex");
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

      gmsopt.collect(
         std::string("bool:") + SoPlex::Settings::boolParam.name[i],
         SoPlex::Settings::boolParam.description[i], std::string(),
         SoPlex::Settings::boolParam.defaultValue[i]);
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

      // TODO?? recognize intenums
      int defaultval = SoPlex::Settings::intParam.defaultValue[i];
      int minval = SoPlex::Settings::intParam.lower[i];
      int maxval = SoPlex::Settings::intParam.upper[i];
      std::string defaultdescr;

      if( i == SoPlex::ITERLIMIT )
#ifdef GAMS_BUILD
         defaultdescr = "\\ref GAMSAOiterlim \"GAMS iterlim\"";
#else
         defaultdescr = "GAMS iterlim";
#endif
      if( i == SoPlex::TIMER )
         defaultval = SoPlex::TIMER_WALLCLOCK;

      gmsopt.collect(
         std::string("int:") + SoPlex::Settings::intParam.name[i],
         SoPlex::Settings::intParam.description[i], std::string(),
         defaultval, minval, maxval, defaultdescr);
   }

   for( int i = 0; i < SoPlex::REALPARAM_COUNT; ++i )
   {
      if( i == SoPlex::RATREC_FREQ )
         continue;
      if( i == SoPlex::OBJ_OFFSET )
         continue;

      double defaultval = translateSoplexInfinity(SoPlex::Settings::realParam.defaultValue[i]);
      double minval = translateSoplexInfinity(SoPlex::Settings::realParam.lower[i]);
      double maxval = translateSoplexInfinity(SoPlex::Settings::realParam.upper[i]);
      std::string defaultdescr;

#ifdef GAMS_BUILD
      if( i == SoPlex::TIMELIMIT )
         defaultdescr = "\\ref GAMSAOreslim \"GAMS reslim\"";
      else if( i == SoPlex::OBJLIMIT_UPPER )
         defaultdescr = "\\ref GAMSAOcutoff \"GAMS cutoff\", if minimizing, else +&infin;";
      else if( i == SoPlex::OBJLIMIT_LOWER )
         defaultdescr = "\\ref GAMSAOcutoff \"GAMS cutoff\", if maximizing, else -&infin;";
#else
      if( i == SoPlex::TIMELIMIT )
         defaultdescr = "GAMS reslim";
      else if( i == SoPlex::OBJLIMIT_UPPER )
         defaultdescr = "GAMS cutoff, if minimizing, else +&infin;";
      else if( i == SoPlex::OBJLIMIT_LOWER )
         defaultdescr = "GAMS cutoff, if maximizing, else -&infin;";
#endif

      gmsopt.collect(
         std::string("real:") + SoPlex::Settings::realParam.name[i],
         SoPlex::Settings::realParam.description[i], std::string(),
         defaultval, minval, maxval, true, true, defaultdescr);
   }
   gmsopt.finalize();

#ifdef GAMS_BUILD
   gmsopt.writeDoxygen(true);
#else
   gmsopt.writeMarkdown();
#endif
}
