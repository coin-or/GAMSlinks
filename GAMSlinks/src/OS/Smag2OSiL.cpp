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
#include "GamsNLinstr.h"
#include "SmagExtra.h"

#include "OSInstance.h"
#include "CoinHelperFunctions.hpp"

#include "smag.h"

Smag2OSiL::Smag2OSiL(smagHandle_t smag_)
: osinstance(NULL), keep_original_instr(true), smag(smag_)
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
	
		SparseVector* objectiveCoefficients = new SparseVector(smagObjColCount(smag)-smagObjColCountNL(smag));
		j=0;
		for (smagObjGradRec_t* og = smag->objGrad;  og;  og = og->next) {
			if (og->degree>1) continue; // skip nonlinear terms
			objectiveCoefficients->indexes[j] = og->j;
			objectiveCoefficients->values[j] = og->dfdj;
			j++;
		  assert(j<=smagObjColCount(smag)-smagObjColCountNL(smag));
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
		
		if (smagObjColCountNL(smag))
			++osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions;
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
    for (smagConGradRec_t* cGrad = smag->conGrad[i];  cGrad;  cGrad = cGrad->next) {
    	if (cGrad->degree>1) continue; // skip nonlinear variables
    	values[j]=cGrad->dcdj;
    	colindexes[j]=cGrad->j;
    	j++;
    }
    
    if (smag->snlData.numInstr && smag->snlData.numInstr[smag->rowMapS2G[i]])
    	++osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions;
  }
  assert(j<=smagNZCount(smag));
  rowstarts[smagRowCount(smag)]=j;

	if (!osinstance->setLinearConstraintCoefficients(j, false, 
		values, 0, j-1,
		colindexes, 0, j-1,
		rowstarts, 0, smagRowCount(smag)))
		return false;
	
	if (!setupTimeDomain())
		return false;

	if (!smagColCountNL(smag)) // everything linear -> finished
		return true;
	
	//TODO: if model is quadratic (QCP or MIQCP), call setupQuadraticTerms() ?
	
	//nonlinear stuff
	osinstance->instanceData->nonlinearExpressions->nl = CoinCopyOfArrayOrZero((Nl**)NULL, osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);
	int iNLidx = 0;
		
	strippedNLdata_t *snl=&smag->snlData;
	assert(snl->numInstr);
	OSnLNode* nl;
	if (smagObjColCountNL(smag)>0) {
//		std::clog << "parsing nonlinear objective instructions" << std::endl;
		nl=parseGamsInstructions(snl->instr+snl->startIdx[smag->gObjRow]-1, snl->numInstr[smag->gObjRow], snl->nlCons);
		if (!nl) return false;
		if (smag->gObjFactor == -1.) {
			OSnLNode* negnode = new OSnLNodeNegate;
			negnode->m_mChildren[0] = nl;
			nl = negnode;
		} else if (smag->gObjFactor != 1.) {
			OSnLNodeNumber* numbernode = new OSnLNodeNumber();
			numbernode->value = smag->gObjFactor;
			OSnLNodeTimes* timesnode = new OSnLNodeTimes();
			timesnode->m_mChildren[0] = nl;
			timesnode->m_mChildren[1] = numbernode;
			nl = timesnode;
		}
		assert(iNLidx < osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx] = new Nl();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->idx = -1;
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree = new OSExpressionTree();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree->m_treeRoot = nl;
		++iNLidx;
	}

	for (i=0; i<smagRowCount(smag); ++i) {
		if (!smag->snlData.numInstr[smag->rowMapS2G[i]]) continue;
//		std::clog << "parsing " << smag->snlData.numInstr[smag->rowMapS2G[i]] << " nonlinear instructions of constraint " << osinstance->getConstraintNames()[i] << std::endl;
		nl = parseGamsInstructions(snl->instr+snl->startIdx[smag->rowMapS2G[i]]-1, snl->numInstr[smag->rowMapS2G[i]], snl->nlCons);
		if (!nl) return false;
		assert(iNLidx < osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx] = new Nl();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->idx = i; // correct that this is the con. number?
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree = new OSExpressionTree();
		osinstance->instanceData->nonlinearExpressions->nl[ iNLidx]->osExpressionTree->m_treeRoot = nl;
		++iNLidx;
	}
	assert(iNLidx == osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);

	// TODO: separate quadratic terms from expression trees?
	
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

