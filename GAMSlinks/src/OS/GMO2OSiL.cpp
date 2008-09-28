// Copyright (C) 2007-2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GMO2OSiL.hpp"

#include "OSInstance.h"
#include "CoinHelperFunctions.hpp"

extern "C" {
#include "gmocc.h"
}

GMO2OSiL::GMO2OSiL(gmoHandle_t gmo_)
: osinstance(NULL), gmo(gmo_)
{ }

GMO2OSiL::~GMO2OSiL() {
	delete osinstance;
}

bool GMO2OSiL::createOSInstance() {
	osinstance = new OSInstance();  
	int i, j;
//	char buffer[255];
	
//	GamsHandlerSmag gamshandler(smag);
//	GamsDictionary dict(gamshandler);
//	dict.readDictionary();

	// unfortunately, we do not know the model name
	osinstance->setInstanceDescription("Generated from GAMS GMO problem");
	
	osinstance->setVariableNumber(gmoN(gmo));
	char* var_types = new char[gmoN(gmo)];
	std::string* varnames = NULL;
	std::string* vartext = NULL;
//	if (dict.haveNames()) {
//		varnames=new std::string[smagColCount(smag)];
//		vartext=new std::string[smagColCount(smag)];
//	}
	for(i = 0; i < gmoN(gmo); ++i) {
		switch (gmoGetVarTypeOne(gmo, i)) {
			case var_X:
				var_types[i]='C';
				break;
			case var_B:
				var_types[i]='B';
				break;
			case var_I:
				var_types[i]='I';
				break;
			default : {
				// TODO: how to represent semicontinuous var. and SOS in OSiL ? 
				gmoLogStat(gmo, "Error: Unsupported variable type.");
				return false;
			}
		}
//		if (dict.haveNames() && dict.getColName(i, buffer, 256))
//			varnames[i]=buffer;
//		if (dict.haveNames() && dict.getColText(i, buffer, 256))
//			vartext[i]=buffer;
	}
	
	double* varlow = new double[gmoN(gmo)];
	double* varup  = new double[gmoN(gmo)];
	double* varlev = new double[gmoN(gmo)];
	
	gmoGetVarLower(gmo, varlow);
	gmoGetVarUpper(gmo, varup);
	gmoGetVarL(gmo, varlev);

	// store the descriptive text of a variables in the initString argument to have it stored somewhere
	if (!osinstance->setVariables(gmoN(gmo), varnames, varlow, varup, var_types, varlev, vartext))
		return false;
	delete[] var_types;
	delete[] varnames;
	delete[] vartext;
	delete[] varlow;
	delete[] varup;
	delete[] varlev;

//	gmoModelTypeTxt(gmo, buffer);
//	printf("model type: %s\n", buffer);
	
	if (false /* strcmp(buffer, "cns")==0 */) { // no objective in constraint satisfaction models
		osinstance->setObjectiveNumber(0);
	} else { // setup objective
		osinstance->setObjectiveNumber(1);
	
		SparseVector* objectiveCoefficients = new SparseVector(gmoObjNZ(gmo) - gmoObjNLNZ(gmo));

		int* colidx = new int[gmoObjNZ(gmo)];
		double* val = new double[gmoObjNZ(gmo)];
		int* nlflag = new int[gmoObjNZ(gmo)];
		int* dummy  = new int[gmoObjNZ(gmo)];
		
		gmoGetObjSparse(gmo, colidx, val, nlflag, dummy, dummy);
		for (i = 0, j = 0; i < gmoObjNZ(gmo); ++i) {
			if (nlflag[i]) continue;
			objectiveCoefficients->indexes[j] = colidx[i];
			objectiveCoefficients->values[j] = val[i];
			j++;
		  assert(j <= gmoObjNZ(gmo) - gmoObjNLNZ(gmo));
		}
		
		delete[] colidx;
		delete[] val;
		delete[] nlflag;
		delete[] dummy;

		std::string objname;
//		if (dict.haveNames() && dict.getObjName(buffer, 256))
//			objname=buffer;

		if (!osinstance->addObjective(-1, objname, gmoSense(gmo)==Obj_Min ? "min" : "max", gmoObjConst(gmo), 1., objectiveCoefficients)) {
			delete objectiveCoefficients;
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
				gmoLogStat(gmo, "Error: Unknown row type. Exiting ...");
				return false;
		}
//		std::string conname(dict.haveNames() ? dict.getRowName(i, buffer, 255) : NULL); 
		std::string conname;
		if (!osinstance->addConstraint(i, conname, lb, ub, 0.))
			return false;
	}
	
	double* values  = new double[gmoNZ(gmo)];
	int* colstarts  = new int[gmoN(gmo)+1];
	int* rowindexes = new int[gmoNZ(gmo)];
	int* nlflags    = new int[gmoNZ(gmo)];
	
	gmoGetMatrixCol(gmo, colstarts, rowindexes, values, nlflags);
	for (i = 0; i < gmoNZ(gmo); ++i)
		if (nlflags[i]) values[i] = 0.;
	colstarts[gmoN(gmo)] = gmoNZ(gmo);
	
	if (!osinstance->setLinearConstraintCoefficients(gmoNZ(gmo), true, 
		values, 0, gmoNZ(gmo)-1,
		rowindexes, 0, gmoNZ(gmo)-1,
		colstarts, 0, gmoN(gmo))) {
		delete[] nlflags;
		return false;
	}

	// values, colstarts, rowindexes are deleted by OSInstance
	delete[] nlflags;

	if (!gmoObjNLNZ(gmo) && !gmoNLNZ(gmo)) // everything linear -> finished
//	if (!gmoNLN(gmo)) // everything linear -> finished
		return true;
		
//	osinstance->instanceData->nonlinearExpressions->numberOfNonlinearExpressions = gmoNLM(gmo) + gmoObjNLNZ(gmo) ? 1 : 0;
	
	//TODO: nonlinear stuff
	
	return false;
}
