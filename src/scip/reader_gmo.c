/* Copyright (C) GAMS Development and others
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

#define _USE_MATH_DEFINES   /* to get M_PI on Windows */

#include <assert.h>
#include <math.h>
#include <string.h>

#include "reader_gmo.h"

#include "gmomcc.h"
#include "gevmcc.h"
#include "gdxcc.h"
#include "optcc.h"
#include "GamsNLinstr.h"

#include "scip/cons_linear.h"
#include "scip/cons_bounddisjunction.h"
#include "scip/cons_nonlinear.h"
#include "scip/cons_indicator.h"
#include "scip/cons_sos1.h"
#include "scip/cons_sos2.h"
#include "scip/cons_and.h"
#include "scip/cons_or.h"
#include "scip/cons_xor.h"
#include "scip/expr_abs.h"
#include "scip/expr_entropy.h"
#include "scip/expr_exp.h"
#include "scip/expr_log.h"
#include "scip/expr_pow.h"
#include "scip/expr_product.h"
#include "scip/expr_sum.h"
#include "scip/expr_trig.h"
#include "scip/expr_value.h"
#include "scip/expr_var.h"
#include "scip/dialog_default.h"

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
   int                   mipstart;           /**< how to handle initial variable levels */
   char*                 indicatorfile;      /**< name of GAMS options file that contains definitions on indicators */
   SCIP_Bool             ipoptlicensed;      /**< whether GAMS/IpoptH is licensed */
};

/** problem data */
struct SCIP_ProbData
{
   int                   nvars;              /**< number of variables in vars array */
   SCIP_VAR**            vars;               /**< SCIP variables as corresponding to GMO variables */
   SCIP_VAR*             objvar;             /**< SCIP variable used to model objective function */
   SCIP_VAR*             objconst;           /**< SCIP variable used to model objective constant */
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

   assert((*probdata)->vars != NULL || (*probdata)->nvars > 0);

   for( i = 0; i < (*probdata)->nvars; ++i )
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

/*
 * Local methods of reader
 */

/** creates an expression from the addition of two given expression, with coefficients, and a constant
 * argument expressions are released
 */
static
SCIP_RETCODE exprAdd(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_EXPR**           expr,               /**< pointer to store resulting expression */
   SCIP_Real             coef1,              /**< coefficient of first term */
   SCIP_EXPR*            term1,              /**< expression of first term */
   SCIP_Real             coef2,              /**< coefficient of second term */
   SCIP_EXPR*            term2,              /**< expression of second term */
   SCIP_Real             constant            /**< constant term to add */
)
{
   assert(scip != NULL);
   assert(expr != NULL);

   if( term1 != NULL && SCIPisExprValue(scip, term1) )
   {
      constant += coef1 * SCIPgetValueExprValue(term1);
      SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
   }

   if( term2 != NULL && SCIPisExprValue(scip, term2) )
   {
      constant += coef2 * SCIPgetValueExprValue(term2);
      SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
   }

   if( term1 == NULL && term2 == NULL )
   {
      SCIP_CALL( SCIPcreateExprValue(scip, expr, constant, NULL, NULL) );
      return SCIP_OKAY;
   }

   if( term1 != NULL && SCIPisExprSum(scip, term1) && coef1 != 1.0 )
   {
      /* multiply coefficients and constant of linear expression term1 by coef1 */
      SCIPmultiplyByConstantExprSum(term1, coef1);

      coef1 = 1.0;
   }

   if( term2 != NULL && SCIPisExprSum(scip, term2) && coef2 != 1.0 )
   {
      /* multiply coefficients and constant of linear expression term2 by coef2 */
      SCIPmultiplyByConstantExprSum(term2, coef2);

      coef2 = 1.0;
   }

   if( term1 == NULL || term2 == NULL )
   {
      if( term1 == NULL )
      {
         term1 = term2;
         coef1 = coef2;
         /* term2 = NULL; */
      }
      if( constant != 0.0 || coef1 != 1.0 )
      {
         if( SCIPisExprSum(scip, term1) )
         {
            assert(coef1 == 1.0);

            /* add constant to existing linear expression */
            SCIPsetConstantExprSum(term1, SCIPgetConstantExprSum(term1) + constant);
            *expr = term1;
         }
         else
         {
            /* create new linear expression for coef1 * term1 + constant */
            SCIP_CALL( SCIPcreateExprSum(scip, expr, 1, &term1, &coef1, constant, NULL, NULL) );
            SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
         }
      }
      else
      {
         assert(constant == 0.0);
         assert(coef1 == 1.0);
         *expr = term1;
      }

      return SCIP_OKAY;
   }

#if 0
   if( SCIPexprGetOperator(term1) == SCIP_EXPR_LINEAR && SCIPexprGetOperator(term2) == SCIP_EXPR_LINEAR )
   {
      assert(coef1 == 1.0);
      assert(coef2 == 1.0);

      SCIP_CALL( exprLinearAdd(blkmem, term1, SCIPexprGetNChildren(term2), SCIPexprGetLinearCoefs(term2), SCIPexprGetChildren(term2), SCIPexprGetLinearConstant(term2) + constant) );
      exprLinearFree(blkmem, &term2);

      *expr = term1;

      return SCIP_OKAY;
   }
#endif

   if( !SCIPisExprSum(scip, term1) && SCIPisExprSum(scip, term2) )
   {
      /* if only term2 is linear, then swap */
      assert(coef2 == 1.0);

      SCIPswapPointers((void**)&term1, (void**)&term2);
      coef2 = coef1;
      coef1 = 1.0;
   }

   if( SCIPisExprSum(scip, term1) )
   {
      /* add coef2*term2 as extra child to linear expression term1 */
      assert(coef1 == 1.0);

      SCIP_CALL( SCIPappendExprSumExpr(scip, term1, term2, coef2) );
      SCIPsetConstantExprSum(term1, SCIPgetConstantExprSum(term1) + constant);
      *expr = term1;

      SCIP_CALL( SCIPreleaseExpr(scip, &term2) );

      return SCIP_OKAY;
   }

   /* both terms are not linear, then create new linear term for sum */
   {
      SCIP_Real coefs[2];
      SCIP_EXPR* children[2];

      coefs[0] = coef1;
      coefs[1] = coef2;
      children[0] = term1;
      children[1] = term2;

      SCIP_CALL( SCIPcreateExprSum(scip, expr, 2, children, coefs, constant, NULL, NULL) );

      SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
      SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
   }

   return SCIP_OKAY;
}

/** creates an expression from given GAMS nonlinear instructions */
static
SCIP_RETCODE makeExpr(
   SCIP*                 scip,               /**< SCIP data structure */
   gmoHandle_t           gmo,                /**< GAMS Model Object */
   int                   codelen,
   int*                  opcodes,
   int*                  fields,
   SCIP_Real*            constants,
   int*                  logiccount,         /**< counter on number of variables/constraints introduced for logical functions */
   SCIP_EXPR**           expr                /**< buffer where to store expression tree */
)
{
   SCIP_PROBDATA* probdata;
   SCIP_EXPR**   stack;
   int           stackpos;
   int           stacksize;
   int           pos;
   SCIP_EXPR*    e;
   SCIP_EXPR*    term1;
   SCIP_EXPR*    term2;
   GamsOpCode    opcode;
   int           address;
   int           nargs;
   SCIP_RETCODE  rc;

   assert(scip != NULL);
   assert(opcodes != NULL);
   assert(fields != NULL);
   assert(constants != NULL);
   assert(logiccount != NULL);
   assert(expr != NULL);

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);
   assert(probdata->vars != NULL);

   stackpos = 0;
   stacksize = 20;
   SCIP_CALL( SCIPallocBufferArray(scip, &stack, stacksize) );

   nargs = -1;
   rc = SCIP_OKAY;

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

            SCIP_CALL( SCIPcreateExprVar(scip, &e, probdata->vars[address], NULL, NULL) );

