/* Copyright (C) GAMS Development and others 2011
 * All Rights Reserved.
 * This code is published under the Eclipse Public License.
 *
 * Author: Stefan Vigerske
 */

/**@file   reader_gmo.c
 * @ingroup FILEREADERS
 * @brief  GMO file reader
 * @author Stefan Vigerske
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>

#include "reader_gmo.h"

#include "gmomcc.h"
#include "gevmcc.h"
#include "GamsCompatibility.h"
#include "GamsNLinstr.h"

#include "scip/cons_linear.h"
#include "scip/cons_bounddisjunction.h"
#include "scip/cons_quadratic.h"
#include "scip/cons_sos1.h"
#include "scip/cons_sos2.h"
#if 0
#include "cons_nonlinear.h"
#endif

#define READER_NAME             "gmoreader"
#define READER_DESC             "Gams Control file reader (using GMO API)"
#define READER_EXTENSION        "dat"


/*
 * Data structures
 */

/** data for gmo reader */
struct SCIP_ReaderData
{
   gmoHandle_t           gmo;                /**< GAMS model object */
   gevHandle_t           gev;                /**< GAMS environment */
};

/** problem data */
struct SCIP_ProbData
{
   gmoHandle_t           gmo;                /**< GAMS model object */
   gevHandle_t           gev;                /**< GAMS environment */
   SCIP_VAR**            vars;               /**< SCIP variables as corresponding to GMO variables */
   SCIP_VAR*             objvar;             /**< SCIP variable used to model objective function */
   SCIP_VAR*             objconst;           /**< SCIP variable used to model objective constant */
#if 0
   char*                 conssattrfile;
#endif
};

/*
 * Callback methods of probdata
 */

/** frees user data of original problem (called when the original problem is freed)
 */
static
SCIP_DECL_PROBDELORIG(probdataDelOrigGmo)
{
   int i;

   assert((*probdata)->gmo != NULL);
   assert((*probdata)->gev != NULL);
   assert((*probdata)->vars != NULL);

   for( i = 0; i < gmoN((*probdata)->gmo); ++i )
   {
      SCIP_CALL( SCIPreleaseVar(scip, &(*probdata)->vars[i]) );
   }
   SCIPfreeMemoryArray(scip, &(*probdata)->vars);

   if( (*probdata)->objvar != NULL )
   {
      SCIP_CALL( SCIPreleaseVar(scip, &(*probdata)->objvar) );
   }

   if( (*probdata)->objconst != NULL )
   {
      SCIP_CALL( SCIPreleaseVar(scip, &(*probdata)->objconst) );
   }

   SCIPfreeMemory(scip, probdata);

   return SCIP_OKAY;
}

/** creates user data of transformed problem by transforming the original user problem data
 *  (called after problem was transformed)
 */
#if 0
static
SCIP_DECL_PROBTRANS(probdataTransGmo)
{

}
#else
#define probdataTransGmo NULL
#endif

/** frees user data of transformed problem (called when the transformed problem is freed)
 */
#if 0
static
SCIP_DECL_PROBDELTRANS(probdataDelTransGmo)
{
   return SCIP_OKAY;
}
#else
#define probdataDelTransGmo NULL
#endif


/** solving process initialization method of transformed data (called before the branch and bound process begins)
 */
#if 0
static
SCIP_DECL_PROBINITSOL(probdataInitSolGmo)
{

}
#else
#define probdataInitSolGmo NULL
#endif

/** solving process deinitialization method of transformed data (called before the branch and bound data is freed)
 */
#if 0
static
SCIP_DECL_PROBEXITSOL(probdataExitSolGmo)
{
   return SCIP_OKAY;
}
#else
#define probdataExitSolGmo NULL
#endif

/** copies user data of source SCIP for the target SCIP
 */
#if 0
static
SCIP_DECL_PROBCOPY(probdataCopyGmo)
{

}
#else
#define probdataCopyGmo NULL
#endif

/*
 * Local methods of reader
 */


#if 0
/** ensures that an array of variables has at least a given length */
static
SCIP_RETCODE ensureVarsSize(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR***           vars,               /**< pointer to variables array */
   int*                  varssize,           /**< pointer to length of variables array */
   int                   nvars               /**< desired minimal length of array */
   )
{
   assert(scip != NULL);
   assert(vars != NULL);
   assert(varssize != NULL);

   if( nvars < *varssize )
      return SCIP_OKAY;

   *varssize = SCIPcalcMemGrowSize(scip, nvars);
   SCIP_CALL( SCIPreallocBufferArray(scip, vars, *varssize) );
   assert(nvars <= *varssize);

   return SCIP_OKAY;
}

