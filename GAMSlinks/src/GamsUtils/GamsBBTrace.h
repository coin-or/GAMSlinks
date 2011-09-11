/* Copyright (C) GAMS Development and others 2011
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * Author: Stefan Vigerske
 */

#ifndef GAMSBBTRACE_H_
#define GAMSBBTRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GAMS_bbtrace GAMS_BBTRACE;    /**< GAMS branch-and-bound trace data structure */

/** creates a GAMS branch-and-bound trace data structure and initializes trace file for writing
 * @return 0, if successful; nonzero, if failure
 */
extern
int GAMSbbtraceCreate(
   GAMS_BBTRACE**        bbtrace,            /**< buffer to store pointer of GAMS branch-and-bound trace data structure */
   const char*           filename,           /**< name of trace file to write */
   const char*           solverid,           /**< solver identifier string */
   double                infinity,           /**< solver value for infinity */
   int                   nodefreq,           /**< interval in number of nodes when to write N-lines to trace files, 0 to disable N-lines */
   double                timefreq            /**< interval in seconds when to write T-lines to trace files, 0 to disable T-lines */
);

/** closes trace file and frees GAMS branch-and-bound trace data structure */
extern
void GAMSbbtraceFree(
   GAMS_BBTRACE**        bbtrace             /**< pointer to GAMS branch-and-bound trace data structure to be freed */
);

/** adds line to GAMS branch-and-bound trace file */
extern
void GAMSbbtraceAddLine(
   GAMS_BBTRACE*         bbtrace,            /**< GAMS branch-and-bound trace data structure */
   long int              nnodes,             /**< number of enumerated nodes so far */
   double                seconds,            /**< elapsed time in seconds */
   double                dualbnd,            /**< current dual bound */
   double                primalbnd           /**< current primal bound */
);

#ifdef __cplusplus
}
#endif

#endif /* GAMSBBTRACE_H_ */
