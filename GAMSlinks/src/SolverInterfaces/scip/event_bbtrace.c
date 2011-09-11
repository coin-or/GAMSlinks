/* Copyright (C) GAMS Development and others 2011
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * Author: Stefan Vigerske
 */

/**@file   event_bbtrace.c
 * @ingroup EVENTS 
 * @brief  eventhdlr to write GAMS branch-and-bound trace file
 * @author Stefan Vigerske
 */

/*--+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include "event_bbtrace.h"
#include "GamsBBTrace.h"

#include <assert.h>

#define EVENTHDLR_NAME         "bbtrace"
#define EVENTHDLR_DESC         "event handler to write GAMS branch-and-bound trace file"


/*
 * Data structures
 */

/** event handler data */
struct SCIP_EventhdlrData
{
   GAMS_BBTRACE*         bbtrace;            /**< GAMS bbtrace object */
   char*                 filename;           /**< name of trace file */
   int                   nodefreq;           /**< node frequency of writing to trace file */
   SCIP_Real             timefreq;           /**< time frequency of writing to trace file */
   int                   filterpos;          /**< position of node event in SCIPs eventfilter */
};

/*
 * Local methods
 */

/* put your local methods here, and declare them static */



/*
 * Callback methods of event handler
 */

/** copy method for event handler plugins (called when SCIP copies plugins) */
#if 0
static
SCIP_DECL_EVENTCOPY(eventCopyBBtrace)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of bbtrace dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define eventCopyBBtrace NULL
#endif

/** destructor of event handler to free user data (called when SCIP is exiting) */
#if 1
static
SCIP_DECL_EVENTFREE(eventFreeBBtrace)
{  /*lint --e{715}*/
   SCIP_EVENTHDLRDATA* eventhdlrdata;

   eventhdlrdata = SCIPeventhdlrGetData(eventhdlr);
   assert(eventhdlrdata != NULL);
   assert(eventhdlrdata->bbtrace == NULL);

   SCIPfreeMemory(scip, &eventhdlrdata);

   return SCIP_OKAY;
}
#else
#define eventFreeBBtrace NULL
#endif

/** initialization method of event handler (called after problem was transformed) */
#if 1
static
SCIP_DECL_EVENTINIT(eventInitBBtrace)
{  /*lint --e{715}*/
   SCIP_EVENTHDLRDATA* eventhdlrdata;
   char solverid[20];
   int rc;

   eventhdlrdata = SCIPeventhdlrGetData(eventhdlr);
   assert(eventhdlrdata != NULL);

   if( eventhdlrdata->filename[0] == '\0' )
      return SCIP_OKAY;

#if SCIP_SUBVERSION > 0
   (void) SCIPsnprintf(solverid, sizeof(solverid), "SCIP %d.%d.%d.%d", SCIP_VERSION/100, (SCIP_VERSION%100)/10, SCIP_VERSION%10, SCIP_SUBVERSION);
#else
   (void) SCIPsnprintf(solverid, sizeof(solverid), "SCIP %d.%d.%d", SCIP_VERSION/100, (SCIP_VERSION%100)/10, SCIP_VERSION%10);
#endif
   rc = GAMSbbtraceCreate(&eventhdlrdata->bbtrace, eventhdlrdata->filename, solverid, SCIPinfinity(scip), eventhdlrdata->nodefreq, eventhdlrdata->timefreq);
   if( rc != 0 )
   {
      SCIPerrorMessage("GAMSbbtraceCreate returned with error %d, trace file name = %s\n", rc, eventhdlrdata->filename);
      return SCIP_ERROR;
   }

   SCIP_CALL( SCIPcatchEvent(scip, SCIP_EVENTTYPE_NODESOLVED, eventhdlr, (SCIP_EVENTDATA*)eventhdlrdata->bbtrace, &eventhdlrdata->filterpos) );

   return SCIP_OKAY;
}
#else
#define eventInitBBtrace NULL
#endif