/** creates an expression tree from given GAMS nonlinear instructions */
static
SCIP_RETCODE makeExprtree(
   SCIP*                 scip,               /**< SCIP data structure */
   int                   codelen,
   int*                  opcodes,
   int*                  fields,
   int                   constantlen,
   SCIP_Real*            constants,
   SCIP_Real             factor,             /**< a factor to multiply the resulting expression with */
   SCIP_EXPRTREE**       exprtree            /**< buffer where to store expression tree */
)
{
   SCIP_PROBDATA* probdata;
   gmoHandle_t   gmo;
   gevHandle_t   gev;
   BMS_BLKMEM*   blkmem;
   SCIP_HASHMAP* var2idx;
   SCIP_EXPR**   stack;
   int           stackpos;
   int           stacksize;
   SCIP_VAR**    vars;
   int           nvars;
   int           varssize;
   int           pos;
   SCIP_EXPR*    e;
   SCIP_EXPR*    term1;
   SCIP_EXPR*    term2;
   GamsOpCode    opcode;
   int           address;
   int           varidx;
   int           nargs;

   assert(scip != NULL);
   assert(opcodes != NULL);
   assert(fields != NULL);
   assert(constants != NULL);
   assert(exprtree != NULL);

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);
   assert(probdata->gmo != NULL);
   assert(probdata->gev != NULL);
   assert(probdata->vars != NULL);

   gmo = probdata->gmo;
   gev = probdata->gev;

   blkmem = SCIPblkmem(scip);

   stackpos = 0;
   stacksize = 20;
   SCIP_CALL( SCIPallocBufferArray(scip, &stack, stacksize) );

   nvars = 0;
   varssize = 10;
   SCIP_CALL( SCIPallocBufferArray(scip, &vars, varssize) );
   SCIP_CALL( SCIPhashmapCreate(&var2idx, blkmem, SCIPcalcHashtableSize(SCIPgetNVars(scip))) );

   for( pos = 0; pos < codelen; ++pos )
   {
      opcode = (GamsOpCode)opcodes[pos];
      address = fields[pos]-1;

      SCIPdebugMessage("%s: ", GamsOpCodeName[opcode]);

      e = NULL;

      switch( opcode )
      {
         case nlNoOp: /* no operation */
         case nlStore: /* store row */
         case nlHeader:
         {
            SCIPdebugPrintf("ignored\n");
            break;
         }

         case nlPushV: /* push variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("push variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            if( !SCIPhashmapExists(var2idx, probdata->vars[address]) )
            {
               /* add variable to list of variables */
               SCIP_CALL( ensureVarsSize(scip, &vars, &varssize, nvars+1) );
               assert(nvars < varssize);
               vars[nvars] = probdata->vars[address];
               varidx = nvars;
               ++nvars;
               SCIP_CALL( SCIPhashmapInsert(var2idx, (void*)vars[varidx], (void*)(size_t)varidx) );
            }
            else
            {
               varidx = (int)(size_t)SCIPhashmapGetImage(var2idx, (void*)probdata->vars[address]);
               assert(varidx >= 0);
               assert(varidx < nvars);
               assert(vars[varidx] = probdata->vars[address]);
            }
            SCIPexprCreate(blkmem, &e, SCIP_EXPR_VARIDX, varidx);
            break;
         }

         case nlPushI: /* push constant */
         {
            SCIPdebugPrintf("push constant %g\n", constants[address]);
            SCIPexprCreate(blkmem, &e, SCIP_EXPR_CONST, constants[address]);
            break;
         }

         case nlPushZero: /* push zero */
         {
            SCIPdebugPrintf("push constant zero\n");

            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_CONST, 0.0) );
            break;
         }

         case nlAdd : /* add */
         {
            SCIPdebugPrintf("add\n");
            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_PLUS, term1, term2) );

            break;
         }

         case nlAddV: /* add variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("add variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            if( !SCIPhashmapExists(var2idx, probdata->vars[address]) )
            {
               /* add variable to list of variables */
               SCIP_CALL( ensureVarsSize(scip, &vars, &varssize, nvars+1) );
               assert(nvars < varssize);
               vars[nvars] = probdata->vars[address];
               varidx = nvars;
               ++nvars;
               SCIP_CALL( SCIPhashmapInsert(var2idx, (void*)vars[varidx], (void*)(size_t)varidx) );
            }
            else
            {
               varidx = (int)(size_t)SCIPhashmapGetImage(var2idx, (void*)probdata->vars[address]);
               assert(varidx >= 0);
               assert(varidx < nvars);
               assert(vars[varidx] = probdata->vars[address]);
            }

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_VARIDX, varidx) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_PLUS, term1, term2) );
            break;
         }

         case nlAddI: /* add immediate */
         {
            SCIPdebugPrintf("add constant %g\n", constants[address]);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_CONST, constants[address]) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_PLUS, term1, term2) );
            break;
         }

         case nlSub: /* substract */
         {
            SCIPdebugPrintf("substract\n");

            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MINUS, term2, term1) );
            break;
         }

         case nlSubV: /* subtract variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("subtract variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            if( !SCIPhashmapExists(var2idx, probdata->vars[address]) )
            {
               /* add variable to list of variables */
               SCIP_CALL( ensureVarsSize(scip, &vars, &varssize, nvars+1) );
               assert(nvars < varssize);
               vars[nvars] = probdata->vars[address];
               varidx = nvars;
               ++nvars;
               SCIP_CALL( SCIPhashmapInsert(var2idx, (void*)vars[varidx], (void*)(size_t)varidx) );
            }
            else
            {
               varidx = (int)(size_t)SCIPhashmapGetImage(var2idx, (void*)probdata->vars[address]);
               assert(varidx >= 0);
               assert(varidx < nvars);
               assert(vars[varidx] = probdata->vars[address]);
            }

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_VARIDX, varidx) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MINUS, term1, term2) );
            break;
         }

         case nlSubI: /* subtract immediate */
         {
            SCIPdebugPrintf("subtract constant %g\n", constants[address]);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_CONST, constants[address]) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MINUS, term1, term2) );
            break;
         }

         case nlMul: /* multiply */
         {
            SCIPdebugPrintf("subtract\n");

            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term2, term1) );
            break;
         }

         case nlMulV: /* multiply variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("multiply variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            if( !SCIPhashmapExists(var2idx, probdata->vars[address]) )
            {
               /* add variable to list of variables */
               SCIP_CALL( ensureVarsSize(scip, &vars, &varssize, nvars+1) );
               assert(nvars < varssize);
               vars[nvars] = probdata->vars[address];
               varidx = nvars;
               ++nvars;
               SCIP_CALL( SCIPhashmapInsert(var2idx, (void*)vars[varidx], (void*)(size_t)varidx) );
            }
            else
            {
               varidx = (int)(size_t)SCIPhashmapGetImage(var2idx, (void*)probdata->vars[address]);
               assert(varidx >= 0);
               assert(varidx < nvars);
               assert(vars[varidx] = probdata->vars[address]);
            }

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_VARIDX, varidx) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term1, term2) );
            break;
         }

         case nlMulI: /* multiply immediate */
         {
            SCIPdebugPrintf("multiply constant %g\n", constants[address]);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_CONST, constants[address]) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term1, term2) );
            break;
         }

         case nlMulIAdd:
         {
            SCIP_EXPR* c;
            SCIP_EXPR* prod;

            SCIPdebugPrintf("multiply constant %g and add\n", constants[address]);

            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &c, SCIP_EXPR_CONST, constants[address]) );
            SCIP_CALL( SCIPexprCreate(blkmem, &prod, SCIP_EXPR_MUL, term1, c) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_PLUS, prod, term2) );
            break;
         }

         case nlDiv: /* divide */
         {
            SCIPdebugPrintf("divide\n");

            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_DIV, term2, term1) );
            break;
         }

         case nlDivV: /* divide variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("divide variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            if( !SCIPhashmapExists(var2idx, probdata->vars[address]) )
            {
               /* add variable to list of variables */
               SCIP_CALL( ensureVarsSize(scip, &vars, &varssize, nvars+1) );
               assert(nvars < varssize);
               vars[nvars] = probdata->vars[address];
               varidx = nvars;
               ++nvars;
               SCIP_CALL( SCIPhashmapInsert(var2idx, (void*)vars[varidx], (void*)(size_t)varidx) );
            }
            else
            {
               varidx = (int)(size_t)SCIPhashmapGetImage(var2idx, (void*)probdata->vars[address]);
               assert(varidx >= 0);
               assert(varidx < nvars);
               assert(vars[varidx] = probdata->vars[address]);
            }

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_VARIDX, varidx) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_DIV, term1, term2) );
            break;
         }

         case nlDivI: /* divide immediate */
         {
            SCIPdebugPrintf("divide constant %g\n", constants[address]);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_CONST, constants[address]) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term1, term2) );
            break;
         }

         case nlUMin: /* unary minus */
         {
            SCIPdebugPrintf("negate\n");

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_CONST, -1.0) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term1, term2) );
            break;
         }

         case nlUMinV: /* unary minus variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("push negated variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            if( !SCIPhashmapExists(var2idx, probdata->vars[address]) )
            {
               /* add variable to list of variables */
               SCIP_CALL( ensureVarsSize(scip, &vars, &varssize, nvars+1) );
               assert(nvars < varssize);
               vars[nvars] = probdata->vars[address];
               varidx = nvars;
               ++nvars;
               SCIP_CALL( SCIPhashmapInsert(var2idx, (void*)vars[varidx], (void*)(size_t)varidx) );
            }
            else
            {
               varidx = (int)(size_t)SCIPhashmapGetImage(var2idx, (void*)probdata->vars[address]);
               assert(varidx >= 0);
               assert(varidx < nvars);
               assert(vars[varidx] = probdata->vars[address]);
            }

            SCIP_CALL( SCIPexprCreate(blkmem, &term1, SCIP_EXPR_CONST, -1.0) );
            SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_VARIDX, varidx) );
            SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term1, term2) );
            break;
         }

         case nlFuncArgN:
         {
            SCIPdebugPrintf("set number of arguments = %d\n", address);
            nargs = address;
            break;
         }

         case nlCallArg1:
         case nlCallArg2:
         case nlCallArgN:
         {
            GamsFuncCode func;

            SCIPdebugPrintf("call function ");

            func = (GamsFuncCode)(address+1); /* here the shift by one was not a good idea */

            switch( func )
            {
               case fnmin:
               {
                  SCIPdebugPrintf("min\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MIN, term1, term2) );
                  break;
               }

               case fnmax:
               {
                  SCIPdebugPrintf("max\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MAX, term1, term2) );
                  break;
               }

               case fnsqr:
               {
                  SCIPdebugPrintf("square\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_SQUARE, term1) );
                  break;
               }

               case fnexp:
               {
                  SCIPdebugPrintf("exp\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_EXP, term1) );
                  break;
               }

               case fnlog:
               {
                  SCIPdebugPrintf("log\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_LOG, term1) );
                  break;
               }

               case fnlog10:
               case fnsllog10:
               case fnsqlog10:
               {
                  SCIPdebugPrintf("log10 = ln * 1/ln(10)\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_LOG, term1) );
                  SCIP_CALL( SCIPexprCreate(blkmem, &term1, SCIP_EXPR_CONST, 1.0/log(10.0)) );
                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term2, term1) );
                  break;
               }

               case fnlog2:
               {
                  SCIPdebugPrintf("log2 = ln * 1/ln(2)\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_LOG, term1) );
                  SCIP_CALL( SCIPexprCreate(blkmem, &term1, SCIP_EXPR_CONST, 1.0/log(2.0)) );
                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, term2, term1) );
                  break;
               }

               case fnsqrt:
               {
                  SCIPdebugPrintf("sqrt\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_SQRT, term1) );
                  break;
               }

               case fncos:
               {
                  SCIPdebugPrintf("cos\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_COS, term1) );
                  break;
               }

               case fnsin:
               {
                  SCIPdebugPrintf("sin\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_SIN, term1) );
                  break;
               }

               case fntan:
               {
                  SCIPdebugPrintf("tan\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_TAN, term1) );
                  break;
               }

               case fnpower: /* x ^ y */
               case fnrpower: /* x ^ y */
               case fncvpower: /* constant ^ x */
               case fnvcpower: /* x ^ constant */
               {
                  SCIPdebugPrintf("power\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  assert(func != fncvpower || SCIPexprGetOperator(term2) == SCIP_EXPR_CONST);
                  assert(func != fnvcpower || SCIPexprGetOperator(term1) == SCIP_EXPR_CONST);

                  if( SCIPexprGetOperator(term1) == SCIP_EXPR_CONST )
                  {
                     /* use intpower if exponent is an integer constant, otherwise use realpower */
                     if( SCIPisIntegral(scip, SCIPexprGetOpReal(term1)) )
                     {
                        SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_INTPOWER, term2, (int)SCIPexprGetOpReal(term1)) );
                        SCIPexprFreeDeep(blkmem, &term1);
                     }
                     else
                     {
                        SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_REALPOWER, term2, SCIPexprGetOpReal(term1)) );
                        SCIPexprFreeDeep(blkmem, &term1);
                     }
                  }
                  else
                  {
                     /* term2^term1 = exp(log(term2)*term1) */
                     SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_LOG, term2) );
                     SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_MUL, e, term1) );
                     SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_EXP, e) );
                  }

                  break;
               }

               case fnpi:
               {
                  SCIPdebugPrintf("pi\n");

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_CONST, M_PI) );
                  break;
               }

               case fndiv:
               case fndiv0:
               {
                  SCIPdebugPrintf("divide\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_DIV, term2, term1) );
                  break;
               }

               case fnslrec:
               case fnsqrec: /* 1/x */
               {
                  SCIPdebugPrintf("reciprocal\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &term2, SCIP_EXPR_CONST, 1.0) );
                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_DIV, term2, term1) );
                  break;
               }

               case fnabs:
               {
                  SCIPdebugPrintf("abs\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_ABS, term1) );
                  break;
               }

               /* @todo some of these we could also support */
               case fnerrf:
               case fnceil: case fnfloor: case fnround:
               case fnmod: case fntrunc: case fnsign:
               case fnarctan: case fndunfm:
               case fndnorm: case fnerror: case fnfrac: case fnerrorl:
               case fnfact /* factorial */:
               case fnunfmi /* uniform random number */:
               case fnncpf /* fischer: sqrt(x1^2+x2^2+2*x3) */:
               case fnncpcm /* chen-mangasarian: x1-x3*ln(1+exp((x1-x2)/x3))*/:
               case fnentropy /* x*ln(x) */: case fnsigmoid /* 1/(1+exp(-x)) */:
               case fnboolnot: case fnbooland:
               case fnboolor: case fnboolxor: case fnboolimp:
               case fnbooleqv: case fnrelopeq: case fnrelopgt:
               case fnrelopge: case fnreloplt: case fnrelople:
               case fnrelopne: case fnifthen:
               case fnedist /* euclidian distance */:
               case fncentropy /* x*ln((x+d)/(y+d))*/:
               case fngamma: case fnloggamma: case fnbeta:
               case fnlogbeta: case fngammareg: case fnbetareg:
               case fnsinh: case fncosh: case fntanh:
               case fnsignpower /* sign(x)*abs(x)^c */:
               case fnncpvusin /* veelken-ulbrich */:
               case fnncpvupow /* veelken-ulbrich */:
               case fnbinomial:
               case fnarccos:
               case fnarcsin: case fnarctan2 /* arctan(x2/x1) */:
               case fnpoly: /* simple polynomial */
               default :
               {
                  SCIPdebugPrintf("nr. %d - unsupported. Error.\n", (int)func);
                  SCIPerrorMessage("GAMS function %d not supported\n", func);
                  return SCIP_ERROR;
               }
            }
            break;
         }

         case nlEnd: /* end of instruction list */
         default:
         {
            SCIPerrorMessage("GAMS opcode %d not supported - Error.\n", opcode);
            return SCIP_ERROR;
         }
      }

      if( e != NULL )
      {
         if( stackpos >= stacksize )
         {
            stacksize = SCIPcalcMemGrowSize(scip, stackpos+1);
            SCIP_CALL( SCIPreallocBufferArray(scip, &stack, stacksize) );
         }
         assert(stackpos < stacksize);
         stack[stackpos] = e;
         ++stackpos;
      }
   }

   /* there should be exactly one element on the stack, which will be the root of our expression tree */
   assert(stackpos == 1);

   /* multiply expression on stack with factor, if not trivial */
   if( factor != 1.0 )
   {
      SCIP_CALL( SCIPexprCreate(blkmem, &e, SCIP_EXPR_CONST, factor) );
      SCIP_CALL( SCIPexprCreate(blkmem, &stack[0], SCIP_EXPR_MUL, stack[0], e) );
   }

   SCIP_CALL( SCIPexprtreeCreate(blkmem, exprtree, stack[0], nvars, 0, NULL) );
   SCIP_CALL( SCIPexprtreeSetVars(*exprtree, nvars, vars) );

   SCIPfreeBufferArray(scip, &vars);
   SCIPfreeBufferArray(scip, &stack);
   SCIPhashmapFree(&var2idx);

   return SCIP_OKAY;
}
#endif

