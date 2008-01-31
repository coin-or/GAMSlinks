// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Stefan Vigerske

#include "Smag2OSiL.hpp"

extern "C" {
#include "SmagExtra.h"
}

Smag2OSiL::Smag2OSiL(smagHandle_t smag_)
: osinstance(NULL), smag(smag_)
{ }

Smag2OSiL::~Smag2OSiL() {
	delete osinstance;	
}

bool Smag2OSiL::createOSInstance() {
	osinstance = new OSInstance();  
	int i, j;

	// TODO: put model name
	osinstance->setInstanceDescription("Generated from GAMS smag problem");
	
	osinstance->setVariableNumber(smagColCount(smag));
	char* var_types=new char[smagColCount(smag)];
	for(i = 0; i < smagColCount(smag); ++i) {
		switch (smag->colType[i]) {
			case SMAG_VAR_CONT:
				var_types[i]='C';
				break;
			case SMAG_VAR_BINARY:
				var_types[i]='B';
				break;
			case SMAG_VAR_INTEGER:
				var_types[i]='I';
				break;
			default : {
				// TODO: how to represent semicontinuous var. and SOS in OSiL ? 
				smagStdOutputPrint(smag, SMAG_ALLMASK, "Error: Unsupported variable type.\n"); 			
	      smagStdOutputFlush(smag, SMAG_ALLMASK);
				return false;
			}
		}
	}
	// TODO: variable names
	if (!osinstance->setVariables(smagColCount(smag), NULL, smag->colLB, smag->colUB, var_types, smag->colLev, NULL))
		return false;
	delete[] var_types;

	// TODO: would be 0 for model type CNS
	osinstance->setObjectiveNumber(1);
	
	SparseVector* objectiveCoefficients = new SparseVector(smagObjColCount(smag));
	j=0;
  for (smagObjGradRec_t* og = smag->objGrad;  og;  og = og->next, ++j) {
  	objectiveCoefficients->indexes[j] = og->j;
  	objectiveCoefficients->values[j] = og->dfdj;
  }

	if (!osinstance->addObjective(-1, "",
		smagMinim(smag)==1 ? "min" : "max",
		smag->gms.grhs[smag->gms.slplro-1]*smag->gObjFactor,
		1., objectiveCoefficients)) {
		delete objectiveCoefficients;
		return false;
	}
	delete objectiveCoefficients;
	
	osinstance->setConstraintNumber(smagRowCount(smag));

	double lb, ub;
	for (int i = 0;  i < smagRowCount(smag);  i++) {
		switch (smag->rowType[i]) {
			case SMAG_EQU_EQ:
				lb = ub = smag->rowRHS[i];
				break;
			case SMAG_EQU_LT:
				lb = -smag->inf;
				ub =  smag->rowRHS[i];
				break;
			case SMAG_EQU_GT:
				lb = smag->rowRHS[i];
				ub = smag->inf;
				break;
			default:
				smagStdOutputPrint(smag, SMAG_ALLMASK, "Error: Unknown SMAG row type. Exiting ...\n");
				smagStdOutputFlush(smag, SMAG_ALLMASK);
				return false;
		}
		if (!osinstance->addConstraint(i, "", lb, ub, 0.)) // TODO: constraint names
			return false;
	}

	double* values=new double[smagNZCount(smag)];
	int* rowstarts=new int[smagRowCount(smag)+1];
	int* colindexes=new int[smagNZCount(smag)];
	j = 0;
  for (i = 0;  i < smagRowCount(smag);  ++i) {
  	rowstarts[i]=j;
    for (smagConGradRec_t* cGrad = smag->conGrad[i];  cGrad;  cGrad = cGrad->next, ++j) {
    	values[j]=cGrad->dcdj;
    	colindexes[j]=cGrad->j;
    }
  }
  assert(j==smagNZCount(smag));
  rowstarts[smagRowCount(smag)]=j;

	if (!osinstance->setLinearConstraintCoefficients(smagNZCount(smag), false, 
		values, 0, smagNZCount(smag)-1,
		colindexes, 0, smagNZCount(smag)-1,
		rowstarts, 0, smagRowCount(smag)))
		return false;

	//TODO; if there are nonlinearities, we finish here, since they are not implemented yet
	if (smagRowCountNL(smag) || smagObjColCountNL(smag)) {
		if (!getQuadraticTerms()) return false; //TODO: should call this only if it is a QQP
		return true;
	}
	
	//nonlinear stuff
	osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions = 
		smagRowCountNL(smag) + (smagObjColCountNL(smag)>0 ? 1 : 0);
	osinstance->instanceData->nonlinearExpressions->nl = new Nl*[osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions];
	int iNLidx = 0;
		
	strippedNLdata_t *snl=&smag->snlData;
	OSnLNode* nl;
	if (smagObjColCountNL(smag)>0) {
		nl=parseGamsInstructions(snl->instr+snl->startIdx[smag->gObjRow]-1, snl->numInstr[smag->gObjRow], snl->nlCons);
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx] = new Nl();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->idx = -1;
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree = new OSExpressionTree();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree->m_treeRoot = nl;
		++iNLidx;
	}

	for (i=0; i<smagRowCount(smag); ++i) {
		if (!smag->snlData.numInstr || smag->snlData.numInstr[i]==0) continue;
		nl=parseGamsInstructions(snl->instr+snl->startIdx[smag->rowMapS2G[i]]-1, snl->numInstr[smag->rowMapS2G[i]], snl->nlCons);
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx] = new Nl();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->idx = i; // correct that this is the con. number?
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree = new OSExpressionTree();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree->m_treeRoot = nl;
		++iNLidx;
	}

	// TODO: separate quadratic terms from expression trees

	return true;
}