/** deinitialization method of event handler (called before transformed problem is freed) */
#if 1
static
SCIP_DECL_EVENTEXIT(eventExitBBtrace)
{  /*lint --e{715}*/
   SCIP_EVENTHDLRDATA* eventhdlrdata;

   eventhdlrdata = SCIPeventhdlrGetData(eventhdlr);
   assert(eventhdlrdata != NULL);

   if( eventhdlrdata->bbtrace == NULL )
      return SCIP_OKAY;

   SCIP_CALL( SCIPdropEvent(scip, SCIP_EVENTTYPE_NODESOLVED, eventhdlr, (SCIP_EVENTDATA*)eventhdlrdata->bbtrace, eventhdlrdata->filterpos) );

   GAMSbbtraceAddEndLine(eventhdlrdata->bbtrace, SCIPgetNTotalNodes(scip),
      SCIPgetDualbound(scip),
      SCIPgetNSols(scip) > 0 ? SCIPgetSolOrigObj(scip, SCIPgetBestSol(scip)) : SCIPgetObjsense(scip) * SCIPinfinity(scip));

   GAMSbbtraceFree(&eventhdlrdata->bbtrace);
   assert(eventhdlrdata->bbtrace == NULL);

   return SCIP_OKAY;
}
#else
#define eventExitBBtrace NULL
#endif

/** solving process initialization method of event handler (called when branch and bound process is about to begin) */
#if 0
static
SCIP_DECL_EVENTINITSOL(eventInitsolBBtrace)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of bbtrace event handler not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define eventInitsolBBtrace NULL
#endif

/** solving process deinitialization method of event handler (called before branch and bound process data is freed) */
#if 0
static
SCIP_DECL_EVENTEXITSOL(eventExitsolBBtrace)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of bbtrace event handler not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define eventExitsolBBtrace NULL
#endif

/** frees specific event data */
#if 0
static
SCIP_DECL_EVENTDELETE(eventDeleteBBtrace)
{  /*lint --e{715}*/
   assert(eventhdlr != NULL);

   return SCIP_OKAY;
}
#else
#define eventDeleteBBtrace NULL
#endif

/** execution method of event handler */
static
SCIP_DECL_EVENTEXEC(eventExecBBtrace)
{  /*lint --e{715}*/
   assert(event != NULL);
   assert(SCIPeventGetType(event) & SCIP_EVENTTYPE_NODESOLVED);
   assert(eventdata != NULL);

   GAMSbbtraceAddLine((GAMS_BBTRACE*)eventdata, SCIPgetNTotalNodes(scip),
      SCIPgetDualbound(scip),
      SCIPgetNSols(scip) > 0 ? SCIPgetSolOrigObj(scip, SCIPgetBestSol(scip)) : SCIPgetObjsense(scip) * SCIPinfinity(scip));

   return SCIP_OKAY;
}

/** creates event handler for bbtrace event */
SCIP_RETCODE SCIPincludeEventHdlrBBtrace(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_EVENTHDLRDATA* eventhdlrdata;

   /* create bbtrace event handler data */
   SCIP_CALL( SCIPallocMemory(scip, &eventhdlrdata) );
   eventhdlrdata->bbtrace = NULL;
   eventhdlrdata->filename = NULL;

   /* include event handler into SCIP */
   SCIP_CALL( SCIPincludeEventhdlr(scip, EVENTHDLR_NAME, EVENTHDLR_DESC,
         eventCopyBBtrace,
         eventFreeBBtrace, eventInitBBtrace, eventExitBBtrace,
         eventInitsolBBtrace, eventExitsolBBtrace, eventDeleteBBtrace, eventExecBBtrace,
         eventhdlrdata) );

   /* add bbtrace event handler parameters */
   SCIP_CALL( SCIPaddStringParam(scip, "gams/miptrace/file",
      "name of file where to write branch-and-bound trace information too",
      &eventhdlrdata->filename, FALSE, "", NULL, NULL) );

   SCIP_CALL( SCIPaddIntParam(scip, "gams/miptrace/nodefreq",
      "frequency in number of nodes when to write branch-and-bound trace information, 0 to disable",
      &eventhdlrdata->nodefreq, FALSE, 100, 0, INT_MAX, NULL, NULL) );

   SCIP_CALL( SCIPaddRealParam(scip, "gams/miptrace/timefreq",
      "frequency in seconds when to write branch-and-bound trace information, 0.0 to disable",
      &eventhdlrdata->timefreq, FALSE, 5.0, 0.0, SCIP_REAL_MAX, NULL, NULL) );

   return SCIP_OKAY;
}