/** creates a SCIP problem from a GMO */
static
SCIP_RETCODE createProblem(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_READERDATA*      readerdata          /**< Reader data */
)
{
   char buffer[GMS_SSSIZE];
   gmoHandle_t gmo;
   gevHandle_t gev;
   SCIP_Bool objnonlinear;
   SCIP_VAR** vars;
   SCIP_Real minprior;
   SCIP_Real maxprior;
   int i;
   SCIP_Real* coefs;
   int* indices;
   int* nlflag;
   SCIP_VAR** consvars;
   SCIP_VAR** quadvars1;
   SCIP_VAR** quadvars2;
   SCIP_Real* quadcoefs;
   int* qrow;
   int* qcol;
   SCIP_CONS* con;
   int numSos1, numSos2, nzSos;
   SCIP_PROBDATA* probdata;
   int* opcodes;
   int* fields;
   
   assert(scip != NULL);
   assert(readerdata != NULL);

   gmo = readerdata->gmo;
   assert(gmo != NULL);
   gev = readerdata->gev;
   assert(gev != NULL);

   /* we want a real objective function, if it is linear, otherwise keep the GAMS single-variable-objective? */
   gmoObjReformSet(gmo, 1);
   gmoObjStyleSet(gmo, gmoObjType_Fun);
#if 0
   if( gmoObjNLNZ(gmo) > 0 )
      gmoObjStyleSet(gmo, gmoObjType_Var);
   objnonlinear = FALSE;
#else
   objnonlinear = gmoObjStyle(gmo) == gmoObjType_Fun && gmoObjNLNZ(gmo) > 0;
#endif

   /* we want to start indexing at 0 */
   gmoIndexBaseSet(gmo, 0);

   /* we want GMO to use SCIP's value for infinity */
   gmoPinfSet(gmo,  SCIPinfinity(scip));
   gmoMinfSet(gmo, -SCIPinfinity(scip));

   /* create SCIP problem */
   SCIP_CALL( SCIPallocMemory(scip, &probdata) );
   BMSclearMemory(probdata);
   probdata->gmo = gmo;
   probdata->gev = gev;

   gmoNameInput(gmo, buffer);
   SCIP_CALL( SCIPcreateProb(scip, buffer,
      probdataDelOrigGmo, probdataTransGmo, probdataDelTransGmo,
      probdataInitSolGmo, probdataExitSolGmo, probdataCopyGmo,
      probdata) );

   /* initialize QMaker, if nonlinear */
   if( gmoNLNZ(gmo) > 0 || objnonlinear )
      gmoUseQSet(gmo, 1);

   SCIP_CALL( SCIPallocMemoryArray(scip, &probdata->vars, gmoN(gmo)) );
   vars = probdata->vars;
   
   /* compute range of variable priorities */ 
   minprior = SCIPinfinity(scip);
   maxprior = 0.0;
   if( gmoPriorOpt(gmo) && gmoNDisc(gmo) > 0 )
   { 
      for (i = 0; i < gmoN(gmo); ++i)
      {
         if( gmoGetVarTypeOne(gmo, i) == gmovar_X )
            continue; /* GAMS forbids branching priorities for continuous variables */
         if( gmoGetVarPriorOne(gmo,i) < minprior )
            minprior = gmoGetVarPriorOne(gmo,i);
         if( gmoGetVarPriorOne(gmo,i) > maxprior )
            maxprior = gmoGetVarPriorOne(gmo,i);
      }
   }

   /* get objective functions coefficients */
   SCIP_CALL( SCIPallocBufferArray(scip, &coefs, gmoN(gmo)+1) ); /* +1 if we have to transform the objective into a constraint */
   if( !objnonlinear )
   {
      if( gmoObjStyle(gmo) == gmoObjType_Fun )
         gmoGetObjVector(gmo, coefs, NULL);
      else
         coefs[gmoObjVar(gmo)] = 1.0;
   }
   else
   {
      BMSclearMemoryArray(coefs, gmoN(gmo));
   }
   
   /* add variables */
   for( i = 0; i < gmoN(gmo); ++i )
   {
      SCIP_VARTYPE vartype;
      SCIP_Real lb, ub;
      lb = gmoGetVarLowerOne(gmo, i);
      ub = gmoGetVarUpperOne(gmo, i);
      switch( gmoGetVarTypeOne(gmo, i) )
      {
         case gmovar_SC:
            lb = 0.0;
         case gmovar_X:
         case gmovar_S1:
         case gmovar_S2:
            vartype = SCIP_VARTYPE_CONTINUOUS;
            break;
         case gmovar_B:
            vartype = SCIP_VARTYPE_BINARY;
            break;
         case gmovar_SI:
            lb = 0.0;
         case gmovar_I:
            vartype = SCIP_VARTYPE_INTEGER;
            break;
         default:
            SCIPerrorMessage("Unknown variable type.\n");
            return SCIP_INVALIDDATA;
      }
      if( gmoDict(gmo) )
         gmoGetVarNameOne(gmo, i, buffer);
      else
         sprintf(buffer, "x%d", i);
      SCIP_CALL( SCIPcreateVar(scip, &vars[i], buffer, lb, ub, coefs[i], vartype, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL) );
      SCIP_CALL( SCIPaddVar(scip, vars[i]) );
      SCIPdebugMessage("added variable ");
      SCIPdebug( SCIPprintVar(scip, vars[i], NULL) );
      
      if( gmoPriorOpt(gmo) && minprior < maxprior && gmoGetVarTypeOne(gmo, i) != gmovar_X )
      {
         /* in GAMS: higher priorities are given by smaller .prior values
            in SCIP: variables with higher branch priority are always preferred to variables with lower priority in selection of branching variable
            thus, we scale the values from GAMS to lie between 0 (lowest prior) and 1000 (highest prior)
         */
         int branchpriority = (int)(1000.0 / (maxprior - minprior) * (maxprior - gmoGetVarPriorOne(gmo, i)));
         SCIP_CALL( SCIPchgVarBranchPriority(scip, vars[i], branchpriority) );
      }
   }
   
   /* setup bound disjunction constraints for semicontinuous/semiinteger variables by saying x <= 0 or x >= gmoGetVarLower */
   if( gmoGetVarTypeCnt(gmo, gmovar_SC) || gmoGetVarTypeCnt(gmo, gmovar_SI) )
   {
      SCIP_BOUNDTYPE bndtypes[2];
      SCIP_Real      bnds[2];
      SCIP_VAR*      bndvars[2];
      SCIP_CONS*     cons;
      char           name[SCIP_MAXSTRLEN];
      
      bndtypes[0] = SCIP_BOUNDTYPE_UPPER;
      bndtypes[1] = SCIP_BOUNDTYPE_LOWER;
      bnds[0] = 0;

      for( i = 0; i < gmoN(gmo); ++i )
      {
         if( gmoGetVarTypeOne(gmo, i) != gmovar_SC && gmoGetVarTypeOne(gmo, i) != gmovar_SI )
            continue;
         
         bndvars[0] = vars[i];
         bndvars[1] = vars[i];
         bnds[1] = gmoGetVarLowerOne(gmo, i);
         (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "semi%s_%s", gmoGetVarTypeOne(gmo, i) == gmovar_SC ? "con" : "int", SCIPvarGetName(vars[i]));
         SCIP_CALL( SCIPcreateConsBounddisjunction(scip, &cons, name, 2, bndvars, bndtypes, bnds,
            TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE) );
         SCIP_CALL( SCIPaddCons(scip, cons) );
         SCIPdebugMessage("added constraint ");
         SCIPdebug( SCIPprintCons(scip, cons, NULL) );
         SCIP_CALL( SCIPreleaseCons(scip, &cons) );
      }
   }
   
   SCIP_CALL( SCIPallocBufferArray(scip, &consvars, gmoN(gmo)+1) ); /* +1 if we have to transform the objective into a constraint */

   /* setup SOS constraints */
   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);
   if( nzSos > 0 )
   {
      int numSos;
      int* sostype;
      int* sosbeg;
      int* sosind;
      double* soswt;
      int j, k;
      
      numSos = numSos1 + numSos2;
      SCIPallocBufferArray(scip, &sostype, numSos);
      SCIPallocBufferArray(scip, &sosbeg,  numSos+1);
      SCIPallocBufferArray(scip, &sosind,  nzSos);
      SCIPallocBufferArray(scip, &soswt,   nzSos);

      gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);
      
      for( i = 0; i < numSos; ++i )
      {
         for( j = sosbeg[i], k = 0; j < sosbeg[i+1]; ++j, ++k )
         {
            consvars[k] = vars[sosind[j]];
            assert(gmoGetVarTypeOne(gmo, sosind[j]) == (sostype[i] == 1 ? gmovar_S1 : gmovar_S2));
         }
         
         sprintf(buffer, "sos%d", i);
         if( sostype[i] == 1 )
         {
            SCIP_CALL( SCIPcreateConsSOS1(scip, &con, buffer, k, consvars, &soswt[sosbeg[i]], TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
         }
         else
         {
            assert(sostype[i] == 2);
            SCIP_CALL( SCIPcreateConsSOS2(scip, &con, buffer, k, consvars, &soswt[sosbeg[i]], TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
         }
         
         SCIP_CALL( SCIPaddCons(scip, con) );
         SCIPdebugMessage("added constraint ");
         SCIPdebug( SCIPprintCons(scip, con, NULL) );
         SCIP_CALL( SCIPreleaseCons(scip, &con) );
      }
      
      SCIPfreeBufferArray(scip, &sostype);
      SCIPfreeBufferArray(scip, &sosbeg);
      SCIPfreeBufferArray(scip, &sosind);
      SCIPfreeBufferArray(scip, &soswt);
   }
   
   /* setup regular constraints */
   SCIP_CALL( SCIPallocBufferArray(scip, &indices, gmoN(gmo)) );
   
   /* alloc some memory, if nonlinear */
   if( gmoNLNZ(gmo) > 0 || objnonlinear )
   {
      SCIP_CALL( SCIPallocBufferArray(scip, &nlflag, gmoN(gmo)) );

      SCIP_CALL( SCIPallocBufferArray(scip, &quadvars1, gmoMaxQnz(gmo)) );
      SCIP_CALL( SCIPallocBufferArray(scip, &quadvars2, gmoMaxQnz(gmo)) );
      SCIP_CALL( SCIPallocBufferArray(scip, &quadcoefs, gmoMaxQnz(gmo)) );
      SCIP_CALL( SCIPallocBufferArray(scip, &qrow, gmoMaxQnz(gmo)) );
      SCIP_CALL( SCIPallocBufferArray(scip, &qcol, gmoMaxQnz(gmo)) );

      SCIP_CALL( SCIPallocBufferArray(scip, &opcodes, gmoMaxSingleFNL(gmo)+1) );
      SCIP_CALL( SCIPallocBufferArray(scip, &fields, gmoMaxSingleFNL(gmo)+1) );
   }
   else
   {
      nlflag = NULL;

      quadvars1 = NULL;
      quadvars2 = NULL;
      quadcoefs = NULL;
      qrow = NULL;
      qcol = NULL;

      opcodes = NULL;
      fields = NULL;
   }

   for( i = 0; i < gmoM(gmo); ++i )
   {
      double lhs;
      double rhs;
      switch( gmoGetEquTypeOne(gmo, i) )
      {
         case gmoequ_E:
            lhs = rhs = gmoGetRhsOne(gmo, i);
            break;
         case gmoequ_G:
            lhs = gmoGetRhsOne(gmo, i);
            rhs = SCIPinfinity(scip);
            break;
         case gmoequ_L:
            lhs = -SCIPinfinity(scip);
            rhs = gmoGetRhsOne(gmo, i);
            break;
         case gmoequ_N:
            lhs = -SCIPinfinity(scip);
            rhs =  SCIPinfinity(scip);
            break;
         case gmoequ_X:
            SCIPerrorMessage("External functions not supported by SCIP.\n");
            return SCIP_INVALIDDATA;
         case gmoequ_C:
            SCIPerrorMessage("Conic constraints not supported by SCIP interface yet.\n");
            return SCIP_INVALIDDATA;
         case gmoequ_B:
            SCIPerrorMessage("Logic constraints not supported by SCIP interface yet.\n");
            return SCIP_INVALIDDATA;
         default:
            SCIPerrorMessage("unknown equation type.\n");
            return SCIP_INVALIDDATA;
      }

      if( gmoDict(gmo) )
         gmoGetEquNameOne(gmo, i, buffer);
      else
         sprintf(buffer, "e%d", i);

      switch( gmoGetEquOrderOne(gmo, i) )
      {
         case gmoorder_L:
         {
            /* linear constraint */
            int j, nz, nlnz;
            gmoGetRowSparse(gmo, i, indices, coefs, NULL, &nz, &nlnz);
            assert(nlnz == 0);

            for( j = 0; j < nz; ++j )
               consvars[j] = vars[indices[j]];

            SCIP_CALL( SCIPcreateConsLinear(scip, &con, buffer, nz, consvars, coefs, lhs, rhs,
                  TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE) );

            break;
         }
         
         case gmoorder_Q:
         {
            /* quadratic constraint */
            int j, nz, nlnz, qnz;
            
            gmoGetRowSparse(gmo, i, indices, coefs, NULL, &nz, &nlnz);
            for( j = 0; j < nz; ++j )
               consvars[j] = vars[indices[j]];
            
            qnz = gmoGetRowQNZOne(gmo,i);
            gmoGetRowQ(gmo, i, qcol, qrow, quadcoefs);
            for( j = 0; j < qnz; ++j )
            {
               assert(qcol[j] >= 0);
               assert(qrow[j] >= 0);
               assert(qcol[j] < gmoN(gmo));
               assert(qrow[j] < gmoN(gmo));
               quadvars1[j] = vars[qcol[j]];
               quadvars2[j] = vars[qrow[j]];
               if( qcol[j] == qrow[j] )
                  quadcoefs[j] /= 2.0; /* for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO. */
            }

            SCIP_CALL( SCIPcreateConsQuadratic(scip, &con, buffer, nz, consvars, coefs, qnz, quadvars1, quadvars2, quadcoefs, lhs, rhs,
                  TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
            break;
         }

#if 0
         case gmoorder_NL:
         {
            /* nonlinear constraint */
            int j, nz, nlnz, linnz;
            int codelen;
            SCIP_EXPRTREE* exprtree;

            gmoGetRowSparse(gmo, i, indices, coefs, nlflag, &nz, &nlnz);
            linnz = 0;
            for( j = 0; j < nz; ++j )
            {
               if( !nlflag[j] )
               {
                  consvars[linnz] = vars[indices[j]];
                  coefs[linnz] = coefs[j];
                  ++linnz;
               }
            }

            gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
            SCIP_CALL( makeExprtree(scip, codelen, opcodes, fields, gmoNLConst(gmo), (SCIP_Real*)gmoPPool(gmo), 1.0, &exprtree) );

            SCIP_CALL( SCIPcreateConsNonlinear(scip, &con, buffer, linnz, consvars, coefs, 1, &exprtree, NULL, lhs, rhs,
                  TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE) );

            SCIP_CALL( SCIPexprtreeFree(&exprtree) );
            break;
         }
#else
         case gmoorder_NL:
         {
            SCIPerrorMessage("General nonlinear constraints not supported by SCIP yet.\n");
            return SCIP_INVALIDDATA;
         }
#endif

         default:
            SCIPerrorMessage("Unexpected equation order.\n");
            return SCIP_INVALIDDATA;
      }
      
      SCIP_CALL( SCIPaddCons(scip, con) );      
      SCIPdebugMessage("added constraint ");
      SCIPdebug( SCIPprintCons(scip, con, NULL) );
      SCIP_CALL( SCIPreleaseCons(scip, &con) );
   }
   
   if( objnonlinear )
   {
      /* make constraint out of nonlinear objective function */
      int j, nz, nlnz, qnz;
      double lhs, rhs;
      
      assert(gmoGetObjOrder(gmo) == gmoorder_L || gmoGetObjOrder(gmo) == gmoorder_Q || gmoGetObjOrder(gmo) == gmoorder_NL);

      SCIP_CALL( SCIPcreateVar(scip, &probdata->objvar, "xobj", -SCIPinfinity(scip), SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL) );
      SCIP_CALL( SCIPaddVar(scip, probdata->objvar) );
      SCIPdebugMessage("added objective variable ");
      SCIPdebug( SCIPprintVar(scip, probdata->objvar, NULL) );

      if( gmoGetObjOrder(gmo) != gmoorder_NL )
      {
         gmoGetObjSparse(gmo, indices, coefs, NULL, &nz, &nlnz);
         for( j = 0; j < nz; ++j )
            consvars[j] = vars[indices[j]];

         consvars[nz] = probdata->objvar;
         coefs[nz] = -1.0;
         ++nz;

         qnz = gmoObjQNZ(gmo);
         gmoGetObjQ(gmo, qcol, qrow, quadcoefs);
         for( j = 0; j < qnz; ++j )
         {
            assert(qcol[j] >= 0);
            assert(qrow[j] >= 0);
            assert(qcol[j] < gmoN(gmo));
            assert(qrow[j] < gmoN(gmo));
            quadvars1[j] = vars[qcol[j]];
            quadvars2[j] = vars[qrow[j]];
            if( qcol[j] == qrow[j] )
               quadcoefs[j] /= 2.0; /* for some strange reason, the coefficients on the diagonal are multiplied by 2 in GMO */
         }

         if( gmoSense(gmo) == gmoObj_Min )
         {
            lhs = -SCIPinfinity(scip);
            rhs = -gmoObjConst(gmo);
         }
         else
         {
            lhs = -gmoObjConst(gmo);
            rhs = SCIPinfinity(scip);
         }

         SCIP_CALL( SCIPcreateConsQuadratic(scip, &con, "objective", nz, consvars, coefs, qnz, quadvars1, quadvars2, quadcoefs, lhs, rhs,
            TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
      }
      else
#if 0
      {
         SCIP_Real objfactor;
         int linnz;
         int codelen;
         SCIP_EXPRTREE* exprtree;

         gmoGetObjSparse(gmo, indices, coefs, nlflag, &nz, &nlnz);
         linnz = 0;
         for( j = 0; j < nz; ++j )
         {
            if( !nlflag[j] )
            {
               coefs[linnz] = coefs[j];
               consvars[linnz] = vars[indices[j]];
               ++linnz;
            }
         }

         consvars[linnz] = probdata->objvar;
         coefs[linnz] = -1.0;
         ++linnz;

         objfactor = -(gmoSense(gmo) == Obj_Min ? 1.0 : -1.0) / gmoObjJacVal(gmo);

         gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);
         /* @todo put objfactor as nonlincoef in SCIPcreateConsNonlinear */
         SCIP_CALL( makeExprtree(scip, codelen, opcodes, fields, gmoNLConst(gmo), (SCIP_Real*)gmoPPool(gmo), objfactor, &exprtree) );

         if( gmoSense(gmo) == gmoObj_Min )
         {
            lhs = -SCIPinfinity(scip);
            rhs = -gmoObjConst(gmo);
         }
         else
         {
            lhs = -gmoObjConst(gmo);
            rhs = SCIPinfinity(scip);
         }

         SCIP_CALL( SCIPcreateConsNonlinear(scip, &con, "objective", linnz, consvars, coefs, 1, &exprtree, NULL, lhs, rhs,
               TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE) );

         SCIP_CALL( SCIPexprtreeFree(&exprtree) );
      }
#else
      {
         SCIPerrorMessage("General nonlinear objective not supported by SCIP yet.\n");
         return SCIP_INVALIDDATA;
      }
#endif

      SCIP_CALL( SCIPaddCons(scip, con) );
      SCIPdebugMessage("added objective constraint ");
      SCIPdebug( SCIPprintCons(scip, con, NULL) );
      SCIP_CALL( SCIPreleaseCons(scip, &con) );
   }
   else if( !SCIPisZero(scip, gmoObjConst(gmo)) )
   {
      /* handle constant term in linear objective by adding a fixed variable */
      SCIP_CALL( SCIPcreateVar(scip, &probdata->objconst, "objconst", 1.0, 1.0, gmoObjConst(gmo), SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL) );
      SCIP_CALL( SCIPaddVar(scip, probdata->objconst) );
      SCIPdebugMessage("added variable for objective constant: ");
      SCIPdebug( SCIPprintVar(scip, probdata->objconst, NULL) );
   }

   if( gmoSense(gmo) == gmoObj_Max )
      SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE) );

   SCIPfreeBufferArray(scip, &coefs);
   SCIPfreeBufferArray(scip, &indices);
   SCIPfreeBufferArray(scip, &consvars);
   SCIPfreeBufferArrayNull(scip, &nlflag);
   SCIPfreeBufferArrayNull(scip, &quadvars1);
   SCIPfreeBufferArrayNull(scip, &quadvars2);
   SCIPfreeBufferArrayNull(scip, &quadcoefs);
   SCIPfreeBufferArrayNull(scip, &qrow);
   SCIPfreeBufferArrayNull(scip, &qcol);
   SCIPfreeBufferArrayNull(scip, &opcodes);
   SCIPfreeBufferArrayNull(scip, &fields);
   

   /* set objective limit, if enabled */
   if( gevGetIntOpt(gev, gevUseCutOff) )
   {
      SCIP_CALL( SCIPsetObjlimit(scip, gevGetDblOpt(gev, gevCutOff)) );
   }

   /* deinitialize QMaker, if nonlinear */
   if( gmoNLNZ(gmo) > 0 || objnonlinear )
      gmoUseQSet(gmo, 0);

   return SCIP_OKAY;
}

/** tries to pass solution stored in GMO to SCIP */
static
SCIP_RETCODE tryGmoSol(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_Bool*            stored              /**< buffer to store whether solution has been stored by SCIP */
   )
{
   gmoHandle_t gmo;
   gevHandle_t gev;
   SCIP_SOL* sol;
   SCIP_Real* vals;
   SCIP_PROBDATA* probdata;

   assert(scip != NULL);
   assert(stored != NULL);

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);
   assert(probdata->gmo != NULL);
   assert(probdata->gev != NULL);
   assert(probdata->vars != NULL);

   gmo = probdata->gmo;
   gev = probdata->gev;

   SCIP_CALL( SCIPtransformProb(scip) );

   SCIP_CALL( SCIPcreateOrigSol(scip, &sol, NULL) );

   SCIP_CALL( SCIPallocBufferArray(scip, &vals, gmoN(gmo)) );
   gmoGetVarL(gmo, vals);

   SCIP_CALL( SCIPsetSolVals(scip, sol, gmoN(gmo), probdata->vars, vals) );

   /* if we have extra variable for objective, then need to set its value too */
   if( probdata->objvar != NULL )
   {
      double objval;
      int numErr;
      gmoEvalFuncObj(gmo, vals, &objval, &numErr);
      if( numErr == 0 )
      {
         SCIP_CALL( SCIPsetSolVal(scip, sol, probdata->objvar, objval) );
      }
   }

   /* if we have extra variable for objective constant, then need to set its value to 1.0 here too */
   if( probdata->objconst != NULL )
   {
      SCIP_CALL( SCIPsetSolVal(scip, sol, probdata->objconst, 1.0) );
   }

   SCIPfreeBufferArray(scip, &vals);

   /* print constraint violations if one-bit of integer5 is set */
   SCIP_CALL( SCIPtrySolFree(scip, &sol, gevGetIntOpt(gev, gevInteger5) & 0x1, TRUE, TRUE, TRUE, stored) );

   return SCIP_OKAY;
}

/** stores solve information (solution, statistics) in a GMO */
static
SCIP_RETCODE writeGmoSolution(
   SCIP*                 scip                /**< SCIP data structure */
)
{
   gmoHandle_t gmo;
   gevHandle_t gev;
   SCIP_PROBDATA* probdata;
   int nrsol;

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);
   assert(probdata->gmo  != NULL);
   assert(probdata->gev  != NULL);
   assert(probdata->vars != NULL);

   gmo = probdata->gmo;
   gev = probdata->gev;

   nrsol = SCIPgetNSols(scip);

   switch( SCIPgetStatus(scip) )
   {
      default:
      case SCIP_STATUS_UNKNOWN: /* the solving status is not yet known */
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         break;
      case SCIP_STATUS_USERINTERRUPT: /* the user interrupted the solving process (by pressing Ctrl-C) */
         gmoSolveStatSet(gmo, gmoSolveStat_User);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? gmoModelStat_Integer : gmoModelStat_OptimalLocal) : gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_NODELIMIT:      /* the solving process was interrupted because the node limit was reached */
      case SCIP_STATUS_STALLNODELIMIT: /* the solving process was interrupted because the node limit was reached */
         gmoSolveStatSet(gmo, gmoSolveStat_Iteration);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? gmoModelStat_Integer : gmoModelStat_OptimalLocal) : gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_TIMELIMIT: /* the solving process was interrupted because the time limit was reached */
      case SCIP_STATUS_MEMLIMIT:  /* the solving process was interrupted because the memory limit was reached */
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? gmoModelStat_Integer : gmoModelStat_OptimalLocal) : gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_GAPLIMIT: /* the solving process was interrupted because the gap limit was reached */
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, nrsol > 0 ? (SCIPgetGap(scip) > 0.0 ? (gmoNDisc(gmo) ? gmoModelStat_Integer : gmoModelStat_OptimalLocal) : gmoModelStat_OptimalGlobal): gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_SOLLIMIT: /* the solving process was interrupted because the solution limit was reached */
      case SCIP_STATUS_BESTSOLLIMIT: /* the solving process was interrupted because the solution improvement limit was reached */
         gmoSolveStatSet(gmo, gmoSolveStat_Resource);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? gmoModelStat_Integer : gmoModelStat_OptimalLocal) : gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_OPTIMAL: /* the problem was solved to optimality, an optimal solution is available */
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
         break;
      case SCIP_STATUS_INFEASIBLE: /* the problem was proven to be infeasible */
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_InfeasibleNoSolution);
         break;
      case SCIP_STATUS_UNBOUNDED: /* the problem was proven to be unbounded */
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, nrsol > 0 ? gmoModelStat_Unbounded : gmoModelStat_UnboundedNoSolution);
         break;
      case SCIP_STATUS_INFORUNBD: /* the problem was proven to be either infeasible or unbounded */
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         break;
   }

   if( nrsol > 0 )
   {
      SCIP_SOL* sol;
      SCIP_Real* collev;
      SCIP_Real* lambda;

      sol = SCIPgetBestSol(scip);
      assert(sol != NULL);

      SCIP_CALL( SCIPallocBufferArray(scip, &collev, gmoN(gmo)) );
      SCIP_CALL( SCIPallocBufferArray(scip, &lambda, gmoM(gmo)) );
      BMSclearMemoryArray(lambda, gmoM(gmo));

      SCIP_CALL( SCIPgetSolVals(scip, sol, gmoN(gmo), probdata->vars, collev) );
      gmoSetSolution2(gmo, collev, lambda);

      SCIPfreeBufferArray(scip, &collev);
      SCIPfreeBufferArray(scip, &lambda);

      gmoSetHeadnTail(gmo, gmoHobjval, SCIPgetSolOrigObj(scip, sol));
   }

   if( SCIPgetStage(scip) == SCIP_STAGE_SOLVING || SCIPgetStage(scip) == SCIP_STAGE_SOLVED )
      gmoSetHeadnTail(gmo, gmoTmipbest, SCIPgetDualbound(scip));
   gmoSetHeadnTail(gmo, gmoTmipnod,  (int)SCIPgetNNodes(scip));
   gmoSetHeadnTail(gmo, gmoHresused, SCIPgetSolvingTime(scip));

   return SCIP_OKAY;
}

