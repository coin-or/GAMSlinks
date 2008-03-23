// Copyright (C) 2007-2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "Smag2OSiL.hpp"
#include "GamsDictionary.hpp"
#include "GamsHandlerSmag.hpp"

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
	char buffer[255];
	
	GamsHandlerSmag gamshandler(smag);
	GamsDictionary dict(gamshandler);
	dict.readDictionary();

	// unfortunately, we do not know the model name
	osinstance->setInstanceDescription("Generated from GAMS smag problem");
	
	osinstance->setVariableNumber(smagColCount(smag));
	char* var_types=new char[smagColCount(smag)];
	std::string* varnames=NULL;
	std::string* vartext=NULL;
	if (dict.haveNames()) {
		varnames=new std::string[smagColCount(smag)];
		vartext=new std::string[smagColCount(smag)];
	}
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
		if (dict.haveNames() && dict.getColName(i, buffer, 256))
			varnames[i]=buffer;
		if (dict.haveNames() && dict.getColText(i, buffer, 256))
			vartext[i]=buffer;
	}
	
	// store the descriptive text of a variables in the initString argument to have it stored somewhere 
	if (!osinstance->setVariables(smagColCount(smag), varnames, smag->colLB, smag->colUB, var_types, smag->colLev, vartext))
		return false;
	delete[] var_types;
	delete[] varnames;
	delete[] vartext;

	if (smag->gObjRow<0) { // we seem to have no objective, i.e., a CNS model
		osinstance->setObjectiveNumber(0);
	} else { // setup objective
		osinstance->setObjectiveNumber(1);
	
		SparseVector* objectiveCoefficients = new SparseVector(smagObjColCount(smag));
		j=0;
		for (smagObjGradRec_t* og = smag->objGrad;  og;  og = og->next, ++j) {
			objectiveCoefficients->indexes[j] = og->j;
			objectiveCoefficients->values[j] = og->dfdj;
		}

		std::string objname;
		if (dict.haveNames() && dict.getObjName(buffer, 256))
			objname=buffer;

		if (!osinstance->addObjective(-1, objname,
				smagMinim(smag)==1 ? "min" : "max",
						smag->gms.grhs[smag->gms.slplro-1]*smag->gObjFactor,
						1., objectiveCoefficients)) {
			delete objectiveCoefficients;
			return false;
		}
		delete objectiveCoefficients;
	}
	
	osinstance->setConstraintNumber(smagRowCount(smag));

	double lb, ub;
	for (i = 0;  i < smagRowCount(smag);  i++) {
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
		std::string conname(dict.haveNames() ? dict.getRowName(i, buffer, 255) : NULL); 
		if (!osinstance->addConstraint(i, conname, lb, ub, 0.))
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

	if (smagColCountNL(smag)) {
		if (!setupQuadraticTerms()) return false; //TODO: should call this only if it is a QQP
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


bool Smag2OSiL::setupQuadraticTerms() {
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
	
	// the real size of the hessian should be given in the end of the rowStart array 
	hesSize=rowStart[smagRowCount(smag)+1];
	
	int* rowindices=new int[hesSize];
	for (int j=0; j<smagRowCount(smag); ++j)
		for (int i=rowStart[j]; i<rowStart[j+1]; ++i)
			rowindices[i]=j;
	for (int i=rowStart[smagRowCount(smag)]; i<rowStart[smagRowCount(smag)+1]; ++i)
		rowindices[i]=-1;

	// coefficients for diagonal elements need to be divided by two
	for (int i=0; i<hesSize; ++i)
		if (hesRowIdx[i]==hesColIdx[i]) hesValues[i]*=.5;
	
	if (!osinstance->setQuadraticTerms(hesSize, rowindices, hesRowIdx, hesColIdx, hesValues, 0, hesSize-1)) {
		smagStdOutputPrint(smag, SMAG_ALLMASK, "Error setting quadratic terms in OSInstance. Exiting ...\n");
		smagStdOutputFlush(smag, SMAG_ALLMASK);
		return false;
	}

	delete[] hesRowIdx;
	delete[] hesColIdx;
	delete[] hesValues;
	delete[] rowStart;
	delete[] rowindices;

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

bool Smag2OSiL::setupTimeDomain() {
	int* colTimeStage=new int[smagColCount(smag)];
	int minStage=0;
	int maxStage=0;
	
	// timestages in GAMS are stored in the scaling attribute, but only if the variable is continuous
	// otherwise we assume they are stored in the branching priority attribute
	for (int i=0; i<smagColCount(smag); ++i) {
		if (smag->colType[i]!=SMAG_VAR_CONT)
			colTimeStage[i]=(int)smag->colPriority[i];
		else
			colTimeStage[i]=(int)smag->colScale[i];
		
		if (i==0) minStage=maxStage=colTimeStage[i];
		else if (minStage>colTimeStage[i]) minStage=colTimeStage[i];
		else if (maxStage<colTimeStage[i]) maxStage=colTimeStage[i];
	}
	
	if (minStage==maxStage) { // there seem to be only one stage, so we think that this is not a stochastic program
		delete[] colTimeStage;
		return true;
	}
	smagStdOutputPrint(smag, SMAG_LOGMASK, "Found stage information. Setting up TimeDomain.\n");
	
	TimeDomainInterval* interval=new TimeDomainInterval;
	interval->intervalStart=minStage;
	interval->intervalHorizon=maxStage;
	
	TimeDomainStages* stages=new TimeDomainStages;
	stages->numberOfStages=maxStage-minStage+1;
	stages->stage=new TimeDomainStage*[stages->numberOfStages];
	
	for (int i=0; i<stages->numberOfStages; ++i)
		stages->stage[i]=new TimeDomainStage;
	
	// shift column timestages to start with 0; count number of variables per timestage
	for (int i=0; i<smagColCount(smag); ++i) {
		colTimeStage[i]-=minStage;
		++stages->stage[colTimeStage[i]]->nvar;
	}
	maxStage-=minStage;

	int* fillUp=new int[stages->numberOfStages]; // to remember how many items (vars or rows) we have put in the corresponding arrays at the stages already
	
	for (int i=0; i<stages->numberOfStages; ++i) { 	// initializes variables arrays
		stages->stage[i]->variables=new int[stages->stage[i]->nvar];
		fillUp[i]=0;
	}
	
	for (int i=0; i<smagColCount(smag); ++i) // fill variables arrays
		stages->stage[colTimeStage[i]]->variables[fillUp[colTimeStage[i]]++]=i;


	// and now compute the timestages of the constraints
	// we just say that a constraint is in stage t if it has a variable at stage t, but nothing at later ones
	
	int* rowTimeStage=new int[smagRowCount(smag)];
	
	for (int i=0; i<smagRowCount(smag); ++i) {
		rowTimeStage[i]=0;
		// rowTimeStage[i] is maximum of the colTimeStage's for all variables appearing in row i 
		for (smagConGradRec_t* cGrad = smag->conGrad[i];  cGrad;  cGrad = cGrad->next)
			if (rowTimeStage[i]<colTimeStage[cGrad->j])
				rowTimeStage[i]=colTimeStage[cGrad->j];
		++stages->stage[rowTimeStage[i]]->ncon;
	}

	for (int i=0; i<stages->numberOfStages; ++i) { 	// initializes variables arrays
		stages->stage[i]->constraints=new int[stages->stage[i]->ncon];
		fillUp[i]=0;
	}
	
	for (int i=0; i<smagRowCount(smag); ++i) // fill constraints arrays
		stages->stage[rowTimeStage[i]]->constraints[fillUp[rowTimeStage[i]]++]=i;

	// and finally the timestage for the objective (if we have any)
	if (smag->gObjRow>=0) {
		int objTimeStage=0;
		for (smagObjGradRec_t* oGrad = smag->objGrad; oGrad; oGrad = oGrad->next)
			if (objTimeStage<colTimeStage[oGrad->j])
				objTimeStage=colTimeStage[oGrad->j];
		stages->stage[objTimeStage]->nobj=1;
		stages->stage[objTimeStage]->objectives=new int[1];
		stages->stage[objTimeStage]->objectives[0]=-1;
	}

	osinstance->instanceData->timeDomain=new TimeDomain;
	osinstance->instanceData->timeDomain->stages=stages;
	osinstance->instanceData->timeDomain->interval=interval;
	
	delete[] colTimeStage;
	delete[] rowTimeStage;
	delete[] fillUp;
	
	return true;
}
