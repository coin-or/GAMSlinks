// Copyright (C) GAMS Development and others 2022
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "convert_nl.h"
#include "gamsnl.h"

#include "gmomcc.h"

#define AMPLINFTY    1.0e50

#ifdef CONVERTNL_WITH_ASL

/* ASL includes */
#include "nlp.h"
/* #include "getstub.h" */
#include "opcode.hd"
#include "asl.h"
#undef filename

/** prints a double precision floating point number to a string without loosing precision
 *
 * uses DTOA from ASL;  see also https://ampl.com/REFS/rounding.pdf
 * NOTE that DTOA is not threadsafe (?)
 * this routine is adapted from os_dtoa_format of OptimizationServices/OSMathUtil.cpp
 *
 * @return buf
 */
char* convertDoubleToString(
   char*       buf,     /**< string buffer that should be long enough to hold the result */
   double      val      /**< value to convert to string */
)
{
   char* bufstart;
   char s[100];
   int slen;
   int decimalPointPos;
   int sign;

   assert(buf != NULL);

   bufstart = buf;

   *buf = '\0';
   if( dtoa_r(val, 0, 0, &decimalPointPos, &sign, NULL, s, sizeof(s)) == NULL )
   {
      fprintf(stderr, "DTOA failed for value %g\n", val);
      abort();
   }
   slen = strlen(s);

   /* print the sign if negative */
   if( sign == 1 )
      *(buf++) = '-';

   /* take s without change if we have nan or infinity or are integral */
   if( decimalPointPos == 9999 || decimalPointPos == slen )
   {
      memcpy(buf, s, slen * sizeof(char));
      buf += slen;
   }
   else if( decimalPointPos > slen )
   {
      if( slen == 1 )
      {
         *(buf++) = s[0];
         if( decimalPointPos <= 3 )
         {
            memset(buf, '0', decimalPointPos-1);  /* -1 because we s[0] counted as first position */
            buf += decimalPointPos-1;
         }
         else
         {
            *(buf++) = 'e';
            buf += sprintf(buf, "%d", decimalPointPos-1);
         }
      }
      else
      {
         /* use exponential format */
         *(buf++) = s[0];
         *(buf++) = '.';
         memcpy(buf, s+1, (slen-1) * sizeof(char));
         buf += slen-1;
         *(buf++) = 'e';
         buf += sprintf(buf, "%d", decimalPointPos-1);
      }
   }
   else if( decimalPointPos >= 0 )
   {
      /* print in float format (%f) */
      memcpy(buf, s, decimalPointPos);
      buf += decimalPointPos;
      *(buf++) = '.';
      memcpy(buf, s+decimalPointPos, slen-decimalPointPos);
      buf += slen-decimalPointPos;
   }
   else
   {
      assert(fabs(val) < 1);
      /* print in exponential format (%e) */
      *(buf++) = s[0];
      if( slen > 1 )
      {
         *(buf++) = '.';
         memcpy(buf, s+1, slen-1);
         buf += slen-1;
      }
      *(buf++) = 'e';
      buf += sprintf(buf, "%d", decimalPointPos-1);
   }

   *buf = '\0';

   return bufstart;
}
#endif

