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

/** initializing licensing */
void GAMSinitLicensing(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);

/** checks for a GAMS/CPLEX license
 * @return True if a GAMS/CPLEX license is available, false otherwise
 */
bool GAMScheckCPLEXLicense(
   struct palRec*     pal                 /**< GAMS audit and license object */
);

/** checks for GAMS/Ipopt commercial license
 * @return True if GAMS/Ipopt commercial license is available, false otherwise
 */
bool GAMScheckIpoptLicense(
   struct palRec*     pal                 /**< GAMS audit and license object */
);

/** checks for GAMS/SCIP license (academic GAMS license or commercial GAMS/SCIP license)
 * @return True if GAMS/SCIP license was found, false otherwise
 */
bool GAMScheckSCIPLicense(
   struct palRec*     pal                 /**< GAMS audit and license object */
);

#endif /*GAMSLICENSING_H_ */
