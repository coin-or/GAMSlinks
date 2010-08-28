// Copyright (C) GAMS Development and others 2007-2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsOSxL.hpp"
#include "GamsNLinstr.h"

#include "OSInstance.h"
#include "OSResult.h"
#include "OSrLReader.h"
#include "OSErrorClass.h"
#include "CoinHelperFunctions.hpp"

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

#include <sstream>

#include "gmomcc.h"
#include "gevmcc.h"
/* value for not available/applicable */
#if GMOAPIVERSION >= 7
#define GMS_SV_NA     gmoValNA(gmo)
#else
#define GMS_SV_NA     2.0E300
#endif

#if GMOAPIVERSION < 8
#define Hresused     HresUsed
#define Hobjval      HobjVal
#endif

GamsOSxL::GamsOSxL(gmoHandle_t gmo_)
: gmo(gmo_), gev(gmo_ ? (gevRec*)gmoEnvironment(gmo_) : NULL), gmo_is_our(false), osinstance(NULL)
{ }

GamsOSxL::GamsOSxL(const char* datfile)
: gmo(NULL), gev(NULL), gmo_is_our(false), osinstance(NULL)
{ initGMO(datfile);
}

GamsOSxL::~GamsOSxL() {
	delete osinstance;

	if (gmo_is_our) {
	  //close output channels
	  //gmoCloseGms(gmo);
	  gmoFree(&gmo);
	  gevFree(&gev);
	  gmoLibraryUnload();
	  gevLibraryUnload();
	}
}

void GamsOSxL::setGMO(struct gmoRec* gmo_) {
  assert(gmo == NULL);
  assert(gev == NULL);
  assert(osinstance == NULL);
  assert(gmo_is_our == false);
  assert(gmo_ != NULL);

  gmo = gmo_;
  gev = (gevRec*)gmoEnvironment(gmo);
}

bool GamsOSxL::initGMO(const char* datfile) {
  assert(gmo == NULL);
  assert(gev == NULL);
  assert(osinstance == NULL);

  char msg[1024];
  int rc;

  if (!gmoCreate(&gmo, msg, sizeof(msg))) {
    fprintf(stderr, "%s\n",msg);
    return false;
  }
  gmo_is_our = true;

  if (!gevCreate(&gev, msg, sizeof(msg))) {
    fprintf(stderr, "%s\n",msg);
    return false;
  }

  // load control file
  if ((rc = gevInitEnvironmentLegacy(gev, datfile))) {
    fprintf(stderr, "Could not init gams environment: %s Rc = %d\n", datfile, rc);
    gmoFree(&gmo);
    gevFree(&gev);
    return false;
  }

  if ((rc = gmoRegisterEnvironment(gmo, gev, msg))) {
    gevLogStat(gev, "Could not register environment.");
    gmoFree(&gmo);
    gevFree(&gev);
    return false;
  }

  // setup GAMS output channels
//  if ((rc = gmoOpenGms( gmo))) {
//    fprintf(stderr, "Could not open GAMS environment. Rc = %d\n", rc);
//    gmoFree(&gmo);
//    return false;
//  }

  if ((rc = gmoLoadDataLegacy(gmo, msg))) {
    gevLogStat(gev, "Could not load model data.");
//    gmoCloseGms(gmo);
    gmoFree(&gmo);
    gevFree(&gev);
    return false;
  }

  gmoMinfSet(gmo, -OSDBL_MAX);
  gmoPinfSet(gmo,  OSDBL_MAX);
  gmoObjReformSet(gmo, 1);
  gmoObjStyleSet(gmo, ObjType_Fun);
  gmoIndexBaseSet(gmo, 0);

  return true;
}