OSnLNode* Smag2OSiL::parseGamsInstructions(unsigned int* instr_, int num_instr, double* constants) {
	std::vector<OSnLNode*> nlNodeVec;
	
	const bool debugoutput = false;

//	for (int i=0; i<num_instr; ++i)
//		std::clog << i << '\t' << GamsOpCodeName[getInstrOpCode(instr[i])] << '\t' << getInstrAddress(instr[i]) << std::endl;
	
	unsigned int* instr = keep_original_instr ? CoinCopyOfArray(instr_, num_instr) : instr_;
	
	// reorder instructions such that there are no PushS, Popup, or Swap left
	reorderInstr(instr, num_instr);

	nlNodeVec.reserve(num_instr);
	
	for (int i=0; i<num_instr; ++i) {
		GamsOpCode opcode = getInstrOpCode(instr[i]);
		int address = getInstrAddress(instr[i]);

		if (debugoutput) std::clog << '\t' << GamsOpCodeName[opcode] << ": ";
		switch(opcode) {
			case nlNoOp : { // no operation
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushV : { // push variable
				address = smag->colMapG2S[address];
				if (debugoutput) std::clog << "push variable " << osinstance->getVariableNames()[address] << std::endl;
				OSnLNodeVariable *nlNode = new OSnLNodeVariable();
				nlNode->idx=address;
				nlNodeVec.push_back( nlNode );
			} break;
			case nlPushI : { // push constant
				if (debugoutput) std::clog << "push constant " << constants[address] << std::endl;
				OSnLNodeNumber *nlNode = new OSnLNodeNumber();
				nlNode->value = constants[address];
				nlNodeVec.push_back( nlNode );
			} break;
			case nlStore: { // store row
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlAdd : { // add
				if (debugoutput) std::clog << "add" << std::endl;
				nlNodeVec.push_back( new OSnLNodePlus() );
			} break;
			case nlAddV: { // add variable
				address = smag->colMapG2S[address];
				if (debugoutput) std::clog << "add variable " << osinstance->getVariableNames()[address] << std::endl;
				OSnLNodeVariable *nlNode = new OSnLNodeVariable();
				nlNode->idx=address;
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodePlus() );
			} break;
			case nlAddI: { // add immediate
				if (debugoutput) std::clog << "add constant " << constants[address] << std::endl;
				OSnLNodeNumber *nlNode = new OSnLNodeNumber();
				nlNode->value = constants[address];
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodePlus() );
			} break;
			case nlSub: { // minus
				if (debugoutput) std::clog << "minus" << std::endl;
				nlNodeVec.push_back( new OSnLNodeMinus() );
			} break;
			case nlSubV: { // subtract variable
				address = smag->colMapG2S[address];
				if (debugoutput) std::clog << "substract variable " << osinstance->getVariableNames()[address] << std::endl;
				OSnLNodeVariable *nlNode = new OSnLNodeVariable();
				nlNode->idx=address;
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodeMinus() );
			} break;
			case nlSubI: { // subtract immediate
				if (debugoutput) std::clog << "substract constant " << constants[address] << std::endl;
				OSnLNodeNumber *nlNode = new OSnLNodeNumber();
				nlNode->value = constants[address];
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodeMinus() );
			} break;
			case nlMul: { // multiply
				if (debugoutput) std::clog << "multiply" << std::endl;
				nlNodeVec.push_back( new OSnLNodeTimes() );				
			} break;
			case nlMulV: { // multiply variable
				address = smag->colMapG2S[address];
				if (debugoutput) std::clog << "multiply variable " << osinstance->getVariableNames()[address] << std::endl;
				OSnLNodeVariable *nlNode = new OSnLNodeVariable();
				nlNode->idx=address;
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodeTimes() );
			} break;
			case nlMulI: { // multiply immediate
				if (debugoutput) std::clog << "multiply constant " << constants[address] << std::endl;
				OSnLNodeNumber *nlNode = new OSnLNodeNumber();
				nlNode->value = constants[address];
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodeTimes() );
			} break;
			case nlDiv: { // divide
				if (debugoutput) std::clog << "divide" << std::endl;
				nlNodeVec.push_back( new OSnLNodeDivide() );				
			} break;
			case nlDivV: { // divide variable
				address = smag->colMapG2S[address];
				if (debugoutput) std::clog << "divide variable " << osinstance->getVariableNames()[address] << std::endl;
				OSnLNodeVariable *nlNode = new OSnLNodeVariable();
				nlNode->idx=address;
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodeDivide() );
			} break;
			case nlDivI: { // divide immediate
				if (debugoutput) std::clog << "divide constant " << constants[address] << std::endl;
				OSnLNodeNumber *nlNode = new OSnLNodeNumber();
				nlNode->value = constants[address];
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodeDivide() );
			} break;
			case nlUMin: { // unary minus
				if (debugoutput) std::clog << "negate" << std::endl;
				nlNodeVec.push_back( new OSnLNodeNegate() );				
			} break;
			case nlUMinV: { // unary minus variable
				address = smag->colMapG2S[address];
				if (debugoutput) std::clog << "push negated variable " << osinstance->getVariableNames()[address] << std::endl;
				OSnLNodeVariable *nlNode = new OSnLNodeVariable();
				nlNode->idx = address;
				nlNode->coef = -1.;
				nlNodeVec.push_back( nlNode );
			} break;
			case nlCallArg1 :
			case nlCallArg2 :
			case nlCallArgN : {
				if (debugoutput) std::clog << "call function ";
				GamsFuncCode func = GamsFuncCode(address+1); // here the shift by one was not a good idea
				switch (func) {
					case fnmin : {
						if (debugoutput) std::clog << "min" << std::endl;
						nlNodeVec.push_back( new OSnLNodeMin() );
					} break;
					case fnmax : {
						if (debugoutput) std::clog << "max" << std::endl;
						nlNodeVec.push_back( new OSnLNodeMax() );
					} break;
					case fnsqr : {
						if (debugoutput) std::clog << "square" << std::endl;
						nlNodeVec.push_back( new OSnLNodeSquare() );
					} break;
					case fnexp:
					case fnslexp:
					case fnsqexp: {
						if (debugoutput) std::clog << "exp" << std::endl;
						nlNodeVec.push_back( new OSnLNodeExp() );
					} break;
					case fnlog : {
						if (debugoutput) std::clog << "ln" << std::endl;
						nlNodeVec.push_back( new OSnLNodeLn() );
					} break;
					case fnlog10:
					case fnsllog10:
					case fnsqlog10: {
						if (debugoutput) std::clog << "log10 = ln * 1/ln(10)" << std::endl;
						nlNodeVec.push_back( new OSnLNodeLn() );
						OSnLNodeNumber *nlNode = new OSnLNodeNumber();
						nlNode->value = 1./log(10.);
						nlNodeVec.push_back( nlNode );
						nlNodeVec.push_back( new OSnLNodeTimes() );
					} break;
					case fnlog2 : {
						if (debugoutput) std::clog << "log2 = ln * 1/ln(2)" << std::endl;
						nlNodeVec.push_back( new OSnLNodeLn() );
						OSnLNodeNumber *nlNode = new OSnLNodeNumber();
						nlNode->value = 1./log(2.);
						nlNodeVec.push_back( nlNode );
						nlNodeVec.push_back( new OSnLNodeTimes() );
					} break;
					case fnsqrt: {
						if (debugoutput) std::clog << "sqrt" << std::endl;
						nlNodeVec.push_back( new OSnLNodeSqrt() );
					} break;
					case fnabs: {
						if (debugoutput) std::clog << "abs" << std::endl;
						nlNodeVec.push_back( new OSnLNodeAbs() );						
					} break;
					case fncos: {
						if (debugoutput) std::clog << "cos" << std::endl;
						nlNodeVec.push_back( new OSnLNodeCos() );						
					} break;
					case fnsin: {
						if (debugoutput) std::clog << "sin" << std::endl;
						nlNodeVec.push_back( new OSnLNodeSin() );						
					} break;
					case fnpower:
					case fnrpower: // x ^ y
					case fncvpower: // constant ^ x
					case fnvcpower: { // x ^ constant {
						if (debugoutput) std::clog << "power" << std::endl;
						nlNodeVec.push_back( new OSnLNodePower() );						
					} break;
					case fnpi: {
						if (debugoutput) std::clog << "pi" << std::endl;
						nlNodeVec.push_back( new OSnLNodePI() );
					} break;
					case fndiv:
					case fndiv0: {
						nlNodeVec.push_back( new OSnLNodeDivide() );
					} break;
					case fnslrec: // 1/x
					case fnsqrec: { // 1/x
						if (debugoutput) std::clog << "divide" << std::endl;
						nlNodeVec.push_back( new OSnLNodeLn() );
						OSnLNodeNumber *nlNode = new OSnLNodeNumber();
						nlNode->value = 1.;
						nlNodeVec.push_back( nlNode );
						nlNodeVec.push_back( new OSnLNodeDivide() );
					} break;
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
			    case fnpoly: /* simple polynomial */
					default : {
						if (debugoutput) std::cerr << "nr. " << func << " - unsuppored. Error." << std::endl;
						return NULL;
					}				
				}
			} break;
			case nlMulIAdd: {
				if (debugoutput) std::clog << "multiply constant " << constants[address] << " and add " << std::endl;
				OSnLNodeNumber *nlNode = new OSnLNodeNumber();
				nlNode->value = constants[address];
				nlNodeVec.push_back( nlNode );
				nlNodeVec.push_back( new OSnLNodeTimes() );
				nlNodeVec.push_back( new OSnLNodePlus() );
			} break;
			case nlFuncArgN : {
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlArg: {
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlHeader: { // header
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushZero: {
				if (debugoutput) std::clog << "push constant zero" << std::endl;
				nlNodeVec.push_back( new OSnLNodeNumber() );
			} break;
			case nlStoreS: { // store scaled row
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			// the following three should have been taken out by reorderInstr above; the remaining ones seem to be unused by now
			case nlPushS: // duplicate value from address levels down on top of stack
			case nlPopup: // duplicate value from this level to at address levels down and pop entries in between
			case nlSwap: // swap two positions on top of stack
			case nlAddL: // add local
			case nlSubL: // subtract local
			case nlMulL: // multiply local
			case nlDivL: // divide local
			case nlPushL: // push local
			case nlPopL: // pop local
			case nlPopDeriv: // pop derivative
			case nlUMinL: // push umin local
			case nlPopDerivS: // store scaled gradient
			case nlEquScale: // equation scale
			case nlEnd: // end of instruction list
			default: {
				std::cerr << "not supported - Error." << std::endl;
				return NULL;
			}
		}
	}
	
	if (keep_original_instr)
		delete[] instr;
	
	if (!nlNodeVec.size()) return NULL;	
	// the vector is in postfix format - create expression tree and return it
	return nlNodeVec[0]->createExpressionTreeFromPostfix(nlNodeVec);
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