/*
 * Callback methods of reader
 */


/** copy method for reader plugins (called when SCIP copies plugins) */
#if 0
static
SCIP_DECL_READERCOPY(readerCopyGmo)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of gmo reader not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/
 
   return SCIP_OKAY;
}
#else
#define readerCopyGmo NULL
#endif

/** destructor of reader to free user data (called when SCIP is exiting) */
#if 1
static
SCIP_DECL_READERFREE(readerFreeGmo)
{
   SCIP_READERDATA* readerdata;

   readerdata = SCIPreaderGetData(reader);
   assert(readerdata != NULL);

#if 0 /* TODO should do this if we created gmo */
   if( readerdata->gmo != NULL )
   {
      /* write solution file */
      gmoUnloadSolutionLegacy(readerdata->gmo);

      gmoFree(&readerdata->gmo);
      gevFree(&readerdata->gev);

      gmoLibraryUnload();
      gevLibraryUnload();
   }
#endif

   SCIPfreeMemory(scip, &readerdata);

   return SCIP_OKAY;
}
#else
#define readerFreeGmo NULL
#endif


/** problem reading method of reader */
#if 1
static
SCIP_DECL_READERREAD(readerReadGmo)
{
   SCIP_READERDATA* readerdata;
   gmoHandle_t gmo;
   gevHandle_t gev;
   char buffer[1024];
   
   *result = SCIP_DIDNOTRUN;

   readerdata = SCIPreaderGetData(reader);
   assert(readerdata != NULL);

   if( readerdata->gmo == NULL )
   {
      /* initialize GMO and GEV libraries */
      if( !gmoCreate(&readerdata->gmo, buffer, sizeof(buffer)) || !gevCreate(&readerdata->gev, buffer, sizeof(buffer)) )
      {
         SCIPerrorMessage(buffer);
         return SCIP_ERROR;
      }

      gmo = readerdata->gmo;
      gev = readerdata->gev;

      /* load control file */
      if( gevInitEnvironmentLegacy(gev, filename) )
      {
         SCIPerrorMessage("Could not load control file %s\n", filename);
         gmoFree(&gmo);
         gevFree(&gev);
         return SCIP_READERROR;
      }

      if( gmoRegisterEnvironment(gmo, gev, buffer) )
      {
         SCIPerrorMessage("Error registering GAMS Environment: %s\n", buffer);
         gmoFree(&gmo);
         gevFree(&gev);
         return SCIP_ERROR;
      }

      if( gmoLoadDataLegacy(gmo, buffer) )
      {
         SCIPerrorMessage("Could not load model data.\n");
         gmoFree(&gmo);
         gevFree(&gev);
         return SCIP_READERROR;
      }
   }

   SCIP_CALL( createProblem(scip, readerdata) );

   *result = SCIP_SUCCESS;

   return SCIP_OKAY;
}
#else
#define readerReadGmo NULL
#endif