            break;
         }

         case nlPushI: /* push constant */
         {
            SCIPdebugPrintf("push constant %g\n", constants[address]);
            SCIP_CALL( SCIPcreateExprValue(scip, &e, constants[address], NULL, NULL) );
            break;
         }

         case nlPushZero: /* push zero */
         {
            SCIPdebugPrintf("push constant zero\n");

            SCIP_CALL( SCIPcreateExprValue(scip, &e, 0.0, NULL, NULL) );
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

            SCIP_CALL( exprAdd(scip, &e, 1.0, term1, 1.0, term2, 0.0) );

            break;
         }

         case nlAddV: /* add variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("add variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPcreateExprVar(scip, &term2, probdata->vars[address], NULL, NULL) );
            SCIP_CALL( exprAdd(scip, &e, 1.0, term1, 1.0, term2, 0.0) );

            break;
         }

         case nlAddI: /* add immediate */
         {
            SCIPdebugPrintf("add constant %g\n", constants[address]);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( exprAdd(scip, &e, 1.0, term1, 1.0, NULL, constants[address]) );

            break;
         }

         case nlSub: /* subtract */
         {
            SCIPdebugPrintf("subtract\n");

            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( exprAdd(scip, &e, 1.0, term2, -1.0, term1, 0.0) );

            break;
         }

         case nlSubV: /* subtract variable */
         {
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("subtract variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPcreateExprVar(scip, &term2, probdata->vars[address], NULL, NULL) );
            SCIP_CALL( exprAdd(scip, &e, 1.0, term1, -1.0, term2, 0.0) );

            break;
         }

         case nlSubI: /* subtract immediate */
         {
            SCIPdebugPrintf("subtract constant %g\n", constants[address]);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( exprAdd(scip, &e, 1.0, term1, 1.0, NULL, -constants[address]) );

            break;
         }

         case nlMul: /* multiply */
         {
            SCIP_EXPR* terms[2];
            SCIPdebugPrintf("multiply\n");

            assert(stackpos >= 2);
            terms[1] = stack[stackpos-1];
            --stackpos;
            terms[0] = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPcreateExprProduct(scip, &e, 2, terms, 1.0, NULL, NULL) );

            SCIP_CALL( SCIPreleaseExpr(scip, &terms[0]) );
            SCIP_CALL( SCIPreleaseExpr(scip, &terms[1]) );

            break;
         }

         case nlMulV: /* multiply variable */
         {
            SCIP_EXPR* terms[2];
            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("multiply variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            terms[0] = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPcreateExprVar(scip, &terms[1], probdata->vars[address], NULL, NULL) );

            SCIP_CALL( SCIPcreateExprProduct(scip, &e, 2, terms, 1.0, NULL, NULL) );

            SCIP_CALL( SCIPreleaseExpr(scip, &terms[0]) );
            SCIP_CALL( SCIPreleaseExpr(scip, &terms[1]) );

            break;
         }

         case nlMulI: /* multiply immediate */
         {
            SCIPdebugPrintf("multiply constant %g\n", constants[address]);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( exprAdd(scip, &e, constants[address], term1, 1.0, NULL, 0.0) );

            break;
         }

         case nlMulIAdd:
         {
            SCIPdebugPrintf("multiply constant %g and add\n", constants[address]);

            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( exprAdd(scip, &e, constants[address], term1, 1.0, term2, 0.0) );

            break;
         }

         case nlDiv: /* divide */
         {
            SCIP_EXPR* terms[2];

            SCIPdebugPrintf("divide\n");

            assert(stackpos >= 2);
            term1 = stack[stackpos-1];
            --stackpos;
            term2 = stack[stackpos-1];
            --stackpos;

            terms[0] = term2;
            SCIP_CALL( SCIPcreateExprPow(scip, &terms[1], term1, -1.0, NULL, NULL) );
            SCIP_CALL( SCIPcreateExprProduct(scip, &e, 2, terms, 1.0, NULL, NULL) );

            SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
            SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
            SCIP_CALL( SCIPreleaseExpr(scip, &terms[1]) );

            break;
         }

         case nlDivV: /* divide variable */
         {
            SCIP_EXPR* terms[2];

            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("divide variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            assert(stackpos >= 1);
            terms[0] = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( SCIPcreateExprVar(scip, &term2, probdata->vars[address], NULL, NULL) );
            SCIP_CALL( SCIPcreateExprPow(scip, &terms[1], term2, -1.0, NULL, NULL) );
            SCIP_CALL( SCIPcreateExprProduct(scip, &e, 2, terms, 1.0, NULL, NULL) );

            SCIP_CALL( SCIPreleaseExpr(scip, &terms[0]) );
            SCIP_CALL( SCIPreleaseExpr(scip, &terms[1]) );
            SCIP_CALL( SCIPreleaseExpr(scip, &term2) );

            break;
         }

         case nlDivI: /* divide immediate */
         {
            SCIPdebugPrintf("divide constant %g\n", constants[address]);
            assert(constants[address] != 0.0);

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( exprAdd(scip, &e, 1.0/constants[address], term1, 1.0, NULL, 0.0) );

            break;
         }

         case nlUMin: /* unary minus */
         {
            SCIPdebugPrintf("negate\n");

            assert(stackpos >= 1);
            term1 = stack[stackpos-1];
            --stackpos;

            SCIP_CALL( exprAdd(scip, &e, -1.0, term1, 1.0, NULL, 0.0) );

            break;
         }

         case nlUMinV: /* unary minus variable */
         {
            SCIP_Real minusone;

            address = gmoGetjSolver(gmo, address);
            SCIPdebugPrintf("push negated variable %d = <%s>\n", address, SCIPvarGetName(probdata->vars[address]));

            SCIP_CALL( SCIPcreateExprVar(scip, &term1, probdata->vars[address], NULL, NULL) );

            minusone = -1.0;
            SCIP_CALL( SCIPcreateExprSum(scip, &e, 1, &term1, &minusone, 0.0, NULL, NULL) );

            SCIP_CALL( SCIPreleaseExpr(scip, &term1) );

            break;
         }

         case nlFuncArgN:
         {
            SCIPdebugPrintf("set number of arguments = %d\n", address);
            nargs = address + 1;  /* undo shift by one */
            break;
         }

         case nlCallArg1:
            nargs = 1;
            /* no break */
         case nlCallArg2:
            if( opcode == nlCallArg2 )
               nargs = 2;
               /* no break */
         case nlCallArgN:
         {
            GamsFuncCode func;

            SCIPdebugPrintf("call function ");

            func = (GamsFuncCode)(address+1); /* here the shift by one was not a good idea */

            switch( func )
            {
               case fnmin:
               {
                  SCIP_EXPR* diff;
                  SCIP_EXPR* terms[3];
                  SCIP_Real coefs[3];

                  SCIPdebugPrintf("min\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  /* min(term1, term2) = 0.5 (term1+term2) - 0.5 abs(term1-term2) */

                  /* abs(term1-term2) */
                  terms[0] = term1;
                  terms[1] = term2;
                  coefs[0] = 1.0;
                  coefs[1] = -1.0;
                  SCIP_CALL( SCIPcreateExprSum(scip, &diff, 2, terms, coefs, 0.0, NULL, NULL) );

                  SCIP_CALL( SCIPcreateExprAbs(scip, &terms[2], diff, NULL, NULL) );  /* |term1-term2| */

                  coefs[0] = 0.5;
                  coefs[1] = 0.5;
                  coefs[2] = -0.5;
                  SCIP_CALL( SCIPcreateExprSum(scip, &e, 3, terms, coefs, 0.0, NULL, NULL) );

                  SCIP_CALL( SCIPreleaseExpr(scip, &terms[0]) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &terms[1]) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &terms[2]) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &diff) );

                  break;
               }

               case fnmax:
               {
                  SCIP_EXPR* diff;
                  SCIP_EXPR* terms[3];
                  SCIP_Real coefs[3];

                  SCIPdebugPrintf("max\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  /* max(term1, term2) = 0.5 (term1+term2) + 0.5 abs(term1-term2) */

                  /* abs(term1-term2) */
                  terms[0] = term1;
                  terms[1] = term2;
                  coefs[0] = 1.0;
                  coefs[1] = -1.0;
                  SCIP_CALL( SCIPcreateExprSum(scip, &diff, 2, terms, coefs, 0.0, NULL, NULL) );

                  SCIP_CALL( SCIPcreateExprAbs(scip, &terms[2], diff, NULL, NULL) );  /* |term1-term2| */

                  coefs[0] = 0.5;
                  coefs[1] = 0.5;
                  coefs[2] = 0.5;
                  SCIP_CALL( SCIPcreateExprSum(scip, &e, 3, terms, coefs, 0.0, NULL, NULL) );

                  SCIP_CALL( SCIPreleaseExpr(scip, &terms[0]) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &terms[1]) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &terms[2]) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &diff) );

                  break;
               }

               case fnsqr:
               {
                  SCIPdebugPrintf("square\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprPow(scip, &e, term1, 2.0, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  break;
               }

               case fnexp:
               {
                  SCIPdebugPrintf("exp\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprExp(scip, &e, term1, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  break;
               }

               case fnlog:
               {
                  SCIPdebugPrintf("log\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprLog(scip, &e, term1, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  break;
               }

               case fnlog10:
               {
                  SCIP_Real coef;

                  SCIPdebugPrintf("log10 = ln * 1/ln(10)\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprLog(scip, &term2, term1, NULL, NULL) );
                  coef = 1.0/log(10.0);
                  SCIP_CALL( SCIPcreateExprSum(scip, &e, 1, &term2, &coef, 0.0, NULL, NULL) );

                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term2) );

                  break;
               }

               case fnlog2:
               {
                  SCIP_Real coef;

                  SCIPdebugPrintf("log2 = ln * 1/ln(2)\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprLog(scip, &term2, term1, NULL, NULL) );
                  coef = 1.0/log(2.0);
                  SCIP_CALL( SCIPcreateExprSum(scip, &e, 1, &term2, &coef, 0.0, NULL, NULL) );

                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term2) );

                  break;
               }

               case fnsqrt:
               {
                  SCIPdebugPrintf("sqrt\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprPow(scip, &e, term1, 0.5, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  break;
               }

               case fncos:
               {
                  SCIPdebugPrintf("cos\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprCos(scip, &e, term1, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  break;
               }

               case fnsin:
               {
                  SCIPdebugPrintf("sin\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprSin(scip, &e, term1, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
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

                  assert(func != fncvpower || SCIPisExprValue(scip, term2));
                  assert(func != fnvcpower || SCIPisExprValue(scip, term1));

                  if( SCIPisExprValue(scip, term1) )
                  {
                     SCIP_CALL( SCIPcreateExprPow(scip, &e, term2, SCIPgetValueExprValue(term1), NULL, NULL) );
                  }
                  else
                  {
                     /* term2^term1 = exp(log(term2)*term1) */
                     SCIP_EXPR* terms[2];
                     SCIP_EXPR* prod;

                     SCIP_CALL( SCIPcreateExprLog(scip, &terms[0], term2, NULL, NULL) );
                     terms[1] = term1;
                     SCIP_CALL( SCIPcreateExprProduct(scip, &prod, 2, terms, 1.0, NULL, NULL) );
                     SCIP_CALL( SCIPcreateExprExp(scip, &e, prod, NULL, NULL) );

                     SCIP_CALL( SCIPreleaseExpr(scip, &terms[0]) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &prod) );
                  }

                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term2) );

                  break;
               }

               case fnsignpower: /* sign(x)*abs(x)^c */
               {
                  SCIPdebugPrintf("signpower\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  if( SCIPisExprValue(scip, term1) )
                  {
                     SCIP_CALL( SCIPcreateExprSignpower(scip, &e, term2, SCIPgetValueExprValue(term1), NULL, NULL) );
                  }
                  else
                  {
                     SCIPerrorMessage("signpower with non-constant exponent not supported.\n");
                     rc = SCIP_READERROR;
                     goto TERMINATE;
                  }

                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term2) );

                  break;
               }

               case fnpi:
               {
                  SCIPdebugPrintf("pi\n");

                  SCIP_CALL( SCIPcreateExprValue(scip, &e, M_PI, NULL, NULL) );
                  break;
               }

               case fndiv:
               {
                  SCIP_EXPR* terms[2];

                  SCIPdebugPrintf("divide\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  terms[0] = term2;
                  SCIP_CALL( SCIPcreateExprPow(scip, &terms[1], term1, -1.0, NULL, NULL) );
                  SCIP_CALL( SCIPcreateExprProduct(scip, &e, 2, terms, 1.0, NULL, NULL) );

                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &terms[1]) );

                  break;
               }

               case fnabs:
               {
                  SCIPdebugPrintf("abs\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprAbs(scip, &e, term1, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  break;
               }

               case fnpoly: /* univariate polynomial */
               {
                  SCIPdebugPrintf("univariate polynomial of degree %d\n", nargs-2);
                  assert(nargs >= 0);
                  switch( nargs )
                  {
                     case 0:
                     {
                        term1 = stack[stackpos-1];
                        --stackpos;

                        SCIP_CALL( SCIPcreateExprValue(scip, &e, 0.0, NULL, NULL) );
                        SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                        break;
                     }

                     case 1: /* "constant" polynomial */
                     {
                        e = stack[stackpos-1];
                        --stackpos;

                        /* delete variable of polynomial */
                        SCIP_CALL( SCIPreleaseExpr(scip, &stack[stackpos-1]) );
                        --stackpos;

                        break;
                     }

                     default: /* univariate polynomial is at least linear */
                     {
                        SCIP_EXPR** monomials;
                        SCIP_Real* coefs;
                        SCIP_Real exponent;
                        SCIP_Real constant;
                        int nmonomials;

                        nmonomials = nargs-2;
                        SCIP_CALL( SCIPallocBufferArray(scip, &monomials, nargs-2) );
                        SCIP_CALL( SCIPallocBufferArray(scip, &coefs, nargs-2) );

                        /* the argument (like a variable) of the polynomial */
                        term2 = stack[stackpos - nargs];

                        constant = 0.0;
                        for( ; nargs > 1; --nargs )
                        {
                           assert(stackpos > 0);

                           term1 = stack[stackpos-1];
                           assert(SCIPisExprValue(scip, term1));

                           if( nargs > 2 )
                           {
                              exponent = (SCIP_Real)(nargs-2);
                              SCIP_CALL( SCIPcreateExprPow(scip, &monomials[nargs-3], term2, exponent, NULL, NULL) );
                              coefs[nargs-3] = SCIPgetValueExprValue(term1);
                           }
                           else
                           {
                              constant = SCIPgetValueExprValue(term1);
                           }

                           SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                           --stackpos;
                        }

                        assert(stackpos > 0);
                        assert(stack[stackpos-1] == term2);
                        --stackpos;

                        SCIP_CALL( SCIPcreateExprSum(scip, &e, nmonomials, monomials, coefs, constant, NULL, NULL) );

                        SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                        for( nargs = 0; nargs < nmonomials; ++nargs )
                        {
                           SCIP_CALL( SCIPreleaseExpr(scip, &monomials[nargs]) );
                        }

                        SCIPfreeBufferArray(scip, &coefs);
                        SCIPfreeBufferArray(scip, &monomials);
                     }
                  }
                  nargs = -1;
                  break;
               }

               case fnentropy:
               {
                  SCIPdebugPrintf("entropy\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  SCIP_CALL( SCIPcreateExprEntropy(scip, &e, term1, NULL, NULL) );
                  SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                  break;
               }

               case fnboolnot:
               {
                  SCIPdebugPrintf("bool_not\n");

                  assert(stackpos >= 1);
                  term1 = stack[stackpos-1];
                  --stackpos;

                  if( SCIPisExprValue(scip, term1) )
                  {
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, SCIPgetValueExprValue(term1) == 0.0 ? 1.0 : 0.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     break;
                  }

                  if( SCIPisExprVar(scip, term1) && SCIPvarIsBinary(SCIPgetVarExprVar(term1)) )
                  {
                     SCIP_VAR* negvar;
                     SCIP_CALL( SCIPgetNegatedVar(scip, SCIPgetVarExprVar(term1), &negvar) );
                     SCIP_CALL( SCIPcreateExprVar(scip, &e, negvar, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     break;
                  }

                  SCIPerrorMessage("Binary variable or constant required as argument to bool_not.\n");
                  rc = SCIP_READERROR;
                  goto TERMINATE;
               }

               case fnbooland:
               {
                  SCIPdebugPrintf("bool_and\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  if( SCIPisExprValue(scip, term1) && SCIPisExprValue(scip, term2) )
                  {
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, SCIPgetValueExprValue(term1) != 0.0 && SCIPgetValueExprValue(term2) != 0.0 ? 1.0 : 0.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( (SCIPisExprValue(scip, term1) && SCIPgetValueExprValue(term1) == 0.0) ||
                      (SCIPisExprValue(scip, term2) && SCIPgetValueExprValue(term2) == 0.0) )
                  {
                     /* 0 and y == 0, x and 0 == 0 */
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, 0.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term1) )
                  {
                     /* nonzero and y == y */
                     e = term2;
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term2) )
                  {
                     /* x and nonzero == x */
                     e = term1;
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprVar(scip, term1) && SCIPvarIsBinary(SCIPgetVarExprVar(term1)) &&
                      SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                  {
                     SCIP_VAR* vars[2];
                     SCIP_VAR* resvar;
                     SCIP_CONS* cons;
                     char name[30];

                     sprintf(name, "_logic%d", (*logiccount)++);
                     SCIP_CALL( SCIPcreateVarBasic(scip, &resvar, name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
                     SCIP_CALL( SCIPaddVar(scip, resvar) );
                     SCIP_CALL( SCIPcreateExprVar(scip, &e, resvar, NULL, NULL) );

                     vars[0] = SCIPgetVarExprVar(term1);
                     vars[1] = SCIPgetVarExprVar(term2);
                     strcat(name, "def");
                     SCIP_CALL( SCIPcreateConsBasicAnd(scip, &cons, name, resvar, 2, vars) );
                     SCIP_CALL( SCIPaddCons(scip, cons) );

                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     SCIP_CALL( SCIPreleaseVar(scip, &resvar) );
                     SCIP_CALL( SCIPreleaseCons(scip, &cons) );
                     break;
                  }

                  SCIPerrorMessage("Binary variable or constant required as arguments to bool_and.\n");
                  rc = SCIP_READERROR;
                  goto TERMINATE;
               }

               case fnboolor:
               {
                  SCIPdebugPrintf("bool_or\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  if( SCIPisExprValue(scip, term1) && SCIPisExprValue(scip, term2) )
                  {
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, SCIPgetValueExprValue(term1) != 0.0 || SCIPgetValueExprValue(term2) != 0.0 ? 1.0 : 0.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( (SCIPisExprValue(scip, term1) && SCIPgetValueExprValue(term1) != 0.0) ||
                      (SCIPisExprValue(scip, term2) && SCIPgetValueExprValue(term2) != 0.0) )
                  {
                     /* nonzero or y == 1, x or nonzero == 1 */
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, 1.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term1) )
                  {
                     /* zero or y == y */
                     e = term2;
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term2) )
                  {
                     /* zero or x == x */
                     e = term1;
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprVar(scip, term1) && SCIPvarIsBinary(SCIPgetVarExprVar(term1)) &&
                      SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                  {
                     SCIP_VAR* vars[2];
                     SCIP_VAR* resvar;
                     SCIP_CONS* cons;
                     char name[30];

                     sprintf(name, "_logic%d", (*logiccount)++);
                     SCIP_CALL( SCIPcreateVarBasic(scip, &resvar, name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
                     SCIP_CALL( SCIPaddVar(scip, resvar) );
                     SCIP_CALL( SCIPcreateExprVar(scip, &e, resvar, NULL, NULL) );

                     vars[0] = SCIPgetVarExprVar(term1);
                     vars[1] = SCIPgetVarExprVar(term2);
                     strcat(name, "def");
                     SCIP_CALL( SCIPcreateConsBasicOr(scip, &cons, name, resvar, 2, vars) );
                     SCIP_CALL( SCIPaddCons(scip, cons) );

                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     SCIP_CALL( SCIPreleaseVar(scip, &resvar) );
                     SCIP_CALL( SCIPreleaseCons(scip, &cons) );
                     break;
                  }

                  SCIPerrorMessage("Binary variable or constant required as arguments to bool_or.\n");
                  rc = SCIP_READERROR;
                  goto TERMINATE;
               }

               case fnboolxor:
               {
                  SCIPdebugPrintf("bool_xor\n");

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  if( SCIPisExprValue(scip, term1) && SCIPisExprValue(scip, term2) )
                  {
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, (SCIPgetValueExprValue(term1) != 0.0) ^ (SCIPgetValueExprValue(term2) != 0.0) ? 1.0 : 0.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term2) )
                     SCIPswapPointers((void**)&term1, (void**)&term2);

                  if( SCIPisExprValue(scip, term1) )
                  {
                     /* zero xor y == y
                      * nonzero xor y == ~y
                      */
                     if( SCIPgetValueExprValue(term1) == 0.0 )
                     {
                        e = term2;
                        SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                        break;
                     }
                     else if( SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                     {
                        SCIP_VAR* negvar;
                        SCIP_CALL( SCIPgetNegatedVar(scip, SCIPgetVarExprVar(term2), &negvar) );
                        SCIP_CALL( SCIPcreateExprVar(scip, &e, negvar, NULL, NULL) );
                        SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                        SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                        break;
                     }
                  }

                  if( SCIPisExprVar(scip, term1) && SCIPvarIsBinary(SCIPgetVarExprVar(term1)) &&
                      SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                  {
                     SCIP_VAR* vars[3];
                     SCIP_CONS* cons;
                     char name[30];

                     sprintf(name, "_logic%d", (*logiccount)++);
                     SCIP_CALL( SCIPcreateVarBasic(scip, &vars[0], name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
                     SCIP_CALL( SCIPaddVar(scip, vars[0]) );
                     SCIP_CALL( SCIPcreateExprVar(scip, &e, vars[0], NULL, NULL) );

                     vars[1] = SCIPgetVarExprVar(term1);
                     vars[2] = SCIPgetVarExprVar(term2);
                     strcat(name, "def");
                     SCIP_CALL( SCIPcreateConsBasicXor(scip, &cons, name, FALSE, 3, vars) );
                     SCIP_CALL( SCIPaddCons(scip, cons) );

                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     SCIP_CALL( SCIPreleaseVar(scip, &vars[0]) );
                     SCIP_CALL( SCIPreleaseCons(scip, &cons) );
                     break;
                  }

                  SCIPerrorMessage("Binary variable or constant required as arguments to bool_xor.\n");
                  rc = SCIP_READERROR;
                  goto TERMINATE;
               }

               case fnboolimp:
               {
                  SCIPdebugPrintf("bool_imp\n");
                  /* term2 -> term1, i.e., term1 || !term2 */

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  if( SCIPisExprValue(scip, term1) && SCIPisExprValue(scip, term2) )
                  {
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, SCIPgetValueExprValue(term1) != 0.0 || SCIPgetValueExprValue(term2) == 0.0 ? 1.0 : 0.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( (SCIPisExprValue(scip, term1) && SCIPgetValueExprValue(term1) != 0.0) ||
                      (SCIPisExprValue(scip, term2) && SCIPgetValueExprValue(term2) == 0.0) )
                  {
                     /* nonzero or !y == 1, x or !zero == 1 */
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, 1.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term1) && SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                  {
                     /* zero || !y == !y */
                     SCIP_VAR* negvar;
                     SCIP_CALL( SCIPgetNegatedVar(scip, SCIPgetVarExprVar(term2), &negvar) );
                     SCIP_CALL( SCIPcreateExprVar(scip, &e, negvar, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term2) )
                  {
                     /* x or !nonzero == x */
                     e = term1;
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprVar(scip, term1) && SCIPvarIsBinary(SCIPgetVarExprVar(term1)) &&
                      SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                  {
                     SCIP_VAR* vars[2];
                     SCIP_VAR* resvar;
                     SCIP_CONS* cons;
                     char name[30];

                     sprintf(name, "_logic%d", (*logiccount)++);
                     SCIP_CALL( SCIPcreateVarBasic(scip, &resvar, name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
                     SCIP_CALL( SCIPaddVar(scip, resvar) );
                     SCIP_CALL( SCIPcreateExprVar(scip, &e, resvar, NULL, NULL) );

                     vars[0] = SCIPgetVarExprVar(term1);
                     SCIP_CALL( SCIPgetNegatedVar(scip, SCIPgetVarExprVar(term2), &vars[1]) );
                     strcat(name, "def");
                     SCIP_CALL( SCIPcreateConsBasicOr(scip, &cons, name, resvar, 2, vars) );
                     SCIP_CALL( SCIPaddCons(scip, cons) );

                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     SCIP_CALL( SCIPreleaseVar(scip, &resvar) );
                     SCIP_CALL( SCIPreleaseCons(scip, &cons) );
                     break;
                  }

                  SCIPerrorMessage("Binary variable or constant required as arguments to bool_imp.\n");
                  rc = SCIP_READERROR;
                  goto TERMINATE;
               }

               case fnbooleqv:
               {
                  SCIPdebugPrintf("bool_eqv\n");
                  /* !(term1 ^ term2) */

                  assert(stackpos >= 2);
                  term1 = stack[stackpos-1];
                  --stackpos;
                  term2 = stack[stackpos-1];
                  --stackpos;

                  if( SCIPisExprValue(scip, term1) && SCIPisExprValue(scip, term2) )
                  {
                     SCIP_CALL( SCIPcreateExprValue(scip, &e, (SCIPgetValueExprValue(term1) != 0.0) == (SCIPgetValueExprValue(term2) != 0.0) ? 1.0 : 0.0, NULL, NULL) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     break;
                  }

                  if( SCIPisExprValue(scip, term2) )
                     SCIPswapPointers((void**)&term1, (void**)&term2);

                  if( SCIPisExprValue(scip, term1) )
                  {
                     /* nonzero eqv y == y
                      * zero eqv y == ~y
                      */
                     if( SCIPgetValueExprValue(term1) != 0.0 )
                     {
                        e = term2;
                        SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                        break;
                     }
                     else if( SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                     {
                        SCIP_VAR* negvar;
                        SCIP_CALL( SCIPgetNegatedVar(scip, SCIPgetVarExprVar(term2), &negvar) );
                        SCIP_CALL( SCIPcreateExprVar(scip, &e, negvar, NULL, NULL) );
                        SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                        SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                        break;
                     }
                  }

                  if( SCIPisExprVar(scip, term1) && SCIPvarIsBinary(SCIPgetVarExprVar(term1)) &&
                      SCIPisExprVar(scip, term2) && SCIPvarIsBinary(SCIPgetVarExprVar(term2)) )
                  {
                     SCIP_VAR* vars[3];
                     SCIP_CONS* cons;
                     char name[30];

                     sprintf(name, "_logic%d", (*logiccount)++);
                     SCIP_CALL( SCIPcreateVarBasic(scip, &vars[0], name, 0.0, 1.0, 0.0, SCIP_VARTYPE_BINARY) );
                     SCIP_CALL( SCIPaddVar(scip, vars[0]) );
                     SCIP_CALL( SCIPcreateExprVar(scip, &e, vars[0], NULL, NULL) );

                     vars[1] = SCIPgetVarExprVar(term1);
                     vars[2] = SCIPgetVarExprVar(term2);
                     strcat(name, "def");
                     SCIP_CALL( SCIPcreateConsBasicXor(scip, &cons, name, TRUE, 3, vars) );
                     SCIP_CALL( SCIPaddCons(scip, cons) );

                     SCIP_CALL( SCIPreleaseExpr(scip, &term1) );
                     SCIP_CALL( SCIPreleaseExpr(scip, &term2) );
                     SCIP_CALL( SCIPreleaseVar(scip, &vars[0]) );
                     SCIP_CALL( SCIPreleaseCons(scip, &cons) );
                     break;
                  }

                  SCIPerrorMessage("Binary variable or constant required as arguments to bool_eqv.\n");
                  rc = SCIP_READERROR;
                  goto TERMINATE;
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
               case fnsigmoid /* 1/(1+exp(-x)) */:
               case fnrelopeq: case fnrelopgt:
               case fnrelopge: case fnreloplt: case fnrelople:
               case fnrelopne: case fnifthen:
               case fnedist /* euclidian distance */:
               case fncentropy /* x*ln((x+d)/(y+d))*/:
               case fngamma: case fnloggamma: case fnbeta:
               case fnlogbeta: case fngammareg: case fnbetareg:
               case fntan:
               case fnsinh: case fncosh: case fntanh:
               case fnncpvusin /* veelken-ulbrich */:
               case fnncpvupow /* veelken-ulbrich */:
               case fnbinomial:
               case fnarccos:
               case fnarcsin: case fnarctan2 /* arctan(x2/x1) */:
               {
                  SCIPdebugPrintf("nr. %d - unsupported. Error.\n", (int)func);
                  SCIPinfoMessage(scip, NULL, "Error: GAMS function %s not supported.\n", GamsFuncCodeName[func]);
                  rc = SCIP_READERROR;
                  goto TERMINATE;
               }
               default :
               {
                  SCIPdebugPrintf("nr. %d - unsupported. Error.\n", (int)func);
                  SCIPinfoMessage(scip, NULL, "Error: new GAMS function %d not supported.\n", (int)func);
                  rc = SCIP_READERROR;
                  goto TERMINATE;
               }
            } /*lint !e788*/
            break;
         }

         case nlEnd: /* end of instruction list */
         default:
         {
            SCIPinfoMessage(scip, NULL, "Error: GAMS opcode %s not supported.\n", GamsOpCodeName[opcode]);
            rc = SCIP_READERROR;
            goto TERMINATE;
         }
      } /*lint !e788*/

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
   *expr = stack[0];

TERMINATE:
   SCIPfreeBufferArray(scip, &stack);

   return rc;
}

/** creates a SCIP problem from a GMO */
SCIP_RETCODE SCIPcreateProblemReaderGmo(
   SCIP*                 scip,               /**< SCIP data structure */
   gmoRec_t*             gmo,                /**< GAMS Model Object */
   const char*           indicatorfile,      /**< name of file with indicator specification, or NULL */
   int                   mipstart            /**< how to pass initial variable levels from GMO to SCIP */
)
{
   char buffer[GMS_SSSIZE];
   gevHandle_t gev;
   SCIP_Bool objnonlinear;
   SCIP_VAR** vars;
   SCIP_Real minprior;
   SCIP_Real maxprior;
   int i;
   SCIP_Real* coefs = NULL;
   int* indices = NULL;
   int* nlflag;
   SCIP_VAR** consvars = NULL;
   SCIP_CONS* con;
   int numSos1, numSos2, nzSos;
   SCIP_PROBDATA* probdata;
   int* opcodes;
   int* fields;
   SCIP_Real* constants;
   int nindics;
   int* indicrows;
   int* indiccols;
   int* indiconvals;
   int indicidx;
   size_t namemem;
   SCIP_RETCODE rc = SCIP_OKAY;
   int maxstage;
   SCIP_Bool havedecomp;
   int logiccount = 0;
   SCIP_Real infbound;
   SCIP_Bool haveinfbound;
   int nboundschanged = 0;
   
   assert(scip != NULL);
   assert(gmo != NULL);

   gev = (gevHandle_t) gmoEnvironment(gmo);
   assert(gev != NULL);

   /* we want a real objective function, if it is linear, otherwise keep the GAMS single-variable-objective? */
   gmoObjReformSet(gmo, 1);
   gmoObjStyleSet(gmo, (int)gmoObjType_Fun);
#if 0
   if( gmoObjNLNZ(gmo) > 0 )
      gmoObjStyleSet(gmo, gmoObjType_Var);
   objnonlinear = FALSE;
#else
   objnonlinear = gmoObjStyle(gmo) == (int)gmoObjType_Fun && gmoObjNLNZ(gmo) > 0;
#endif

   /* we want to start indexing at 0 */
   gmoIndexBaseSet(gmo, 0);

   /* we want GMO to use SCIP's value for infinity */
   gmoPinfSet(gmo,  SCIPinfinity(scip));
   gmoMinfSet(gmo, -SCIPinfinity(scip));

   SCIP_CALL( SCIPgetRealParam(scip, "gams/infbound", &infbound) );
   haveinfbound = !SCIPisInfinity(scip, infbound);

   /* create SCIP problem */
   SCIP_CALL( SCIPallocMemory(scip, &probdata) );
   BMSclearMemory(probdata);

   (void) gmoNameInput(gmo, buffer);
   SCIP_CALL( SCIPcreateProb(scip, buffer,
      probdataDelOrigGmo, NULL, NULL, NULL, NULL, NULL,
      probdata) );

   /* get data on indicator constraints from options object */
   nindics = 0;
   indicrows = NULL;
   indiccols = NULL;
   indiconvals = NULL;
   if( indicatorfile != NULL && *indicatorfile != '\0' )
   {
      optHandle_t opt;
      int itype;

      if( !optCreate(&opt, buffer, sizeof(buffer)) )
      {
         SCIPerrorMessage("*** Could not create optionfile handle: %s\n", buffer);
         return SCIP_ERROR;
      }

      if( optReadDefinitionFromPChar(opt, (char*)"indic indicator\ngeneral group 1 1 Dot options and indicators") )
      {
         for( i = 1; i <= optMessageCount(opt); ++i )
         {
            optGetMessage(opt, i, buffer, &itype);
            if( itype <= (int) optMsgFileLeave || itype == (int) optMsgUserError )
               gevLogStat(gev, buffer);
         }
         optClearMessages(opt);
         return SCIP_ERROR;
      }

      (void) optReadParameterFile(opt, indicatorfile);
      for( i = 1; i <= optMessageCount(opt); ++i )
      {
         optGetMessage(opt, i, buffer, &itype);
         if( itype <= (int) optMsgFileLeave || itype == (int) optMsgUserError )
            gevLogStat(gev, buffer);
      }
      optClearMessages(opt);

      if( optIndicatorCount(opt, &i) > 0 )
      {
         SCIP_CALL( SCIPallocBufferArray(scip, &indicrows, gmoM(gmo)) );
         SCIP_CALL( SCIPallocBufferArray(scip, &indiccols, gmoM(gmo)) );
         SCIP_CALL( SCIPallocBufferArray(scip, &indiconvals, gmoM(gmo)) );
         if( gmoGetIndicatorMap(gmo, opt, 1, &nindics, indicrows, indiccols, indiconvals) != 0 )
         {
            SCIPerrorMessage("failed to get indicator mapping\n");
            return SCIP_ERROR;
         }
      }

      (void) optFree(&opt);
   }
   assert(indicrows != NULL || nindics == 0);
   assert(indiccols != NULL || nindics == 0);
   assert(indiconvals != NULL || nindics == 0);

   namemem = (gmoN(gmo) + gmoM(gmo)) * sizeof(char*);

   probdata->nvars = gmoN(gmo);
   SCIP_CALL( SCIPallocMemoryArray(scip, &probdata->vars, probdata->nvars) ); /*lint !e666*/
   vars = probdata->vars;
   
   /* compute range of variable priorities */ 
   minprior = SCIPinfinity(scip);
   maxprior = 0.0;
   if( gmoPriorOpt(gmo) && gmoNDisc(gmo) > 0 )
   {
      for (i = 0; i < gmoN(gmo); ++i)
      {
         if( gmoGetVarTypeOne(gmo, i) == (int) gmovar_X )
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
      if( gmoObjStyle(gmo) == (int) gmoObjType_Fun )
         (void) gmoGetObjVector(gmo, coefs, NULL);
      else
         coefs[gmoObjVar(gmo)] = 1.0;
   }
   else
   {
      BMSclearMemoryArray(coefs, gmoN(gmo));
   }
   
   /* add variables, handle priority; get maxstage */
   maxstage = 0;
   havedecomp = FALSE;
   for( i = 0; i < gmoN(gmo); ++i )
   {
      SCIP_VARTYPE vartype;
      SCIP_Real lb, ub, stage;
      lb = gmoGetVarLowerOne(gmo, i);
      ub = gmoGetVarUpperOne(gmo, i);
      switch( gmoGetVarTypeOne(gmo, i) )
      {
         case gmovar_SC:
            lb = 0.0;  /* TODO treat lb < 0 (but GAMS forbids this currently) */
            /* no break */
         case gmovar_X:
         case gmovar_S1:
         case gmovar_S2:
            vartype = SCIP_VARTYPE_CONTINUOUS;
            break;
         case gmovar_B:
            vartype = SCIP_VARTYPE_BINARY;
            break;
         case gmovar_SI:
            lb = 0.0;  /* TODO treat lb < 0 (but GAMS forbids this currently) */
            /* no break */
         case gmovar_I:
            vartype = SCIP_VARTYPE_INTEGER;
            break;
         default:
            SCIPerrorMessage("Unknown variable type.\n");
            return SCIP_INVALIDDATA;
      }
      if( gmoDict(gmo) )
      {
         (void) gmoGetVarNameOne(gmo, i, buffer);
         if( nindics == 0 )
            namemem += strlen(buffer) + 1;
      }
      else
         sprintf(buffer, "x%d", i);

      if( haveinfbound )
      {
         /* bound unbounded variable */
         if( SCIPisInfinity(scip, -lb) )
         {
            lb = -infbound;
            ++nboundschanged;
         }
         if( SCIPisInfinity(scip, ub) )
         {
            ub = infbound;
            ++nboundschanged;
         }
      }

      SCIP_CALL( SCIPcreateVar(scip, &vars[i], buffer, lb, ub, coefs[i], vartype, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL) );
      SCIP_CALL( SCIPaddVar(scip, vars[i]) );
      SCIPdebugMessage("added variable ");
      SCIPdebug( SCIPprintVar(scip, vars[i], NULL) );
      
      if( gmoPriorOpt(gmo) && minprior < maxprior && gmoGetVarTypeOne(gmo, i) != (int) gmovar_X )
      {
         /* in GAMS: higher priorities are given by smaller .prior values
            in SCIP: variables with higher branch priority are always preferred to variables with lower priority in selection of branching variable
            thus, we scale the values from GAMS to lie between 0 (lowest prior) and 1000 (highest prior)
         */
         int branchpriority = (int)(1000.0 / (maxprior - minprior) * (maxprior - gmoGetVarPriorOne(gmo, i)));
         SCIP_CALL( SCIPchgVarBranchPriority(scip, vars[i], branchpriority) );
      }

      stage = gmoGetVarStageOne(gmo, i);
      if( stage != 1.0 )
         havedecomp = TRUE;
      if( EPSISINT(stage, 0.0) )
         maxstage = MAX(maxstage, (int)stage);
   }
   
   /* setup bound disjunction constraints for semicontinuous/semiinteger variables by saying x <= 0 or x >= gmoGetVarLower */
   if( gmoGetVarTypeCnt(gmo, (int) gmovar_SC) || gmoGetVarTypeCnt(gmo, (int) gmovar_SI) )
   {
      SCIP_BOUNDTYPE bndtypes[2];
      SCIP_Real      bnds[2];
      SCIP_VAR*      bndvars[2];
      SCIP_CONS*     cons;
      char           name[SCIP_MAXSTRLEN];
      
      bndtypes[0] = SCIP_BOUNDTYPE_UPPER;
      bndtypes[1] = SCIP_BOUNDTYPE_LOWER;
      bnds[0] = 0.0;

      for( i = 0; i < gmoN(gmo); ++i )
      {
         if( gmoGetVarTypeOne(gmo, i) != (int) gmovar_SC && gmoGetVarTypeOne(gmo, i) != (int) gmovar_SI )
            continue;

         bnds[1] = gmoGetVarLowerOne(gmo, i);
         /* skip bound disjunction if lower bound is 0 (if continuous) or 1 (if integer)
          * since this is a trivial disjunction, but would raise an assert in SCIPcreateConsBounddisjunction
          */
         if( gmoGetVarTypeOne(gmo, i) == (int) gmovar_SC && !SCIPisPositive(scip, bnds[1]) )
            continue;
         if( gmoGetVarTypeOne(gmo, i) == (int) gmovar_SI && !SCIPisPositive(scip, bnds[1]-1.0) )
            continue;

         bndvars[0] = vars[i];
         bndvars[1] = vars[i];
         (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "semi%s_%s", gmoGetVarTypeOne(gmo, i) == (int) gmovar_SC ? "con" : "int", SCIPvarGetName(vars[i]));
         SCIP_CALL( SCIPcreateConsBounddisjunction(scip, &cons, name, 2, bndvars, bndtypes, bnds,
            TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE) );
         SCIP_CALL( SCIPaddCons(scip, cons) );
         SCIPdebugMessage("added constraint ");
         SCIPdebug( SCIPprintCons(scip, cons, NULL) );
         SCIP_CALL( SCIPreleaseCons(scip, &cons) );
      }
   }
   
   if( haveinfbound )
   {
      SCIPinfoMessage(scip, NULL, "\nChanged %d missing variable bounds to +/-%g\n", nboundschanged, infbound);
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
      SCIP_CALL( SCIPallocBufferArray(scip, &sostype, numSos) );
      SCIP_CALL( SCIPallocBufferArray(scip, &sosbeg,  numSos+1) );
      SCIP_CALL( SCIPallocBufferArray(scip, &sosind,  nzSos) );
      SCIP_CALL( SCIPallocBufferArray(scip, &soswt,   nzSos) );

      (void) gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);
      
      for( i = 0; i < numSos; ++i )
      {
         for( j = sosbeg[i], k = 0; j < sosbeg[i+1]; ++j, ++k )
         {
            consvars[k] = vars[sosind[j]];
            assert(gmoGetVarTypeOne(gmo, sosind[j]) == (sostype[i] == 1 ? (int) gmovar_S1 : (int) gmovar_S2));
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
   indicidx = 0;
   
   /* alloc some memory, if nonlinear */
   if( gmoNLNZ(gmo) > 0 || objnonlinear )
   {
      SCIP_CALL( SCIPallocBufferArray(scip, &nlflag, gmoN(gmo)) );

      SCIP_CALL( SCIPallocBufferArray(scip, &opcodes, gmoNLCodeSizeMaxRow(gmo)+1) );
      SCIP_CALL( SCIPallocBufferArray(scip, &fields, gmoNLCodeSizeMaxRow(gmo)+1) );
      SCIP_CALL( SCIPduplicateBufferArray(scip, &constants, (double*)gmoPPool(gmo), gmoNLConst(gmo)) );

      /* translate special GAMS constants into SCIP variants (gmo does not seem to do this...) */
      for( i = 0; i < gmoNLConst(gmo); ++i )
      {
         if( constants[i] == GMS_SV_PINF )
            constants[i] =  SCIPinfinity(scip);
         else if( constants[i] == GMS_SV_MINF )
            constants[i] = -SCIPinfinity(scip);
         else if( constants[i] == GMS_SV_EPS )
            constants[i] = 0.0;
         else if( constants[i] == GMS_SV_UNDEF || constants[i] == GMS_SV_NA || constants[i] == GMS_SV_NAINT || constants[i] == GMS_SV_ACR )
         {
            SCIPwarningMessage(scip, "Constant %e in nonlinear expressions constants pool cannot be handled by SCIP.\n", constants[i]);
            constants[i] = SCIP_INVALID;
         }
         else if( constants[i] <= -SCIPinfinity(scip) )
            constants[i] = -SCIPinfinity(scip);
         else if( constants[i] >=  SCIPinfinity(scip) )
            constants[i] =  SCIPinfinity(scip);
      }
   }
   else
   {
      nlflag = NULL;

      opcodes = NULL;
      fields = NULL;
      constants = NULL;
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
            SCIPerrorMessage("Conic constraints not supported by SCIP interface.\n");
            return SCIP_INVALIDDATA;
         case gmoequ_B:
            SCIPerrorMessage("Logic constraints not supported by SCIP interface yet.\n");
            return SCIP_INVALIDDATA;
         default:
            SCIPerrorMessage("unknown equation type.\n");
            return SCIP_INVALIDDATA;
      }

      if( gmoDict(gmo) )
      {
         (void) gmoGetEquNameOne(gmo, i, buffer);
         if( nindics == 0 )
            namemem += strlen(buffer) + 1;
      }
      else
         sprintf(buffer, "e%d", i);

      con = NULL;
      switch( gmoGetEquOrderOne(gmo, i) )
      {
         case gmoorder_L:
         {
            /* linear constraint */
            int j, nz, nlnz;
            (void) gmoGetRowSparse(gmo, i, indices, coefs, NULL, &nz, &nlnz);
            assert(nlnz == 0);

            for( j = 0; j < nz; ++j )
               consvars[j] = vars[indices[j]];

            /* create indicator constraint, if we are at one */
            if( indicidx < nindics && indicrows[indicidx] == i ) /*lint !e613*/
            {
               SCIP_VAR* binvar;

               binvar = vars[indiccols[indicidx]]; /*lint !e613*/
               if( SCIPvarGetType(binvar) != SCIP_VARTYPE_BINARY )
               {
                  SCIPerrorMessage("Indicator variable <%s> is not of binary type.\n", SCIPvarGetName(binvar));
                  return SCIP_ERROR;
               }

               assert(indiconvals[indicidx] == 0 || indiconvals[indicidx] == 1); /*lint !e613*/
               if( indiconvals[indicidx] == 0 ) /*lint !e613*/
               {
                  SCIP_CALL( SCIPgetNegatedVar(scip, binvar, &binvar) );
               }

               if( !SCIPisInfinity(scip, rhs) )
               {
                  SCIP_CALL( SCIPcreateConsIndicator(scip, &con, buffer, binvar, nz, consvars, coefs, rhs,
                     TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );

                  if( !SCIPisInfinity(scip, -lhs) )
                  {
                     SCIP_CALL( SCIPaddCons(scip, con) );
                     SCIPdebugMessage("added constraint ");
                     SCIPdebug( SCIPprintCons(scip, con, NULL) );
                     SCIP_CALL( SCIPreleaseCons(scip, &con) );
                     con = NULL;
                  }
               }
               if( !SCIPisInfinity(scip, -lhs) )
               {
                  for( j = 0; j < nz; ++j )
                     coefs[j] = -coefs[j];
                  SCIP_CALL( SCIPcreateConsIndicator(scip, &con, buffer, binvar, nz, consvars, coefs, -lhs,
                     TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE) );
               }

               ++indicidx;
            }
            else
            {
               SCIP_CALL( SCIPcreateConsLinear(scip, &con, buffer, nz, consvars, coefs, lhs, rhs,
                     TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE) );
            }

            break;
         }
         
         case gmoorder_NL:
         {
            /* nonlinear constraint */
            int j, nz, nlnz;
            int codelen;
            SCIP_EXPR* expr;

            /* create expression and nonlinear constraint */
            (void) gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
            rc = makeExpr(scip, gmo, codelen, opcodes, fields, constants, &logiccount, &expr);
            if( rc == SCIP_READERROR )
            {
               SCIPinfoMessage(scip, NULL, "Error processing nonlinear instructions of equation %s.\n", buffer);
               goto TERMINATE;
            }
            SCIP_CALL( rc );

            SCIP_CALL( SCIPcreateConsBasicNonlinear(scip, &con, buffer, expr, lhs, rhs) );
            SCIP_CALL( SCIPreleaseExpr(scip, &expr) );

            /* add linear part */
            assert(nlflag != NULL);
            (void) gmoGetRowSparse(gmo, i, indices, coefs, nlflag, &nz, &nlnz);
            for( j = 0; j < nz; ++j )
            {
               if( !nlflag[j] )
               {
                  SCIP_CALL( SCIPaddLinearVarNonlinear(scip, con, vars[indices[j]], coefs[j]) );
               }
            }

            break;
         }

         default:
            SCIPerrorMessage("Unexpected equation order.\n");
            return SCIP_INVALIDDATA;
      }
      
      assert(con != NULL);
      SCIP_CALL( SCIPaddCons(scip, con) );      
      SCIPdebugMessage("added constraint ");
      SCIPdebug( SCIPprintCons(scip, con, NULL) );
      SCIP_CALL( SCIPreleaseCons(scip, &con) );

      /* @todo do something about this */
      if( indicidx < nindics && indicrows[indicidx] == i ) /*lint !e613*/
      {
         SCIPerrorMessage("Only linear constraints can be indicatored, currently.\n");
         return SCIP_ERROR;
      }
   }
   
   if( objnonlinear )
   {
      /* make constraint out of nonlinear objective function */
      int j, nz, nlnz;
      double lhs, rhs;
      SCIP_Real objfactor;
      int codelen;
      SCIP_EXPR* expr;
      
      assert(gmoGetObjOrder(gmo) == (int) gmoorder_NL);

      SCIP_CALL( SCIPcreateVar(scip, &probdata->objvar, "xobj", -SCIPinfinity(scip), SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL) );
      SCIP_CALL( SCIPaddVar(scip, probdata->objvar) );
      SCIPdebugMessage("added objective variable ");
      SCIPdebug( SCIPprintVar(scip, probdata->objvar, NULL) );

      assert(nlflag != NULL);

      objfactor = -1.0 / gmoObjJacVal(gmo);

      (void) gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);
      rc = makeExpr(scip, gmo, codelen, opcodes, fields, constants, &logiccount, &expr);
      if( rc == SCIP_READERROR )
      {
         SCIPinfoMessage(scip, NULL, "Error processing nonlinear instructions of objective %s.\n", gmoGetObjName(gmo, buffer));
         goto TERMINATE;
      }
      SCIP_CALL( rc );

      if( gmoSense(gmo) == (int) gmoObj_Min )
      {
         lhs = -SCIPinfinity(scip);
         rhs = -gmoObjConst(gmo);
      }
      else
      {
         lhs = -gmoObjConst(gmo);
         rhs = SCIPinfinity(scip);
      }

      if( objfactor != 1.0 )
      {
         SCIP_EXPR* tmp;

         SCIP_CALL( exprAdd(scip, &tmp, objfactor, expr, 1.0, NULL, 0.0) );
         expr = tmp;
      }

      SCIP_CALL( SCIPcreateConsBasicNonlinear(scip, &con, "objective", expr, lhs, rhs) );
      SCIP_CALL( SCIPreleaseExpr(scip, &expr) );

      /* add linear part */
      (void) gmoGetObjSparse(gmo, indices, coefs, nlflag, &nz, &nlnz);
      for( j = 0; j < nz; ++j )
      {
         if( !nlflag[j] )
         {
            SCIP_CALL( SCIPaddLinearVarNonlinear(scip, con, vars[indices[j]], coefs[j]) );
         }
      }

      SCIP_CALL( SCIPaddLinearVarNonlinear(scip, con, probdata->objvar, -1.0) );

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

   if( gmoSense(gmo) == (int) gmoObj_Max )
      SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE) );
   
   /* set objective limit, if enabled */
   if( gevGetIntOpt(gev, gevUseCutOff) )
   {
      SCIP_CALL( SCIPsetObjlimit(scip, gevGetDblOpt(gev, gevCutOff)) );
   }

   /* handle initial solution values */
   switch( mipstart )
   {
      case 0 :
      {
         /* don't pass any initial values to SCIP */
         break;
      }

      case 2:
      case 3:
      {
         /* pass all initial values to SCIP and let SCIP check feasibility (2) or repair (3)
          * NOTE: mipstart=3 does not work as expected: heur_completesol does not run if values for all vars are given and for all integer variables integral values are given
          */
         SCIP_SOL* sol;
         SCIP_Real* vals;
         SCIP_Bool stored;

         if( mipstart == 2 )
         {
            /* with this, SCIP will only check feasibility */
            SCIP_CALL( SCIPcreateOrigSol(scip, &sol, NULL) );
         }
         else
         {
            /* with this, SCIP will try to find a feasible solution close by to the initial values */
            SCIP_CALL( SCIPcreatePartialSol(scip, &sol, NULL) );
         }

         SCIP_CALL( SCIPallocBufferArray(scip, &vals, gmoN(gmo)) );
         (void) gmoGetVarL(gmo, vals);

         SCIP_CALL( SCIPsetSolVals(scip, sol, gmoN(gmo), probdata->vars, vals) );

         /* if we have extra variable for objective, then need to set its value too */
         if( probdata->objvar != NULL )
         {
            double objval;
            int numErr;
            (void) gmoEvalFuncObj(gmo, vals, &objval, &numErr);
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

         SCIP_CALL( SCIPaddSolFree(scip, &sol, &stored) );
         assert(stored);

         SCIPfreeBufferArray(scip, &vals);

         if( mipstart == 3 )
         {
            SCIPinfoMessage(scip, NULL, "Passed partial solution with values for all variables to SCIP.");
         }

         break;
      }

      case 1:
      case 4:
      {
         /* pass some initial value to SCIP and let SCIP complete solution */
         SCIP_SOL* sol;
         SCIP_Bool stored;
         double tryint = 0.0;
         int nknown;

         if( mipstart == 4 )
            tryint = gevGetDblOpt(gev, gevTryInt);

         SCIP_CALL( SCIPcreatePartialSol(scip, &sol, NULL) );

         nknown = 0;
         for( i = 0; i < gmoN(gmo); ++i )
         {
            if( mipstart == 1 && (gmoGetVarTypeOne(gmo, i) == gmovar_B || gmoGetVarTypeOne(gmo, i) == gmovar_I || gmoGetVarTypeOne(gmo, i) == gmovar_SI) )
            {
               /* 1: set all integer variables */
               SCIP_CALL( SCIPsetSolVal(scip, sol, probdata->vars[i], gmoGetVarLOne(gmo, i)) );
               ++nknown;
            }

            if( mipstart == 4 && (gmoGetVarTypeOne(gmo, i) == gmovar_B || gmoGetVarTypeOne(gmo, i) == gmovar_I || gmoGetVarTypeOne(gmo, i) == gmovar_SI) )
            {
               /* 4: set only integer variables with level close to an integral value, closeness decided by tryint */
               SCIP_Real val;

               val = gmoGetVarLOne(gmo, i);
               if( fabs(round(val)-val) <= tryint )
               {
                  SCIP_CALL( SCIPsetSolVal(scip, sol, probdata->vars[i], val) );
                  ++nknown;
               }
            }
         }

         /* if we have extra variable for objective constant, then can set its value to 1.0 here too */
         if( probdata->objconst != NULL )
         {
            SCIP_CALL( SCIPsetSolVal(scip, sol, probdata->objconst, 1.0) );
         }

         SCIP_CALL( SCIPaddSolFree(scip, &sol, &stored) );
         assert(stored);

         SCIPinfoMessage(scip, NULL, "Passed partial solution with values for %d variables (%.1f%%) to SCIP.", nknown, 100.0*(double)nknown/gmoN(gmo));

         break;
      }

      default:
      {
         SCIPwarningMessage(scip, "Setting mipstart = %d not supported. Ignored.\n", mipstart);
      }
   }

   if( namemem > 1024 * 1024 && nindics == 0 )
   {
      namemem <<= 1;  /* transformed problem has copy of names, so duplicate estimate */
      SCIPinfoMessage(scip, NULL, "Space for names approximately %0.2f MB. Use statement '<modelname>.dictfile=0;' to turn dictionary off.\n", namemem/(1024.0*1024.0));
   }

   /* pass on decomposition info derived from variable stage attributes
    * ignoring stage 0 to do the same as GAMS/CPLEX
    */
   if( maxstage > 0 && havedecomp )
   {
      SCIP_DECOMP* decomp;
      SCIP_VAR** labeledvars;
      int* labels;
      int nlabels = 0;
      SCIP_Bool dispstat;

      SCIP_CALL( SCIPcreateDecomp(scip, &decomp, maxstage, TRUE, FALSE) );

      SCIP_CALL( SCIPallocBufferArray(scip, &labeledvars, gmoN(gmo)) );
      SCIP_CALL( SCIPallocBufferArray(scip, &labels, gmoN(gmo)) );

      for( i = 0; i < gmoN(gmo); ++i )
      {
         double stage;

         stage = gmoGetVarStageOne(gmo, i);
         if( !EPSISINT(stage, 0.0) || stage <= 0.0 )
            continue;

         labeledvars[nlabels] = vars[i];
         labels[nlabels] = (int)stage - 1;  /* I believe that SCIP want's labels to start from 0 */

         SCIPdebugMsg(scip, "var <%s> with label %d\n", SCIPvarGetName(vars[i]), labels[nlabels]);

         ++nlabels;
      }

      SCIP_CALL( SCIPdecompSetVarsLabels(decomp, labeledvars, labels, nlabels) );
      SCIP_CALL( SCIPcomputeDecompConsLabels(scip, decomp, SCIPgetConss(scip), SCIPgetNConss(scip)) );

      SCIP_CALL( SCIPgetBoolParam(scip, "display/statistics", &dispstat) );
      if( dispstat )
      {
         char decompstats[SCIP_MAXSTRLEN];
         SCIP_CALL( SCIPcomputeDecompStats(scip, decomp, TRUE) );
         SCIPinfoMessage(scip, NULL, SCIPdecompPrintStats(decomp, decompstats) );
      }

      SCIP_CALL( SCIPaddDecomp(scip, decomp) );

      SCIPfreeBufferArray(scip, &labels);
      SCIPfreeBufferArray(scip, &labeledvars);
   }

TERMINATE:
   SCIPfreeBufferArrayNull(scip, &coefs);
   SCIPfreeBufferArrayNull(scip, &indices);
   SCIPfreeBufferArrayNull(scip, &consvars);
   SCIPfreeBufferArrayNull(scip, &nlflag);
   SCIPfreeBufferArrayNull(scip, &opcodes);
   SCIPfreeBufferArrayNull(scip, &fields);
   SCIPfreeBufferArrayNull(scip, &constants);
   SCIPfreeBufferArrayNull(scip, &indicrows);
   SCIPfreeBufferArrayNull(scip, &indiccols);
   SCIPfreeBufferArrayNull(scip, &indiconvals);

   return rc;
}

/** stores solve information (solution, statistics) in a GMO */
static
SCIP_RETCODE writeGmoSolution(
   SCIP*                 scip,               /**< SCIP data structure */
   gmoHandle_t           gmo                 /**< GAMS Model Object */
)
{
   SCIP_PROBDATA* probdata;
   int nrsol;
   SCIP_Real dualbound;

   probdata = SCIPgetProbData(scip);
   assert(probdata != NULL);
   assert(probdata->vars != NULL);

   nrsol = SCIPgetNSols(scip);

   switch( SCIPgetStatus(scip) )
   {
      default:
      case SCIP_STATUS_UNKNOWN: /* the solving status is not yet known */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_SystemErr);
         gmoModelStatSet(gmo, (int) gmoModelStat_ErrorNoSolution);
         nrsol = 0;
         break;
      case SCIP_STATUS_USERINTERRUPT: /* the user interrupted the solving process (by pressing Ctrl-C) */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_User);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? (int) gmoModelStat_Integer : (int) gmoModelStat_Feasible) : (int) gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_NODELIMIT:      /* the solving process was interrupted because the node limit was reached */
      case SCIP_STATUS_STALLNODELIMIT: /* the solving process was interrupted because the node limit was reached */
      case SCIP_STATUS_TOTALNODELIMIT:
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Iteration);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? (int) gmoModelStat_Integer : (int) gmoModelStat_Feasible) : (int) gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_TIMELIMIT: /* the solving process was interrupted because the time limit was reached */
      case SCIP_STATUS_MEMLIMIT:  /* the solving process was interrupted because the memory limit was reached */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Resource);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? (int) gmoModelStat_Integer : (int) gmoModelStat_Feasible) : (int) gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_GAPLIMIT: /* the solving process was interrupted because the gap limit was reached */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Normal);
         gmoModelStatSet(gmo, nrsol > 0 ? (SCIPgetGap(scip) > 0.0 ? (gmoNDisc(gmo) ? (int) gmoModelStat_Integer : (int) gmoModelStat_Feasible) : (int) gmoModelStat_OptimalGlobal): (int) gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_SOLLIMIT: /* the solving process was interrupted because the solution limit was reached */
      case SCIP_STATUS_BESTSOLLIMIT: /* the solving process was interrupted because the solution improvement limit was reached */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Resource);
         gmoModelStatSet(gmo, nrsol > 0 ? (gmoNDisc(gmo) ? (int) gmoModelStat_Integer : (int) gmoModelStat_Feasible) : (int) gmoModelStat_NoSolutionReturned);
         break;
      case SCIP_STATUS_OPTIMAL: /* the problem was solved to optimality, an optimal solution is available */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Normal);
         gmoModelStatSet(gmo, (int) gmoModelStat_OptimalGlobal);
         break;
      case SCIP_STATUS_INFEASIBLE: /* the problem was proven to be infeasible */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Normal);
         gmoModelStatSet(gmo, (int) gmoModelStat_InfeasibleNoSolution);
         nrsol = 0;
         break;
      case SCIP_STATUS_UNBOUNDED: /* the problem was proven to be unbounded */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Normal);
         gmoModelStatSet(gmo, nrsol > 0 ? (int) gmoModelStat_Unbounded : (int) gmoModelStat_UnboundedNoSolution);
         break;
      case SCIP_STATUS_INFORUNBD: /* the problem was proven to be either infeasible or unbounded */
         gmoSolveStatSet(gmo, (int) gmoSolveStat_Normal);
         gmoModelStatSet(gmo, (int) gmoModelStat_NoSolutionReturned);
         nrsol = 0;
         break;
   }

   if( SCIPgetStage(scip) == SCIP_STAGE_SOLVING || SCIPgetStage(scip) == SCIP_STAGE_SOLVED )
      dualbound = SCIPgetDualbound(scip);
   else
      dualbound = gmoValNA(gmo);
   gmoSetHeadnTail(gmo, (int) gmoTmipbest, dualbound);
   gmoSetHeadnTail(gmo, (int) gmoTmipnod,  (double) SCIPgetNNodes(scip));
   gmoSetHeadnTail(gmo, (int) gmoHresused, SCIPgetSolvingTime(scip));
   gmoSetHeadnTail(gmo, (int) gmoHiterused, (double) SCIPgetNLPIterations(scip));
   gmoSetHeadnTail(gmo, (int) gmoHdomused, 0.0);

   /* dump all solutions, if more than one found and parameter is set */
   if( nrsol > 1)
   {
      char* indexfilename;

      SCIP_CALL( SCIPgetStringParam(scip, "gams/dumpsolutions", &indexfilename) );
      if( indexfilename != NULL && indexfilename[0] )
      {
         char buffer[SCIP_MAXSTRLEN];
         gdxHandle_t gdx;
         int rc;

         if( !gdxCreate(&gdx, buffer, sizeof(buffer)) )
         {
            SCIPerrorMessage("failed to load GDX I/O library: %s\n", buffer);
            return SCIP_OKAY;
         }

         SCIPinfoMessage(scip, NULL, "\nDumping %d alternate solutions:\n", nrsol-1);
         /* create index GDX file */
#if GDXAPIVERSION >= 8
         gdxStoreDomainSetsSet(gdx, 0);
#endif
         if( gdxOpenWrite(gdx, indexfilename, "SCIP DumpSolutions Index File", &rc) == 0 )
         {
            rc = gdxGetLastError(gdx);
            (void) gdxErrorStr(gdx, rc, buffer);
            SCIPerrorMessage("problem writing GDX file %s: %s\n", indexfilename, buffer);
         }
         else
         {
            gdxStrIndexPtrs_t keys;
            gdxStrIndex_t     keysX;
            gdxValues_t       vals;
            SCIP_Real* collev;
            int sloc;
            int i;

            /* create index file */
            GDXSTRINDEXPTRS_INIT(keysX, keys);
            (void) gdxDataWriteStrStart(gdx, "index", "Dumpsolutions index", 1, (int) dt_set, 0);
            for( i = 1; i < nrsol; ++i)
            {
               (void) SCIPsnprintf(buffer, SCIP_MAXSTRLEN, "soln_scip_p%d.gdx", i);
               (void) gdxAddSetText(gdx, buffer, &sloc);
               (void) SCIPsnprintf(keys[0], GMS_SSSIZE, "file%d", i);
               vals[GMS_VAL_LEVEL] = sloc;
               (void) gdxDataWriteStr(gdx, (const char**)keys, vals);
            }
            (void) gdxDataWriteDone(gdx);
            (void) gdxClose(gdx);

            SCIP_CALL( SCIPallocBufferArray(scip, &collev, gmoN(gmo)) );

            /* create point files */
            for( i = 1; i < nrsol; ++i)
            {
               (void) SCIPsnprintf(buffer, SCIP_MAXSTRLEN, "soln_scip_p%d.gdx", i);

               SCIP_CALL( SCIPgetSolVals(scip, SCIPgetSols(scip)[i], probdata->nvars, probdata->vars, collev) );
               (void) gmoSetVarL(gmo, collev);
               if( gmoUnloadSolutionGDX(gmo, buffer, 0, 1, 0) )
               {
                  SCIPerrorMessage("Problems creating point file %s\n", buffer);
               }
               else
               {
                  SCIPinfoMessage(scip, NULL, "Created point file %s\n", buffer);
               }
            }

            SCIPfreeBufferArray(scip, &collev);
         }

         (void) gdxFree(&gdx);
      }

      SCIP_CALL( SCIPgetStringParam(scip, "gams/dumpsolutionsmerged", &indexfilename) );
      if( indexfilename != NULL && indexfilename[0] )
      {
         int solnvarsym;

         if( gmoCheckSolPoolUEL(gmo, "soln_scip_p", &solnvarsym) )
         {
            SCIPerrorMessage("Solution pool scenario label 'soln_scip_p' contained in model dictionary. Cannot dump merged solutions pool.\n");
         }
         else
         {
            void* handle;

            handle = gmoPrepareSolPoolMerge(gmo, indexfilename, nrsol-1, "soln_scip_p");
            if( handle != NULL )
            {
               SCIP_Real* collev;
               int k, i;

               SCIP_CALL( SCIPallocBufferArray(scip, &collev, gmoN(gmo)) );
               for ( k = 0; k < solnvarsym; k++ )
               {
                  gmoPrepareSolPoolNextSym(gmo, handle);
                  for( i = 1; i < nrsol; ++i )
                  {
                     SCIP_CALL( SCIPgetSolVals(scip, SCIPgetSols(scip)[i], probdata->nvars, probdata->vars, collev) );
                     (void) gmoSetVarL(gmo, collev);
                     if( gmoUnloadSolPoolSolution (gmo, handle, i-1) )
                     {
                        SCIPerrorMessage("Problems unloading solution point %d symbol %d\n", i, k);
                     }
                  }
               }
               if( gmoFinalizeSolPoolMerge(gmo, handle) )
               {
                  SCIPerrorMessage("Problems finalizing merged solution pool\n");
               }
               SCIPfreeBufferArray(scip, &collev);
            }
            else
            {
               SCIPerrorMessage("Problems preparing merged solution pool\n");
            }
         }
      }
   }

   /* pass best solution to GMO, if any */
   if( nrsol > 0 )
   {
      SCIP_SOL* sol;
      SCIP_Real* collev;

      sol = SCIPgetBestSol(scip);
      assert(sol != NULL);

      SCIP_CALL( SCIPallocBufferArray(scip, &collev, gmoN(gmo)) );
      SCIP_CALL( SCIPgetSolVals(scip, sol, probdata->nvars, probdata->vars, collev) );

      (void) gmoSetSolutionPrimal(gmo, collev);

      SCIPfreeBufferArray(scip, &collev);
   }

   if( gmoModelType(gmo) == (int) gmoProc_cns )
      switch( gmoModelStat(gmo) )
      {
         case gmoModelStat_OptimalGlobal:
         case gmoModelStat_OptimalLocal:
         case gmoModelStat_Feasible:
         case gmoModelStat_Integer:
            gmoModelStatSet(gmo, (int) gmoModelStat_Solved);
            gmoSetHeadnTail(gmo, gmoHobjval, 0.0);
      } /*lint !e744*/

   return SCIP_OKAY;
}

/*
 * Callback methods of reader
 */

/** destructor of reader to free user data (called when SCIP is exiting) */
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


/** problem reading method of reader */
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
         (void) gmoFree(&gmo);
         (void) gevFree(&gev);
         return SCIP_READERROR;
      }

      if( gmoRegisterEnvironment(gmo, gev, buffer) )
      {
         SCIPerrorMessage("Error registering GAMS Environment: %s\n", buffer);
         (void) gmoFree(&gmo);
         (void) gevFree(&gev);
         return SCIP_ERROR;
      }

      if( gmoLoadDataLegacy(gmo, buffer) )
      {
         SCIPerrorMessage("Could not load model data.\n");
         (void) gmoFree(&gmo);
         (void) gevFree(&gev);
         return SCIP_READERROR;
      }
   }

   SCIP_CALL( SCIPcreateProblemReaderGmo(scip, readerdata->gmo, readerdata->indicatorfile, readerdata->mipstart) );

   *result = SCIP_SUCCESS;

   return SCIP_OKAY;
}

/*
 * Constructs SCIP problem from the one in GMO.
 */

#define DIALOG_READGAMS_NAME                 "readgams"
#define DIALOG_READGAMS_DESC                 "initializes SCIP problem to the one stored in a GAMS modeling object"
#define DIALOG_READGAMS_ISSUBMENU            FALSE

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

      SCIP_CALL( SCIPcreateProblemReaderGmo(scip, readerdata->gmo, readerdata->indicatorfile, readerdata->mipstart) );
   }

   SCIPverbMessage(scip, SCIP_VERBLEVEL_NORMAL, NULL,
      "\noriginal problem has %d variables (%d bin, %d int, %d cont) and %d constraints\n",
      SCIPgetNOrigVars(scip), SCIPgetNOrigBinVars(scip), SCIPgetNOrigIntVars(scip), SCIPgetNOrigContVars(scip),
      SCIPgetNConss(scip));

   *nextdialog = SCIPdialoghdlrGetRoot(dialoghdlr);

   return SCIP_OKAY;
}


/*
 * Writing solution information to GMO dialog
 */

#define DIALOG_WRITEGAMSSOL_NAME             "gamssol"
#define DIALOG_WRITEGAMSSOL_DESC             "writes solution information into GAMS Modeling Object"
#define DIALOG_WRITEGAMSSOL_ISSUBMENU        FALSE

/** execution method of dialog */
static
SCIP_DECL_DIALOGEXEC(dialogExecWriteGamsSol)
{  /*lint --e{715}*/
   SCIP_READERDATA* readerdata;

   SCIP_CALL( SCIPdialoghdlrAddHistory(dialoghdlr, dialog, NULL, FALSE) );

   readerdata = (SCIP_READERDATA*) SCIPdialogGetData(dialog);
   assert(readerdata != NULL);

   SCIP_CALL( writeGmoSolution(scip, readerdata->gmo) );

   *nextdialog = SCIPdialoghdlrGetRoot(dialoghdlr);

   return SCIP_OKAY;
}


/*
 * Loading GAMS and user option file dialog
 */

#define DIALOG_SETTINGSLOADGAMS_NAME         "loadgams"
#define DIALOG_SETTINGSLOADGAMS_DESC         "loads GAMS settings and SCIP option file specified in GAMS model"
#define DIALOG_SETTINGSLOADGAMS_ISSUBMENU    FALSE

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
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_Bool             ipoptlicensed       /**< whether GAMS/IpoptH is licensed */
   )
{
   SCIP_READERDATA* readerdata;
   SCIP_DIALOG* dialog;
   SCIP_DIALOG* parentdialog;

   /* create gmo reader data */
   SCIP_CALL( SCIPallocMemory(scip, &readerdata) );
   BMSclearMemory(readerdata);
   readerdata->ipoptlicensed = ipoptlicensed;
   
   /* include gmo reader */
   SCIP_CALL( SCIPincludeReader(scip, READER_NAME, READER_DESC, READER_EXTENSION,
         NULL, readerFreeGmo, readerReadGmo, NULL, readerdata) );

   SCIP_CALL( SCIPaddStringParam(scip, "gams/dumpsolutions",
      "name of solutions index gdx file for writing all alternate solutions",
      NULL, FALSE, "", NULL, NULL) );

   SCIP_CALL( SCIPaddStringParam(scip, "gams/dumpsolutionsmerged",
      "name of gdx file for writing all alternate solutions into a single file",
      NULL, FALSE, "", NULL, NULL) );

   SCIP_CALL( SCIPaddIntParam(scip, "gams/mipstart",
      "how to handle initial variable levels",
      &readerdata->mipstart, FALSE, 2, 0, 4, NULL, NULL) );

   SCIP_CALL( SCIPaddStringParam(scip, "gams/indicatorfile",
      "name of GAMS options file that contains definitions on indicators",
      &readerdata->indicatorfile, FALSE, "", NULL, NULL) );

   SCIP_CALL( SCIPaddRealParam(scip, "gams/infbound",
      "value to use for variable bounds that are missing or exceed numerics/infinity",
      NULL, FALSE, SCIP_REAL_MAX, 0.0, SCIP_REAL_MAX, NULL, NULL) );

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
            NULL, dialogExecReadGams, NULL, NULL,
            DIALOG_READGAMS_NAME, DIALOG_READGAMS_DESC, DIALOG_READGAMS_ISSUBMENU, (SCIP_DIALOGDATA*)readerdata) );
      SCIP_CALL( SCIPaddDialogEntry(scip, SCIPgetRootDialog(scip), dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );
   }


   /* create, include, and release dialog */
   if( !SCIPdialogHasEntry(parentdialog, DIALOG_WRITEGAMSSOL_NAME) )
   {
      SCIP_CALL( SCIPincludeDialog(scip, &dialog,
            NULL, dialogExecWriteGamsSol, NULL, NULL,
            DIALOG_WRITEGAMSSOL_NAME, DIALOG_WRITEGAMSSOL_DESC, DIALOG_WRITEGAMSSOL_ISSUBMENU, (SCIP_DIALOGDATA*)readerdata) );
      SCIP_CALL( SCIPaddDialogEntry(scip, parentdialog, dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );
   }

   /* get parent dialog "set" */
   SCIP_CALL( SCIPincludeDialogDefaultSet(scip) );
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
            NULL, dialogExecSettingsLoadGams, NULL, NULL,
            DIALOG_SETTINGSLOADGAMS_NAME, DIALOG_SETTINGSLOADGAMS_DESC, DIALOG_SETTINGSLOADGAMS_ISSUBMENU, NULL) );
      SCIP_CALL( SCIPaddDialogEntry(scip, parentdialog, dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );
   }

   /* set gams */
   if( !SCIPdialogHasEntry(parentdialog, "gams") )
   {
      SCIP_CALL( SCIPincludeDialog(scip, &dialog,
            NULL, SCIPdialogExecMenu, NULL, NULL,
            "gams", "change parameters for GAMS interface", TRUE, NULL) );
      SCIP_CALL( SCIPaddDialogEntry(scip, parentdialog, dialog) );
      SCIP_CALL( SCIPreleaseDialog(scip, &dialog) );
   }

   return SCIP_OKAY;
}