bool Smag2OSiL::getQuadraticTerms() {
	int hesSize=(int)(smag->gms.workFactor * 10 * smag->gms.nnz);

	int* hesRowIdx=new int[hesSize];
	int* hesColIdx=new int [hesSize];
	double* hesValues=new double[hesSize];
	int* rowStart=new int[smagRowCount(smag)+2];
	
	int rc=smagSingleHessians(smag, hesRowIdx, hesColIdx, hesValues, hesSize, rowStart);
	if (rc) {
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Error getting structure and values of Hessians. Exiting ...\n");
		smagStdOutputFlush(smag, SMAG_ALLMASK);
		return false;
	}

	delete[] hesRowIdx;
	delete[] hesColIdx;
	delete[] hesValues;
	delete[] rowStart;
	
	return true;
}

//#include "GamsNLcodes.hpp"

OSnLNode* Smag2OSiL::parseGamsInstructions(unsigned int* instr, int num_instr, double* constants) {
	std::vector<OSnLNode*> nlNodeVec;
	nlNodeVec.reserve(num_instr);

	//TODO
/* the following code from OS/examples/../instanceGenerator.cpp shows how to generate a nl instruction in postfix format and use a OS method to create a corresponding expression tree
	// the objective function nonlinear term abs( x0 + 1)
	OSnLNode *nlNodePoint;
	OSnLNodeVariable *nlNodeVariablePoint;
	OSnLNodeNumber *nlNodeNumberPoint;
	OSnLNodeMax *nlNodeMaxPoint;
	// create a variable nl node for x0
	nlNodeVariablePoint = new OSnLNodeVariable();
	nlNodeVariablePoint->idx=0;
	nlNodeVec.push_back( nlNodeVariablePoint);
	// create the nl node for number 1
	nlNodeNumberPoint = new OSnLNodeNumber(); 
	nlNodeNumberPoint->value = 1.0;
	nlNodeVec.push_back( nlNodeNumberPoint);
	// create the nl node for +
	nlNodePoint = new OSnLNodePlus();
	nlNodeVec.push_back( nlNodePoint);
	// create the nl node for max
	nlNodePoint = new OSnLNodeAbs();
	nlNodeVec.push_back( nlNodePoint);

	// the vectors are in postfix format
	// now the expression tree
	return nlNodeVec[0]->createExpressionTreeFromPostfix(nlNodeVec);
*/
	
	return NULL;
}