#if 0
/** problem writing method of reader */
static
SCIP_DECL_READERWRITE(readerWriteGmo)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of gmo reader not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define readerWriteGmo NULL
#endif

/*
 * Reading solution from GMO and pass to SCIP
 */

#define DIALOG_READGAMS_NAME                 "readgams"
#define DIALOG_READGAMS_DESC                 "initializes SCIP problem to the one stored in a GAMS modeling object"
#define DIALOG_READGAMS_ISSUBMENU            FALSE

/** copy method for dialog plugins (called when SCIP copies plugins) */
#if 0
static
SCIP_DECL_DIALOGCOPY(dialogCopyReadGams)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of ReadGams dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogCopyReadGams NULL
#endif

/** destructor of dialog to free user data (called when the dialog is not captured anymore) */
#if 0
static
SCIP_DECL_DIALOGFREE(dialogFreeReadGams)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of ReadGams dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogFreeReadGams NULL
#endif

/** description output method of dialog */
#if 0
static
SCIP_DECL_DIALOGDESC(dialogDescReadGams)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of ReadGams dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogDescReadGams NULL
#endif


/** execution method of dialog */
static
SCIP_DECL_DIALOGEXEC(dialogExecReadGams)
{  /*lint --e{715}*/
   SCIP_READERDATA* readerdata;

   assert(dialoghdlr != NULL);
   assert(dialog != NULL);
   assert(scip != NULL);

   readerdata = (SCIP_READERDATA*)SCIPdialogGetData(dialog);
   assert(readerdata != NULL);

   SCIP_CALL( SCIPdialoghdlrAddHistory(dialoghdlr, dialog, NULL, FALSE) );

   if( readerdata->gmo == NULL )
   {
      SCIPerrorMessage("GMO not initialized, cannot setup GAMS problem\n");
   }
   else
   {
      /* free previous problem and solution data, if existing */
      SCIP_CALL( SCIPfreeProb(scip) );

      SCIP_CALL( createProblem(scip, readerdata) );
   }

   *nextdialog = SCIPdialoghdlrGetRoot(dialoghdlr);

   return SCIP_OKAY;
}