/** sets the GMO object to use in reader
 * If GMO is set in reader, then reader does not read from file when executed, but sets up problem from GMO
 */
void SCIPsetGMOReaderGmo(
   SCIP*                 scip,               /**< SCIP data structure */
   gmoRec_t*             gmo                 /**< GMO object, or NULL to reset to default behaviour */
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

   readerdata->gmo = gmo;
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
      SCIP_CALL( SCIPsetLongintParam(scip, "limits/nodes", (long long)gevGetIntOpt(gev, gevNodeLim)) );
   }
   if( !SCIPisInfinity(scip, gevGetDblOpt(gev, gevResLim)) )
   {
      SCIP_CALL( SCIPsetRealParam(scip, "limits/time", gevGetDblOpt(gev, gevResLim)) );
   }
   SCIP_CALL( SCIPsetRealParam(scip, "limits/gap", gevGetDblOpt(gev, gevOptCR)) );
   if( !SCIPisInfinity(scip, gevGetDblOpt(gev, gevOptCA)) )
   {
      SCIP_CALL( SCIPsetRealParam(scip, "limits/absgap", gevGetDblOpt(gev, gevOptCA)) );
   }
   else
   {
      SCIPwarningMessage(scip, "Value for optca = %g >= value for infinity. Setting solution limit to 1 instead.\n", gevGetDblOpt(gev, gevOptCA));
      SCIP_CALL( SCIPsetIntParam(scip, "limits/solutions", 1) );
   }
   if( gevGetDblOpt(gev, gevWorkSpace) > 0.0 )
   {
      SCIP_CALL( SCIPsetRealParam(scip, "limits/memory", gevGetDblOpt(gev, gevWorkSpace)) );
   }
   SCIP_CALL( SCIPsetIntParam(scip, "lp/threads", gevThreads(gev)) );
   if( SCIPgetParam(scip, "presolving/milp/threads") != NULL )
   {
      SCIP_CALL( SCIPsetIntParam(scip, "presolving/milp/threads", gevThreads(gev)) );
   }

   /* if log is not kept, then can also set SCIP verblevel to 0 */
   if( gevGetIntOpt(gev, gevLogOption) == 0 )
   {
      SCIP_CALL( SCIPsetIntParam(scip, "display/verblevel", 0) );
   }

