/* Copyright (C) GAMS Development and others 2011
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * Author: Stefan Vigerske
 */

#include "GamsSolveTrace.h"

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
struct GAMS_solvetrace
{
   FILE*                 tracefile;          /**< trace file */
   double                infinity;           /**< solver value for infinity */
   int                   nodefreq;           /**< interval in number of nodes when to write N-lines to trace files */
   double                timefreq;           /**< interval in seconds when to write T-lines to trace files */
   long int              linecount;          /**< line counter */
   double                starttime;          /**< time when the S-line was written */
   double                lasttime;           /**< last time when a T-line was written */
};

/** creates a GAMS solve trace data structure and initializes trace file for writing
 * @return 0, if successful; nonzero, if failure
 */
int GAMSsolvetraceCreate(
   GAMS_SOLVETRACE**     solvetrace,         /**< buffer to store pointer of GAMS solve trace data structure */
   const char*           filename,           /**< name of trace file to write */
   const char*           solverid,           /**< solver identifier string */
   double                infinity,           /**< solver value for infinity */
   int                   nodefreq,           /**< interval in number of nodes when to write N-lines to trace files, 0 to disable N-lines */
   double                timefreq            /**< interval in seconds when to write T-lines to trace files, 0 to disable T-lines */
)
{
   assert(solvetrace != NULL);
   assert(filename != NULL);
   assert(solverid != NULL);
   assert(nodefreq >= 0);
   assert(timefreq >= 0.0);

   *solvetrace = (GAMS_SOLVETRACE*)malloc(sizeof(GAMS_SOLVETRACE));
   assert(*solvetrace != NULL);

   (*solvetrace)->tracefile = fopen(filename, "w");
   if( (*solvetrace)->tracefile == NULL )
      return 3;

   fprintf((*solvetrace)->tracefile, "* miptrace file %s: ID = %s\n", filename, solverid);
   fprintf((*solvetrace)->tracefile, "* fields are lineNum, seriesID, node, seconds, bestFound, bestBound\n");
   fflush((*solvetrace)->tracefile);

   (*solvetrace)->infinity = infinity;
   (*solvetrace)->nodefreq = nodefreq;
   (*solvetrace)->timefreq = timefreq;

   (*solvetrace)->linecount = 1;
   (*solvetrace)->starttime = 0.0;
   (*solvetrace)->lasttime  = 0.0;

   return 0;
}

/** closes trace file and frees GAMS solve trace data structure */
void GAMSsolvetraceFree(
   GAMS_SOLVETRACE**     solvetrace          /**< pointer to GAMS solve trace data structure to be freed */
)
{
   assert(solvetrace != NULL);
   assert(*solvetrace != NULL);
   assert((*solvetrace)->tracefile != NULL);

   fprintf((*solvetrace)->tracefile, "* miptrace file closed\n");
   fclose((*solvetrace)->tracefile);

   free(*solvetrace);
   *solvetrace = NULL;
}

/** adds line to GAMS solve trace file, given series identifier */
static
void addLine(
   GAMS_SOLVETRACE*      solvetrace,         /**< GAMS solve trace data structure */
   char                  seriesid,           /**< series identifier */
   long int              nnodes,             /**< number of enumerated nodes so far */
   double                seconds,            /**< elapsed time in seconds */
   double                dualbnd,            /**< current dual bound */
   double                primalbnd           /**< current primal bound */
)
{
   fprintf(solvetrace->tracefile, "%ld, %c, %ld, %g", solvetrace->linecount, seriesid, nnodes, seconds);

   if( primalbnd > -solvetrace->infinity && primalbnd < solvetrace->infinity )
      fprintf(solvetrace->tracefile, ", %.10g", primalbnd);
   else
      fputs(", na", solvetrace->tracefile);

   if( dualbnd > -solvetrace->infinity && dualbnd < solvetrace->infinity )
      fprintf(solvetrace->tracefile, ", %.10g", dualbnd);
   else
      fputs(", na", solvetrace->tracefile);

   fputs("\n", solvetrace->tracefile);

   solvetrace->linecount++;
}

/** adds line to GAMS solve trace file */
void GAMSsolvetraceAddLine(
   GAMS_SOLVETRACE*      solvetrace,         /**< GAMS solve trace data structure */
   long int              nnodes,             /**< number of enumerated nodes so far */
   double                dualbnd,            /**< current dual bound */
   double                primalbnd           /**< current primal bound */
)
{
   double time;

   assert(solvetrace != NULL);
   assert(solvetrace->tracefile != NULL);

   time = getTime();

   if( solvetrace->linecount == 1 )
   {
      addLine(solvetrace, 'S', nnodes, 0.0, dualbnd, primalbnd);
      solvetrace->starttime = time;
      solvetrace->lasttime = time;
   }
   else if( solvetrace->nodefreq > 0 && (nnodes % solvetrace->nodefreq == 0) )
   {
      addLine(solvetrace, 'N', nnodes, time - solvetrace->starttime, dualbnd, primalbnd);
   }

   if( solvetrace->timefreq > 0.0 && (time - solvetrace->lasttime >= solvetrace->timefreq) )
   {
      addLine(solvetrace, 'T', nnodes, time - solvetrace->starttime, dualbnd, primalbnd);
      solvetrace->lasttime = time;
   }

   fflush(solvetrace->tracefile);
}

/** adds end line to GAMS solve trace file */
void GAMSsolvetraceAddEndLine(
   GAMS_SOLVETRACE*      solvetrace,         /**< GAMS solve trace data structure */
   long int              nnodes,             /**< number of enumerated nodes so far */
   double                dualbnd,            /**< current dual bound */
   double                primalbnd           /**< current primal bound */
)
{
   assert(solvetrace != NULL);
   assert(solvetrace->tracefile != NULL);

   addLine(solvetrace, 'E', nnodes, getTime() - solvetrace->starttime, dualbnd, primalbnd);

   fflush(solvetrace->tracefile);
}

/** set a new value for infinity */
void GAMSsolvetraceSetInfinity(
   GAMS_SOLVETRACE*      solvetrace,         /**< GAMS solve trace data structure */
   double                infinity            /**< new value for infinity */
)
{
   assert(solvetrace != NULL);

   solvetrace->infinity = infinity;
}