/*
 * Reading solution from GMO and pass to SCIP
 */

#define DIALOG_TRYGAMSSOL_NAME               "trygamssol"
#define DIALOG_TRYGAMSSOL_DESC               "passes solution from GAMS Modeling Object to SCIP"
#define DIALOG_TRYGAMSSOL_ISSUBMENU          FALSE

/** copy method for dialog plugins (called when SCIP copies plugins) */
#if 0
static
SCIP_DECL_DIALOGCOPY(dialogCopyTryGamsSol)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of TryGamsSol dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogCopyTryGamsSol NULL
#endif

/** destructor of dialog to free user data (called when the dialog is not captured anymore) */
#if 0
static
SCIP_DECL_DIALOGFREE(dialogFreeTryGamsSol)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of TryGamsSol dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogFreeTryGamsSol NULL
#endif

/** description output method of dialog */
#if 0
static
SCIP_DECL_DIALOGDESC(dialogDescTryGamsSol)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of TryGamsSol dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogDescTryGamsSol NULL
#endif


/** execution method of dialog */
static
SCIP_DECL_DIALOGEXEC(dialogExecTryGamsSol)
{  /*lint --e{715}*/
   SCIP_Bool stored;

   assert(dialoghdlr != NULL);
   assert(dialog != NULL);
   assert(scip != NULL);
   assert(SCIPgetStage(scip) >= SCIP_STAGE_PROBLEM);

   SCIP_CALL( SCIPdialoghdlrAddHistory(dialoghdlr, dialog, NULL, FALSE) );

   SCIP_CALL( tryGmoSol(scip, &stored) );
   SCIPinfoMessage(scip, NULL, "Initial GAMS solution %s by SCIP.\n", stored ? "accepted" : "rejected");

   *nextdialog = SCIPdialoghdlrGetRoot(dialoghdlr);

   return SCIP_OKAY;
}


