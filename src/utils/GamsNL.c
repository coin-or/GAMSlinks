// Copyright (C) GAMS Development and others 2022
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "GamsNL.h"

#include "gmomcc.h"

RETURN gamsnlCreate(
   gamsnl_node**   n,
   gamsnl_opcode   op
   )
{
   assert(n != NULL);

   *n = (gamsnl_node*) calloc(1, sizeof(gamsnl_node));

   (*n)->op = op;
   switch( (*n)->op )
   {
      case gamsnl_opvar :
         (*n)->coef = 1.0;
         (*n)->argssize = 0;
         break;
      case gamsnl_opconst :
         (*n)->argssize = 0;
         break;
      case gamsnl_opsum :
      case gamsnl_opprod :
      case gamsnl_opmin :
      case gamsnl_opmax :
         (*n)->argssize = 5;
         break;
      case gamsnl_opsub:
      case gamsnl_opdiv:
         (*n)->argssize = 2;
         break;
      case gamsnl_opnegate:
      default :
         (*n)->argssize = 1;
         break;
   }

   if( (*n)->argssize > 0 )
      (*n)->args = (gamsnl_node**) malloc((*n)->argssize * sizeof(gamsnl_node*));

   return RETURN_OK;
}

RETURN gamsnlCreateVar(
   gamsnl_node**  n,
   int            varidx
   )
{
   CHECK( gamsnlCreate(n, gamsnl_opvar) );
   (*n)->varidx = varidx;

   return RETURN_OK;
}

void gamsnlFree(
   gamsnl_node**  n
   )
{
   int i;

   assert(*n != NULL);

   for( i = 0; i < (*n)->nargs; ++i )
      gamsnlFree(&(*n)->args[i]);

   free((*n)->args);
   free(*n);
   *n = NULL;
}

RETURN gamsnlCopy(
   gamsnl_node**  target,
   gamsnl_node*   src
   )
{
   int i;

   assert(target != NULL);
   assert(src != NULL);

   *target = (gamsnl_node*) malloc(sizeof(gamsnl_node));
   **target = *src;

   if( src->nargs == 0 )
   {
      (*target)->args = NULL;
      (*target)->argssize = 0;
      return RETURN_OK;
   }

   (*target)->args = (gamsnl_node**) malloc((*target)->argssize * sizeof(gamsnl_node*));
   for( i = 0; i < src->nargs; ++i )
      gamsnlCopy(&(*target)->args[i], src->args[i]);

   return RETURN_OK;
}

/* should be used for sum, product, min, and max only */
RETURN gamsnlAddArg(
   gamsnl_node*   n,
   gamsnl_node*   arg
   )
{
   assert(n != NULL);
   assert(n->op == gamsnl_opsum || n->op == gamsnl_opprod || n->op == gamsnl_opmin || n->op == gamsnl_opmax);
   assert(arg != NULL);

   if( n->argssize <= n->nargs + 1 )
   {
      n->argssize = 2 * (n->nargs + 1);
      n->args = (gamsnl_node**) realloc(n->args, n->argssize * sizeof(gamsnl_node*));
   }
   assert(n->argssize > n->nargs);

   n->args[n->nargs++] = arg;

   return RETURN_OK;
}

/* should be used for sum, product, min, and max only */
RETURN gamsnlAddArgFront(
   gamsnl_node*   n,
   gamsnl_node*   arg
   )
{
   int i;

   assert(n != NULL);
   assert(n->op == gamsnl_opsum || n->op == gamsnl_opprod || n->op == gamsnl_opmin || n->op == gamsnl_opmax);
   assert(arg != NULL);

   if( n->argssize <= n->nargs + 1 )
   {
      n->argssize = 2 * (n->nargs + 1);
      n->args = (gamsnl_node**) realloc(n->args, n->argssize * sizeof(gamsnl_node*));
   }
   assert(n->argssize > n->nargs);

   /* move all existing args one position to the right, then add new one at front */
   for( i = n->nargs; i > 0; --i )
      n->args[i] = n->args[i-1];
   n->args[0] = arg;
   ++n->nargs;

   return RETURN_OK;
}

