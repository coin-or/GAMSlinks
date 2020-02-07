// Copyright (C) GAMS Development and others 2009-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsOsiHelper.hpp"
#include "GAMSlinksConfig.h"

#include <cstdlib>
#include <cmath>

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStartBasis.hpp"
#include "OsiSolverInterface.hpp"

#include "gmomcc.h"
#include "gevmcc.h"

#include "GAMSlinksConfig.h"

bool gamsOsiLoadProblem(
   struct gmoRec*        gmo,                /**< GAMS modeling object */
   OsiSolverInterface&   solver,             /**< OSI solver interface */
   bool                  setupnames          /**< should col/row names be setup in Osi? */
)
{
   assert(gmo != NULL);

   struct gevRec* gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   // objective
   double* objcoeff = new double[gmoN(gmo)];
   gmoGetObjVector(gmo, objcoeff, NULL);
   solver.setDblParam(OsiObjOffset, -gmoObjConst(gmo));

   // matrix
   int nz = gmoNZ(gmo);
   double* values  = new double[nz];
   int* colstarts  = new int[gmoN(gmo)+1];
   int* rowindexes = new int[nz];
   int* nlflags    = new int[nz];

   gmoGetMatrixCol(gmo, colstarts, rowindexes, values, nlflags);
   colstarts[gmoN(gmo)] = nz;

   // squeeze zero elements
   int shift = 0;
   for( int col = 0; col < gmoN(gmo); ++col )
   {
      colstarts[col+1] -= shift;
      for( int k = colstarts[col]; k < colstarts[col+1]; )
      {
         values[k] = values[k+shift];
         rowindexes[k] = rowindexes[k+shift];
         if( !values[k] )
         {
            ++shift;
            --colstarts[col+1];
         }
         else
         {
            ++k;
         }
      }
   }
   nz -= shift;

   // variable bounds
   double* varlow = new double[gmoN(gmo)];
   double* varup  = new double[gmoN(gmo)];
   gmoGetVarLower(gmo, varlow);
   gmoGetVarUpper(gmo, varup);

   // row sense
   char* rowsense = new char[gmoM(gmo)];
   for( int i = 0; i < gmoM(gmo); ++i )
      switch( (enum gmoEquType)gmoGetEquTypeOne(gmo, i) )
      {
         case gmoequ_E: rowsense[i] = 'E'; break;
         case gmoequ_G: rowsense[i] = 'G'; break;
         case gmoequ_L: rowsense[i] = 'L'; break;
         case gmoequ_N: rowsense[i] = 'N'; break;
         case gmoequ_C:
            gevLogStat(gev, "Error: Conic constraints not supported by OSI.");
            return false;
         default:
            gevLogStat(gev, "Error: Unsupported equation type.");
            return false;
      }
   // right-hand-side
   double* rhs = new double[gmoM(gmo)];
   gmoGetRhs(gmo, rhs);

   //	printf("%d columns:\n", gmoN(gmo));
   //	for (int i = 0; i < gmoN(gmo); ++i)
   //		printf("lb %g\t ub %g\t obj %g\t colstart %d\n", varlow[i], varup[i], objcoeff[i], colstarts[i]);
   //	printf("%d rows:\n", gmoM(gmo));
   //	for (int i = 0; i < gmoM(gmo); ++i)
   //		printf("rhs %g\t sense %c\t rng %g\n", rhs[i], rowsense[i], rowrng[i]);
   //	printf("%d nonzero values:", nz);
   //	for (int i = 0; i < nz; ++i)
   //		printf("%d:%g ", rowindexes[i], values[i]);

   solver.loadProblem(gmoN(gmo), gmoM(gmo), colstarts, rowindexes, values, varlow, varup, objcoeff, rowsense, rhs, NULL);

   delete[] colstarts;
   delete[] rowindexes;
   delete[] values;
   delete[] nlflags;
   delete[] varlow;
   delete[] varup;
   delete[] objcoeff;
   delete[] rowsense;
   delete[] rhs;

   // objective sense
   switch( gmoSense(gmo) )
   {
      case gmoObj_Min:
         solver.setObjSense(1.0);
         break;
      case gmoObj_Max:
         solver.setObjSense(-1.0);
         break;
      default:
         gevLogStat(gev, "Error: Unsupported objective sense.");
         return false;
   }

   // tell solver which variables are discrete
   if( gmoNDisc(gmo) )
   {
      int* discrind;
      int j = 0;

      discrind = new int[gmoNDisc(gmo)];
      for( int i = 0; i < gmoN(gmo); ++i )
      {
         switch( (enum gmoVarType)gmoGetVarTypeOne(gmo, i) )
         {
            case gmovar_B:  // binary
            case gmovar_I:  // integer
            case gmovar_SI: // semiinteger
               assert(j < gmoNDisc(gmo));
               discrind[j++] = i;
               break;
            case gmovar_X:  // probably this means continuous variable
            case gmovar_S1: // in SOS1
            case gmovar_S2: // in SOS2
            case gmovar_SC: // semicontinuous
               break;
         }
      }
      solver.setInteger(discrind, j);
      delete[] discrind;
   }

   char inputname[GMS_SSSIZE];
   gmoNameInput(gmo, inputname);
   solver.setStrParam(OsiProbName, inputname);

   // setup column/row/obj names, if available
   if( setupnames && gmoDict(gmo) != NULL )
   {
      char buffer[GMS_SSSIZE];
      size_t nameMem;

      solver.setIntParam(OsiNameDiscipline, 2);
      nameMem = (gmoN(gmo) + gmoM(gmo)) * sizeof(char*);
      for( int j = 0; j < gmoN(gmo); ++j )
      {
         gmoGetVarNameOne(gmo, j, buffer);
         solver.setColName(j, buffer);
         nameMem += strlen(buffer)+1;
      }
      for( int j = 0; j < gmoM(gmo); ++j )
      {
         gmoGetEquNameOne(gmo, j, buffer);
         solver.setRowName(j, buffer);
         nameMem += strlen(buffer) + 1;
      }
      gmoGetObjName(gmo, buffer);
      solver.setObjName(buffer);
      nameMem += strlen(buffer) + 1;

      if( (nameMem >> 20) > 0 )
      {
         sprintf(buffer, "Space for names approximately %u MB.\nUse statement '<modelname>.dictfile=0;' to turn dictionary off.\n", (unsigned int)(nameMem>>20));
         gevLogStatPChar(gev, buffer);
      }
   }

   return true;
}

