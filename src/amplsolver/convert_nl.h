// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef CONVERT_NL_H_
#define CONVERT_NL_H_

#include <stdio.h>

#include "def.h"

typedef struct
{
   /* parameters */
   const char* filename;    /**< name of nl file to write */
   int         binary;      /**< whether to print binary .nl */
   int         comments;    /**< whether to print many comments to .nl (text only) */
   int         shortfloat;  /**< whether to print float as short as possible or like AMPL (text only) */

   /* private */
   FILE*       f;           /**< nl file stream */
} convertWriteNLopts;

#ifdef __cplusplus
extern "C" {
#endif

/** prints a double precision floating point number to a string without loosing precision
 *
 * uses DTOA from ASL;  see also https://ampl.com/REFS/rounding.pdf
 * NOTE that DTOA is not threadsafe
 * this routine is adapted from os_dtoa_format of OptimizationServices/OSMathUtil.cpp
 *
 * @return buf
 */
extern
char* convertDoubleToString(
   char*       buf,     /**< string buffer that should be long enough to hold the result */
   double      val      /**< value to convert to string */
);

extern
RETURN convertWriteNL(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
);

/** reads an AMPL solution file and stores solution in GMO */
extern
RETURN convertReadAmplSol(
   struct gmoRec*     gmo,
   const char*        filename
);

#ifdef __cplusplus
}
#endif

#endif /* CONVERT_NL_H_ */