void gamsnlPrint(
   gamsnl_node*   n
   )
{
   switch( n->op )
   {
      case gamsnl_opvar :
         printf("x%d", n->varidx);
         break;

      case gamsnl_opconst :
         printf("%g", n->coef);
         break;

      case gamsnl_opsum :
         printf("sum");
         break;

      case gamsnl_opprod :
         printf("prod");
         break;

      case gamsnl_opmin :
         printf("min");
         break;

      case gamsnl_opmax :
         printf("max");
         break;

      case gamsnl_opsub :
         printf("sub");
         break;

      case gamsnl_opdiv :
         printf("div");
         break;

      case gamsnl_opnegate :
         printf("-");
         break;

      case gamsnl_opfunc :
         printf(GamsFuncCodeName[n->func]);
         break;
   }

   if( n->nargs > 0 )
   {
      int i;

      printf("(");
      for( i = 0; i < n->nargs; ++i )
      {
         if( i > 0 )
            printf(", ");
         gamsnlPrint(n->args[i]);
      }

      printf(")");
   }
}

static
RETURN nlnodeApplyUnaryOperation(
   gamsnl_node**  stack,
   int*           stackpos,
   gamsnl_opcode  op,
   gamsnl_mode    mode
)
{
   gamsnl_node* n;

   n = stack[*stackpos];
   if( op == gamsnl_opnegate )
   {
      if( n->op == gamsnl_opnegate )
      {
         /* -(-) = + */
         stack[*stackpos] = n->args[0];
         n->nargs = 0;
         gamsnlFree(&n);
         return RETURN_OK;
      }
      if( n->op == gamsnl_opconst )
      {
         n->coef *= -1.0;
         return RETURN_OK;
      }
      if( n->op == gamsnl_opvar && mode == gamsnl_osil )
      {
         n->coef *= -1.0;
         return RETURN_OK;
      }
      if( n->op == gamsnl_opprod )
      {
         /* see whether we can find a constant or variable (if osil) with coef that we can negate */
         int i;
         int varcoef1 = -1; /* a variable with coef 1 */
         for( i = 0; i < n->nargs; ++i )
         {
            if( n->args[i]->op == gamsnl_opconst )
            {
               n->args[i]->coef *= -1.0;
               return RETURN_OK;
            }
            if( n->args[i]->op == gamsnl_opvar && mode == gamsnl_osil )
            {
               if( n->args[i]->coef != 1.0 )
               {
                  n->args[i]->coef *= -1.0;
                  return RETURN_OK;
               }
               varcoef1 = i;  /* remember for later, in case we don't find another good node */
            }
         }
         if( varcoef1 >= 0 )
         {
            n->args[varcoef1]->coef = -1.0;
            return RETURN_OK;
         }
      }
   }

   CHECK( gamsnlCreate(&stack[*stackpos], op) );
   assert(stack[*stackpos]->argssize >= 1);
   stack[*stackpos]->args[0] = n;
   stack[*stackpos]->nargs = 1;

   return RETURN_OK;
}

static
RETURN nlnodeApplyBinaryOperation(
   gamsnl_node**  stack,
   int*      stackpos,
   gamsnl_opcode  op,
   gamsnl_mode    mode
)
{
   gamsnl_node* n;
   assert(*stackpos >= 1);

   if( op == gamsnl_opsub )
   {
      /* rewrite as A + (-B) as we can have many children in a sum and we can sometimes reform a negation away */
      CHECK( nlnodeApplyUnaryOperation(stack, stackpos, gamsnl_opnegate, mode) );
      CHECK( nlnodeApplyBinaryOperation(stack, stackpos, gamsnl_opsum, mode) );
      return RETURN_OK;
   }

   if( mode == gamsnl_osil )
   {
      if( op == gamsnl_opprod && stack[*stackpos]->op == gamsnl_opconst && stack[*stackpos-1]->op == gamsnl_opvar )
      {
         /* constant * variable -> store constant as coef in variable */
         stack[*stackpos-1]->coef *= stack[*stackpos]->coef;
         gamsnlFree(&stack[(*stackpos)--]);

         return RETURN_OK;
      }

      if( op == gamsnl_opprod && stack[*stackpos]->op == gamsnl_opvar && stack[*stackpos-1]->op == gamsnl_opconst )
      {
         /* variable * constant -> store constant as coef in variable */
         stack[*stackpos]->coef *= stack[*stackpos-1]->coef;
         gamsnlFree(&stack[*stackpos-1]);
         stack[*stackpos-1] = stack[*stackpos];
         --*stackpos;

         return RETURN_OK;
      }
   }

   /* if operator can take arbitrarily many operands, then merge children of children of same operator into new one
    * e.g., (a+b) + c = a + b + c
    * if ampl, then prod can take only two operands
    */
   if( op == gamsnl_opsum || (op == gamsnl_opprod && mode != gamsnl_ampl) || op == gamsnl_opmin || op == gamsnl_opmax )
   {
      if( stack[*stackpos-1]->op == op )
      {
         CHECK( gamsnlAddArg(stack[*stackpos-1], stack[*stackpos]) );
         --*stackpos;
         return RETURN_OK;
      }
      if( stack[*stackpos]->op == op )
      {
         CHECK( gamsnlAddArgFront(stack[*stackpos], stack[*stackpos-1]) );
         stack[*stackpos-1] = stack[*stackpos];
         --*stackpos;
         return RETURN_OK;
      }
   }

   CHECK( gamsnlCreate(&n, op) );
   if( n->argssize < 2 )
   {
      n->args = (gamsnl_node**) realloc(n->args, 2 * sizeof(gamsnl_node*));
      n->argssize = 2;
   }

   n->args[1] = stack[(*stackpos)--];
   n->args[0] = stack[(*stackpos)--];
   n->nargs = 2;
   stack[++(*stackpos)] = n;

   return RETURN_OK;
}

