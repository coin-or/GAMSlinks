/* Copyright (C) GAMS Development and others 2009-2019
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * @file GamsLicensing.c
 * @author Stefan Vigerske
 */

#include "GamsLicensing.h"

#include <stdlib.h>
#include <assert.h>

#include "palmcc.h"
#include "gmomcc.h"
#include "gevmcc.h"

void GAMSinitLicensing(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
   )
{
   char ll[80];
   gevHandle_t gev;

   assert(pal != NULL);

   gev = (gevHandle_t)gmoEnvironment(gmo);

   palLicenseRegisterGAMS(pal, 1, gevGetStrOpt(gev, "License1", ll));
   palLicenseRegisterGAMS(pal, 2, gevGetStrOpt(gev, "License2", ll));
   palLicenseRegisterGAMS(pal, 3, gevGetStrOpt(gev, "License3", ll));
   palLicenseRegisterGAMS(pal, 4, gevGetStrOpt(gev, "License4", ll));
   palLicenseRegisterGAMS(pal, 5, gevGetStrOpt(gev, "License5", ll));
   palLicenseRegisterGAMS(pal, 6, gevGetStrOpt(gev, "License6", ll));
   palLicenseRegisterGAMSDone(pal);

   palLicenseCheck(pal, gmoM(gmo), gmoN(gmo), gmoNZ(gmo), gmoNLNZ(gmo), gmoNDisc(gmo));
}

bool GAMScheckCPLEXLicense(
   struct palRec*     pal,                /**< GAMS audit and license object */
   bool               strict              /**< whether a CPLEX license code needs to be available */
)
{
   assert(pal != NULL);

   if( !strict && !palLicenseSolverCheck(pal, (char*)"OCCPCL") )
      return true;

   if( !palLicenseCheckSubSys(pal, (char*)"OCCPCL") )
      return true;

   return false;
}

bool GAMScheckIpoptLicense(
   struct palRec*     pal,                /**< GAMS audit and license object */
   bool               strict              /**< whether an IPOPT license code needs to be available */
)
{
   assert(pal != NULL);

   if( !strict && !palLicenseSolverCheck(pal, (char*)"IP") )
      return true;

   if( !palLicenseCheckSubSys(pal, (char*)"IP") )
      return true;

   return false;
}

bool GAMScheckSCIPLicense(
   struct palRec*     pal,                /**< GAMS audit and license object */
   bool               strict              /**< whether a SCIP license code or GAMS academic license needs to be available */
)
{
   assert(pal != NULL);

   if( !strict && !palLicenseSolverCheck(pal, (char*)"SC") )
      return true;

   if( palLicenseIsAcademic(pal) || !palLicenseCheckSubSys(pal, (char*)"SC") )
      return true;

   return false;
}
