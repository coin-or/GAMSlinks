// Copyright (C) GAMS Development and others 2007-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsOSxL.hpp"
#include "GamsNLinstr.h"
#include "GAMSlinksConfig.h"

#include "OSInstance.h"
#include "OSResult.h"
#include "OSrLReader.h"
#include "OSErrorClass.h"
#include "CoinHelperFunctions.hpp"

#include <cassert>
#include <cmath>

#include "gmomcc.h"
#include "gevmcc.h"

#include "GamsCompatibility.h"

GamsOSxL::~GamsOSxL()
{
   delete osinstance;

   if( gmo_is_our )
   {
      gmoFree(&gmo);
      gevFree(&gev);
   }
}

void GamsOSxL::setGMO(
   struct gmoRec*     gmo_                /**< GAMS modeling object */
)
{
   assert(gmo == NULL);
   assert(gev == NULL);
   assert(osinstance == NULL);
   assert(gmo_is_our == false);
   assert(gmo_ != NULL);

   gmo = gmo_;
   gev = (gevRec*)gmoEnvironment(gmo);
}

bool GamsOSxL::initGMO(
   const char*        datfile             /**< name of file with compiled GAMS model */
)
{
   assert(gmo == NULL);
   assert(gev == NULL);
   assert(osinstance == NULL);

   char msg[1024];
   int rc;

   if( !gmoCreate(&gmo, msg, sizeof(msg)) )
   {
      fprintf(stderr, "%s\n",msg);
      return false;
   }
   gmo_is_our = true;

   if( !gevCreate(&gev, msg, sizeof(msg)) )
   {
      fprintf(stderr, "%s\n",msg);
      return false;
   }

   // load control file
   if( (rc = gevInitEnvironmentLegacy(gev, datfile)) != 0 )
   {
      fprintf(stderr, "Could not init gams environment: %s error code %d\n", datfile, rc);
      gmoFree(&gmo);
      gevFree(&gev);
      return false;
   }

   if( (rc = gmoRegisterEnvironment(gmo, gev, msg)) != 0 )
   {
      gevLogStat(gev, "Could not register environment.");
      gmoFree(&gmo);
      gevFree(&gev);
      return false;
   }

   if( (rc = gmoLoadDataLegacy(gmo, msg)) != 0 )
   {
      gevLogStat(gev, "Could not load model data.");
      gmoFree(&gmo);
      gevFree(&gev);
      return false;
   }

   gmoMinfSet(gmo, -OSDBL_MAX);
   gmoPinfSet(gmo,  OSDBL_MAX);
   gmoObjReformSet(gmo, 1);
   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoIndexBaseSet(gmo, 0);

   return true;
}