RETURN gamsnlParseGamsInstructions(
   gamsnl_node**   nl,                 /**< buffer to store created root node */
   struct gmoRec*  gmo,                /**< GMO */
   int             codelen,            /**< length of GAMS instructions */
   int*            opcodes,            /**< opcodes of GAMS instructions */
   int*            fields,             /**< fields of GAMS instructions */
   double*         constants,          /**< GAMS constants pool */
   double          factor,             /**< extra factor to multiply expression with */
   double          constant,           /**< extra constrant to add to expression (after applying factor) */
   gamsnl_mode     mode                /**< for which purpose the nl is created */
)
{
   gamsnl_node** stack;
   int stacksize;
   int stackpos = -1;
   int nargs = -1;
   int address;
   int i;

   stacksize = 20;
   stack = (gamsnl_node**) malloc(stacksize * sizeof(gamsnl_node*));

   for( i = 0; i < codelen; ++i )
   {
      GamsOpCode opcode = (GamsOpCode)opcodes[i];
      address = fields[i]-1;

      if( 0 )  /* debugging */
      {
         int j = 0;
         for( j = /* stackpos > 3 ? stackpos-3 :*/ 0; j <= stackpos; ++j )
         {
            printf("%2d: ", j);
            gamsnlPrint(stack[j]);
            printf("\n");
         }
         printf("apply %s (field %d)\n", GamsOpCodeName[opcode], address+1);
      }

      if( stackpos > stacksize-3 )
      {
         stacksize *= 2;
         stack = (gamsnl_node**) realloc(stack, stacksize * sizeof(gamsnl_node*));
      }

      switch( opcode )
      {
         case nlNoOp:   /* no operation */
         case nlStore:  /* store row */
         case nlHeader: /* header */
            break;

         case nlPushV: /* push variable */
         {
            CHECK( gamsnlCreateVar(&stack[++stackpos], gmoGetjSolver(gmo, address)) );
            break;
         }

         case nlPushI: /* push constant */
         {
            CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
            stack[stackpos]->coef = constants[address];
            break;
         }

         case nlPushZero: /* push zero */
         {
            CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
            stack[stackpos]->coef = 0.0;
            break;
         }

         case nlAdd: /* add */
         {
            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );
            break;
         }

         case nlAddV: /* add variable */
         {
            CHECK( gamsnlCreateVar(&stack[++stackpos], gmoGetjSolver(gmo, address)) );

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );
            break;
         }

         case nlAddI: /* add immediate */
         {
            CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
            stack[stackpos]->coef = constants[address];

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );
            break;
         }

         case nlSub: /* minus */
         {
            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsub, mode) );
            break;
         }

         case nlSubV: /* subtract variable */
         {
            CHECK( gamsnlCreateVar(&stack[++stackpos], gmoGetjSolver(gmo, address)) );

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsub, mode) );
            break;
         }

         case nlSubI: /* subtract immediate */
         {
            CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
            stack[stackpos]->coef = constants[address];

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsub, mode) );
            break;
         }

         case nlMul: /* multiply */
         {
            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );
            break;
         }

         case nlMulV: /* multiply variable */
         {
            CHECK( gamsnlCreateVar(&stack[++stackpos], gmoGetjSolver(gmo, address)) );

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );
            break;
         }

         case nlMulI: /* multiply immediate */
         {
            CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
            stack[stackpos]->coef = constants[address];

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );
            break;
         }

         case nlMulIAdd: /* multiply immediate and add */
         {
            CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
            stack[stackpos]->coef = constants[address];

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );
            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );
            break;
         }

         case nlDiv: /* divide */
         {
            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opdiv, mode) );
            break;
         }

         case nlDivV: /* divide variable */
         {
            CHECK( gamsnlCreateVar(&stack[++stackpos], gmoGetjSolver(gmo, address)) );

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opdiv, mode) );
            break;
         }

         case nlDivI: /* divide immediate */
         {
            CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
            stack[stackpos]->coef = constants[address];

            CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opdiv, mode) );
            break;
         }

         case nlUMin: /* unary minus */
         {
            CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opnegate, mode) );
            break;
         }

         case nlUMinV: /* unary minus variable */
         {
            CHECK( gamsnlCreateVar(&stack[++stackpos], gmoGetjSolver(gmo, address)) );
            CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opnegate, mode) );
            break;
         }

         case nlFuncArgN: /* number of function arguments */
         {
            nargs = address + 1; /* undo shift by 1 */
            break;
         }

         case nlCallArg1 :
         case nlCallArg2 :
         case nlCallArgN :
         {
            GamsFuncCode funccode = (GamsFuncCode)(address+1);  /* undo shift by 1 */
            switch( funccode )
            {
               case fnmin:
               {
                  assert(opcode == nlCallArg2);
                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opmin, mode) );
                  break;
               }

               case fnmax :
               {
                  assert(opcode == nlCallArg2);
                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opmax, mode) );
                  break;
               }

               case fnlog2:
               {
                  if( mode == gamsnl_osil )
                  {
                     /* use OSnL log(base,arg) for log2, thus add base=2 as argument first */
                     CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
                     stack[stackpos]->coef = 2.0;

                     CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opfunc, mode) );
                     stack[stackpos]->func = fnlog2;  /* use fnlog2 to signal OSnL log(,) */
                  }
                  else
                  {
                     CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opfunc, mode) );
                     stack[stackpos]->func = fnlog2;
                  }
                  break;
               }

               case fnerrf :
               {
                  /* errorf = 0.5 * [1+erf(x/sqrt(2))] */
                  CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
                  stack[stackpos]->coef = sqrt(2.0) / 2.0;

                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );

                  CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opfunc, mode) );
                  stack[stackpos]->func = fnerrf;  /* use fnerrf to signal OSnL erf */

                  CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
                  stack[stackpos]->coef = 1.0;

                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );

                  CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
                  stack[stackpos]->coef = 0.5;

                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );

                  break;
               }

               case fncentropy:
               {
                  /* x*ln((x+Z)/(y+Z)), with Z optional, default = 1e-20 */
                  gamsnl_node* x;
                  gamsnl_node* y;
                  double z = 1e-20;

                  x = stack[stackpos--];
                  y = stack[stackpos--];
                  if( opcode == nlCallArgN )
                  {
                     assert(nargs == 3);
                     assert(stack[stackpos]->op == gamsnl_opconst);
                     z = stack[stackpos]->coef;
                     gamsnlFree(&stack[stackpos--]);
                  }

                  stack[++stackpos] = y;
                  CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
                  stack[stackpos]->coef = z;
                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );

                  CHECK( gamsnlCopy(&stack[++stackpos], x) );
                  CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
                  stack[stackpos]->coef = z;
                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );

                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opdiv, mode) );

                  CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opfunc, mode) );
                  stack[stackpos]->func = fnlog;

                  stack[++stackpos] = x;
                  CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );

                  break;
               }

               case fnedist: /* euclidian distance */
               {
                  gamsnl_node* edist;
                  gamsnl_node* m;
                  int j;

                  if( opcode == nlCallArg1 )
                     nargs = 1;
                  else if( opcode == nlCallArg2 )
                     nargs = 2;
                  assert(nargs >= 1);

                  if( nargs == 1 )
                  {
                     /* sqrt(x1^2) = abs(x1) */
                     CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opfunc, mode) );
                     stack[stackpos]->func = fnabs;
                     break;
                  }

                  CHECK( gamsnlCreate(&edist, gamsnl_opfunc) );
                  edist->func = fnsqrt;

                  CHECK( gamsnlCreate(&edist->args[0], gamsnl_opsum) );
                  edist->nargs = 1;

                  for( j = 0; j < nargs; ++j )
                  {
                     CHECK( gamsnlCreate(&m, gamsnl_opfunc) );
                     m->func = fnsqr;
                     m->args[0] = stack[stackpos--];
                     m->nargs = 1;

                     CHECK( gamsnlAddArg(edist->args[0], m) );
                  }

                  stack[++stackpos] = edist;

                  break;
               }

               case fnarctan2: /* arctan(x1/x2) */
               {
                  if( mode == gamsnl_osil )
                  {
                     CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opdiv, mode) );
                     CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opfunc, mode) );
                     stack[stackpos]->func = fnarctan;
                  }
                  else
                  {
                     CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opfunc, mode) );
                     stack[stackpos]->func = fnarctan2;
                  }

                  break;
               }

               case fnpoly :
               {
                  gamsnl_node* x;
                  gamsnl_node* poly;
                  gamsnl_node* term;
                  gamsnl_node* pow;
                  gamsnl_node* ministack[3];
                  int ministackpos;
                  int j;

                  assert(opcode == nlCallArgN);
                  assert(nargs >= 4);

                  /* variable of polynomial is at bottom */
                  x = stack[stackpos-nargs+1];

                  CHECK( gamsnlCreate(&poly, gamsnl_opsum) );

                  /* constant term */
                  CHECK( gamsnlAddArg(poly, stack[stackpos-nargs+2]) );

                  /* linear term */
                  ministack[0] = stack[stackpos-nargs+3];
                  CHECK( gamsnlCopy(&ministack[1], x) );
                  ministackpos = 1;
                  CHECK( nlnodeApplyBinaryOperation(ministack, &ministackpos, gamsnl_opprod, mode) );
                  CHECK( gamsnlAddArg(poly, ministack[ministackpos]) );

                  /* other terms */
                  for( j = 2; j < nargs-1; ++j )
                  {
                     CHECK( gamsnlCreate(&pow, gamsnl_opfunc) );
                     if( j == 2 )
                     {
                        /* square term: use original x (do not copy) */
                        pow->func = fnsqr;
                        pow->args[0] = x;
                        pow->nargs = 1;
                     }
                     else
                     {
                        pow->func = fnvcpower;
                        pow->args = (gamsnl_node**) realloc(pow->args, 2 * sizeof(gamsnl_node*));
                        pow->argssize = 2;
                        CHECK( gamsnlCopy(&pow->args[0], x) );
                        CHECK( gamsnlCreate(&pow->args[1], gamsnl_opconst) );
                        pow->args[1]->coef = j;
                        pow->nargs = 2;
                     }

                     CHECK( gamsnlCreate(&term, gamsnl_opprod) );
                     term->args[0] = stack[stackpos-nargs+2+j];
                     term->args[1] = pow;
                     term->nargs = 2;

                     CHECK( gamsnlAddArg(poly, term) );
                  }

                  stackpos -= nargs;
                  stack[++stackpos] = poly;
                  break;
               }

               default:
               {
                  gamsnl_node* n;
                  int j;

                  if( opcode == nlCallArg1 )
                     nargs = 1;
                  else if( opcode == nlCallArg2 )
                     nargs = 2;

                  CHECK( gamsnlCreate(&n, gamsnl_opfunc) );
                  n->func = funccode;

                  if( nargs > n->argssize )
                  {
                     n->args = (gamsnl_node**) realloc(n->args, nargs * sizeof(gamsnl_node*));
                     n->argssize = nargs;
                  }

                  for( j = nargs-1; j >= 0; --j )
                     n->args[j] = stack[stackpos--];
                  n->nargs = nargs;

                  stack[++stackpos] = n;
                  break;
               }
            }
            break;
         }

         default:
         {
            fprintf(stderr, "Error: Unsupported GAMS opcode %s.\n", GamsOpCodeName[opcode]);
            while( stackpos >= 0 )
               gamsnlFree(&stack[stackpos--]);

            return RETURN_ERROR;
         }
      }
   }

   assert(stackpos == 0);

   if( factor == -1.0 )
   {
      CHECK( nlnodeApplyUnaryOperation(stack, &stackpos, gamsnl_opnegate, mode) );
   }
   else if( factor != 1.0 )
   {
      if( stack[0]->op == gamsnl_opconst )
      {
         stack[0]->coef *= factor;
      }
      else
      {
         CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
         stack[stackpos]->coef = factor;

         CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opprod, mode) );
         assert(stackpos == 0);
      }
   }

   if( constant != 0.0 )
   {
      if( stack[0]->op == gamsnl_opconst )
      {
         stack[0]->coef += constant;
      }
      else
      {
         CHECK( gamsnlCreate(&stack[++stackpos], gamsnl_opconst) );
         stack[stackpos]->coef = constant;

         CHECK( nlnodeApplyBinaryOperation(stack, &stackpos, gamsnl_opsum, mode) );
         assert(stackpos == 0);
      }
   }

   *nl = stack[0];
   /* gamsnlPrint(*nl); printf("\n"); */

   free(stack);

   return RETURN_OK;
}