static
RETURN writeNLPrintf(
   convertWriteNLopts writeopts,
   const char*        fmt,
   ...
   )
{
   va_list ap;

   assert(writeopts.f != NULL);
   assert(fmt != NULL);

   va_start(ap, fmt);

   if( !writeopts.binary )
   {
#ifdef CONVERTNL_WITH_ASL
      /* vfprintf(writeopts.f, fmt, ap); */
      /* the following code is adapted from ASL's fg_write.c:aprintf() */
      for( ;; )
      {
         int nspace = 0;
         for( ;; )
         {
            char c;
            switch( c = *fmt++ )
            {
               case '\0':
                  /* end of format */
                  goto TERMINATE;
               case '%':
                  /* format specifier: handle below */
                  break;
               case ' ':
                  /* do not print space yet, but only count */
                  ++nspace;
                  continue;
               case '#':
                  if( !writeopts.comments )
                  {
                     /* skip until end of comment */
                     while( *fmt != '\n' && *fmt != '\0' )
                        ++fmt;
                     continue;
                  }
                  /* fall through */
                  /* no break */
               default:
                  /* any other character: write to f, but first print whitespace
                   * but if at end of line, then forget about whitespace
                   */
                  if( c == '\n' )
                     nspace = 0;
                  for( ; nspace > 0; --nspace )
                     putc(' ', writeopts.f);
                  putc(c, writeopts.f);
                  continue;
            }
            break;
         }
         for( ; nspace > 0; --nspace )
            putc(' ', writeopts.f);

         /* handle format specifier */
         switch( *fmt++ )
         {
            case 'c':
               putc((char)va_arg(ap, int), writeopts.f);
               continue;

            case 'd':
               fprintf(writeopts.f, "%d", va_arg(ap, int));
               continue;

            case '.':
               /* ignore precision specification, continue in 'g' case */
               while( *fmt++ != 'g' );
               /* fall through */
               /* no break */
            case 'g':
            {
               char buf[100];
               double v;

               v = va_arg(ap, double);
               if( writeopts.shortfloat )
               {
                  /* use dtoa to get a good string that is short, too */
                  convertDoubleToString(buf, v);
                  fputs(buf, writeopts.f);
               }
               else
               {
                  /* try to write the same way how AMPL writes .nl text files:
                   * use ASL g_fmt (which calls dtoa as well), unless we have an int with many zeros:
                   * AMPL somehow prints D00...00 in scientific notation if D is only one digit and at least 5 zeros
                   * even though g_fmt() doesn't do that
                   */
                  if( fabs(v) >= 1e5 && v == (int)v )
                  {
                     int zeros = (int)log10(fabs(v));
                     int firstdigit = (int)v / (int)pow(10, zeros);
                     /* printf("val %g, firstdigit %d, zeros %d; %g\n", val, firstdigit, zeros, firstdigit * pow(10, zeros)); */
                     if( v == firstdigit * pow(10, zeros) )
                     {
                        fprintf(writeopts.f, "%.2g", v);
                        continue;
                     }
                  }

                  g_fmt(buf, v);
                  fputs(buf, writeopts.f);
               }
               continue;
            }

            case 's':
            {
               fputs(va_arg(ap, char*), writeopts.f);
               continue;
            }

            default:
               fprintf(stderr, "writeNLPrintf bug: unexpected format %s\n", fmt-1);
               return RETURN_ERROR;
         }
      }
#else
      fprintf(stderr, "writeNLPrintf: text output not available without ASL\n");
      return RETURN_ERROR;
#endif
   }
   else
   {
      /* the following code is adapted from ASL's fg_write.c:bprintf() */

      /* if fmt starts with a character, then we print this, it seems
       * for any other characters, we expect format strings
       */
      if( *fmt != '%' && *fmt != '#' )
      {
         /* printf("%c", *fmt); */
         fwrite(fmt++, 1, 1, writeopts.f);
      }

      for( ;; )
      {
         union { double x; short sh; long L; int i; char c; } u;
         size_t len = 0;

         /* skip comments */
         if( *fmt == '#' )
         {
            while( *fmt != '\n' && *fmt != '\0' )
               ++fmt;
            continue;
         }

         /* skip whitespace */
         if( *fmt == ' ' || *fmt == '\n' )
         {
            /* if( *fmt == '\n' )
               printf("\n"); */
            fmt++;
            continue;
         }

         if( *fmt == '\0' )
            break;

         /* while the first character in fmt can be anything, afterwards there should only be whitespace, comments, and formatters */
         if( *fmt++ != '%' )
         {
            fprintf(stderr, "writeNLPrintf bug: unexpected '%s'\n", fmt-1);
            return RETURN_ERROR;
         }

         switch(*fmt++)
         {
            case 'c':
               u.c = (char)va_arg(ap, int);
               len = 1;
               /* printf(" %c", u.c); */
               break;

            case 'd':
               u.i = va_arg(ap, int);
               len = sizeof(int);
               /* printf(" %d", u.i); */
               break;

            case '.':
               /* ignore precision specification, continue in 'g' case */
               while(*fmt++ != 'g');
               /* fall through */
               /* no break */
            case 'g':
               u.x = va_arg(ap, double);
               len = sizeof(double);
               /* printf(" %g", u.x); */
               break;

            case 'h':
               u.sh = (short)va_arg(ap, int);
               len = sizeof(short);
               if( *fmt == 'd' )
                  fmt++;
               /* printf(" %hd", u.sh); */
               break;

            case 'l':
               u.L = (long)va_arg(ap, long);
               len = sizeof(long);
               if( *fmt == 'd' )
                  fmt++;
               /* printf(" %ld", u.L); */
               break;

            case 's':
            {
               char* s;
               s = va_arg(ap, char*);
               u.i = strlen(s);
               fwrite((void*)&u.i, sizeof(int), 1, writeopts.f);
               fwrite(s, 1, u.i, writeopts.f);
               /* printf(" %s", s); */
               break;
            }

            default:
               fprintf(stderr, "writeNLPrintf bug: unexpected format %s\n", fmt-1);
               return RETURN_ERROR;
         }
         if( len > 0 )
            fwrite((void*)&u.L, len, 1, writeopts.f);
      }
   }

 TERMINATE:
   va_end(ap);

   return RETURN_OK;
}

/* variable types by which variables need to be ordered for .nl
 * (names are taken from pyomo nl writer)
 */
typedef enum
{
   Nonlinear_Vars_in_Objs_and_Constraints = 0,
   Discrete_Nonlinear_Vars_in_Objs_and_Constraints = 1,
   ConNonlinearVars = 2,     /* only in cons */
   ConNonlinearVarsInt = 3,  /* only in cons */
   ObjNonlinearVars = 4,     /* only in obj */
   ObjNonlinearVarsInt = 5,  /* only in obj */
   LinearVars = 6,
   LinearVarsBool = 7,
   LinearVarsInt = 8
} NlVarType;