#ifdef _WIN32
   if( !gevGetIntOpt(gev, gevIDEFlag) )
   {
      SCIP_CALL( SCIPsetIntParam(scip, "display/width", 80) );
      SCIP_CALL( SCIPsetIntParam(scip, "display/lpavgiterations/active", 0) );
      SCIP_CALL( SCIPsetIntParam(scip, "display/maxdepth/active", 0) );
      SCIP_CALL( SCIPsetIntParam(scip, "display/time/active", 2) );
   }
#endif

   if( gmoNDisc(gmo) == 0 && (gmoNLNZ(gmo) > 0 || (gmoObjStyle(gmo) == (int) gmoObjType_Fun && gmoObjNLNZ(gmo) > 0)) )
   {
      /* add linearizations in new solutions if solving NLP
       * makes testlib qcp02 solve fast
       * should also be good in general (any minlp), but experiments during SCIP dev were not conclusive
       */
      SCIP_CALL( SCIPsetCharParam(scip, "constraints/nonlinear/linearizeheursol", 'i') );
   }

   /* don't print reason why start solution is infeasible, per default */
   SCIP_CALL( SCIPsetBoolParam(scip, "misc/printreason", FALSE) );

   if( SCIPfindNlpi(scip, "ipopt") != NULL )
   {
      if( readerdata->ipoptlicensed )
      {
         SCIP_CALL( SCIPsetStringParam(scip, "nlpi/ipopt/linear_solver", "ma27") );
         SCIP_CALL( SCIPsetStringParam(scip, "nlpi/ipopt/linear_system_scaling", "mc19") );
      }
      else
      {
         SCIP_CALL( SCIPsetStringParam(scip, "nlpi/ipopt/linear_solver", "mumps") );
      }
   }

   if( gmoOptFile(gmo) > 0 )
   {
      char optfilename[1024];
      SCIP_RETCODE ret;

      (void) gmoNameOptFile(gmo, optfilename);
      SCIPinfoMessage(scip, NULL, "\nreading option file %s\n", optfilename);
      ret = SCIPreadParams(scip, optfilename);
      if( ret != SCIP_OKAY )
      {
         SCIPwarningMessage(scip, "Reading of optionfile %s failed with SCIP return code <%d>!\n", optfilename, ret);
      }
   }

   return SCIP_OKAY;
}