bool GamsOSxL::createOSInstance() {
	int i, j;
	char buffer[255];

  /* delete possible old instance and create a new one */
  delete osinstance;
  osinstance = new OSInstance();

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

		SparseVector* objectiveCoefficients = NULL;

		if (gmoN(gmo)) {
			objectiveCoefficients = new SparseVector(gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

			int* colidx = new int[gmoObjNZ(gmo)];
			double* val = new double[gmoObjNZ(gmo)];
			int* nlflag = new int[gmoObjNZ(gmo)];
			int dummy;

			if (gmoObjNZ(gmo)) nlflag[0] = 0; // workaround for gmo bug
			gmoGetObjSparse(gmo, colidx, val, nlflag, &dummy, &dummy);
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
		} else {
			objectiveCoefficients = new SparseVector(0);
		}

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

OSInstance* GamsOSxL::takeOverOSInstance() {
	OSInstance* osinst = osinstance;
	osinstance = NULL;
	return osinst;
}

OSnLNode* GamsOSxL::parseGamsInstructions(int codelen, int* opcodes, int* fields, int constantlen, double* constants) {
	std::vector<OSnLNode*> nlNodeVec;

	const bool debugoutput = false;

//	for (int i=0; i<codelen; ++i)
//		std::clog << i << '\t' << GamsOpCodeName[opcodes[i+1]] << '\t' << fields[i+1] << std::endl;

	nlNodeVec.reserve(codelen);

	int nargs = -1;

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
			    case fnpoly: { /* simple polynomial */
						if (debugoutput) std::clog << "polynom of degree " << nargs-1 << std::endl;
						assert(nargs >= 0);
						switch (nargs) {
							case 0:
								delete nlNodeVec.back(); nlNodeVec.pop_back(); // delete variable of polynom
								nlNodeVec.push_back( new OSnLNodeNumber() ); // push zero
								break;
							case 1: { // "constant" polynom
								assert(nlNodeVec.size() >= 2);
								// delete variable
								delete nlNodeVec[nlNodeVec.size()-2];
								// put constant there
								nlNodeVec[nlNodeVec.size()-2] = nlNodeVec.back();
								// forget last element
								nlNodeVec.pop_back();
							} break;
							default: { // polynom is at least linear
								std::vector<OSnLNode*> coeff(nargs);
								while (nargs) {
									assert(!nlNodeVec.empty());
									coeff[nargs-1] = nlNodeVec.back();
									nlNodeVec.pop_back();
									--nargs;
								}
								assert(!nlNodeVec.empty());
								OSnLNode* var = nlNodeVec.back(); nlNodeVec.pop_back();

								nlNodeVec.push_back(coeff[0]);

								nlNodeVec.push_back(coeff[1]);
								nlNodeVec.push_back(var);
								nlNodeVec.push_back(new OSnLNodeTimes());

								nlNodeVec.push_back(new OSnLNodePlus());
								for (size_t i = 2; i < coeff.size(); ++i) {
									nlNodeVec.push_back(coeff[i]);
									nlNodeVec.push_back(var);
									if (i == 2) {
										nlNodeVec.push_back(new OSnLNodeSquare());
									} else {
										OSnLNodeNumber* exponent = new OSnLNodeNumber(); exponent->value = (double)i;
										nlNodeVec.push_back(exponent);
										nlNodeVec.push_back(new OSnLNodePower());
									}
									nlNodeVec.push_back(new OSnLNodeTimes());
									nlNodeVec.push_back(new OSnLNodePlus());
								}
							}
						}
						nargs = -1;
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
					default : {
						if (debugoutput) std::cerr << "nr. " << func << " - unsuppored. Error." << std::endl;
		            std::cerr << "GAMS function " << func << "not supported - Error." << std::endl;
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
				nargs = address;
				if (debugoutput) std::clog << nargs << "arguments" << std::endl;
			} break;
#if GMOAPIVERSION < 8
			case nlArg: {
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
#endif
			case nlHeader: { // header
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
			case nlPushZero: {
				if (debugoutput) std::clog << "push constant zero" << std::endl;
				nlNodeVec.push_back( new OSnLNodeNumber() );
			} break;
#if GMOAPIVERSION < 8
			case nlStoreS: { // store scaled row
				if (debugoutput) std::clog << "ignored" << std::endl;
			} break;
#endif
			default: {
				std::cerr << "GAMS instruction " << opcode << "not supported - Error." << std::endl;
				return NULL;
			}
		}
	}

	if (!nlNodeVec.size()) return NULL;
	// the vector is in postfix format - create expression tree and return it
	return nlNodeVec[0]->createExpressionTreeFromPostfix(nlNodeVec);
}

void GamsOSxL::writeSolution(OSResult& osresult) {
  if (osresult.general == NULL) {
    gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
    gmoSolveStatSet(gmo, SolveStat_SolverErr);
    gevLogStat(gev, "Error: OS result does not have header.");
    return;
  } else if (osresult.getGeneralStatusType() == "error") {
    gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
    gmoSolveStatSet(gmo, SolveStat_SolverErr);
    gevLogStatPChar(gev, "Error: OS result reports error: ");
    gevLogStatPChar(gev, osresult.getGeneralMessage().c_str());
    gevLogStat(gev, "");
    return;
  } else if (osresult.getGeneralStatusType() == "warning") {
    gevLogStatPChar(gev, "Warning: OS result reports warning: ");
    gevLogStatPChar(gev, osresult.getGeneralMessage().c_str());
    gevLogStat(gev, "");
  }

  gmoSolveStatSet(gmo, SolveStat_Normal);

  if (osresult.getSolutionNumber() == 0) {
    gmoModelStatSet(gmo, ModelStat_NoSolutionReturned);
  } else if (osresult.getSolutionStatusType(0) == "unbounded") {
    gmoModelStatSet(gmo, ModelStat_Unbounded);
  } else if (osresult.getSolutionStatusType(0) == "globallyOptimal") {
    gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
  } else if (osresult.getSolutionStatusType(0) == "locallyOptimal") {
    gmoModelStatSet(gmo, ModelStat_OptimalLocal);
  } else if (osresult.getSolutionStatusType(0) == "optimal") {
    gmoModelStatSet(gmo, ModelStat_OptimalGlobal);
  } else if (osresult.getSolutionStatusType(0) == "bestSoFar") {
    gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed); // or should we report integer solution if integer var.?
  } else if (osresult.getSolutionStatusType(0) == "feasible") {
    gmoModelStatSet(gmo, ModelStat_NonOptimalIntermed); // or should we report integer solution if integer var.?
  } else if (osresult.getSolutionStatusType(0) == "infeasible") {
    gmoModelStatSet(gmo, ModelStat_InfeasibleGlobal);
  } else if (osresult.getSolutionStatusType(0) == "stoppedByLimit") {
    gmoSolveStatSet(gmo, SolveStat_Iteration); // just a guess
    gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
  } else if (osresult.getSolutionStatusType(0) == "unsure") {
    gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
  } else if (osresult.getSolutionStatusType(0) == "error") {
    gmoModelStatSet(gmo, ModelStat_ErrorUnknown);
  } else if (osresult.getSolutionStatusType(0) == "other") {
    gmoModelStatSet(gmo, ModelStat_InfeasibleIntermed);
  } else {
    gmoModelStatSet(gmo, ModelStat_ErrorUnknown);
  }

  if (osresult.getVariableNumber() != gmoN(gmo)) {
    gevLogStat(gev, "Error: Number of variables in OS result does not match with gams model.");
    gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
    gmoSolveStatSet(gmo, SolveStat_SystemErr);
    return;
  }
  if (osresult.getConstraintNumber() != gmoM(gmo)) {
    gevLogStat(gev, "Error: Number of constraints in OS result does not match with gams model.");
    gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
    gmoSolveStatSet(gmo, SolveStat_SystemErr);
    return;
  }

  OptimizationSolution* sol = osresult.optimization->solution[0];

  int* colBasStat = CoinCopyOfArray((int*)NULL, gmoN(gmo), (int)Bstat_Super);
  int* colIndic   = CoinCopyOfArray((int*)NULL, gmoN(gmo), (int)Cstat_OK);
  double* colMarg = CoinCopyOfArray((double*)NULL, gmoN(gmo), GMS_SV_NA);
  double* colLev  = CoinCopyOfArray((double*)NULL, gmoN(gmo), GMS_SV_NA);

  int* rowBasStat = CoinCopyOfArray((int*)NULL, gmoM(gmo), (int)Bstat_Super);
  int* rowIndic   = CoinCopyOfArray((int*)NULL, gmoM(gmo), (int)Cstat_OK);
  double* rowLev  = CoinCopyOfArray((double*)NULL, gmoM(gmo), GMS_SV_NA);
  double* rowMarg = CoinCopyOfArray((double*)NULL, gmoM(gmo), GMS_SV_NA);
  //TODO
//  if (sol->constraints && sol->constraints->values) // set row levels, if available
//    for (std::vector<ConValue*>::iterator it(sol->constraints->values->con.begin());
//    it!=sol->constraints->values->con.end(); ++it)
//      rowLev[(*it)->idx]=(*it)->value;
  if (sol->constraints && sol->constraints->dualValues) // set row dual values, if available
    for (int i = 0; i < sol->constraints->dualValues->numberOfCon; ++i)
      rowMarg[sol->constraints->dualValues->con[i]->idx] = sol->constraints->dualValues->con[i]->value;
  if (sol->variables && sol->variables->values) // set var values, if available
    for (int i = 0; i < sol->variables->values->numberOfVar; ++i)
      colLev[sol->variables->values->var[i]->idx] = sol->variables->values->var[i]->value;
  if (sol->variables)
    for (int i=0; i<sol->variables->numberOfOtherVariableResults; ++i) {
      if (sol->variables->other[i]->name=="reduced costs") {
        for (int j = 0; j < sol->variables->other[i]->numberOfVar; ++j)
          colMarg[sol->variables->other[i]->var[j]->idx] = atof(sol->variables->other[i]->var[j]->value.c_str());
        break;
      }
    }
  gmoSetSolution8(gmo, colLev, colMarg, rowMarg, rowLev, colBasStat, colIndic, rowBasStat, rowIndic);

  delete[] rowLev;
  delete[] rowMarg;
  delete[] rowBasStat;
  delete[] rowIndic;
  delete[] colLev;
  delete[] colMarg;
  delete[] colBasStat;
  delete[] colIndic;

  if (sol->objectives && sol->objectives->values && sol->objectives->values->obj[0])
    gmoSetHeadnTail(gmo, Hobjval, sol->objectives->values->obj[0]->value);
}

void GamsOSxL::writeSolution(std::string& osrl) {
  OSResult* osresult = NULL;
  OSrLReader osrl_reader;
  try {
    osresult = osrl_reader.readOSrL(osrl);
  } catch(const ErrorClass& error) {
    gevLogStat(gev, "Error parsing the OS result string:");
    gevLogStatPChar(gev, error.errormsg.c_str());
    gmoModelStatSet(gmo, ModelStat_ErrorNoSolution);
    gmoSolveStatSet(gmo, SolveStat_SystemErr);
    return;
  }
  if (osresult)
    writeSolution(*osresult);
}