/** write NL header and construct variable and equation permutations */
static
RETURN writeNLHeader(
   struct gmoRec*      gmo,
   convertWriteNLopts  writeopts,
   int**               varperm,
   int**               equperm
)
{
   int nlvars_cons = 0;
   int nlvars_obj = 0;
   int nlvars_both = 0;
   int binvars_lin = 0;
   int intvars_lin = 0;
   int discrvars_nlcons = 0;  /* in constraints and not objective */
   int discrvars_nlobj = 0;   /* in objective and not constraints */
   int discrvars_nlboth = 0;  /* in both objective and constraints */
   int maxnamelen_cons = 0;
   int maxnamelen_vars = 0;
   char buf[GMS_SSSIZE];
   int i;
   int type;
   int nvar;
   int nequ;
   NlVarType* vartype;

   assert(gmo != NULL);
   assert(varperm != NULL);

   vartype = (NlVarType*)malloc(gmoN(gmo) * sizeof(NlVarType));

   /* collect statistics on variables; determine variable types */
   for( i = 0; i < gmoN(gmo); ++i )
   {
      int nz, qnz, nlnz, objnz;
      int isdiscrete;
      int namelen;

      /* objnz = -1 if linear +1 if non-linear 0 otherwise */
      gmoGetColStat(gmo, i, &nz, &qnz, &nlnz, &objnz);

      isdiscrete = gmoGetVarTypeOne(gmo, i) == gmovar_B || gmoGetVarTypeOne(gmo, i) == gmovar_I;

      /* this is how Pyomo counts vars (nlvars_* = nlvb,c,o) when writing NL
       * https://github.com/Pyomo/pyomo/blob/main/pyomo/repn/plugins/ampl/ampl_.py#L1202
       * this, together the ominous line below, seems to correspond to what AMPL writes
       */
      if( nlnz > 0 && objnz == 1 )
      {
         /* nonlinear in constraints and objective */
         ++nlvars_both;
         ++nlvars_cons;
         ++nlvars_obj;
         if( isdiscrete )
         {
            ++discrvars_nlboth;
            vartype[i] = Discrete_Nonlinear_Vars_in_Objs_and_Constraints;
         }
         else
            vartype[i] = Nonlinear_Vars_in_Objs_and_Constraints;
      }
      else if( nlnz > 0 )
      {
         /* nonlinear in constraints only */
         ++nlvars_cons;
         ++nlvars_obj;
         if( isdiscrete )
         {
            ++discrvars_nlcons;
            vartype[i] = ConNonlinearVarsInt;
         }
         else
            vartype[i] = ConNonlinearVars;
      }
      else if( objnz == 1 )
      {
         /* nonlinear in objective only */
         ++nlvars_obj;
         if( isdiscrete )
         {
            ++discrvars_nlobj;
            vartype[i] = ObjNonlinearVarsInt;
         }
         else
            vartype[i] = ObjNonlinearVars;
      }
      else
      {
         /* linear */
         if( isdiscrete )
         {
            /* for compatibility with AMPL generated nl files, count integer with 0/1 bounds as binary, too */
            if( gmoGetVarLowerOne(gmo, i) >= 0.0 && gmoGetVarUpperOne(gmo, i) <= 1.0 )
            {
               ++binvars_lin;
               vartype[i] = LinearVarsBool;
            }
            else
            {
               ++intvars_lin;
               vartype[i] = LinearVarsInt;
            }
         }
         else
            vartype[i] = LinearVars;
      }

      if( gmoDict(gmo) != NULL )
      {
         namelen = strlen(gmoGetVarNameOne(gmo, i, buf));
         if( namelen > maxnamelen_vars )
            maxnamelen_vars = namelen;
      }
   }

   /* setup var permutation */
   *varperm = (int*)malloc(gmoN(gmo) * sizeof(int));
   nvar = 0;
   for( type = Nonlinear_Vars_in_Objs_and_Constraints; type <= LinearVarsInt; ++type )
   {
      for( i = 0; i < gmoN(gmo); ++i )
         if( vartype[i] == (NlVarType)type )
         {
            assert(nvar < gmoN(gmo));
            (*varperm)[nvar++] = i;
            /* puts(gmoGetVarNameOne(gmo, i, buf)); */
         }
   }
   assert(nvar == gmoN(gmo));

   free(vartype);

   maxnamelen_cons = 0;
   if( gmoModelType(gmo) != gmoProc_cns )
      maxnamelen_cons = 3;  /* objective name "obj" */

   /* collect statistics on constraint names */
   if( gmoDict(gmo) != NULL )
   {
      for( i = 0; i < gmoM(gmo); ++i )
      {
         int namelen;

         namelen = strlen(gmoGetEquNameOne(gmo, i, buf));
         if( namelen > maxnamelen_cons )
            maxnamelen_cons = namelen;
      }
   }

   /* setup equ permutation: nonlinear before linear */
   *equperm = (int*)malloc(gmoM(gmo) * sizeof(int));
   nequ = 0;
   for( i = 0; i < gmoM(gmo); ++i )
      if( gmoGetEquOrderOne(gmo, i) != gmoorder_L )
      {
         assert(nequ < gmoM(gmo));
         (*equperm)[nequ++] = i;
      }
   for( i = 0; i < gmoM(gmo); ++i )
      if( gmoGetEquOrderOne(gmo, i) == gmoorder_L )
      {
         assert(nequ < gmoM(gmo));
         (*equperm)[nequ++] = i;
      }
   assert(nequ == gmoM(gmo));

   /* write NL header: this is always text, and we always write comments here */

   gmoNameInput(gmo, buf);
   if( writeopts.binary )
      fprintf(writeopts.f, "b3 0 1 0\t# problem %s\n", buf);
   else
      fprintf(writeopts.f, "g3 0 1 0\t# problem %s\n", buf);

   fprintf(writeopts.f, " %d %d %d 0 %d\t# vars, constraints, objectives, ranges, eqns\n",
      gmoN(gmo),
      gmoM(gmo),
      gmoModelType(gmo) == gmoProc_cns ? 0 : 1,
      gmoGetEquTypeCnt(gmo, gmoequ_E)
   );

   fprintf(writeopts.f, " %d %d\t# nonlinear constraints, objectives\n",
      gmoNLM(gmo),
      gmoGetObjOrder(gmo) == gmoorder_NL ? 1 : 0);

   fprintf(writeopts.f, " 0 0\t# network constraints: nonlinear, linear\n");

   /* Pyomo has this ominous line:
      if (idx_nl_obj == idx_nl_con):
         idx_nl_obj = idx_nl_both
      that is: if there are no variables that are nonlinear in objective only,
        then set nlvars_obj to the number of nonlinear variables in both objective and constraints
        otherwise, it is set to the total number of nonlinear variables (i.e., nonlin in objective or constraints)
    */
   if( nlvars_obj == nlvars_cons )
      nlvars_obj = nlvars_both;

   fprintf(writeopts.f, " %d %d %d\t# nonlinear vars in constraints, objectives, both\n",
      nlvars_cons,
      nlvars_obj,
      nlvars_both);

   /* setting arith flag as in AMPL generated .nl files
    * for text: always 0
    * for binary: Arith_Kind_ASL, which is determined by arithchk; 1 on my system
    */
   if( writeopts.binary )
      fprintf(writeopts.f, " 0 0 1 1\t# linear network variables; functions; arith, flags\n");
   else
      fprintf(writeopts.f, " 0 0 0 1\t# linear network variables; functions; arith, flags\n");

   fprintf(writeopts.f, " %d %d %d %d %d\t# discrete variables: binary, integer, nonlinear (b,c,o)\n",
      binvars_lin,
      intvars_lin,
      discrvars_nlboth,
      discrvars_nlcons,
      discrvars_nlobj);

   fprintf(writeopts.f, " %d %d\t# nonzeros in Jacobian, gradients\n",
      gmoNZ(gmo),
      gmoModelType(gmo) == gmoProc_cns ? 0 : gmoObjNZ(gmo));

   fprintf(writeopts.f, " %d %d\t# max name lengths: constraints, variables\n",
      maxnamelen_cons,
      maxnamelen_vars);

   fputs(" 0 0 0 0 0\t# common exprs: b,c,o,c1,o1\n", writeopts.f);

   return RETURN_OK;
}