bool GamsOSxL::createOSInstance()
{
   char buffer[GMS_SSSIZE];

   try
   {
      // delete possible old instance and create a new one
      delete osinstance;
      osinstance = new OSInstance();

      gmoNameInput(gmo, buffer);
      osinstance->setInstanceName(buffer);

      // setup variables
      osinstance->setVariableNumber(gmoN(gmo));

      char*        vartypes = new char[gmoN(gmo)];
      std::string* varnames = new std::string[gmoN(gmo)];
      for( int i = 0; i < gmoN(gmo); ++i )
      {
         switch( gmoGetVarTypeOne(gmo, i) )
         {
            case gmovar_X:
               vartypes[i] = 'C';
               break;
            case gmovar_B:
               vartypes[i] = 'B';
               break;
            case gmovar_I:
               vartypes[i] = 'I';
               break;
            case gmovar_SC:
               vartypes[i] = 'D';
               break;
            case gmovar_SI:
               vartypes[i] = 'J';
               break;
            case gmovar_S1:
            case gmovar_S2:
               vartypes[i] = 'C';
               // ignore SOS1, SOS2 for now; we should only get here if called by convert, in which case we do something special
               break;
            default:
            {
               gevLogStat(gev, "Error: Unsupported variable type.");
               return false;
            }
         }
         if( gmoDict(gmo) )
            gmoGetVarNameOne(gmo, i, buffer);
         else
            sprintf(buffer, "x%08d", i);
         varnames[i] = buffer;
      }

      double* varlow = new double[gmoN(gmo)];
      double* varup  = new double[gmoN(gmo)];
      gmoGetVarLower(gmo, varlow);
      gmoGetVarUpper(gmo, varup);

      if( gmoN(gmo) > 0 && !osinstance->setVariables(gmoN(gmo), varnames, varlow, varup, vartypes) )
      {
         gevLogStat(gev, "Error: OSInstance::setVariables did not succeed.\n");
         return false;
      }

      delete[] vartypes;
      delete[] varnames;
      delete[] varlow;
      delete[] varup;

      // setup linear part of objective, if any
      if( gmoModelType(gmo) == gmoProc_cns )
      {
         // no objective in constraint satisfaction models
         osinstance->setObjectiveNumber(0);
      }
      else
      {
         osinstance->setObjectiveNumber(1);

         SparseVector* objectiveCoefficients = NULL;

         if( gmoN(gmo) > 0 )
         {
            objectiveCoefficients = new SparseVector(gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

            int* colidx = new int[gmoObjNZ(gmo)];
            double* val = new double[gmoObjNZ(gmo)];
            int* nlflag = new int[gmoObjNZ(gmo)];
            int dummy;

            //         if( gmoObjNZ(gmo) > 0 )
            //            nlflag[0] = 0; // workaround for gmo bug
            gmoGetObjSparse(gmo, colidx, val, nlflag, &dummy, &dummy);
            for( int i = 0, j = 0; i < gmoObjNZ(gmo); ++i )
            {
               if( nlflag[i] )
                  continue;
               objectiveCoefficients->indexes[j] = colidx[i];
               objectiveCoefficients->values[j]  = val[i];
               j++;
               assert(j <= gmoObjNZ(gmo) - gmoObjNLNZ(gmo));
            }

            delete[] colidx;
            delete[] val;
            delete[] nlflag;
         }
         else
         {
            objectiveCoefficients = new SparseVector(0);
         }

         if( gmoDict(gmo) )
            gmoGetObjName(gmo, buffer);
         else
            strcpy(buffer, "objective");

         if( !osinstance->addObjective(-1, buffer, gmoSense(gmo) == gmoObj_Min ? "min" : "max", gmoObjConst(gmo), 1.0, objectiveCoefficients) )
         {
            delete objectiveCoefficients;
            gevLogStat(gev, "Error: OSInstance::addObjective did not succeed.\n");
            return false;
         }
         delete objectiveCoefficients;
      }

      // setup linear part of constraints
      osinstance->setConstraintNumber(gmoM(gmo));

      double lb;
      double ub;
      for( int i = 0; i < gmoM(gmo); ++i )
      {
         switch( gmoGetEquTypeOne(gmo, i) )
         {
            case gmoequ_E:
               lb = ub = gmoGetRhsOne(gmo, i);
               break;

            case gmoequ_L:
               lb = -OSDBL_MAX;
               ub =  gmoGetRhsOne(gmo, i);
               break;

            case gmoequ_G:
               lb = gmoGetRhsOne(gmo, i);
               ub = OSDBL_MAX;
               break;

            case gmoequ_N:
               lb = -OSDBL_MAX;
               ub =  OSDBL_MAX;
               break;

            default:
               gevLogStat(gev, "Error: Unknown row type. Exiting ...");
               return false;
         }

         if( gmoDict(gmo) )
            gmoGetEquNameOne(gmo, i, buffer);
         else
            sprintf(buffer, "e%08d", i);

         if( !osinstance->addConstraint(i, buffer, lb, ub, 0.0) )
         {
            gevLogStat(gev, "Error: OSInstance::addConstraint did not succeed.\n");
            return false;
         }
      }

      int nz = gmoNZ(gmo);
      double* values  = new double[nz];
      int* colstarts  = new int[gmoN(gmo)+1];
      int* rowindexes = new int[nz];
      int* nlflags    = new int[nz];

      // get coefficients matrix
      gmoGetMatrixCol(gmo, colstarts, rowindexes, values, nlflags);
      colstarts[gmoN(gmo)] = nz;

      // remove coefficients corresponding to nonlinear terms
      int shift = 0;
      for( int col = 0; col < gmoN(gmo); ++col )
      {
         colstarts[col+1] -= shift;
         for( int k = colstarts[col]; k < colstarts[col+1]; )
         {
            values[k] = values[k+shift];
            rowindexes[k] = rowindexes[k+shift];
            if( nlflags[k+shift] )
            {
               ++shift;
               --colstarts[col+1];
            }
            else
               ++k;
         }
      }
      nz -= shift;

      // values, colstarts, rowindexes are deleted by OSInstance
      delete[] nlflags;

      if( !osinstance->setLinearConstraintCoefficients(nz, true, values, 0, nz-1, rowindexes, 0, nz-1, colstarts, 0, gmoN(gmo)) )
      {
         gevLogStat(gev, "Error: OSInstance::setLinearConstraintCoefficients did not succeed.\n");
         return false;
      }

      // if everything linear, then we're finished
      if( gmoObjNLNZ(gmo) == 0 && gmoNLNZ(gmo) == 0 )
         return true;

      // setup nonlinear expressions
      osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions = gmoNLM(gmo) + (gmoObjNLNZ(gmo) ? 1 : 0);
      osinstance->instanceData->nonlinearExpressions->nl = CoinCopyOfArrayOrZero((Nl**)NULL, osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);
      int iNLidx = 0;

      int* opcodes = new int[gmoMaxSingleFNL(gmo)+1];
      int* fields  = new int[gmoMaxSingleFNL(gmo)+1];
      int constantlen = gmoNLConst(gmo);
      double* constants = (double*)gmoPPool(gmo);
      int codelen;

      OSnLNode* nl;
      // parse nonlinear objective
      if( gmoObjNLNZ(gmo) > 0 )
      {
         gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);

         nl = parseGamsInstructions(codelen, opcodes, fields, constantlen, constants);
         if( nl == NULL )
         {
            gevLogStat(gev, "Error: failure when parsing GAMS instructions.\n");
            return false;
         }

         double objjacval = gmoObjJacVal(gmo);
         if( objjacval == 1.0 )
         {
            // scale by -1/objjacval = negate
            OSnLNode* negnode = new OSnLNodeNegate;
            negnode->m_mChildren[0] = nl;
            nl = negnode;
         }
         else if( objjacval != -1.0 )
         {
            // scale by -1/objjacval
            OSnLNodeNumber* numbernode = new OSnLNodeNumber();
            numbernode->value = -1/objjacval;
            OSnLNodeTimes* timesnode = new OSnLNodeTimes();
            timesnode->m_mChildren[0] = nl;
            timesnode->m_mChildren[1] = numbernode;
            nl = timesnode;
         }

         assert(iNLidx < osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx] = new Nl();
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx]->idx = -1;
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx]->osExpressionTree = new OSExpressionTree();
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx]->osExpressionTree->m_treeRoot = nl;
         ++iNLidx;
      }

      for( int i = 0; i < gmoM(gmo); ++i )
      {
         gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields);
         if( codelen == 0 )
            continue;

         nl = parseGamsInstructions(codelen, opcodes, fields, constantlen, constants);
         if( nl == NULL )
         {
            gevLogStat(gev, "Error: failure when parsing GAMS instructions.\n");
            return false;
         }

         assert(iNLidx < osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx] = new Nl();
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx]->idx = i; // correct that this is the con. number?
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx]->osExpressionTree = new OSExpressionTree();
         osinstance->instanceData->nonlinearExpressions->nl[iNLidx]->osExpressionTree->m_treeRoot = nl;
         ++iNLidx;
      }

      assert(iNLidx == osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);

   }
   catch( ErrorClass& error )
   {
      gevLogStat(gev, "Error creating the instance. Error message: ");
      gevLogStatPChar(gev, error.errormsg.c_str());
      return false;
   }

   return true;
}

