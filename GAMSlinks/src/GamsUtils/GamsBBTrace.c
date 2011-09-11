/* Copyright (C) GAMS Development and others 2011
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * Author: Stefan Vigerske
 */

#include "GamsBBTrace.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if defined(_MSC_VER)

#include <sys/types.h>
#include <sys/timeb.h>

static
double getTime()
{
   struct _timeb timebuffer;
   _ftime_s(&timebuffer);
   return timebuffer.time + timebuffer.millitm / 1000.0;
}

#else

#include <sys/time.h>

static
double getTime()
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_sec + tv.tv_usec / 1000000.0;
}

#endif

/** GAMS branch-and-bound tracing structure */
struct GAMS_bbtrace
{
   FILE*                 tracefile;          /**< trace file */
   double                infinity;           /**< solver value for infinity */
   int                   nodefreq;           /**< interval in number of nodes when to write N-lines to trace files */
   double                timefreq;           /**< interval in seconds when to write T-lines to trace files */
   long int              linecount;          /**< line counter */
   double                starttime;          /**< time when the S-line was written */
   double                lasttime;           /**< last time when a T-line was written */
};

/** creates a GAMS branch-and-bound trace data structure and initializes trace file for writing
 * @return 0, if successful; nonzero, if failure
 */
int GAMSbbtraceCreate(
   GAMS_BBTRACE**        bbtrace,            /**< buffer to store pointer of GAMS branch-and-bound trace data structure */
   const char*           filename,           /**< name of trace file to write */
   const char*           solverid,           /**< solver identifier string */
   double                infinity,           /**< solver value for infinity */
   int                   nodefreq,           /**< interval in number of nodes when to write N-lines to trace files, 0 to disable N-lines */
   double                timefreq            /**< interval in seconds when to write T-lines to trace files, 0 to disable T-lines */
)
{
   assert(bbtrace != NULL);
   assert(filename != NULL);
   assert(solverid != NULL);
   assert(nodefreq >= 0);
   assert(timefreq >= 0.0);

   *bbtrace = (GAMS_BBTRACE*)malloc(sizeof(GAMS_BBTRACE));
   assert(*bbtrace != NULL);

   (*bbtrace)->tracefile = fopen(filename, "w");
   if( (*bbtrace)->tracefile == NULL )
      return 3;

   fprintf((*bbtrace)->tracefile, "* miptrace file %s: ID = %s\n", filename, solverid);
   fprintf((*bbtrace)->tracefile, "* fields are lineNum, seriesID, node, seconds, bestFound, bestBound\n");
   fflush((*bbtrace)->tracefile);

   (*bbtrace)->infinity = infinity;
   (*bbtrace)->nodefreq = nodefreq;
   (*bbtrace)->timefreq = timefreq;

   (*bbtrace)->linecount = 1;
   (*bbtrace)->starttime = 0.0;
   (*bbtrace)->lasttime  = 0.0;

   return 0;
}

/** closes trace file and frees GAMS branch-and-bound trace data structure */
void GAMSbbtraceFree(
   GAMS_BBTRACE**        bbtrace             /**< pointer to GAMS branch-and-bound trace data structure to be freed */
)
{
   assert(bbtrace != NULL);
   assert(*bbtrace != NULL);
   assert((*bbtrace)->tracefile != NULL);

   fprintf((*bbtrace)->tracefile, "* miptrace file closed\n");
   fclose((*bbtrace)->tracefile);

   free(*bbtrace);
   *bbtrace = NULL;
}

/** adds line to GAMS branch-and-bound trace file, given series identifier */
static
void addLine(
   GAMS_BBTRACE*         bbtrace,            /**< GAMS branch-and-bound trace data structure */
   char                  seriesid,           /**< series identifier */
   long int              nnodes,             /**< number of enumerated nodes so far */
   double                seconds,            /**< elapsed time in seconds */
   double                dualbnd,            /**< current dual bound */
   double                primalbnd           /**< current primal bound */
)
{
   fprintf(bbtrace->tracefile, "%ld, %c, %ld, %g", bbtrace->linecount, seriesid, nnodes, seconds);

   if( primalbnd > -bbtrace->infinity && primalbnd < bbtrace->infinity )
      fprintf(bbtrace->tracefile, ", %.10g", primalbnd);
   else
      fputs(", na", bbtrace->tracefile);

   if( dualbnd > -bbtrace->infinity && dualbnd < bbtrace->infinity )
      fprintf(bbtrace->tracefile, ", %.10g", dualbnd);
   else
      fputs(", na", bbtrace->tracefile);

   fputs("\n", bbtrace->tracefile);

   bbtrace->linecount++;
}

/** adds line to GAMS branch-and-bound trace file */
void GAMSbbtraceAddLine(
   GAMS_BBTRACE*         bbtrace,            /**< GAMS branch-and-bound trace data structure */
   long int              nnodes,             /**< number of enumerated nodes so far */
   double                dualbnd,            /**< current dual bound */
   double                primalbnd           /**< current primal bound */
)
{
   double time;

   assert(bbtrace != NULL);
   assert(bbtrace->tracefile != NULL);

   time = getTime();

   if( bbtrace->linecount == 1 )
   {
      addLine(bbtrace, 'S', nnodes, 0.0, dualbnd, primalbnd);
      bbtrace->starttime = time;
      bbtrace->lasttime = time;
   }
   else if( bbtrace->nodefreq > 0 && (nnodes % bbtrace->nodefreq == 0) )
   {
      addLine(bbtrace, 'N', nnodes, time - bbtrace->starttime, dualbnd, primalbnd);
   }

   if( bbtrace->timefreq > 0.0 && (time - bbtrace->lasttime >= bbtrace->timefreq) )
   {
      addLine(bbtrace, 'T', nnodes, time - bbtrace->starttime, dualbnd, primalbnd);
      bbtrace->lasttime = time;
   }

   fflush(bbtrace->tracefile);
}

/** adds end line to GAMS branch-and-bound trace file */
void GAMSbbtraceAddEndLine(
   GAMS_BBTRACE*         bbtrace,            /**< GAMS branch-and-bound trace data structure */
   long int              nnodes,             /**< number of enumerated nodes so far */
   double                dualbnd,            /**< current dual bound */
   double                primalbnd           /**< current primal bound */
)
{
   assert(bbtrace != NULL);
   assert(bbtrace->tracefile != NULL);

   addLine(bbtrace, 'E', nnodes, getTime() - bbtrace->starttime, dualbnd, primalbnd);

   fflush(bbtrace->tracefile);
}

/** set a new value for infinity */
void GAMSbbtraceSetInfinity(
   GAMS_BBTRACE*         bbtrace,            /**< GAMS branch-and-bound trace data structure */
   double                infinity            /**< new value for infinity */
)
{
   assert(bbtrace != NULL);

   bbtrace->infinity = infinity;
}