/** write SOS into S segment
 * https://ampl.com/faqs/how-can-i-use-the-solvers-special-ordered-sets-feature/
 */
static
RETURN writeNLSOS(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   int numSos1;
   int numSos2;
   int nzSos;
   int numSos;
   int* sostype;
   int* sosbeg;
   int* sosind;
   double* soswt;
   int i, j, k;

   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);

   if( nzSos == 0 )
      return RETURN_OK;

   /* write sosno suffix
    * "The variables in each special ordered set must be given the same unique .sosno value, positive for sets of type 1 or negative for sets of type 2."
    */
   CHECK( writeNLPrintf(writeopts, "S%d %d %s\n", 0, nzSos, "sosno") );
   for( i = 0; i < gmoN(gmo); ++i )
   {
      int sosno;

      switch( gmoGetVarTypeOne(gmo, i) )
      {
         case gmovar_S1:
            sosno = gmoGetVarSosSetOne(gmo, i);
            break;
         case gmovar_S2:
            sosno = -gmoGetVarSosSetOne(gmo, i);
            break;
         default:
            continue;
      }
      CHECK( writeNLPrintf(writeopts, "%d %d\n", i, sosno) );
   }

   /* write sosref suffix
    * this is to make sure that variables within a SOS are ordered correctly
    * therefore, we iterate here over each SOS instead of all variables (as above)
    */
   CHECK( writeNLPrintf(writeopts, "S%d %d %s\n", 0, nzSos, "ref") );

   numSos = numSos1 + numSos2;
   sostype = (int*) malloc(numSos * sizeof(int));
   sosbeg = (int*) malloc((numSos+1) * sizeof(int));
   sosind = (int*) malloc(nzSos * sizeof(int));
   soswt = (double*) malloc(nzSos * sizeof(double));

   gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);

   for( i = 0; i < numSos; ++i )
      for( j = sosbeg[i], k = 1; j < sosbeg[i+1]; ++j, ++k )
         CHECK( writeNLPrintf(writeopts, "%d %d\n", sosind[j], k) );

   free(sostype);
   free(sosbeg);
   free(sosind);
   free(soswt);

   return RETURN_OK;
}

/** write the b segment */
static
RETURN writeNLVarBounds(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   double lb;
   double ub;
   int i;

   assert(gmo != NULL);

   CHECK( writeNLPrintf(writeopts, "b\n") );

   /* write variable bounds */
   for( i = 0; i < gmoN(gmo); ++i )
   {
      lb = gmoGetVarLowerOne(gmo, i);
      ub = gmoGetVarUpperOne(gmo, i);

      if( lb == ub )
      {
         CHECK( writeNLPrintf(writeopts, "4 %g\n", lb) );
      }
      else if( lb == gmoMinf(gmo) )
      {
         if( ub == gmoPinf(gmo) )
            CHECK( writeNLPrintf(writeopts, "3\n") );
         else
            CHECK( writeNLPrintf(writeopts, "1 %g\n", ub) );
      }
      else if( ub == gmoPinf(gmo) )
      {
         CHECK( writeNLPrintf(writeopts, "2 %g\n", lb) );
      }
      else
      {
         CHECK( writeNLPrintf(writeopts, "0 %g %g\n", lb, ub) );
      }
   }

   return RETURN_OK;
}

/** write the x segment */
static
RETURN writeNLInitialPoint(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   int nnz = 0;
   int i;

   assert(gmo != NULL);

   /* count nonzeros */
   for( i = 0; i < gmoN(gmo); ++i )
      if( gmoGetVarLOne(gmo, i) != 0.0 )
         ++nnz;

   if( nnz == 0 )
      return RETURN_OK;

   CHECK( writeNLPrintf(writeopts, "x%d\n", nnz) );

   /* write nonzero level values */
   for( i = 0; i < gmoN(gmo); ++i )
      if( gmoGetVarLOne(gmo, i) != 0.0 )
         CHECK( writeNLPrintf(writeopts, "%d %g\n", i, gmoGetVarLOne(gmo, i)) );

   return RETURN_OK;
}

/** write the r segment */
static
RETURN writeNLConsSides(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   double side;
   int i;

   assert(gmo != NULL);

   if( gmoM(gmo) == 0 )
      return RETURN_OK;

   CHECK( writeNLPrintf(writeopts, "r\n") );

   /* write equation sides */
   for( i = 0; i < gmoM(gmo); ++i )
   {
      side = gmoGetRhsOne(gmo, i);

      /* CHECK( writeNLPrintf(writeopts, "# %s\n", gmoGetEquNameOne(gmo, i, buf)) ); */

      switch( gmoGetEquTypeOne(gmo, i) )
      {
         case gmoequ_B:
         case gmoequ_C:
         case gmoequ_X:
            fprintf(stderr, "Unsupported equation type %d in equ %d\n", gmoGetEquTypeOne(gmo, i), i);
            return RETURN_ERROR;

         case gmoequ_E:
            CHECK( writeNLPrintf(writeopts, "4 %g\n", side) );
            break;

         case gmoequ_G:
            CHECK( writeNLPrintf(writeopts, "2 %g\n", side) );
            break;

         case gmoequ_L:
            CHECK( writeNLPrintf(writeopts, "1 %g\n", side) );
            break;

         case gmoequ_N:
            CHECK( writeNLPrintf(writeopts, "3\n") );
            break;
      }
   }

   return RETURN_OK;
}

