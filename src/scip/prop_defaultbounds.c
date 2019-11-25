/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2011 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file   prop_defaultbounds.c
 * @brief  defaultbounds propagator
 * @author Stefan Vigerske
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>

#include "prop_defaultbounds.h"
#include "scip/cons_quadratic.h"
#include "scip/cons_nonlinear.h"


#define PROP_NAME              "defaultbounds"
#define PROP_DESC              "imposes bounds on unbounded variables in nonlinear nonconvex functions"
#define PROP_PRIORITY                 0 /**< propagator priority */ 
#define PROP_FREQ                    -1 /**< propagator frequency */
#define PROP_DELAY                 TRUE /**< should propagation method be delayed, if other propagators found reductions? */
#define PROP_TIMING             SCIP_PROPTIMING_BEFORELP/**< propagation timing mask */
#define PROP_PRESOL_PRIORITY   -1000000 /**< priority of the presolving method (>= 0: before, < 0: after constraint handlers); combined with presolvers */
#define PROP_PRESOL_DELAY          TRUE /**< should presolving be delay, if other presolvers found reductions?  */
#define PROP_PRESOL_MAXROUNDS        -1 /**< maximal number of presolving rounds the presolver participates in (-1: no
                                         *   limit) */




/*
 * Data structures
 */

/** propagator data */
struct SCIP_PropData
{
   SCIP_Real             defaultbound;       /**< default bound to set for unbounded variables */
   SCIP_Bool             changedbound;       /**< indicates whether propagator changed a bound */
};




/*
 * Local methods
 */

static
SCIP_RETCODE boundVarLb(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR*             var,                /**< variable which lower bound to set */
   SCIP_CONS*            cons,               /**< constraint to which variable belongs to */
   SCIP_PROPDATA*        propdata,           /**< propagator data */
   SCIP_Bool*            cutoff,             /**< buffer to store whether a cutoff was detected */
   int*                  nchgbds             /**< buffer to increase by number of changed bounds, or NULL */
)
{
   SCIP_Bool tightened;

   assert(scip != NULL);
   assert(var != NULL);
   assert(SCIPisInfinity(scip, -SCIPvarGetLbLocal(var)));
   assert(cons != NULL);
   assert(propdata != NULL);
   assert(cutoff != NULL);

   SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "Variable <%s> in constraint <%s> has no finite lower bound, setting to %+g. Modify by setting parameter propagating/"PROP_NAME"/bound.\n", SCIPvarGetName(var), SCIPconsGetName(cons), -propdata->defaultbound);

   SCIP_CALL( SCIPtightenVarLb(scip, var, -propdata->defaultbound, FALSE, cutoff, &tightened) );
   if( *cutoff )
   {
      propdata->changedbound = TRUE;
      return SCIP_OKAY;
   }
   if( tightened )
   {
      if( nchgbds != NULL )
         ++*nchgbds;
      propdata->changedbound = TRUE;
   }

   return SCIP_OKAY;
}

static
SCIP_RETCODE boundVarUb(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR*             var,                /**< variable which lower bound to set */
   SCIP_CONS*            cons,               /**< constraint to which variable belongs to */
   SCIP_PROPDATA*        propdata,           /**< propagator data */
   SCIP_Bool*            cutoff,             /**< buffer to store whether a cutoff was detected */
   int*                  nchgbds             /**< buffer to increase by number of changed bounds, or NULL */
)
{
   SCIP_Bool tightened;

   assert(scip != NULL);
   assert(var != NULL);
   assert(SCIPisInfinity(scip, SCIPvarGetUbLocal(var)));
   assert(cons != NULL);
   assert(propdata != NULL);
   assert(cutoff != NULL);

   SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL, "Variable <%s> in constraint <%s> has no finite upper bound, setting to %+g. Modify by setting parameter propagating/"PROP_NAME"/bound.\n", SCIPvarGetName(var), SCIPconsGetName(cons),  propdata->defaultbound);

   SCIP_CALL( SCIPtightenVarUb(scip, var, propdata->defaultbound, FALSE, cutoff, &tightened) );
   if( *cutoff )
   {
      propdata->changedbound = TRUE;
      return SCIP_OKAY;
   }
   if( tightened )
   {
      if( nchgbds != NULL )
         ++*nchgbds;
      propdata->changedbound = TRUE;
   }

   return SCIP_OKAY;
}