bool gamsOsiStoreSolution(
   struct gmoRec*        gmo,                /**< GAMS modeling object */
   const OsiSolverInterface& solver          /**< OSI solver interface */
)
{
   assert(gmo != NULL);

   struct gevRec* gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   const double* colLevel  = solver.getColSolution();
   const double* colMargin = solver.getReducedCost();
   const double* rowLevel  = solver.getRowActivity();
   const double* rowMargin = solver.getRowPrice();

   assert(gmoN(gmo) == 0 || colLevel);
   assert(gmoN(gmo) == 0 || colMargin);
   assert(gmoM(gmo) == 0 || rowLevel);
   assert(gmoM(gmo) == 0 || rowMargin);

   int* colBasis = new int[solver.getNumCols()];
   int* rowBasis = new int[solver.getNumRows()];
   int* colStatus = new int[solver.getNumCols()];
   int* rowStatus = new int[solver.getNumRows()];

   // workaround for gmo if there are no rows or columns
   double dummy2;
   if( !gmoN(gmo) )
   {
      colLevel  = &dummy2;
      colMargin = &dummy2;
   }
   if( !gmoM(gmo) )
   {
      rowLevel  = &dummy2;
      rowMargin = &dummy2;
   }

   if( solver.optimalBasisIsAvailable() )
   {
      solver.getBasisStatus(colBasis, rowBasis);

      // translate from OSI codes to GAMS codes
      for( int j = 0; j < gmoN(gmo); ++j )
      {
         // only for fully continuous variables we can give a reliable basis status
         if( gmoGetVarTypeOne(gmo, j) != gmovar_X )
            colBasis[j] = gmoBstat_Super;
         else
            switch( colBasis[j] )
            {
               // change onbound to super if value is not on bound as it should be
               case 3: colBasis[j] = (colLevel[j] > gmoGetVarLowerOne(gmo, j) + 1e-6) ? gmoBstat_Super : gmoBstat_Lower; break;
               case 2: colBasis[j] = (colLevel[j] < gmoGetVarUpperOne(gmo, j) - 1e-6) ? gmoBstat_Super : gmoBstat_Upper; break;
               case 1: colBasis[j] = gmoBstat_Basic; break;
               case 0: colBasis[j] = gmoBstat_Super; break;
               default: gevLogStat(gev, "Column basis status unknown!"); return false;
            }
      }
      for( int i = 0; i < gmoM(gmo); ++i )
      {
         switch( rowBasis[i] )
         {
            case 2: rowBasis[i] = gmoBstat_Lower; break;
            case 3: rowBasis[i] = gmoBstat_Upper; break;
            case 1: rowBasis[i] = gmoBstat_Basic; break;
            case 0: rowBasis[i] = gmoBstat_Super; break;
            default: gevLogStat(gev, "Row basis status unknown!"); return false;
         }
      }
   }
   else
   {
      CoinWarmStart* ws = solver.getWarmStart();
      CoinWarmStartBasis* wsb = dynamic_cast<CoinWarmStartBasis*>(ws);
      if( wsb != NULL )
      {
         for( int j = 0; j < gmoN(gmo); ++j )
            if( gmoGetVarTypeOne(gmo, j) != gmovar_X )
               colBasis[j] = gmoBstat_Super;
            else
               switch( wsb->getStructStatus(j) )
               {
                  // change onbound to super if value is not on bound as it should be
                  case CoinWarmStartBasis::atLowerBound: colBasis[j] = (colLevel[j] > gmoGetVarLowerOne(gmo, j) + 1e-6) ? gmoBstat_Super : gmoBstat_Lower; break; // change to super if value is not on bound as it should be
                  case CoinWarmStartBasis::atUpperBound: colBasis[j] = (colLevel[j] < gmoGetVarUpperOne(gmo, j) - 1e-6) ? gmoBstat_Super : gmoBstat_Upper; break;
                  case CoinWarmStartBasis::basic:        colBasis[j] = gmoBstat_Basic; break;
                  case CoinWarmStartBasis::isFree:       colBasis[j] = gmoBstat_Super; break;
                  default: gevLogStat(gev, "Column basis status unknown!"); return false;
               }
         for( int j = 0; j < gmoM(gmo); ++j )
         {
            switch( wsb->getArtifStatus(j) )
            {
               // for Cbc, the basis status seem to be flipped in CoinWarmStartBasis, but not in getBasisStatus
               // change onbound to super if value is not at rhs as it should be
               case CoinWarmStartBasis::atLowerBound: rowBasis[j] = (fabs(rowLevel[j] - gmoGetRhsOne(gmo, j)) > 1e-6 ? gmoBstat_Super : gmoBstat_Upper); break;
               case CoinWarmStartBasis::atUpperBound: rowBasis[j] = (fabs(rowLevel[j] - gmoGetRhsOne(gmo, j)) > 1e-6 ? gmoBstat_Super : gmoBstat_Lower); break;
               case CoinWarmStartBasis::basic:        rowBasis[j] = gmoBstat_Basic; break;
               case CoinWarmStartBasis::isFree:       rowBasis[j] = gmoBstat_Super; break;
               default: gevLogStat(gev, "Row basis status unknown!"); return false;
            }
         }
      }
      else
      {
         CoinFillN(colBasis, gmoN(gmo), (int)gmoBstat_Super);
         CoinFillN(rowBasis, gmoM(gmo), (int)gmoBstat_Super);
      }
      delete ws;
   }

   double* primalray = NULL;
   switch( gmoModelStat(gmo) )
   {
      case gmoModelStat_OptimalGlobal :
      case gmoModelStat_OptimalLocal :
      case gmoModelStat_Integer :
         CoinFillN(colStatus, gmoN(gmo), (int)gmoCstat_OK);
         CoinFillN(rowStatus, gmoM(gmo), (int)gmoCstat_OK);
         break;

      case gmoModelStat_Unbounded :
      {
         std::vector<double*> primalrays = solver.getPrimalRays(1);
         if( !primalrays.empty() )
         {
            primalray = primalrays[0];
            assert(primalray != NULL);
         }
         for( size_t r = 1; r < primalrays.size(); ++r )
            delete[] primalrays[r];
      }
      /* no break */

      default :
      {
         int ninfeas = 0;
         for( int j = 0; j < gmoN(gmo); ++j )
         {
            colStatus[j] = gmoCstat_OK;
            /* infeasible column ? */
            if( colLevel[j] < gmoGetVarLowerOne(gmo, j) - 1e-6 || colLevel[j] > gmoGetVarUpperOne(gmo, j) + 1e-6 )
            {
               colStatus[j] = gmoCstat_Infeas;
               ++ninfeas;
            }
            /* nonoptimal column ? */
            else if( colBasis[j] == gmoBstat_Lower && solver.getObjSense() * colMargin[j] < -1e-6 ) /* at lowerbound and dj<0 */
               colStatus[j] = gmoCstat_NonOpt;
            else if( colBasis[j] == gmoBstat_Upper && solver.getObjSense() * colMargin[j] > 1e-6 ) /* at upperbound and dj>0 */
               colStatus[j] = gmoCstat_NonOpt;
            else if( colBasis[j] == gmoBstat_Basic && fabs(colMargin[j]) > 1e-6 ) /* between bounds and dj <> 0 */
               colStatus[j] = gmoCstat_NonOpt;
            /* unbounded column ? */
            else if( primalray != NULL && fabs(primalray[j]) > 1e-6 )
               colStatus[j] = gmoCstat_UnBnd;
         }

         for( int i = 0; i < gmoM(gmo); ++i )
         {
            rowStatus[i] = gmoCstat_OK;
            /* infeasible row ? */
            if( rowLevel[i] < solver.getRowLower()[i] - 1e-6 || rowLevel[i] > solver.getRowUpper()[i] + 1e-6 )
            {
               rowStatus[i] = gmoCstat_Infeas;
               ++ninfeas;
            }
            else if( solver.getRowSense()[i] != 'E' )
            {
               /* nonoptimal row ? */
               if( rowBasis[i] == gmoBstat_Lower && solver.getObjSense() * rowMargin[i] < -1e-6 ) /* at lowerbound and pi<0 */
                  rowStatus[i] = gmoCstat_NonOpt;
               else if( rowBasis[i] == gmoBstat_Upper && solver.getObjSense() * rowMargin[i] > 1e-6 ) /* at upperbound and pi>0 */
                  rowStatus[i] = gmoCstat_NonOpt;
               else if( rowBasis[i] == gmoBstat_Basic && fabs(rowMargin[i]) > 1e-6 ) /* between bounds and pi <> 0 */
                  rowStatus[i] = gmoCstat_NonOpt;
            }
            /* there seem to be no notion of unbounded equations (in terms of slack variables) in Osi... */
         }

         /* change intermediate infeasible to feasible, if solution does not seem infeasible */
         if( ninfeas == 0 && gmoModelStat(gmo) == gmoModelStat_InfeasibleIntermed )
            gmoModelStatSet(gmo, gmoNDisc(gmo) > 0 ? gmoModelStat_Integer : gmoModelStat_Feasible);

         break;
      }
   }

   /* this also sets the gmoHobjval attribute to the level value of GAMS' objective variable */
   gmoSetSolution8(gmo, colLevel, colMargin, rowMargin, rowLevel, colBasis, colStatus, rowBasis, rowStatus);

   delete[] colBasis;
   delete[] rowBasis;
   delete[] colStatus;
   delete[] rowStatus;
   delete[] primalray;

   return true;
}