static
RETURN writeNLnlnodeEnter(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts,
   gamsnl_node*       n
   )
{
   char buf[GMS_SSSIZE];

   switch( n->op )
   {
      case gamsnl_opvar :
         /* we should always have n->coef == 1 due to ampl mode when nl is created */
         assert(n->coef != 0.0);  /* this should normally not happen */
         if( n->coef == -1.0 )
            CHECK( writeNLPrintf(writeopts, "o%d  #neg\n", 16) );  /* negate */
         else if( n->coef != 1.0 )
         {
            CHECK( writeNLPrintf(writeopts, "o%d   #prod\n", 2) );  /* prod constant var*/
            CHECK( writeNLPrintf(writeopts, "n%g\n", n->coef) );
         }
         CHECK( writeNLPrintf(writeopts, "v%d  #%s\n", n->varidx, gmoDict(gmo) != NULL ? gmoGetVarNameOne(gmo, n->varidx, buf) : "") );
         break;

      case gamsnl_opconst :
         CHECK( writeNLPrintf(writeopts, "n%g\n", n->coef) );
         break;

      case gamsnl_opsum :
         if( n->nargs == 2 )
         {
            CHECK( writeNLPrintf(writeopts, "o%d   #plus\n", 0) );
         }
         else
         {
            CHECK( writeNLPrintf(writeopts, "o%d  #sum\n", 54) );
            CHECK( writeNLPrintf(writeopts, "%d\n", n->nargs) );
         }
         break;

      case gamsnl_opprod :
         /* AMPL has not n-ary product, lets hope for nargs=2 */
         if( n->nargs == 2 )
         {
            CHECK( writeNLPrintf(writeopts, "o%d   #prod\n", 2) );
         }
         else
         {
            fprintf(stderr, "opprod with %d args not supported\n", n->nargs);
            return RETURN_ERROR;
         }
         break;

      case gamsnl_opmin :
         CHECK( writeNLPrintf(writeopts, "o%d  #min\n", 11) );
         CHECK( writeNLPrintf(writeopts, "%d\n", n->nargs) );
         break;

      case gamsnl_opmax :
         CHECK( writeNLPrintf(writeopts, "o%d  #max\n", 12) );
         CHECK( writeNLPrintf(writeopts, "%d\n", n->nargs) );
         break;

      case gamsnl_opsub :
         CHECK( writeNLPrintf(writeopts, "o%d   #minus\n", 1) );
         break;

      case gamsnl_opdiv :
         CHECK( writeNLPrintf(writeopts, "o%d   #div\n", 3) );
         break;

      case gamsnl_opnegate :
         CHECK( writeNLPrintf(writeopts, "o%d  #neg\n", 16) );
         break;

      case gamsnl_opfunc :
         switch( n->func )
         {
            case fnmin:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #min\n", 11) );
               CHECK( writeNLPrintf(writeopts, "%d\n", n->nargs) );
               break;
            }

            case fnmax :
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #max\n", 12) );
               CHECK( writeNLPrintf(writeopts, "%d\n", n->nargs) );
               break;
            }

            case fnsqr:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #pow (sqr)\n", 5) );
               /* exponent will be written in writeNLnodeLeave() */
               break;
            }

            case fnexp:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #exp\n", 44) );
               break;
            }

            case fnlog:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #log\n", 43) );
               break;
            }

            case fnlog10:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #log10\n", 42) );
               break;
            }

            case fnlog2:
            {
               /* log2(x) = ln(x)/ln(2) */
               CHECK( writeNLPrintf(writeopts, "o%d   #prod\n", 2) );
               CHECK( writeNLPrintf(writeopts, "n%g  #1/ln(2)", 1.0/M_LN2) );
               CHECK( writeNLPrintf(writeopts, "o%d  #log", 43) );
               break;
            }

            case fnsqrt:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #sqrt\n", 39) );
               break;
            }

            case fnabs:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #abs\n", 15) );
               break;
            }

            case fncos:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #cos\n", 46) );
               break;
            }

            case fnsin:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #sin\n", 41) );
               break;
            }

            case fntan:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #tan\n", 38) );
               break;
            }

            case fnarccos:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #acos\n", 53) );
               break;
            }

            case fnarcsin:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #asin\n", 51) );
               break;
            }

            case fnarctan:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #atan\n", 49) );
               break;
            }

            case fnarctan2:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #atan2\n", 48) );
               break;
            }

            case fnsinh:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #sinh\n", 40) );
               break;
            }

            case fncosh:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #cosh\n", 45) );
               break;
            }

            case fntanh:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #tanh\n", 37) );
               break;
            }

            case fnpower:
            case fnrpower:
            case fncvpower:
            case fnvcpower:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #pow\n", 5) );
               break;
            }

            case fnpi:
            {
               CHECK( writeNLPrintf(writeopts, "n%g\n", M_PI) );
               break;
            }

            case fndiv:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #div\n", 3) );
               break;
            }

            case fnfloor:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #floor\n", 13) );
               break;
            }

            case fnceil:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #ceil\n", 14) );
               break;
            }

            case fnround:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #round\n", 57) );
               break;
            }

            case fntrunc:
            {
               CHECK( writeNLPrintf(writeopts, "o%d  #trunc\n", 58) );
               break;
            }

            case fnmod:  /* remainder of x/y, both of which can be fractional */
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #rem (mod)\n", 4) );
               break;
            }

            case fnfrac: /* fractional part of x */
            {
               /* frac(x) = mod(x,1) */
               CHECK( writeNLPrintf(writeopts, "o%d   #rem (frac)\n", 4) );
               break;
            }

            case fnboolnot:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #not\n", 34) );
               break;
            }

            case fnbooland:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #and\n", 21) );
               break;
            }

            case fnboolor:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #or\n", 20) );
               break;
            }

            case fnboolxor:
            {
               /* write x^y as !(x=y) */
               CHECK( writeNLPrintf(writeopts, "o%d   #not (xor)\n", 34) );
               CHECK( writeNLPrintf(writeopts, "o%d   #and (xor)\n", 21) );
               break;
            }

            case fnboolimp:
            {
               /* write x => y as !x or y */
               CHECK( writeNLPrintf(writeopts, "o%d   #or (imp)\n", 20) );
               CHECK( writeNLPrintf(writeopts, "o%d   #not (imp)\n", 34) );
               break;
            }

            case fnbooleqv:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #eqv\n", 24) );
               break;
            }

            case fnrelopeq:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #eq\n", 24) );
               break;
            }

            case fnrelopgt:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #gt\n", 29) );
               break;
            }

            case fnrelopge:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #ge\n", 28) );
               break;
            }

            case fnreloplt:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #lt\n", 22) );
               break;
            }

            case fnrelople:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #le\n", 23) );
               break;
            }

            case fnrelopne:
            {
               CHECK( writeNLPrintf(writeopts, "o%d   #ne\n", 30) );
               break;
            }

            /* functions we or AMPL do not support */
            case fnifthen:  /* AMPL' iff is like == and thus much different than GAMS' ifthen */
            case fngamma:
            case fnloggamma:
            case fnerrf:
            case fnsign:
            case fnfact:
            case fnbinomial:
            case fnsignpower:
            case fnentropy: /* -x*ln(x) */
            case fnsigmoid: /* 1/(1+exp(-x)) */
            case fngammareg:  /* regularized gamma function */
            case fnbeta:
            case fnlogbeta:
            case fnbetareg:
            case fndunfm: case fndnorm: case fnunfmi /* uniform random number */:
            case fnncpf /* fischer smoothing: sqrt(x1^2+x2^2+2*x3) */:
            case fnncpcm /* chen-mangasarian smoothing: x1-x3*ln(1+exp((x1-x2)/x3))*/:
            case fnncpvusin /* veelken-ulbrich smoothing */:
            case fnncpvupow /* veelken-ulbrich smoothing */:
            default:
            {
               fprintf(stderr, "Error: Unsupported GAMS function %s.\n", GamsFuncCodeName[n->func]);
               return RETURN_ERROR;
            }

            /* these we have taken care of when putting the expression tree together */
            case fncentropy:
            case fnedist:
            case fnpoly :
            {
               fprintf(stderr, "Error: GAMS function %s should have been reformulated.\n", GamsFuncCodeName[n->func]);
               return RETURN_ERROR;
            }
         }
   }

   return RETURN_OK;
}