static
SCIP_RETCODE boundVariables(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PROP*            prop,               /**< propagator */
   SCIP_Bool*            cutoff,             /**< buffer to store whether a cutoff was detected */
   int*                  nchgbds             /**< buffer to increase by number of changed bounds, or NULL */
   )
{
   SCIP_PROPDATA* propdata;
   SCIP_CONSHDLR* conshdlr;
   SCIP_CONS** conss;
   SCIP_CONS* cons;
   int nconss;
   int c;

   assert(scip != NULL);
   assert(prop != NULL);
   assert(cutoff != NULL);

   *cutoff = FALSE;

   propdata = SCIPpropGetData(prop);
   assert(propdata != NULL);

   if( SCIPisInfinity(scip, propdata->defaultbound) )
      return SCIP_OKAY;

   conshdlr = SCIPfindConshdlr(scip, "quadratic");
   if( conshdlr != NULL )
   {
      SCIP_QUADVARTERM* quadvarterm;
      int nquadvars;
      SCIP_Bool convex;
      SCIP_Bool concave;
      int i;

      conss  = SCIPconshdlrGetConss(conshdlr);
      nconss = SCIPconshdlrGetNConss(conshdlr);

      for( c = 0; c < nconss; ++c )
      {
         cons = conss[c];

         nquadvars = SCIPgetNQuadVarTermsQuadratic(scip, cons);

         convex  = SCIPisConvexQuadratic(scip, cons);
         concave = SCIPisConcaveQuadratic(scip, cons);

         if( convex  && SCIPisInfinity(scip, -SCIPgetLhsQuadratic(scip, cons)) )
            continue;

         if( concave && SCIPisInfinity(scip,  SCIPgetRhsQuadratic(scip, cons)) )
            continue;

         for( i = 0; i < nquadvars; ++i )
         {
            quadvarterm = &SCIPgetQuadVarTermsQuadratic(scip, cons)[i];
            if( quadvarterm->nadjbilin == 0 &&
               (SCIPisInfinity(scip,  SCIPgetRhsQuadratic(scip, cons)) || quadvarterm->sqrcoef > 0.0) &&
               (SCIPisInfinity(scip, -SCIPgetLhsQuadratic(scip, cons)) || quadvarterm->sqrcoef < 0.0) )
               continue; /* skip evidently convex terms */

            if( SCIPisInfinity(scip, -SCIPvarGetLbLocal(quadvarterm->var)) )
            {
               SCIP_CALL( boundVarLb(scip, quadvarterm->var, cons, propdata, cutoff, nchgbds) );

               if( *cutoff )
                  return SCIP_OKAY;
            }

            if( SCIPisInfinity(scip,  SCIPvarGetUbLocal(quadvarterm->var)) )
            {
               SCIP_CALL( boundVarUb(scip, quadvarterm->var, cons, propdata, cutoff, nchgbds) );

               if( *cutoff )
                  return SCIP_OKAY;
            }
         }
      }
   }

   conshdlr = SCIPfindConshdlr(scip, "nonlinear");
   if( conshdlr != NULL )
   {
      SCIP_Real lhs;
      SCIP_Real rhs;
      SCIP_EXPRCURV curvature;
      SCIP_EXPRTREE* exprtree;
      SCIP_VAR** vars;
      int nvars;
      int e;
      int i;

      conss  = SCIPconshdlrGetConss(conshdlr);
      nconss = SCIPconshdlrGetNConss(conshdlr);

      for( c = 0; c < nconss; ++c )
      {
         cons = conss[c];

         lhs = SCIPgetLhsNonlinear(scip, cons);
         rhs = SCIPgetRhsNonlinear(scip, cons);

         SCIP_CALL( SCIPgetCurvatureNonlinear(scip, cons, TRUE, &curvature) );
         if( (curvature & SCIP_EXPRCURV_CONVEX)  && SCIPisInfinity(scip, -lhs) )
            continue;
         if( (curvature & SCIP_EXPRCURV_CONCAVE) && SCIPisInfinity(scip,  rhs) )
            continue;

         /* TODO: do not have exprtrees in nonlinear conss during presolve */
         for( e = 0; e < SCIPgetNExprtreesNonlinear(scip, cons); ++e )
         {
            /* TODO skip if curvature is good, but don't have curvature here */

            exprtree = SCIPgetExprtreesNonlinear(scip, cons)[e];
            nvars = SCIPexprtreeGetNVars(exprtree);
            vars = SCIPexprtreeGetVars(exprtree);

            for( i = 0; i < nvars; ++i )
            {
               if( SCIPisInfinity(scip, -SCIPvarGetLbLocal(vars[i])) )
               {
                  SCIP_CALL( boundVarLb(scip, vars[i], cons, propdata, cutoff, nchgbds) );

                  if( *cutoff )
                     return SCIP_OKAY;
               }

               if( SCIPisInfinity(scip,  SCIPvarGetUbLocal(vars[i])) )
               {
                  SCIP_CALL( boundVarUb(scip, vars[i], cons, propdata, cutoff, nchgbds) );

                  if( *cutoff )
                     return SCIP_OKAY;
               }
            }
         }
      }
   }

   return SCIP_OKAY;
}


/*
 * Callback methods of propagator
 */


/** copy method for propagator plugins (called when SCIP copies plugins) */
#if 0
static
SCIP_DECL_PROPCOPY(propCopyDefaultBounds)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of defaultbounds propagator not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/
 
   return SCIP_OKAY;
}
#else
#define propCopyDefaultBounds NULL
#endif

