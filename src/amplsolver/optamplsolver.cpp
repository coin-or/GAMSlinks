/** Copyright (C) GAMS Development and others 2022
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

   gmsopt.collect("solver", "AMPL solver executable (name or full path)", "", "", -2);

   gmsopt.finalize();

   gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen();
#else
   gmsopt.writeMarkdown();
#endif
}