static
RETURN writeNLnlnodeLeave(
   gmoHandle_t    /* gmo */,
   convertWriteNLopts writeopts,
   gamsnl_node*       n
   )
{
   switch( n->op )
   {
      case gamsnl_opfunc:
         switch( n->func )
         {
            case fnsqr:
               CHECK( writeNLPrintf(writeopts, "n%g\n", 2.0) );
               break;

            case fnround:
            {
               /* AMPL round() is always a binary operation, GAMS can have 1 or 2 args */
               if( n->nargs == 1 )
                  CHECK( writeNLPrintf(writeopts, "n%g\n", 0.0) );
               break;
            }

            case fntrunc:
            {
               /* AMPL trunc() is a binary operation, GAMS has one arg */
               CHECK( writeNLPrintf(writeopts, "n%g\n", 0.0) );
               break;
            }

            case fnfrac:
            {
               /* frac(x) = mod(x,1): here comes the 1 */
               CHECK( writeNLPrintf(writeopts, "n%g\n", 1.0) );
               break;
            }

            default:
               break;
         }
         break;

      default:
         break;
   }

   return RETURN_OK;
}

/** write GAMS expression as AMPL expression */
static
RETURN writeNLExpr(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts,
   int*               opcodes,
   int*               fields,
   int                codelen,
   double             factor,
   double             constant   /**< constant to be added after factor */
)
{
   double* constants;
   gamsnl_node* root;

   gamsnl_node** stack;
   int* nvisited;
   int stacksize;
   int stackpos;
   gamsnl_node* n;
   enum { VISITED, ENTER, VISITING, LEAVE } stage;

   constants = (double*)gmoPPool(gmo);

   /* convert GAMS instructions (reverse polish) into nlnode tree */
   CHECK( gamsnlParseGamsInstructions(&root, gmo, codelen, opcodes, fields, constants, factor, constant, gamsnl_ampl) );

   /* write nlnode tree as AMPL instructions (polish prefix)
    * this is like writeOSILnlnode()
    */
   stacksize = 100;
   stack = (gamsnl_node**) malloc(stacksize * sizeof(gamsnl_node*));
   nvisited = (int*) malloc(stacksize * sizeof(int));

   stack[0] = root;
   nvisited[0] = 0;
   stackpos = 0;
   stage = ENTER;

   while( stackpos >= 0 )
   {
      /* get nlnode on the top of the stack */
      n = stack[stackpos];

      switch( stage )
      {
         case VISITED :
         {
            /* go to next child, if any, otherwise go to leave */
            ++nvisited[stackpos];
            if( nvisited[stackpos] < n->nargs )
            {
               stage = VISITING;
               break;
            }
            stage = LEAVE;

            break;
         }

         case ENTER:
         {
            assert(nvisited[stackpos] == 0);

            if( n->op == gamsnl_opsum && n->nargs == 2 )
            {
               /* undo some reformulation from subtraction to sum, if that introduced negation */
               if( n->args[0]->op == gamsnl_opnegate && n->args[1]->op == gamsnl_opnegate )
               {
                  /* handle this in VISITING */
               }
               else if( n->args[0]->op == gamsnl_opnegate )
               {
                  /* (-a) + b -> b - a */
                  gamsnl_node* a = n->args[0]->args[0];
                  /* free negation node */
                  n->args[0]->nargs = 0;
                  gamsnlFree(&n->args[0]);
                  /* change sum into sub */
                  n->op = gamsnl_opsub;
                  n->args[0] = n->args[1];  /* b */
                  n->args[1] = a;
               }
               else if( n->args[1]->op == gamsnl_opnegate )
               {
                  /* a + (-b) -> a - b */
                  gamsnl_node* b = n->args[1]->args[0];
                  /* free negation node */
                  n->args[1]->nargs = 0;
                  gamsnlFree(&n->args[1]);
                  /* change sum into sub */
                  n->op = gamsnl_opsub;
                  n->args[1] = b;
               }
            }
            else if( n->op == gamsnl_opprod && n->nargs == 2 && n->args[1]->op == gamsnl_opconst )
            {
               /* a * constant -> constant * a, to be like AMPL */
               gamsnl_node* tmp = n->args[1];
               n->args[1] = n->args[0];
               n->args[0] = tmp;
            }

            CHECK( writeNLnlnodeEnter(gmo, writeopts, n) );

            /* go to first child, if any, otherwise leave */
            if( n->nargs > 0 )
               stage = VISITING;
            else
               stage = LEAVE;

            break;
         }

         case VISITING:
         {
            gamsnl_node* child = n->args[nvisited[stackpos]];

            if( child->op == gamsnl_opsum && child->nargs == 2 )
            {
               /* undo some reformulation from subtraction to sum, if that introduced negation
                * handling this case here because we recreate child
                */
               if( child->args[0]->op == gamsnl_opnegate && child->args[1]->op == gamsnl_opnegate )
               {
                  /* (-a) + (-b) -> - (a + b) */
                  gamsnl_node* newsum;
                  gamsnl_node* newchild;

                  CHECK( gamsnlCreate(&newsum, gamsnl_opsum) );
                  CHECK( gamsnlAddArg(newsum, child->args[0]->args[0]) );
                  CHECK( gamsnlAddArg(newsum, child->args[1]->args[0]) );
                  CHECK( gamsnlCreate(&newchild, gamsnl_opnegate) );
                  newchild->args[0] = newsum;
                  newchild->nargs = 1;

                  child->args[0]->nargs = 0;
                  child->args[1]->nargs = 0;
                  gamsnlFree(&child);

                  n->args[nvisited[stackpos]] = newchild;
               }
            }

            /* put child to visit on top of stack */
            if( stackpos+1 >= stacksize )
            {
               stacksize *= 2;
               stack = (gamsnl_node**) realloc(stack, stacksize * sizeof(gamsnl_node*));
               nvisited = (int*) realloc(nvisited, stacksize * sizeof(int));
            }

            stack[stackpos+1] = n->args[nvisited[stackpos]];
            nvisited[stackpos+1] = 0;
            ++stackpos;
            stage = ENTER;

            break;
         }

         case LEAVE:
         {
            CHECK( writeNLnlnodeLeave(gmo, writeopts, n) );

            /* remove current node from stack (or forget that it is there) */
            stage = VISITED;
            --stackpos;

            break;
         }
      }
   }

   free(nvisited);
   free(stack);

   gamsnlFree(&root);

   return RETURN_OK;
}