OSnLNode* GamsOSxL::parseGamsInstructions(
   int                codelen,            /**< length of GAMS instructions */
   int*               opcodes,            /**< opcodes of GAMS instructions */
   int*               fields,             /**< fields of GAMS instructions */
   int                constantlen,        /**< length of GAMS constants pool */
   double*            constants           /**< GAMS constants pool */
)
{
   bool debugoutput = gevGetIntOpt(gev, gevInteger1) & 0x4;
#define debugout if( debugoutput ) std::clog

   std::vector<OSnLNode*> nlNodeVec;

   nlNodeVec.reserve(codelen);

   int nargs = -1;

   for( int i = 0; i < codelen; ++i )
   {
      GamsOpCode opcode = (GamsOpCode)opcodes[i];
      int address = fields[i]-1;

      debugout << '\t' << GamsOpCodeName[opcode] << ": ";
      switch( opcode )
      {
         case nlNoOp:   // no operation
         case nlStore:  // store row
         case nlHeader: // header
         {
            debugout << "ignored" << std::endl;
            break;
         }

         case nlPushV: // push variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "push variable " << osinstance->getVariableNames()[address] << std::endl;
            OSnLNodeVariable* nlNode = new OSnLNodeVariable();
            nlNode->idx = address;
            nlNodeVec.push_back(nlNode);
            break;
         }

         case nlPushI: // push constant
         {
            debugout << "push constant " << constants[address] << std::endl;
            OSnLNodeNumber* nlNode = new OSnLNodeNumber();
            nlNode->value = constants[address];
            nlNodeVec.push_back(nlNode);
            break;
         }

         case nlPushZero: // push zero
         {
            debugout << "push constant zero" << std::endl;
            nlNodeVec.push_back(new OSnLNodeNumber());
            break;
         }

         case nlAdd: // add
         {
            debugout << "add" << std::endl;
            nlNodeVec.push_back(new OSnLNodePlus());
            break;
         }

         case nlAddV: // add variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "add variable " << osinstance->getVariableNames()[address] << std::endl;
            OSnLNodeVariable* nlNode = new OSnLNodeVariable();
            nlNode->idx = address;
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodePlus());
            break;
         }

         case nlAddI: // add immediate
         {
            debugout << "add constant " << constants[address] << std::endl;
            OSnLNodeNumber* nlNode = new OSnLNodeNumber();
            nlNode->value = constants[address];
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodePlus());
            break;
         }

         case nlSub: // minus
         {
            debugout << "minus" << std::endl;
            nlNodeVec.push_back(new OSnLNodeMinus());
            break;
         }

         case nlSubV: // subtract variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "subtract variable " << osinstance->getVariableNames()[address] << std::endl;
            OSnLNodeVariable* nlNode = new OSnLNodeVariable();
            nlNode->idx = address;
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodeMinus());
            break;
         }

         case nlSubI: // subtract immediate
         {
            debugout << "subtract constant " << constants[address] << std::endl;
            OSnLNodeNumber* nlNode = new OSnLNodeNumber();
            nlNode->value = constants[address];
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodeMinus());
            break;
         }

         case nlMul: // multiply
         {
            debugout << "multiply" << std::endl;
            nlNodeVec.push_back(new OSnLNodeTimes());
            break;
         }

         case nlMulV: // multiply variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "multiply variable " << osinstance->getVariableNames()[address] << std::endl;
            OSnLNodeVariable* nlNode = new OSnLNodeVariable();
            nlNode->idx = address;
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodeTimes());
            break;
         }

         case nlMulI: // multiply immediate
         {
            debugout << "multiply constant " << constants[address] << std::endl;
            OSnLNodeNumber* nlNode = new OSnLNodeNumber();
            nlNode->value = constants[address];
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodeTimes());
            break;
         }

         case nlMulIAdd: // multiply immediate and add
         {
            debugout << "multiply constant " << constants[address] << " and add " << std::endl;
            OSnLNodeNumber* nlNode = new OSnLNodeNumber();
            nlNode->value = constants[address];
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodeTimes());
            nlNodeVec.push_back(new OSnLNodePlus());
            break;
         }

         case nlDiv: // divide
         {
            debugout << "divide" << std::endl;
            nlNodeVec.push_back(new OSnLNodeDivide());
            break;
         }

         case nlDivV: // divide variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "divide variable " << osinstance->getVariableNames()[address] << std::endl;
            OSnLNodeVariable* nlNode = new OSnLNodeVariable();
            nlNode->idx = address;
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodeDivide());
            break;
         }

         case nlDivI: // divide immediate
         {
            debugout << "divide constant " << constants[address] << std::endl;
            OSnLNodeNumber* nlNode = new OSnLNodeNumber();
            nlNode->value = constants[address];
            nlNodeVec.push_back(nlNode);
            nlNodeVec.push_back(new OSnLNodeDivide());
            break;
         }

         case nlUMin: // unary minus
         {
            debugout << "negate" << std::endl;
            nlNodeVec.push_back(new OSnLNodeNegate());
            break;
         }

         case nlUMinV: // unary minus variable
         {
            address = gmoGetjSolver(gmo, address);
            debugout << "push negated variable " << osinstance->getVariableNames()[address] << std::endl;
            OSnLNodeVariable* nlNode = new OSnLNodeVariable();
            nlNode->idx = address;
            nlNode->coef = -1.0;
            nlNodeVec.push_back( nlNode );
            break;
         }

         case nlFuncArgN: // number of function arguments
         {
            nargs = address;
            debugout << nargs << "arguments" << std::endl;
            break;
         }

         case nlCallArg1 :
         case nlCallArg2 :
         case nlCallArgN :
         {
            debugout << "call function ";

            switch( GamsFuncCode(address+1) )  // undo shift by 1
            {
               case fnmin:
               {
                  debugout << "min" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeMin());
                  nlNodeVec.back()->inumberOfChildren = 2;
                  nlNodeVec.back()->m_mChildren = new OSnLNode*[2];
                  break;
               }

               case fnmax :
               {
                  debugout << "max" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeMax());
                  nlNodeVec.back()->inumberOfChildren = 2;
                  nlNodeVec.back()->m_mChildren = new OSnLNode*[2];
                  break;
               }

               case fnsqr:
               {
                  debugout << "square" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeSquare());
                  break;
               }

               case fnexp:
               case fnslexp:
               case fnsqexp:
               {
                  debugout << "exp" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeExp());
                  break;
               }

               case fnlog:
               {
                  debugout << "ln" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeLn());
                  break;
               }

               case fnlog10:
               case fnsllog10:
               case fnsqlog10:
               {
                  debugout << "log10 = ln * 1/ln(10)" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeLn());
                  OSnLNodeNumber* nlNode = new OSnLNodeNumber();
                  nlNode->value = 1.0/log(10.0);
                  nlNodeVec.push_back(nlNode);
                  nlNodeVec.push_back(new OSnLNodeTimes());
                  break;
               }

               case fnlog2:
               {
                  debugout << "log2 = ln * 1/ln(2)" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeLn());
                  OSnLNodeNumber *nlNode = new OSnLNodeNumber();
                  nlNode->value = 1.0/log(2.0);
                  nlNodeVec.push_back(nlNode);
                  nlNodeVec.push_back(new OSnLNodeTimes());
                  break;
               }

               case fnsqrt:
               {
                  debugout << "sqrt" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeSqrt());
                  break;
               }

               case fnabs:
               {
                  debugout << "abs" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeAbs());
                  break;
               }

               case fncos:
               {
                  debugout << "cos" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeCos());
                  break;
               }

               case fnsin:
               {
                  debugout << "sin" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeSin());
                  break;
               }

               case fnpower:
               case fnrpower: // x ^ y
               case fncvpower: // constant ^ x
               case fnvcpower: // x ^ constant
               {
                  debugout << "power" << std::endl;
                  nlNodeVec.push_back(new OSnLNodePower());
                  break;
               }

               case fnpi:
               {
                  debugout << "pi" << std::endl;
                  nlNodeVec.push_back(new OSnLNodePI());
                  break;
               }

               case fndiv:
               case fndiv0:
               {
                  debugout << "divide" << std::endl;
                  nlNodeVec.push_back(new OSnLNodeDivide());
                  break;
               }

               case fnslrec: // 1/x
               case fnsqrec: // 1/x
               {
                  debugout << "reciprocal" << std::endl;
                  OSnLNodeNumber *nlNode = new OSnLNodeNumber();
                  nlNode->value = 1.0;
                  nlNodeVec.push_back( nlNode );
                  nlNodeVec.push_back( new OSnLNodeDivide() );
                  break;
               }

               case fnpoly: // simple polynomial
               {
                  debugout << "polynomial of degree " << nargs-1 << std::endl;
                  assert(nargs >= 0);
                  switch( nargs )
                  {
                     case 0:
                     {
                        // delete variable of polynom
                        delete nlNodeVec.back();
                        nlNodeVec.pop_back();
                        // push zero
                        nlNodeVec.push_back(new OSnLNodeNumber());
                        break;
                     }

                     case 1: // "constant" polynomial
                     {
                        assert(nlNodeVec.size() >= 2);
                        // delete variable
                        delete nlNodeVec[nlNodeVec.size()-2];
                        // put constant there
                        nlNodeVec[nlNodeVec.size()-2] = nlNodeVec.back();
                        // forget last element
                        nlNodeVec.pop_back();
                        break;
                     }

                     default: // polynomial is at least linear
                     {
                        std::vector<OSnLNode*> coeff(nargs);
                        while( nargs > 0 )
                        {
                           assert(!nlNodeVec.empty());
                           coeff[nargs-1] = nlNodeVec.back();
                           nlNodeVec.pop_back();
                           --nargs;
                        }
                        assert(!nlNodeVec.empty());
                        OSnLNode* var = nlNodeVec.back();
                        nlNodeVec.pop_back();

                        nlNodeVec.push_back(coeff[0]);

                        nlNodeVec.push_back(coeff[1]);
                        nlNodeVec.push_back(var);
                        nlNodeVec.push_back(new OSnLNodeTimes());

                        nlNodeVec.push_back(new OSnLNodePlus());
                        for( size_t i = 2; i < coeff.size(); ++i )
                        {
                           nlNodeVec.push_back(coeff[i]);
                           nlNodeVec.push_back(var);
                           if( i == 2 )
                           {
                              nlNodeVec.push_back(new OSnLNodeSquare());
                           }
                           else
                           {
                              OSnLNodeNumber* exponent = new OSnLNodeNumber();
                              exponent->value = (double)i;
                              nlNodeVec.push_back(exponent);
                              nlNodeVec.push_back(new OSnLNodePower());
                           }
                           nlNodeVec.push_back(new OSnLNodeTimes());
                           nlNodeVec.push_back(new OSnLNodePlus());
                        }
                     }
                  }
                  nargs = -1;
                  break;
               }

               // TODO some more we could handle
               case fnceil: case fnfloor: case fnround:
               case fnmod: case fntrunc: case fnsign:
               case fnarctan: case fnerrf: case fndunfm:
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
               case fntan: case fnarccos:
               case fnarcsin: case fnarctan2 /* arctan(x2/x1) */:
               default:
               {
                  debugout << "nr. " << address+1 << " - unsuppored. Error." << std::endl;
                  gevLogStat(gev, "Error: Unsupported GAMS function.\n");
                  while( !nlNodeVec.empty() )
                  {
                     delete nlNodeVec.back();
                     nlNodeVec.pop_back();
                  }
                  return NULL;
               }
            }
            break;
         }

         default:
         {
            debugout << "opcode " << opcode << " - unsuppored. Error." << std::endl;
            gevLogStat(gev, "Error: Unsupported GAMS instruction.\n");
            while( !nlNodeVec.empty() )
            {
               delete nlNodeVec.back();
               nlNodeVec.pop_back();
            }
            return NULL;
         }
      }
   }

   if( nlNodeVec.size() == 0 )
      return NULL;

   // the vector is in postfix format - create expression tree and return it
   return nlNodeVec[0]->createExpressionTreeFromPostfix(nlNodeVec);