/** writes the problem stored in an OSI into LP and MPS files
 * set the first bit of formatflags for using writeMps, the second bit for using writeLp, and/or the third for using writeMpsNative
 */
void gamsOsiWriteProblem(
   struct gmoRec*        gmo,                /**< GAMS modeling object */
   OsiSolverInterface&   solver,             /**< OSI solver interface */
   unsigned int          formatflags         /**< in which formats to write the instance */
)
{
   if( formatflags == 0 )
      return;

   struct gevRec* gev;
   char buffer[GMS_SSSIZE+30];
   double objoffset;

   gev = (struct gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

   solver.getDblParam(OsiObjOffset, objoffset);
   if( objoffset != 0.0 )
   {
      snprintf(buffer, sizeof(buffer), "Ignoring objective offset %.20g when writing instance.\n", objoffset);
      gevLogPChar(gev, buffer);
   }

   gmoNameInput(gmo, buffer);

   if( formatflags & 0x1 )
   {
      gevLogPChar(gev, "Writing MPS file ");
      gevLogPChar(gev, buffer);
      gevLogPChar(gev, ".mps\n");

      assert(gmoN(gmo) <= solver.getNumCols());
      assert(gmoM(gmo) <= solver.getNumRows());

      int nameDiscipline;
      if( !solver.getIntParam(OsiNameDiscipline, nameDiscipline) )
         nameDiscipline = 0;
      if( nameDiscipline == 2 )
      {
         char** colnames = new char*[gmoN(gmo)];
         char** rownames = new char*[gmoM(gmo)+1];
         for( int i = 0; i < gmoN(gmo); ++i )
            colnames[i] = strdup(solver.getColName(i).c_str());
         for( int i = 0; i < gmoM(gmo); ++i )
            rownames[i] = strdup(solver.getRowName(i).c_str());
         rownames[gmoM(gmo)] = strdup(solver.getObjName().c_str());

         strcat(buffer, ".mps");
         solver.writeMpsNative(buffer, const_cast<const char**>(rownames), const_cast<const char**>(colnames), 0, 2, 1.0);

         for( int i = 0; i < gmoN(gmo); ++i )
            free(colnames[i]);
         for( int i = 0; i < gmoM(gmo)+1; ++i )
            free(rownames[i]);
         delete[] colnames;
         delete[] rownames;
      }
      else
      {
         solver.writeMps(buffer, "mps", 1.0);
      }
   }

   if( formatflags & 0x2 )
   {
      gevLogPChar(gev, "Writing LP file ");
      gevLogPChar(gev, buffer);
      gevLogPChar(gev, ".lp\n");
      solver.writeLp(buffer, "lp", 1e-9, 10, 15, 0.0, true);
   }

   if( formatflags & 0x4 )
   {
      strcat(buffer, "_native.mps");
      gevLogPChar(gev, "Writing native MPS file ");
      gevLog(gev, buffer);

      assert(gmoN(gmo) <= solver.getNumCols());
      assert(gmoM(gmo) <= solver.getNumRows());
      char** colnames = NULL;
      char** rownames = NULL;
      int nameDiscipline;
      if( !solver.getIntParam(OsiNameDiscipline, nameDiscipline) )
         nameDiscipline = 0;
      if( nameDiscipline == 2 )
      {
         colnames = new char*[gmoN(gmo)];
         rownames = new char*[gmoM(gmo)+1];
         for( int i = 0; i < gmoN(gmo); ++i )
            colnames[i] = strdup(solver.getColName(i).c_str());
         for( int i = 0; i < gmoM(gmo); ++i )
            rownames[i] = strdup(solver.getRowName(i).c_str());
         rownames[gmoM(gmo)] = strdup(solver.getObjName().c_str());
      }

      solver.writeMpsNative(buffer, const_cast<const char**>(rownames), const_cast<const char**>(colnames), 2, 2, 1.0);

      if( nameDiscipline == 2 )
      {
         for( int i = 0; i < gmoN(gmo); ++i )
            free(colnames[i]);
         for( int i = 0; i < gmoM(gmo)+1; ++i )
            free(rownames[i]);
         delete[] colnames;
         delete[] rownames;
      }
   }
}
