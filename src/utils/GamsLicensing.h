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

#ifdef __cplusplus
extern "C" {
#endif

/** initializing licensing */
void GAMSinitLicensing(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);

/** checks for a GAMS/CPLEX license
 *
 * If strict is false, then also returns true if the problem specifications passed on to the GAMS licensing library
 * fit into the demo limit.
 *
 * @return True if a GAMS/CPLEX license is available, false otherwise
 */
bool GAMScheckCPLEXLicense(
   struct palRec*     pal,                /**< GAMS audit and license object */
   bool               strict              /**< whether a CPLEX license code needs to be available */
);

/** checks for GAMS/Ipopt commercial license
 *
 * If strict is false, then also returns true if the problem specifications passed on to the GAMS licensing library
 * fit into the demo limit.
 *
 * @return True if GAMS/Ipopt commercial license is available, false otherwise
 */
bool GAMScheckIpoptLicense(
   struct palRec*     pal,                /**< GAMS audit and license object */
   bool               strict              /**< whether an IPOPT license code needs to be available */
);

/** checks for GAMS/SCIP license (academic GAMS license or commercial GAMS/SCIP license)
 *
 * If strict is false, then also returns true if the problem specifications passed on to the GAMS licensing library
 * fit into the demo limit.
 *
 * @return True if GAMS/SCIP license was found, false otherwise
 */
bool GAMScheckSCIPLicense(
   struct palRec*     pal,                /**< GAMS audit and license object */
   bool               strict              /**< whether an SCIP license code or GAMS academic license needs to be available */
);

/** initializes GAMS licensed HSL routines if a commercial Ipopt license is available
 *
 * @return True if a commercial Ipopt license is available, and false otherwise.
 */
bool GAMSHSLInit(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct palRec*     pal                 /**< GAMS audit and license object */
);

#ifdef __cplusplus
}
#endif

#endif /*GAMSLICENSING_H_ */