/** destructor of propagator to free user data (called when SCIP is exiting) */
#if 1
static
SCIP_DECL_PROPFREE(propFreeDefaultBounds)
{  /*lint --e{715}*/
   SCIP_PROPDATA* propdata;

   propdata = SCIPpropGetData(prop);
   assert(propdata != NULL);

   SCIPfreeMemory(scip, &propdata);

   return SCIP_OKAY;
}
#else
#define propFreeDefaultBounds NULL
#endif


/** initialization method of propagator (called after problem was transformed) */
#if 1
static
SCIP_DECL_PROPINIT(propInitDefaultBounds)
{  /*lint --e{715}*/
   SCIP_PROPDATA* propdata;

   propdata = SCIPpropGetData(prop);
   assert(propdata != NULL);

   propdata->changedbound = FALSE;

   return SCIP_OKAY;
}
#else
#define propInitDefaultBounds NULL
#endif


/** deinitialization method of propagator (called before transformed problem is freed) */
#if 0
static
SCIP_DECL_PROPEXIT(propExitDefaultBounds)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of defaultbounds propagator not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define propExitDefaultBounds NULL
#endif


/** presolving initialization method of propagator (called when presolving is about to begin) */
#if 0
static
SCIP_DECL_PROPINITPRE(propInitpreDefaultBounds)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of defaultbounds propagator not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define propInitpreDefaultBounds NULL
#endif


/** presolving deinitialization method of propagator (called after presolving has been finished) */
#if 0
static
SCIP_DECL_PROPEXITPRE(propExitpreDefaultBounds)
{  /*lint --e{715}*/
   SCIP_Bool cutoff;

   SCIP_CALL( boundVariables(scip, prop, &cutoff, NULL) );

   if( cutoff )
      *result = SCIP_CUTOFF;
   else
      *result = SCIP_FEASIBLE;

   return SCIP_OKAY;
}
#else
#define propExitpreDefaultBounds NULL
#endif


/** solving process initialization method of propagator (called when branch and bound process is about to begin) */
#if 0
static
SCIP_DECL_PROPINITSOL(propInitsolDefaultBounds)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of defaultbounds propagator not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define propInitsolDefaultBounds NULL
#endif


/** solving process deinitialization method of propagator (called before branch and bound process data is freed) */
#if 0
static
SCIP_DECL_PROPEXITSOL(propExitsolDefaultBounds)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of defaultbounds propagator not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define propExitsolDefaultBounds NULL
#endif


/** presolving method of propagator */
#if 1
static
SCIP_DECL_PROPPRESOL(propPresolDefaultBounds)
{  /*lint --e{715}*/
   SCIP_Bool cutoff;
   int nchgbdsbefore;

   nchgbdsbefore = *nchgbds;
   SCIP_CALL( boundVariables(scip, prop, &cutoff, nchgbds) );

   if( cutoff )
      *result = SCIP_CUTOFF;
   else if( *nchgbds > nchgbdsbefore )
      *result = SCIP_SUCCESS;
   else
      *result = SCIP_DIDNOTFIND;

   return SCIP_OKAY;
}
#else
#define propPresolDefaultBounds NULL
#endif


/** execution method of propagator */
static
SCIP_DECL_PROPEXEC(propExecDefaultBounds)
{  /*lint --e{715}*/
   *result = SCIP_DIDNOTRUN;

   return SCIP_OKAY;
}


/** propagation conflict resolving method of propagator */
static
SCIP_DECL_PROPRESPROP(propRespropDefaultBounds)
{  /*lint --e{715}*/
   *result = SCIP_DIDNOTRUN;

   return SCIP_OKAY;
}




/*
 * propagator specific interface methods
 */

/** creates the defaultbounds propagator and includes it in SCIP */
SCIP_RETCODE SCIPincludePropDefaultBounds(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_PROPDATA* propdata;

   /* create defaultbounds propagator data */
   SCIP_CALL( SCIPallocMemory(scip, &propdata) );
   propdata->changedbound = FALSE;

   /* include propagator */
   SCIP_CALL( SCIPincludeProp(scip, PROP_NAME, PROP_DESC, PROP_PRIORITY, PROP_FREQ, PROP_DELAY,
         PROP_TIMING, PROP_PRESOL_PRIORITY, PROP_PRESOL_MAXROUNDS, PROP_PRESOL_DELAY,
         propCopyDefaultBounds, propFreeDefaultBounds, propInitDefaultBounds, propExitDefaultBounds, propInitpreDefaultBounds, propExitpreDefaultBounds,
         propInitsolDefaultBounds, propExitsolDefaultBounds, propPresolDefaultBounds, propExecDefaultBounds, propRespropDefaultBounds,
         propdata) );

   /* add defaultbounds propagator parameters */
   SCIP_CALL( SCIPaddRealParam(scip, "propagating/"PROP_NAME"/bound",
      "default bound to set for unbounded variables in nonconvex terms",
      &propdata->defaultbound, FALSE, 10000.0, 0.0, SCIPinfinity(scip), NULL, NULL) );

   return SCIP_OKAY;
}
