// Copyright (C) 2008 GAMS Development
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Lutz Westermann

#include <iostream>
#include <fstream>
#include <string>
#include <list>

using namespace std;

#include "OSInstance.h"
#include "OSiLReader.h"
#include <OSCommonUtil.h>

void walk(OSnLNode* node, list<char>& str, list<char>::iterator pos) {
   switch (node->inodeInt) {
      case OS_PLUS : {
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, '+');
         walk(node->m_mChildren[1], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_SUM : {
         if(node->inumberOfChildren<2) {
            cout << " Need more children for OSAnlNodeSum!" << endl;
         } else {
            str.insert(pos, '(');
            walk(node->m_mChildren[0], str, pos);
            for(int i=1;i<node->inumberOfChildren;i++) {
               str.insert(pos, '+');
               walk(node->m_mChildren[i], str, pos);
            }
         str.insert(pos, ')');
         str.insert(pos, '\n');
         }
         break;
      }
      case OS_MINUS : {
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, '-');
         walk(node->m_mChildren[1], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_NEGATE : {
         str.insert(pos, '(');
         str.insert(pos, '-');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_TIMES : {
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, '*');
         walk(node->m_mChildren[1], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_DIVIDE : {
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, '/');
         walk(node->m_mChildren[1], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_POWER : {
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, '*');
         str.insert(pos, '*');
         walk(node->m_mChildren[1], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_PRODUCT : {
         if(node->inumberOfChildren<2) {
            cout << " Need more children for OSAnlNodeProduct!" << endl;
         } else {
            str.insert(pos, '(');
            walk(node->m_mChildren[0], str, pos);
            for(int i=1;i<node->inumberOfChildren;i++) {
               str.insert(pos, '*');
               walk(node->m_mChildren[i], str, pos);
            }
            str.insert(pos, ')');
         }
         str.insert(pos, '\n');
         break;
      }
      case OS_ABS : {
         str.insert(pos, 'a');
         str.insert(pos, 'b');
         str.insert(pos, 's');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_SQUARE : {
         str.insert(pos, 's');
         str.insert(pos, 'q');
         str.insert(pos, 'r');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_SQRT : {
         str.insert(pos, 's');
         str.insert(pos, 'q');
         str.insert(pos, 'r');
         str.insert(pos, 't');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_LN : {
         str.insert(pos, 'l');
         str.insert(pos, 'o');
         str.insert(pos, 'g');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_EXP : {
         str.insert(pos, 'e');
         str.insert(pos, 'x');
         str.insert(pos, 'p');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_SIN : {
         str.insert(pos, 's');
         str.insert(pos, 'i');
         str.insert(pos, 'n');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_COS : {
         str.insert(pos, 'c');
         str.insert(pos, 'o');
         str.insert(pos, 's');
         str.insert(pos, '(');
         walk(node->m_mChildren[0], str, pos);
         str.insert(pos, ')');
         str.insert(pos, '\n');
         break;
      }
      case OS_MIN : {
         if(node->inumberOfChildren<2) {
            cout << " Need more children for OSAnlNodeMin!" << endl;
         } else {
            str.insert(pos, 'm');
            str.insert(pos, 'i');
            str.insert(pos, 'n');
            str.insert(pos, '(');
            walk(node->m_mChildren[0], str, pos);
            for(int i=1;i<node->inumberOfChildren;i++) {
               str.insert(pos, ',');
               walk(node->m_mChildren[i], str, pos);
            }
            str.insert(pos, ')');
         }
         str.insert(pos, '\n');
         break;
      }
      case OS_MAX : {
         if(node->inumberOfChildren<2) {
            cout << " Need more children for OSAnlNodeMax!" << endl;
         } else {
            str.insert(pos, 'm');
            str.insert(pos, 'a');
            str.insert(pos, 'x');
            str.insert(pos, '(');
            walk(node->m_mChildren[0], str, pos);
            for(int i=1;i<node->inumberOfChildren;i++) {
               str.insert(pos, ',');
               walk(node->m_mChildren[i], str, pos);
            }
            str.insert(pos, ')');
         }
         str.insert(pos, '\n');
         break;
      }
      case OS_NUMBER : {
         OSnLNodeNumber* numnode = (OSnLNodeNumber*)node;
         stringstream strstr;
         strstr << numnode->value;
         string strst(strstr.str());
         str.insert(pos, strst.begin(), strst.end());
         break;
      }
      case OS_PI : {
         str.insert(pos, 'p');
         str.insert(pos, 'i');
         break;
      }
      case OS_VARIABLE : {
         OSnLNodeVariable* varnode = (OSnLNodeVariable*)node;
         stringstream strstr;
         strstr << varnode->coef << "*x" << varnode->idx;
         string strst(strstr.str());
         str.insert(pos, strst.begin(), strst.end());
         break;
      }
      default:
         cout << node->snodeName << " NOT IMPLEMENTED!!" << endl;
         break;
   }
}

int main(int argc, char** argv) {
   if(argc != 3) {
      cout << "USAGE: " << argv[0] << " INPUTFILE OUTPUTFILE" << endl;
      return 1;
   }
   ifstream osilfile(argv[1]);
   if(!osilfile) {
      cout << "Cannot open file " << argv[1] << endl;
      return 1;
   }
   ofstream out(argv[2]);
   if(!out) {
      cout << "Cannot open file " << argv[2] << endl;
      return 1;
   }

   OSiLReader reader;
   long begin,end;
   begin = osilfile.tellg();
   osilfile.seekg (0, ios::end);
   end   = osilfile.tellg();
   osilfile.seekg (0, ios::beg);
   
   char *osilstr = NULL;
   osilstr       = new char[end-begin];
   osilfile.getline(osilstr, end-begin);
   
   OSInstance* osinstance = reader.readOSiL(osilstr);

   string mdlType = "";
   char* varTypes = osinstance->getVariableTypes();
//   double* varLvl = osinstance->getVariableInitialValues();
   double* varLB  = osinstance->getVariableLowerBounds();
   double* varUB  = osinstance->getVariableUpperBounds();
   char* conTypes = osinstance->getConstraintTypes();
   double* conLB  = osinstance->getConstraintLowerBounds();
   double* conUB  = osinstance->getConstraintUpperBounds();
   double* objCon = osinstance->getObjectiveConstants();
   int conObj     = 0;
   int IntCnt     = 0;
   int BinCnt     = 0;
   int nDefVar    = 0;
   int* conindex  = NULL;

   if(osinstance->getObjectiveNumber() > 1) {
      cout << "*** Cannot handle more than one objective equation!" << endl;
      return 0;
   }

   conindex = new int[osinstance->getConstraintNumber() + 1];

   if(osinstance->getNumberOfBinaryVariables() + osinstance->getNumberOfIntegerVariables() > 0) {
      if(osinstance->getNumberOfNonlinearExpressions() > 0)
         mdlType = "MINLP";
      else
         mdlType = "MIP";
   } else {
      if(osinstance->getNumberOfNonlinearExpressions() > 0)
         mdlType = "NLP";
      else
         mdlType = "LP";
   }

   out << "Equations eqObj";
   for(int j=0; j<osinstance->getConstraintNumber(); j++) {
      conindex[j] = 0;
      out << ", eq" << j << endl;
   }
   out << ";" << endl << endl;

   out << "Variables Obj";
   for(int i=0; i<osinstance->getVariableNumber(); i++)
      if(varTypes[i] == 'C') out << ", x" << i << endl;
   out << ";" << endl << endl;

   if(osinstance->getNumberOfBinaryVariables() > 0) {
      out << "Binary Variables ";
      for(int i=0; i<osinstance->getVariableNumber(); i++)
         if(varTypes[i] == 'B') {
            out << "x" << i;
            BinCnt++;
            if(BinCnt != osinstance->getNumberOfBinaryVariables()) out << ", " << endl;
         }
      out << ";" << endl << endl;
   }

   if(osinstance->getNumberOfIntegerVariables() > 0) {
      out << "Integer Variables ";
      for(int i=0; i<osinstance->getVariableNumber(); i++)
         if(varTypes[i] == 'I') {
            out << "x" << i;
            IntCnt++;
            if(IntCnt != osinstance->getNumberOfIntegerVariables()) out << ", " << endl;
         }
      out << ";" << endl << endl;
   }

   for(int i=0; i<osinstance->getVariableNumber(); i++) {
      nDefVar = 0;
      if(varTypes[i] == 'C' && varLB[i] != -OSDBL_MAX || varTypes[i] == 'B' && varLB[i] != 0) {
         out <<  "x" << i << ".lo = " << varLB[i] << "; " << endl;
         nDefVar = 1;
      }
/* Initial values need to be reworked in OS Options
      if(CommonUtil::ISOSNAN(osinstance->instanceData->variables->var[i]->init) != true) {
         out <<  "x" << i << ".l = " << osinstance->instanceData->variables->var[i]->init << ";";
         nDefVar = 1;
      } */
      if(varTypes[i] == 'C' && varUB[i] != OSDBL_MAX || varTypes[i] == 'B' && varUB[i] != 1)  {
         out << " x" << i << ".up = " << varUB[i] << ";" << endl;
         nDefVar = 1;
      }
      if(nDefVar == 1) out << endl;
   }
   out << endl;

   map<int, OSExpressionTree*> allexptrees(osinstance->getAllNonlinearExpressionTreesMod());
   for(std::map<int,OSExpressionTree*>::iterator exptreeit(allexptrees.begin());
      exptreeit != allexptrees.end(); ++exptreeit) {

      int connumber = exptreeit->first;
      OSExpressionTree* exptree = exptreeit->second;

      if(connumber != -1)
         conindex[connumber] = 1;
      else
         conObj = 1;

      list<char> gms;
      walk(exptree->m_treeRoot, gms, gms.begin());

      string gmsstr(gms.begin(), gms.end());
      if(connumber != -1)
         out << "eq" << connumber << ".. " << gmsstr;
      else
         out << "eqOBJ.. " << gmsstr;

      if(connumber != -1) {
         for(int i = osinstance->getLinearConstraintCoefficientsInRowMajor()->starts[connumber];
                 i < osinstance->getLinearConstraintCoefficientsInRowMajor()->starts[connumber+1]; i++) {
            if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] == 0) continue;
            out << " + ";
            if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] != 1) {
               if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] < 0) out << "(";
               out << osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i];
               if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] < 0) out << ")";
               out << " * ";
            }
            out << "x" << osinstance->getLinearConstraintCoefficientsInRowMajor()->indexes[i] << endl;
         }
      } else {
         for(int i = 0; i < osinstance->getObjectiveCoefficients()[0]->number; i++) {
            if(osinstance->getObjectiveCoefficients()[0]->values[i] == 0) continue;
            out << " + ";
            if(osinstance->getObjectiveCoefficients()[0]->values[i] != 1) {
               if(osinstance->getObjectiveCoefficients()[0]->values[i] < 0) out << "(";
               out << osinstance->getObjectiveCoefficients()[0]->values[i];
               if(osinstance->getObjectiveCoefficients()[0]->values[i] < 0) out << ")";
               out << " * ";
            }
            out << "x" << osinstance->getObjectiveCoefficients()[0]->indexes[i] << endl;
         }
      }

      if(connumber != -1) {
         out << " =" << conTypes[connumber] <<"= ";
         if(conTypes[connumber] == 'L')
            out << conUB[connumber] << ";" << endl << endl;
         else
            out << conLB[connumber] << ";" << endl << endl;
      } else
         out << " + " << objCon[0] << " =E= OBJ;" << endl << endl;
   }

   if(conObj != 1) {
      out << "eqOBJ.. ";
      for(int i = 0; i < osinstance->getObjectiveCoefficients()[0]->number; i++) {
         if(osinstance->getObjectiveCoefficients()[0]->values[i] == 0) continue;
         out << " + ";
         if(osinstance->getObjectiveCoefficients()[0]->values[i] != 1) {
            if(osinstance->getObjectiveCoefficients()[0]->values[i] < 0) out << "(";
            out << osinstance->getObjectiveCoefficients()[0]->values[i];
            if(osinstance->getObjectiveCoefficients()[0]->values[i] < 0) out << ")";
            out << " * ";
         }
         out << "x" << osinstance->getObjectiveCoefficients()[0]->indexes[i] << endl;
      }
      out << " + " << objCon[0] << " =E= OBJ;" << endl << endl;
   }

   for(int j = 0; j < osinstance->getConstraintNumber(); j++) {
      if(conindex[j] == 1) continue;
      out << "eq" << j << ".. ";
      for(int i = osinstance->getLinearConstraintCoefficientsInRowMajor()->starts[ j];
              i < osinstance->getLinearConstraintCoefficientsInRowMajor()->starts[ j+1]; i++) {
         if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] == 0) continue;
         out << " + ";
         if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] != 1) {
            if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] < 0) out << "(";
            out << osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i];
            if(osinstance->getLinearConstraintCoefficientsInRowMajor()->values[i] < 0) out << ")";
            out << " * ";
         }
         out << "x" << osinstance->getLinearConstraintCoefficientsInRowMajor()->indexes[i] << endl;
      }
      switch (conTypes[j]) {
      	case 'L':
          out << " =L= " << conUB[j] << ";" << endl << endl; break;
      	case 'E':
      	case 'G':
          out << " =" << conTypes[j] << "= " << conLB[j] << ";" << endl << endl; break;
      	case 'N':
      		out << " =N= " << conLB[j] << ";" << endl << endl; break;
      	case 'R':
      		cerr << "Ranged constraints not supported by GAMS. Aborting." << endl;
      		return 1;
      }
   }

   out << "Model m /all/;" << endl << endl;

   out << "Solve m " << osinstance->getObjectiveMaxOrMins()[0] << " OBJ using " << mdlType << ";" << endl << endl;

   delete [] conindex;
   delete [] osilstr;
   conindex = NULL;
   osilstr  = NULL;
   out.close();

   return 0;
}