/*
 * Writing solution information to GMO dialog
 */

#define DIALOG_WRITEGAMSSOL_NAME             "gamssol"
#define DIALOG_WRITEGAMSSOL_DESC             "writes solution information into GAMS Modeling Object"
#define DIALOG_WRITEGAMSSOL_ISSUBMENU        FALSE

/** copy method for dialog plugins (called when SCIP copies plugins) */
#if 0
static
SCIP_DECL_DIALOGCOPY(dialogCopyWriteGamsSol)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of WriteGamsSol dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogCopyWriteGamsSol NULL
#endif

/** destructor of dialog to free user data (called when the dialog is not captured anymore) */
#if 0
static
SCIP_DECL_DIALOGFREE(dialogFreeWriteGamsSol)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of WriteGamsSol dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogFreeWriteGamsSol NULL
#endif

/** description output method of dialog */
#if 0
static
SCIP_DECL_DIALOGDESC(dialogDescWriteGamsSol)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of WriteGamsSol dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogDescWriteGamsSol NULL
#endif


/** execution method of dialog */
static
SCIP_DECL_DIALOGEXEC(dialogExecWriteGamsSol)
{  /*lint --e{715}*/
   SCIP_CALL( SCIPdialoghdlrAddHistory(dialoghdlr, dialog, NULL, FALSE) );

   SCIP_CALL( writeGmoSolution(scip) );

   *nextdialog = SCIPdialoghdlrGetRoot(dialoghdlr);

   return SCIP_OKAY;
}


/*
 * Loading GAMS and user option file dialog
 */

#define DIALOG_SETTINGSLOADGAMS_NAME         "loadgams"
#define DIALOG_SETTINGSLOADGAMS_DESC         "loads GAMS settings and SCIP option file specified in GAMS model"
#define DIALOG_SETTINGSLOADGAMS_ISSUBMENU    FALSE

/** copy method for dialog plugins (called when SCIP copies plugins) */
#if 0
static
SCIP_DECL_DIALOGCOPY(dialogCopySettingsLoadGams)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of SettingsLoadGams dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogCopySettingsLoadGams NULL
#endif

/** destructor of dialog to free user data (called when the dialog is not captured anymore) */
#if 0
static
SCIP_DECL_DIALOGFREE(dialogFreeSettingsLoadGams)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of SettingsLoadGams dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogFreeSettingsLoadGams NULL
#endif

/** description output method of dialog */
#if 0
static
SCIP_DECL_DIALOGDESC(dialogDescSettingsLoadGams)
{  /*lint --e{715}*/
   SCIPerrorMessage("method of SettingsLoadGams dialog not implemented yet\n");
   SCIPABORT(); /*lint --e{527}*/

   return SCIP_OKAY;
}
#else
#define dialogDescSettingsLoadGams NULL
#endif


/** execution method of dialog */
static
SCIP_DECL_DIALOGEXEC(dialogExecSettingsLoadGams)
{  /*lint --e{715}*/
   assert(scip       != NULL);
   assert(dialog     != NULL);
   assert(dialoghdlr != NULL);

   SCIP_CALL( SCIPdialoghdlrAddHistory(dialoghdlr, dialog, NULL, FALSE) );

   SCIP_CALL( SCIPreadParamsReaderGmo(scip) );

   *nextdialog = SCIPdialoghdlrGetRoot(dialoghdlr);

   return SCIP_OKAY;
}

/*
 * reader specific interface methods
 */

