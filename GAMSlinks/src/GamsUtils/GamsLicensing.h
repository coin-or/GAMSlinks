/* Copyright (C) GAMS Development and others 2009-2019
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * @file GamsLicensing.h
 * @author Stefan Vigerske
 */

#ifndef GAMSLICENSING_H_
#define GAMSLICENSING_H_

#include <stdbool.h>

typedef struct gmoRec* gmoHandle_t;
typedef struct gevRec* gevHandle_t;
typedef struct palRec* palHandle_t;

void GAMSinitLicensing(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);

#if 0
/** calls GAMS license check, if build by GAMS
 * @return True if license check was skipped or successful or model fits into demo size restrictions
 */
bool GAMScheckGamsLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);
#endif

/** registers a GAMS/CPLEX license, if build by GAMS
 * @return True if license was registered or no CPLEX available
 */
bool GAMScheckCplexLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);

/** checks for GAMS/Ipopt commercial license
 * @return True if Ipopt commercial license was found, false otherwise (even for demo models).
 */
bool GAMScheckIpoptLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);

/** checks for GAMS/Scip commercial license
 * @return True if Scip commercial license was found, false otherwise (even for demo models).
 */
bool GAMScheckScipLicense(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);

#endif /*GAMSLICENSING_H_ */
