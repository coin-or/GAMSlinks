/* Copyright (C) GAMS Development and others 2009-2019
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * @file GamsLicensing.c
 * @author Stefan Vigerske
 */

#include "GamsLicensing.h"

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
   palLicenseRegisterGAMSDone(pal);

   palLicenseCheck(pal, gmoM(gmo), gmoN(gmo), gmoNZ(gmo), gmoNLNZ(gmo), gmoNDisc(gmo));
}

bool GAMScheckCPLEXLicense(
   struct palRec*     pal                 /**< GAMS audit and license object */
)
{
   assert(pal != NULL);

   if( !palLicenseIsDemoCheckout(pal) && palLicenseCheckSubSys(pal, (char*)"OCCPCL") )
      return false;

   return true;
}

bool GAMScheckIpoptLicense(
   struct palRec*     pal                /**< GAMS audit and license object */
)
{
   assert(pal != NULL);

   if( !palLicenseIsDemoCheckout(pal) && palLicenseCheckSubSys(pal, (char*)"IP") )
      return false;

   return true;
}

bool GAMScheckSCIPLicense(
   struct palRec*     pal                /**< GAMS audit and license object */
)
{
   assert(pal != NULL);

   if( !palLicenseIsDemoCheckout(pal) && !palLicenseIsAcademic(pal) && palLicenseCheckSubSys(pal, (char*)"SC") )
      return false;

   return true;
}
