// Copyright (C) GAMS Development and others 2007-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "Gams2OSiL.hpp"
#include "GamsNLinstr.h"

#include "OSInstance.h"
#include "CoinHelperFunctions.hpp"

#include "gmomcc.h"
#include "gevmcc.h"

#include <sstream>

Gams2OSiL::Gams2OSiL(gmoHandle_t gmo_)
: gmo(gmo_), gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL), osinstance(NULL)
{ }

Gams2OSiL::~Gams2OSiL() {
	delete osinstance;
}

bool Gams2OSiL::createOSInstance() {
	osinstance = new OSInstance();
	int i, j;
	char buffer[255];

	// unfortunately, we do not know the model name
	osinstance->setInstanceDescription("Generated from GAMS GMO problem");
	osinstance->setVariableNumber(gmoN(gmo));

	char*        vartypes = new char[gmoN(gmo)];
	std::string* varnames = new std::string[gmoN(gmo)];
	for(i = 0; i < gmoN(gmo); ++i) {
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
				vartypes[i] = 'C';
				break;
			case var_B:
				vartypes[i] = 'B';
				break;
			case var_I:
				vartypes[i] = 'I';
				break;
			default : {
				// TODO: how to represent semicontinuous var. and SOS in OSiL ?
				gevLogStat(gev, "Error: Unsupported variable type.");
				return false;
			}
		}
		if (gmoDict(gmo))
			gmoGetVarNameOne(gmo, i, buffer);
		else
			sprintf(buffer, "x%08d", i);
		varnames[i] = buffer;
	}

	double* varlow = new double[gmoN(gmo)];
	double* varup  = new double[gmoN(gmo)];
	gmoGetVarLower(gmo, varlow);
	gmoGetVarUpper(gmo, varup);

	if (gmoN(gmo) && !osinstance->setVariables(gmoN(gmo), varnames, varlow, varup, vartypes)) {
	   gevLogStat(gev, "Error: OSInstance::setVariables did not succeed.\n");
		return false;
	}

	delete[] vartypes;
	delete[] varnames;
	delete[] varlow;
	delete[] varup;

	if (gmoModelType(gmo) == Proc_cns) { // no objective in constraint satisfaction models
		osinstance->setObjectiveNumber(0);
	} else { // setup objective
		osinstance->setObjectiveNumber(1);

		SparseVector* objectiveCoefficients = new SparseVector(gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

		int* colidx = new int[gmoObjNZ(gmo)];
		double* val = new double[gmoObjNZ(gmo)];
		int* nlflag = new int[gmoObjNZ(gmo)];
		int* dummy  = new int[gmoObjNZ(gmo)];

		if (gmoObjNZ(gmo)) nlflag[0] = 0; // workaround for gmo bug
		gmoGetObjSparse(gmo, colidx, val, nlflag, dummy, dummy);
		for (i = 0, j = 0; i < gmoObjNZ(gmo); ++i) {
			if (nlflag[i]) continue;
			objectiveCoefficients->indexes[j] = colidx[i];
			objectiveCoefficients->values[j]  = val[i];
			j++;
		  assert(j <= gmoObjNZ(gmo) - gmoObjNLNZ(gmo));
		}
	  assert(j == gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

		delete[] colidx;
		delete[] val;
		delete[] nlflag;
		delete[] dummy;

		std::string objname = "objective";
		//TODO objective name
//		if (dict.haveNames() && dict.getObjName(buffer, 256))
//			objname = buffer;
//		std::cout << "gmo obj con: " << gmoObjConst(gmo) << std::endl;
		if (!osinstance->addObjective(-1, objname, gmoSense(gmo) == Obj_Min ? "min" : "max", gmoObjConst(gmo), 1., objectiveCoefficients)) {
			delete objectiveCoefficients;
	      gevLogStat(gev, "Error: OSInstance::addObjective did not succeed.\n");
			return false;
		}
		delete objectiveCoefficients;
	}

	osinstance->setConstraintNumber(gmoM(gmo));

	double lb, ub;
	for (i = 0;  i < gmoM(gmo);  ++i) {
		switch (gmoGetEquTypeOne(gmo, i)) {
			case equ_E:
				lb = ub = gmoGetRhsOne(gmo, i);
				break;
			case equ_L:
				lb = -OSDBL_MAX;
				ub =  gmoGetRhsOne(gmo, i);
				break;
			case equ_G:
				lb = gmoGetRhsOne(gmo, i);
				ub = OSDBL_MAX;
				break;
			case equ_N:
				lb = -OSDBL_MAX;
				ub =  OSDBL_MAX;
				break;
			default:
				gevLogStat(gev, "Error: Unknown row type. Exiting ...");
				return false;
		}
		std::string conname;
		if (gmoDict(gmo))
			gmoGetEquNameOne(gmo, i, buffer);
		else
			sprintf(buffer, "e%08d", i);
		conname = buffer;
		if (!osinstance->addConstraint(i, conname, lb, ub, 0.)) {
	      gevLogStat(gev, "Error: OSInstance::addConstraint did not succeed.\n");
			return false;
		}
	}

	int nz = gmoNZ(gmo);
	double* values  = new double[nz];
	int* colstarts  = new int[gmoN(gmo)+1];
	int* rowindexes = new int[nz];
	int* nlflags    = new int[nz];

	gmoGetMatrixCol(gmo, colstarts, rowindexes, values, nlflags);
//	for (i = 0; i < gmoNZ(gmo); ++i)
//		if (nlflags[i]) values[i] = 0.;
	colstarts[gmoN(gmo)] = nz;

	int shift = 0;
	for (int col = 0; col < gmoN(gmo); ++col) {
		colstarts[col+1] -= shift;
		int k = colstarts[col];
		while (k < colstarts[col+1]) {
			values[k] = values[k+shift];
			rowindexes[k] = rowindexes[k+shift];
			if (nlflags[k+shift]) {
				++shift;
				--colstarts[col+1];
			} else {
				++k;
			}
		}
	}
	nz -= shift;

	if (!osinstance->setLinearConstraintCoefficients(nz, true,
		values, 0, nz-1,
		rowindexes, 0, nz-1,
		colstarts, 0, gmoN(gmo))) {
		delete[] nlflags;
      gevLogStat(gev, "Error: OSInstance::setLinearConstraintCoefficients did not succeed.\n");
		return false;
	}

	// values, colstarts, rowindexes are deleted by OSInstance
	delete[] nlflags;

	if (!gmoObjNLNZ(gmo) && !gmoNLNZ(gmo)) // everything linear -> finished
		return true;

	osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions = gmoNLM(gmo) + (gmoObjNLNZ(gmo) ? 1 : 0);
	osinstance->instanceData->nonlinearExpressions->nl = CoinCopyOfArrayOrZero((Nl**)NULL, osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions);
	int iNLidx = 0;

	int* opcodes = new int[gmoMaxSingleFNL(gmo)+1];
	int* fields  = new int[gmoMaxSingleFNL(gmo)+1];
	int constantlen = gmoNLConst(gmo);
	double* constants = (double*)gmoPPool(gmo);
	int codelen;

	OSnLNode* nl;
	if (gmoObjNLNZ(gmo)) {
//		std::clog << "parsing nonlinear objective instructions" << std::endl;
		gmoDirtyGetObjFNLInstr(gmo, &codelen, opcodes, fields);

		nl = parseGamsInstructions(codelen, opcodes, fields, constantlen, constants);
		if (!nl) {
	      gevLogStat(gev, "Error: failure when parsing GAMS instructions.\n");
		   return false;
		}

		double objjacval = gmoObjJacVal(gmo);
//		std::clog << "obj jac val: " << objjacval << std::endl;
		if (objjacval == 1.) { // scale by -1/objjacval = negate
			OSnLNode* negnode = new OSnLNodeNegate;
			negnode->m_mChildren[0] = nl;
			nl = negnode;
		} else if (objjacval != -1.) { // scale by -1/objjacval
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

	for (i = 0; i < gmoM(gmo); ++i) {
		if (gmoDirtyGetRowFNLInstr(gmo, i, &codelen, opcodes, fields)) {
			std::clog << "got nonzero return at constraint " << i << std::endl;
		}
		if (!codelen) continue;
//		std::clog << "parsing " << codelen << " nonlinear instructions of constraint " << osinstance->getConstraintNames()[i] << std::endl;
		nl = parseGamsInstructions(codelen, opcodes, fields, constantlen, constants);
		if (!nl) {
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

	return true;
}

OSInstance* Gams2OSiL::takeOverOSInstance() {
	OSInstance* osinst = osinstance;
	osinstance = NULL;
	return osinst;
}

OSnLNode* Gams2OSiL::parseGamsInstructions(int codelen, int* opcodes, int* fields, int constantlen, double* constants) {
	std::vector<OSnLNode*> nlNodeVec;

	const bool debugoutput = false;

//	for (int i=0; i<codelen; ++i)
//		std::clog << i << '\t' << GamsOpCodeName[opcodes[i+1]] << '\t' << fields[i+1] << std::endl;

	nlNodeVec.reserve(codelen);

	for (int i=0; i<codelen; ++i) {
		GamsOpCode opcode = (GamsOpCode)opcodes[i];
		int address = fields[i]-1;

		if (debugoutput) std::clog << '\t' << GamsOpCodeName[opcode] << ": ";
//		if (opcode == nlStore) {
//			std::clog << "stop" << std::endl;
//			break;
//		}
		switch(opcode) {
			case nlNoOp : { // no operation
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushV : { // push variable
				address = gmoGetjSolver(gmo, address);
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
				address = gmoGetjSolver(gmo, address);
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
				address = gmoGetjSolver(gmo, address);
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
				address = gmoGetjSolver(gmo, address);
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
				address = gmoGetjSolver(gmo, address);
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
				address = gmoGetjSolver(gmo, address);
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

	if (!nlNodeVec.size()) return NULL;
	// the vector is in postfix format - create expression tree and return it
	return nlNodeVec[0]->createExpressionTreeFromPostfix(nlNodeVec);
}
