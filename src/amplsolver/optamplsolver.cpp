/** Copyright (C) GAMS Development and others
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optamplsolver.cpp
  * @author Stefan Vigerske
 */

#include "GamsOptionsSpecWriter.hpp"

int main(int argc, char** argv)
{
   GamsOptions gmsopt("AMPLSOLVER");

   // General parameters
   gmsopt.setGroup("General Options");

   gmsopt.collect("solver", "AMPL solver executable (name or full path)",
      "This option needs to be specified always.",
      "", -2);

   gmsopt.collect("solvername", "AMPL solver name",
      "The name of the solver as used when specifying solver options in AMPL script. "
      "If not given, then name of executable (with path removed) is used. "
      "On Windows, also the extension of the executable name is removed.",
      "", -2);
   gmsopt.collect("options", "Options string to pass to solver", "", "", -2);

   gmsopt.collect("nlbinary", "Whether .nl file should be written in binary form", "", true);

   GamsOption::EnumVals initvals;
   initvals.append("none", "pass no values");
   initvals.append("nondefault", "pass only values that are not at GAMS default");
   initvals.append("all", "pass on all values");

   gmsopt.collect("initprimal", "Which initial variable level values to pass to AMPL solver", "",
      "all", initvals);

   // reusing initvals would give a double-free at the end; should fix that someday...
   GamsOption::EnumVals initvals2;
   initvals2.append("none", "pass no values");
   initvals2.append("nondefault", "pass only values that are not at GAMS default");
   initvals2.append("all", "pass on all values");

   gmsopt.collect("initdual", "Which initial equation marginal values to pass to AMPL solver", "",
      "all", initvals2);

   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen();
#else
   gmsopt.writeMarkdown();
#endif
}