/** includes the gmo file reader in SCIP */
SCIP_RETCODE SCIPincludeReaderGmo(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_READERDATA* readerdata;
   SCIP_DIALOG* dialog;
   SCIP_DIALOG* parentdialog;

   /* create gmo reader data */
   SCIP_CALL( SCIPallocMemory(scip, &readerdata) );
   BMSclearMemory(readerdata);
   
   /* include gmo reader */
   SCIP_CALL( SCIPincludeReader(scip, READER_NAME, READER_DESC, READER_EXTENSION,
         readerCopyGmo,
         readerFreeGmo, readerReadGmo, readerWriteGmo,
         readerdata) );

   /* get parent dialog "write" */
   if( SCIPdialogFindEntry(SCIPgetRootDialog(scip), "write", &parentdialog) != 1 )
   {
      SCIPerrorMessage("sub menu \"write\" not found\n");
      return SCIP_PLUGINNOTFOUND;
   }
   assert(parentdialog != NULL);

   /* create, include, and release dialog */
   if( !SCIPdialogHasEntry(SCIPgetRootDialog(scip), DIALOG_READGAMS_NAME) )
   {
      SCIP_CALL( SCIPincludeDialog(scip, &dialog,
            dialogCopyReadGams, dialogExecReadGams, dialogDescReadGams, dialogFreeReadGams,
            DIALOG_READGAMS_NAME, DIALOG_READGAMS_DESC, DIALOG_READGAMS_ISSUBMENU, (SCIP_DIALOGDATA*)readerdata) );
      SCIP_CALL( SCIPaddDialogEntry(scip, SCIPgetRootDialog(scip), dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );
   }


   /* create, include, and release dialog */
   if( !SCIPdialogHasEntry(parentdialog, DIALOG_WRITEGAMSSOL_NAME) )
   {
      SCIP_CALL( SCIPincludeDialog(scip, &dialog,
            dialogCopyWriteGamsSol, dialogExecWriteGamsSol, dialogDescWriteGamsSol, dialogFreeWriteGamsSol,
            DIALOG_WRITEGAMSSOL_NAME, DIALOG_WRITEGAMSSOL_DESC, DIALOG_WRITEGAMSSOL_ISSUBMENU, NULL) );
      SCIP_CALL( SCIPaddDialogEntry(scip, parentdialog, dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );
   }


   /* create, include, and release dialog */
   if( !SCIPdialogHasEntry(SCIPgetRootDialog(scip), DIALOG_TRYGAMSSOL_NAME) )
   {
      SCIP_CALL( SCIPincludeDialog(scip, &dialog,
            dialogCopyTryGamsSol, dialogExecTryGamsSol, dialogDescTryGamsSol, dialogFreeTryGamsSol,
            DIALOG_TRYGAMSSOL_NAME, DIALOG_TRYGAMSSOL_DESC, DIALOG_TRYGAMSSOL_ISSUBMENU, NULL) );
      SCIP_CALL( SCIPaddDialogEntry(scip, SCIPgetRootDialog(scip), dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );
   }


   /* get parent dialog "set" */
   if( SCIPdialogFindEntry(SCIPgetRootDialog(scip), "set", &parentdialog) != 1 )
   {
      SCIPerrorMessage("sub menu \"set\" not found\n");
      return SCIP_PLUGINNOTFOUND;
   }
   assert(parentdialog != NULL);

   /* create, include, and release dialog */
   if( !SCIPdialogHasEntry(parentdialog, DIALOG_SETTINGSLOADGAMS_NAME) )
   {
      SCIP_CALL( SCIPincludeDialog(scip, &dialog,
            dialogCopySettingsLoadGams, dialogExecSettingsLoadGams, dialogDescSettingsLoadGams, dialogFreeSettingsLoadGams,
            DIALOG_SETTINGSLOADGAMS_NAME, DIALOG_SETTINGSLOADGAMS_DESC, DIALOG_SETTINGSLOADGAMS_ISSUBMENU, NULL) );
      SCIP_CALL( SCIPaddDialogEntry(scip, parentdialog, dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );

#if 0
      SCIP_CALL( SCIPaddStringParam(scip, "constraints/attrfile",
         "name of file that specifies constraint attributes",
         NULL, FALSE, "", NULL, NULL) );
#endif
   }

   return SCIP_OKAY;
}

/** sets the GMO object to use in reader
 * If GMO is set in reader, then reader does not read from file when executed, but sets up problem from GMO
 */
void SCIPsetGMOReaderGmo(
   SCIP*                 scip,               /**< SCIP data structure */
   void*                 gmo                 /**< GMO object, or NULL to reset to default behaviour */
   )
{
   SCIP_READER* reader;
   SCIP_READERDATA* readerdata;

   assert(scip != NULL);
   assert(SCIPgetStage(scip) == SCIP_STAGE_INIT);

   reader = SCIPfindReader(scip, READER_NAME);
   assert(reader != NULL);

   readerdata = SCIPreaderGetData(reader);
   assert(readerdata != NULL);

   readerdata->gmo = (gmoHandle_t)gmo;
   readerdata->gev = gmo != NULL ? (gevHandle_t)gmoEnvironment(readerdata->gmo) : NULL;
}

/** passes GAMS options to SCIP and initiates reading of user options file, if given in GMO */
SCIP_RETCODE SCIPreadParamsReaderGmo(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_READER* reader;
   SCIP_READERDATA* readerdata;
   gmoHandle_t gmo;
   gevHandle_t gev;

   assert(scip != NULL);

   reader = SCIPfindReader(scip, READER_NAME);
   assert(reader != NULL);

   readerdata = SCIPreaderGetData(reader);
   assert(readerdata != NULL);
   assert(readerdata->gmo != NULL);
   assert(readerdata->gev != NULL);

   gmo = readerdata->gmo;
   gev = readerdata->gev;

   if( gevGetIntOpt(gev, gevNodeLim) > 0 )
   {
      SCIP_CALL( SCIPsetLongintParam(scip, "limits/nodes", gevGetIntOpt(gev, gevNodeLim)) );
   }
   SCIP_CALL( SCIPsetRealParam(scip, "limits/time",   gevGetDblOpt(gev, gevResLim)) );
   SCIP_CALL( SCIPsetRealParam(scip, "limits/gap",    gevGetDblOpt(gev, gevOptCR)) );
   if( !SCIPisInfinity(scip, gevGetDblOpt(gev, gevOptCA)) )
   {
      SCIP_CALL( SCIPsetRealParam(scip, "limits/absgap", gevGetDblOpt(gev, gevOptCA)) );
   }
   else
   {
      SCIPwarningMessage("Value for optca = %g >= value for infinity. Setting solution limit to 1 instead.\n", gevGetDblOpt(gev, gevOptCA));
      SCIP_CALL( SCIPsetIntParam(scip, "limits/solutions", 1) );
   }

#ifdef _WIN32
   SCIP_CALL( SCIPsetIntParam(scip, "display/width", 80) );
   SCIP_CALL( SCIPsetIntParam(scip, "display/lpavgiterations/active", 0) );
   SCIP_CALL( SCIPsetIntParam(scip, "display/maxdepth/active", 0) );
#endif

   if( gmoNLNZ(gmo) > 0 || (gmoObjStyle(gmo) == gmoObjType_Fun && gmoObjNLNZ(gmo) > 0) )
   {
      /* set some MINLP specific options */
      gevLogPChar(gev, "\nEnable some MINLP specific settings.\n");
      SCIP_CALL( SCIPsetIntParam(scip, "display/nexternbranchcands/active", 2) );
      SCIP_CALL( SCIPsetBoolParam(scip, "heuristics/crossover/uselprows", FALSE) );
      SCIP_CALL( SCIPsetBoolParam(scip, "heuristics/dins/uselprows", FALSE) );
      SCIP_CALL( SCIPsetBoolParam(scip, "heuristics/rens/uselprows", FALSE) );
      SCIP_CALL( SCIPsetBoolParam(scip, "heuristics/rins/uselprows", FALSE) );
      SCIP_CALL( SCIPsetBoolParam(scip, "heuristics/localbranching/uselprows", FALSE) );
      SCIP_CALL( SCIPsetBoolParam(scip, "heuristics/mutation/uselprows", FALSE) );
      /* a convex QP like qcp1 cannot be solved by separation in the LP solution solely
       * setting minorthoroot to 0.4 helps in this case to ensure that the linearization in the global optimum makes its way into the LP
       */
      SCIP_CALL( SCIPsetRealParam(scip, "separating/minorthoroot", 0.4) );
   }

   if( gmoOptFile(gmo) > 0 )
   {
      char optfilename[1024];
      SCIP_RETCODE ret;

      gmoNameOptFile(gmo, optfilename);
      ret = SCIPreadParams(scip, optfilename);
      if( ret != SCIP_OKAY )
      {
         SCIPwarningMessage("Reading of optionfile %s failed with SCIP return code <%d>!\n", optfilename, ret);
      }
   }

#if 0
   SCIP_CALL( SCIPgetStringParam(scip, "constraints/attrfile", &attrfile) );
   if( attrfile && *attrfile != '\0' )
   {
      /* find a nicer way to call the ca-filereader */
      SCIP_CALL( SCIPreadProb(scip, attrfile, "ca") );
   }
#endif

   return SCIP_OKAY;
}
