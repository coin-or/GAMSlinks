/* Copyright (C) GAMS Development and others 2009-2019
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * @file GamsLicensing.c
 * @author Stefan Vigerske
 */

#include "GamsLicensing.h"

void GAMSinitLicensing(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
   )
{
#ifdef GAMS_BUILD
   char ll[80];
   gevRec* gev;

   assert(pal != NULL);

   gev = (gevRec*)gmoEnvironment(gmo);

   palLicenseRegisterGAMS(pal, 1, gevGetStrOpt(gev, "License1", ll));
   palLicenseRegisterGAMS(pal, 2, gevGetStrOpt(gev, "License2", ll));
   palLicenseRegisterGAMS(pal, 3, gevGetStrOpt(gev, "License3", ll));
   palLicenseRegisterGAMS(pal, 4, gevGetStrOpt(gev, "License4", ll));
   palLicenseRegisterGAMS(pal, 5, gevGetStrOpt(gev, "License5", ll));
   palLicenseRegisterGAMSDone(pal);

   palLicenseCheck(pal,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo));
#endif
}

#if 0
bool GAMScheckGamsLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
)
{
#ifdef GAMS_BUILD
   gevRec* gev = (gevRec*)gmoEnvironment(gmo);

   if( palLicenseCheck(pal,gmoM(gmo),gmoN(gmo),gmoNZ(gmo),gmoNLNZ(gmo),gmoNDisc(gmo)) )
   {
      char msg[256];
      gevLogStat(gev, "The license check failed:");
      while( palLicenseGetMessage(pal, msg, sizeof(msg)) )
         gevLogStat(gev, msg);
      return false;
   }
#endif
   return true;
}
#endif

bool GAMScheckCplexLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
)
{
#ifdef GAMS_BUILD
   assert(pal != NULL);

   if( !palLicenseIsDemoCheckout(pal) && palLicenseCheckSubSys(pal, const_cast<char*>("OCCPCL")) )
      return false;
#endif
   return true;
}

bool GAMScheckIpoptLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                /**< GAMS audit and license object */
)
{
#ifdef GAMS_BUILD
   assert(pal != NULL);

   if( !palLicenseIsDemoCheckout(pal) && palLicenseCheckSubSys(pal, const_cast<char*>("IP")) )
      return false;
#endif
   return true;
}

bool GAMScheckScipLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                /**< GAMS audit and license object */
)
{
#ifdef GAMS_BUILD
   assert(pal != NULL);

   if( !palLicenseIsDemoCheckout(pal) && !palLicenseIsAcademic(pal) && palLicenseCheckSubSys(pal, const_cast<char*>("SC")) )
      return false;
#endif
   return true;
}