/** write the C and O segments: expressions of nonlinear constraints and objective */
static
RETURN writeNLExprs(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   char buf[GMS_SSSIZE];
   int* opcodes;
   int* fields;
   int codelen;
   int i;

   assert(gmo != NULL);

   opcodes = (int*) malloc((gmoNLCodeSizeMaxRow(gmo)+1) * sizeof(int));
   fields = (int*) malloc((gmoNLCodeSizeMaxRow(gmo)+1) * sizeof(int));

   for( i = 0; i < gmoM(gmo); ++i )
   {
      CHECK( writeNLPrintf(writeopts, "C%d   #%s\n", i, gmoDict(gmo) != NULL ? gmoGetEquNameOne(gmo, i, buf) : NULL) );
      if( gmoGetEquOrderOne(gmo, i) == gmoorder_L )
      {
         /* AMPL writes n0 for linear constraints */
         CHECK( writeNLPrintf(writeopts, "n%g\n", 0.0) );
      }
      else
      {
         gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
         CHECK( writeNLExpr(gmo, writeopts, opcodes, fields, codelen, 1.0, 0.0) );
      }
   }

   if( gmoModelType(gmo) != gmoProc_cns )
   {
      CHECK( writeNLPrintf(writeopts, "O%d %d\n", 0, gmoSense(gmo) == gmoObj_Min ? 0 : 1) );

      if( gmoGetObjOrder(gmo) == gmoorder_L )
      {
         CHECK( writeNLPrintf(writeopts, "n%g\n", gmoObjConst(gmo)) );
      }
      else
      {
         gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);
         CHECK( writeNLExpr(gmo, writeopts, opcodes, fields, codelen, -1.0 / gmoObjJacVal(gmo), gmoObjConst(gmo)) );
      }
   }

   free(fields);
   free(opcodes);

   return RETURN_OK;
}

/** write the k segment */
static
RETURN writeNLJacobianSparsity(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   int nzsum = 0;
   int i;

   assert(gmo != NULL);

   CHECK( writeNLPrintf(writeopts, "k%d\n", gmoN(gmo)-1) );

   for( i = 0; i < gmoN(gmo)-1; ++i )
   {
      int nz, qnz, nlnz, objnz;

      gmoGetColStat(gmo, i, &nz, &qnz, &nlnz, &objnz);
      nzsum += nz;

      CHECK( writeNLPrintf(writeopts, "%d\n", nzsum) );
   }

   return RETURN_OK;
}