#undef debugout
}

void GamsOSxL::writeSolution(
   OSResult&          osresult            /**< optimization result as OSResult object */
)
{
   if( osresult.general == NULL )
   {
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
      gevLogStat(gev, "Error: OS result does not have header.");
      return;
   }
   else if( osresult.getGeneralStatusType() == "error" )
   {
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      gmoSolveStatSet(gmo, gmoSolveStat_SolverErr);
      gevLogStatPChar(gev, "Error: OS result reports error: ");
      gevLogStatPChar(gev, osresult.getGeneralMessage().c_str());
      gevLogStat(gev, "");
      return;
   }
   else if( osresult.getGeneralStatusType() == "warning" )
   {
      gevLogStatPChar(gev, "Warning: OS result reports warning: ");
      gevLogStatPChar(gev, osresult.getGeneralMessage().c_str());
      gevLogStat(gev, "");
   }

   gmoSolveStatSet(gmo, gmoSolveStat_Normal);

   if( osresult.getSolutionNumber() == 0 )
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
   else if( osresult.getSolutionStatusType(0) == "unbounded" )
      gmoModelStatSet(gmo, gmoModelStat_Unbounded);
   else if( osresult.getSolutionStatusType(0) == "globallyOptimal" )
      gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
   else if( osresult.getSolutionStatusType(0) == "locallyOptimal" )
      gmoModelStatSet(gmo, gmoModelStat_OptimalLocal);
   else if( osresult.getSolutionStatusType(0) == "optimal" )
      gmoModelStatSet(gmo, gmoModelStat_OptimalGlobal);
   else if( osresult.getSolutionStatusType(0) == "bestSoFar" )
      gmoModelStatSet(gmo, gmoModelStat_Feasible); // or should we report integer solution if integer var.?
   else if( osresult.getSolutionStatusType(0) == "feasible" )
      gmoModelStatSet(gmo, gmoModelStat_Feasible); // or should we report integer solution if integer var.?
   else if( osresult.getSolutionStatusType(0) == "infeasible" )
      gmoModelStatSet(gmo, gmoModelStat_InfeasibleGlobal);
   else if( osresult.getSolutionStatusType(0) == "stoppedByLimit" )
   {
      gmoSolveStatSet(gmo, gmoSolveStat_Iteration); // just a guess
      gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
   }
   else if (osresult.getSolutionStatusType(0) == "unsure" )
      gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
   else if (osresult.getSolutionStatusType(0) == "error" )
      gmoModelStatSet(gmo, gmoModelStat_ErrorUnknown);
   else if (osresult.getSolutionStatusType(0) == "other" )
      gmoModelStatSet(gmo, gmoModelStat_InfeasibleIntermed);
   else
      gmoModelStatSet(gmo, gmoModelStat_ErrorUnknown);

   if( osresult.getVariableNumber() != gmoN(gmo) )
   {
      gevLogStat(gev, "Error: Number of variables in OS result does not match with gams model.");
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      return;
   }
   if( osresult.getConstraintNumber() != gmoM(gmo) )
   {
      gevLogStat(gev, "Error: Number of constraints in OS result does not match with gams model.");
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      return;
   }

   // TODO some more statistics we can get from OSrL?
   gmoSetHeadnTail(gmo, gmoHresused, osresult.getTimeValue());

   if( osresult.getSolutionNumber() == 0 )
      return;

   OptimizationSolution* sol = osresult.optimization->solution[0];
   assert(sol != NULL);

   int* colBasStat = CoinCopyOfArray((int*)NULL, gmoN(gmo), (int)gmoBstat_Super);
   double* colMarg = CoinCopyOfArray((double*)NULL, gmoN(gmo), gmoValNA(gmo));
   double* colLev  = CoinCopyOfArray((double*)NULL, gmoN(gmo), gmoValNA(gmo));

   int* rowBasStat = CoinCopyOfArray((int*)NULL, gmoM(gmo), (int)gmoBstat_Super);
   double* rowLev  = CoinCopyOfArray((double*)NULL, gmoM(gmo), gmoValNA(gmo));
   double* rowMarg = CoinCopyOfArray((double*)NULL, gmoM(gmo), gmoValNA(gmo));

   // TODO use get-functions

   // TODO constraint values
   //  if (sol->constraints && sol->constraints->values) // set row levels, if available
   //    for (std::vector<ConValue*>::iterator it(sol->constraints->values->con.begin());
   //    it!=sol->constraints->values->con.end(); ++it)
   //      rowLev[(*it)->idx]=(*it)->value;

   // set row dual values, if available
   if( sol->constraints != NULL && sol->constraints->dualValues != NULL )
      for( int i = 0; i < sol->constraints->dualValues->numberOfCon; ++i )
         rowMarg[sol->constraints->dualValues->con[i]->idx] = sol->constraints->dualValues->con[i]->value;

   // set var values, if available
   if( sol->variables != NULL && sol->variables->values != NULL )
      for( int i = 0; i < sol->variables->values->numberOfVar; ++i )
         colLev[sol->variables->values->var[i]->idx] = sol->variables->values->var[i]->value;

   if( sol->variables != NULL )
      for( int i = 0; i < sol->variables->numberOfOtherVariableResults; ++i )
         if( sol->variables->other[i]->name == "reduced costs" )
         {
            for( int j = 0; j < sol->variables->other[i]->numberOfVar; ++j )
               colMarg[sol->variables->other[i]->var[j]->idx] = atof(sol->variables->other[i]->var[j]->value.c_str());
            break;
         }

   // TODO setup basis status

   gmoSetSolution8(gmo, colLev, colMarg, rowMarg, rowLev, colBasStat, NULL, rowBasStat, NULL);

   if( gmoModelType(gmo) == gmoProc_cns )
      switch( gmoModelStat(gmo) )
      {
         case gmoModelStat_OptimalGlobal:
         case gmoModelStat_OptimalLocal:
         case gmoModelStat_Feasible:
         case gmoModelStat_Integer:
            gmoModelStatSet(gmo, gmoModelStat_Solved);
      }

   delete[] rowLev;
   delete[] rowMarg;
   delete[] rowBasStat;
   delete[] colLev;
   delete[] colMarg;
   delete[] colBasStat;
}

void GamsOSxL::writeSolution(
   std::string&       osrl                /**< optimization result as string in OSrL format */
)
{
   OSResult* osresult = NULL;
   OSrLReader osrl_reader;

   try
   {
      osresult = osrl_reader.readOSrL(osrl);
   }
   catch( const ErrorClass& error )
   {
      gevLogStat(gev, "Error parsing the OS result string: ");
      gevLogStatPChar(gev, error.errormsg.c_str());
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      return;
   }

   if( osresult != NULL )
      writeSolution(*osresult);
}
