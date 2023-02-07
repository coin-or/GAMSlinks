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

   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen();
#else
   gmsopt.writeMarkdown();
#endif
}