/** write the J and G segments */
static
RETURN writeNLLinearCoefs(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   char buf[GMS_SSSIZE];
   int nz;
   int qnz;
   int nlnz;
   int i;

   /* note that we do not write (varindex,coef) in increasing order here, due to variable permutations */

   for( i = 0; i < gmoM(gmo); ++i )
   {
      void* jacptr;
      double jacval;
      int colidx;
      int nlflag;

      gmoGetRowStat(gmo, i, &nz, &qnz, &nlnz);
      CHECK( writeNLPrintf(writeopts, "J%d %d  #%s\n", i, nz, gmoDict(gmo) != NULL ? gmoGetEquNameOne(gmo, i, buf) : NULL) );
      if( nz == 0 )
         continue;

      jacptr = NULL;
      do
      {
         gmoGetRowJacInfoOne(gmo, i, &jacptr, &jacval, &colidx, &nlflag);

         /* entries from nonlinear terms are printed as 0 */
         if( nlflag )
            jacval = 0.0;
         CHECK( writeNLPrintf(writeopts, "%d %g  #%s\n", colidx, jacval, gmoDict(gmo) != NULL ? gmoGetVarNameOne(gmo, colidx, buf) : "") );
      }
      while( jacptr != NULL );
   }

   if( gmoModelType(gmo) != gmoProc_cns && gmoObjNZ(gmo) > 0 )
   {
      int* colidx;
      int* nlflag;
      double* jacval;

      CHECK( writeNLPrintf(writeopts, "G%d %d\n", 0, gmoObjNZ(gmo)) );

      colidx = (int*) malloc(gmoObjNZ(gmo) * sizeof(int));
      nlflag = (int*) malloc(gmoObjNZ(gmo) * sizeof(int));
      jacval = (double*) malloc(gmoObjNZ(gmo) * sizeof(double));

      gmoGetObjSparse(gmo, colidx, jacval, nlflag, &nz, &nlnz);
      assert(nz == gmoObjNZ(gmo));

      for( i = 0; i < nz; ++i )
      {
         /* entries from nonlinear terms are printed as 0 */
         if( nlflag[i] )
            jacval[i] = 0.0;
         CHECK( writeNLPrintf(writeopts, "%d %g  #%s\n", colidx[i], jacval[i], gmoDict(gmo) != NULL ? gmoGetVarNameOne(gmo, colidx[i], buf) : "") );
      }

      free(jacval);
      free(nlflag);
      free(colidx);
   }

   return RETURN_OK;
}

/** write .row and .col files, if dictionary is available */
static
RETURN writeNLNames(
   struct gmoRec* gmo,
   const char*    nlfilename
)
{
   FILE* f;
   char buf[GMS_SSSIZE];
   char* filename;
   int stublen;
   int i;

   if( gmoDict(gmo) == NULL )
      return RETURN_OK;

   /* length of .nl filename with 'nl' */
   stublen = strlen(nlfilename)-2;

   filename = (char*)malloc(stublen+4);
   memcpy(filename, nlfilename, stublen);

   strcpy(filename + stublen, "col");
   f = fopen(filename, "w");
   for( i = 0; i < gmoN(gmo); ++i )
      fprintf(f, "%s\n", gmoGetVarNameOne(gmo, i, buf));
   fclose(f);

   strcpy(filename + stublen, "row");
   f = fopen(filename, "w");
   for( i = 0; i < gmoM(gmo); ++i )
      fprintf(f, "%s\n", gmoGetEquNameOne(gmo, i, buf));
   if( gmoModelType(gmo) != gmoProc_cns )
      fputs("obj\n", f);
   fclose(f);

   free(filename);

   return RETURN_OK;
}


/* https://ampl.github.io/nlwrite.pdf
   https://github.com/minotaur-solver/minotaur/blob/master/src/base/NlWriter.cpp
   https://github.com/jump-dev/MathOptInterface.jl/blob/master/src/FileFormats/NL/NL.jl
   https://github.com/Pyomo/pyomo/blob/main/pyomo/repn/plugins/ampl/ampl_.py
 */
RETURN convertWriteNL(
   struct gmoRec*     gmo,
   convertWriteNLopts writeopts
)
{
   int* varperm = NULL;
   int* equperm = NULL;

   assert(gmo != NULL);

   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoUseQSet(gmo, 0);

   if( gmoGetEquTypeCnt(gmo, gmoequ_C) )
   {
      fputs("Instance has conic equations, cannot write in .nl format so far.\n", stderr);
      return RETURN_ERROR;
   }

   if( gmoGetVarTypeCnt(gmo, gmovar_SC) || gmoGetVarTypeCnt(gmo, gmovar_SI) )
   {
      /* GAMS/Convert writes these into .mod files as {0} union interval[lb,ub]
       * but AMPL reformulates this with a new binary var and constraints
       * lets not do this in the nl writer
       */
      fputs("Instance has semi-continuous or semi-integer variables, cannot write in .nl format.\n", stderr);
      return RETURN_ERROR;
   }

   writeopts.f = fopen(writeopts.filename, "wb");
   if( writeopts.f == NULL )
   {
      fprintf(stderr, "Could not open file %s for writing.\n", writeopts.filename);
      return RETURN_ERROR;
   }

   CHECK( writeNLHeader(gmo, writeopts, &varperm, &equperm) );

   gmoSetRvVarPermutation(gmo, varperm, gmoN(gmo));
   gmoSetRvEquPermutation(gmo, equperm, gmoM(gmo));

   CHECK( writeNLSOS(gmo, writeopts) );  /* S segment */

   CHECK( writeNLVarBounds(gmo, writeopts) );  /* b segment */

   CHECK( writeNLInitialPoint(gmo, writeopts) );  /* x segment*/

   CHECK( writeNLConsSides(gmo, writeopts) );  /* r segment */

   CHECK( writeNLExprs(gmo, writeopts) );  /* C and O segments */

   CHECK( writeNLJacobianSparsity(gmo, writeopts) );  /* k segment */

   CHECK( writeNLLinearCoefs(gmo, writeopts) );  /* J and G segments */

   fclose(writeopts.f);

   CHECK( writeNLNames(gmo, writeopts.filename) );  /* .row and .col files */

   free(varperm);
   free(equperm);

   return RETURN_OK;
}
