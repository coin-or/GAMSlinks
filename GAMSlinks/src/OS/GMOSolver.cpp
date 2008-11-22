// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GMOSolver.hpp"

#include "OSiLReader.h"
#include "OSrLWriter.h"
//#include "OSSolverAgent.h"

#include <fstream>
#include <iostream>

#include "cpxcc.h"
#include "concc.h"
extern "C" {
#include "gclgms.h"
}

GMOSolver::GMOSolver()
: gmo(NULL), osilreader(NULL), osrlwriter(NULL)
{ }

GMOSolver::~GMOSolver()
{
	delete osilreader;
	delete osrlwriter;
	//TODO free gmo
}

void GMOSolver::solve() throw (ErrorClass) {
	if (!gmo)
		buildSolverInstance();
	
	
}
	
void GMOSolver::buildSolverInstance() throw(ErrorClass) {
	try{
		if(osil.length() == 0 && osinstance == NULL) throw ErrorClass("there is no instance");
		if(osinstance == NULL){
			osilreader = new OSiLReader();
			osinstance = osilreader->readOSiL( osil);
		}
		buildGmoInstance(osinstance);
	} catch(const ErrorClass& eclass){
		std::cout << "THERE IS AN ERROR" << std::endl;
		osresult->setGeneralMessage(eclass.errormsg);
		osresult->setGeneralStatusType("error");
		if (!osrlwriter)
			osrlwriter = new OSrLWriter();
		osrl = osrlwriter->writeOSrL(osresult);
		throw ErrorClass( osrl) ;
	}
	
}

void msgcallback0(int mode, const char* buf)
{
	printf(buf);
}

void msgcallback255(int mode, const char* buf)
{
//	const_cast<char*>(buf)[255]=0;
	printf(buf);
}

void GMOSolver::buildGmoInstance(OSInstance* osinstance) {
	assert(osinstance);
	
	std::cout << "Creating GMO object" << std::endl;
	
	char msg[255];
  if (!gmoCreate(&gmo, msg, sizeof(msg))) {
  	fprintf(stderr, "%s\n",msg);
    exit(EXIT_FAILURE);
  }

	gmoMsgCallback0Set(gmo, msgcallback0);
//	gmoMsgCallback255Set(gmo, msgcallback255);
  gmoSetIdent(gmo, "OSlink object");
  gmoIndexBaseSet(gmo, 0);
	gmoObjStyleSet(gmo, ObjType_Var);

	int nVar = osinstance->getVariableNumber();
	int nEqu = osinstance->getConstraintNumber();
	int nNZ  = osinstance->getLinearConstraintCoefficientNumber();
	if (osinstance->getObjectiveNumber()) {
		assert(osinstance->getObjectiveNumber() == 1);
		++nVar; // for objective function variable
		++nEqu; // for objective function row
		nNZ += osinstance->getObjectiveCoefficientNumbers()[0] + 1; // for objfunction row incl. objfunc.var.entry
	}
	
	int ProcType = Proc_lp;
	if (osinstance->getNumberOfIntegerVariables() || osinstance->getNumberOfBinaryVariables())
		ProcType = Proc_mip;
	
	gmoInitData(gmo, nVar, nEqu, nNZ, ProcType);
//	gmoMinfSet(gmo, 4e+300); // set user infinity to gams infinity (workaround for other gmo bug)
//	gmoPinfSet(gmo, 3e+300);
	
	int rowE, rowG, rowL, rowN;
	rowE = rowG = rowL = rowN = 0;
	for (int i = 0; i < osinstance->getConstraintNumber(); ++i)
	{
		int matchEqu = 0;
		double mEqu = 0.;
		int basEqu = Bstat_Super;
		
		int typEqu;
		double rhsEqu;
		//TODO take care of constraint constants
		switch (osinstance->getConstraintTypes()[i]) {
			case 'R' :
				std::cerr << "Error: Ranged constraints not supported." << std::endl;
				exit(EXIT_FAILURE);
			case 'L' :
				typEqu = equ_L;
				rhsEqu = osinstance->getConstraintUpperBounds()[i];
				++rowL;
				break;
			case 'G' :
				typEqu = equ_G;
				rhsEqu = osinstance->getConstraintLowerBounds()[i];
				++rowG;
				break;
			case 'E' :
				typEqu = equ_E;
				rhsEqu = osinstance->getConstraintLowerBounds()[i];
				++rowE;
				break;
			case 'U' :
				typEqu = equ_N;
				rhsEqu = 0.;
				++rowN;
				break;
			default:
				std::cerr << "Error: Unsupported constraint type: " << osinstance->getConstraintTypes()[i] << std::endl;
				exit(EXIT_FAILURE);	
		}
		printf("add row %d\n", i);
		// workaround GMO bug : +1
		gmoAddRow(gmo, i+1, typEqu, matchEqu, rhsEqu, mEqu, basEqu);	
	}
	
	if (osinstance->getObjectiveNumber()) {
		printf("add objrow %d\n", nEqu-1);
		// workaround GMO bug : +1
		gmoAddRow(gmo, nEqu-1+1, equ_E, 0 /*?*/, 0., 0., Bstat_Super);
	}
	
	SparseMatrix* mat = osinstance->getLinearConstraintCoefficientsInColumnMajor();
	SparseVector* obj = NULL;
	int obj_it = 0;
	if (osinstance->getObjectiveNumber()) {
		obj = osinstance->getObjectiveCoefficients()[0];
	} else {
		obj_it = nVar;
	}
	
	int colC, colB, colI;
	colC = colB = colI = 0;
	for (int i = 0; i < osinstance->getVariableNumber(); ++i) {
		double objcoef = 0.;
		if (obj && obj_it < obj->number && obj->indexes[obj_it] == i) {
			objcoef = obj->values[obj_it];
			++obj_it;
		}
		
		int nzVar = mat->starts[i+1] - mat->starts[i] + (objcoef ? 1 : 0);
		// TODO: get this out of the loop
		printf("nzVar: %d\n", nzVar);
		int*    rowArr = new    int[nzVar];
		double* jacArr = new double[nzVar];
		int*     nlArr = new    int[nzVar];
		memcpy(rowArr, &mat->indexes[mat->starts[i]], (mat->starts[i+1] - mat->starts[i])*sizeof(int));
		memcpy(jacArr, &mat->values[mat->starts[i]],  (mat->starts[i+1] - mat->starts[i])*sizeof(double));
		memset(nlArr,  0, nzVar*sizeof(int));
		
		if (objcoef) {
			rowArr[nzVar-1] = nEqu - 1;
			jacArr[nzVar-1] = objcoef;
		}
		
		// workaround GMO bug : +1
		for (int j = 0; j < nzVar; ++j) {
			rowArr[j]++;
		}
		
		int typVar;
		switch (osinstance->getVariableTypes()[i]) {
			case 'C' :
				typVar = var_X;
				++colC;
				break;
			case 'B' :
				typVar = var_B;
				++colB;
				break;
			case 'I' :
				typVar = var_I;
				++colI;
				break;
			default:
				std::cerr << "Error: Unsupported variable type: " << osinstance->getVariableTypes()[i] << std::endl;
				exit(EXIT_FAILURE);		
		}
		
		double loVar = osinstance->getVariableLowerBounds()[i];
		double upVar = osinstance->getVariableUpperBounds()[i];
		if (loVar == -OSDBL_MAX) loVar = GMS_SV_MINF;
		if (upVar ==  OSDBL_MAX) upVar = GMS_SV_PINF;
		double lVar = (0. > upVar ? upVar : (0. < loVar ? loVar : 0.));
		double mVar = 0.;
		int basVar = Bstat_Super;
		int sosVar = 0;
		double priorVar = 1.;
		
		// workaround GMO bug : +1
		printf("%d %g %g\n", i, loVar, upVar);
		gmoAddCol(gmo, i+1, typVar, loVar, lVar, upVar, mVar, basVar, sosVar, priorVar, nzVar, rowArr, jacArr, nlArr, NULL);
		
		delete[] rowArr;
		delete[] jacArr;
		delete[] nlArr;
	}
	
	if (osinstance->getObjectiveNumber()) {
		int rowidx = nEqu-1;
		double coef = -1.;
		int nl = 0;
		++rowidx; // workaround GMO bug
		// workaround GMO bug : +1
		gmoAddCol(gmo, nVar-1+1, var_X, GMS_SV_MINF, 0., GMS_SV_PINF, 0., Bstat_Super, 0, 1., 1, &rowidx, &coef, &nl, NULL);
	}
	
	
	
	gmoSetOptI(gmo, I_ModelType, ProcType);
  gmoSetOptI(gmo, I_M, nEqu);
  gmoSetOptI(gmo, I_N, nVar);
  if (osinstance->getObjectiveNumber() && osinstance->getObjectiveMaxOrMins()[0]=="max")
  	gmoSetOptI(gmo, I_Sense, Obj_Max);
  else
  	gmoSetOptI(gmo, I_Sense, Obj_Min);
  if (osinstance->getObjectiveNumber()) {
  	gmoSetOptI(gmo, I_ObjVar, nVar-1+1); // workaround
  	gmoSetOptI(gmo, I_ObjRow, nEqu-1+1); // workaround
  } else {
  	gmoSetOptI(gmo, I_ObjVar, 0);
  	gmoSetOptI(gmo, I_ObjRow, 0);
  }
  gmoSetOptI(gmo, I_OptFile, 0);
  gmoSetOptI(gmo, I_NLCcode, 0);
  gmoSetOptI(gmo, I_NLConst, 0);
  gmoSetOptI(gmo, I_NZ, nNZ);
  gmoSetOptI(gmo, I_NLNZ, 0);
  gmoSetOptI(gmo, I_NLM, 0);
  gmoSetOptI(gmo, I_NLN, 0);
  gmoSetOptI(gmo, I_NLSCratch, 0);
  gmoSetOptI(gmo, I_LogOption, 1);
  gmoSetOptI(gmo, I_XNZ, 0);
  gmoSetOptI(gmo, I_NZObj,2);
  gmoSetOptI(gmo, I_OBJBounds, 0);
  gmoSetOptI(gmo, I_OBJEquType, ObjType_Var);
  gmoSetOptI(gmo, I_Reform, 0);
  gmoSetOptI(gmo, I_DictFile, 0);
  gmoSetOptI(gmo, I_SBBFlag, 0);
  gmoSetOptI(gmo, I_Num_equ_E, rowE);
  gmoSetOptI(gmo, I_Num_equ_G, rowG);
  gmoSetOptI(gmo, I_Num_equ_L, rowL);
  gmoSetOptI(gmo, I_Num_equ_N, rowN);
  gmoSetOptI(gmo, I_Num_equ_C, 0);
  gmoSetOptI(gmo, I_UseCutoff, 0);
  gmoSetOptI(gmo, I_UseCheat, 0);
  gmoSetOptI(gmo, I_Num_Var_X, colC);
  gmoSetOptI(gmo, I_Num_Var_B, colB);
  gmoSetOptI(gmo, I_Num_Var_I, colI);
  gmoSetOptI(gmo, I_DomLim, 0);
  gmoSetOptI(gmo, I_Iterlim, 10000); // TODO maybe just put a very large number here?
  gmoSetOptI(gmo, I_NumFX_var_X, 0);
  gmoSetOptI(gmo, I_NumFX_var_, 0);
  gmoSetOptI(gmo, I_NumFX_Var_B, 0);
  gmoSetOptI(gmo, I_NumFX_Var_I, 0);
  gmoSetOptI(gmo, I_NumFX_Var_X_P, 0);
  gmoSetOptI(gmo, I_NumFX_Var_X_N, 0);
  gmoSetOptI(gmo, I_NumFX_Var_X_F, 0);
  gmoSetOptI(gmo, I_NumFX_Var_S1, 0);
  gmoSetOptI(gmo, I_NumFX_Var_S2, 0);
  gmoSetOptI(gmo, I_NumFX_Var_SC, 0);
  gmoSetOptI(gmo, I_NumFX_Var_SI, 0);
  gmoSetOptI(gmo, I_MaxSingleNL, 0);

  gmoSetOptR(gmo, R_OptCR, 0.1); // TODO: maybe just put 0. ?
  gmoSetOptR(gmo, R_OptCA, 0.0);
  gmoSetOptR(gmo, R_ResLim, 1000); // TODO: maybe just put a very large number here?
  gmoSetOptR(gmo, R_CutOff, 0.0);
  gmoSetOptR(gmo, R_Cheat, 0.0);
  gmoSetOptR(gmo, R_TryInt, 0.0);

  gmoSetOptS(gmo, S_NameCntrFile, "");
  gmoSetOptS(gmo, S_NameLogFile, "OSiLGMOLog.txt");
  gmoSetOptS(gmo, S_NameStaFile, "OSiLGMOStat.txt");
  gmoSetOptS(gmo, S_NameOptFile, "");
  gmoSetOptS(gmo, S_NameSolFile, "");
  gmoSetOptS(gmo, S_NameScrDir, ".");
  gmoSetOptS(gmo, S_NameSysDir, "/home/stefan/work/gams/gams/"); // TODO !!!
  gmoSetOptS(gmo, S_NameWrkDir, "");
  gmoSetOptS(gmo, S_NameDLL, "");
  gmoSetOptS(gmo, S_NameCurDir, "");
	

  gmoLoadInfoGms(gmo, "");
  gmoLoadNL(gmo, 1);
  gmoDetReform(gmo);
  gmoOpenGms(gmo);
  

//  cpxHandle_t cpx = NULL;
//  cpxCreate(&cpx, msg, sizeof(msg));
//  cpxReadyAPI(cpx, gmo, NULL, NULL);
//  cpxCallSolver(cpx);
//  cpxFree(&cpx);
  
  conHandle_t conopt = NULL;
  conCreate(&conopt, msg, sizeof(msg));
  conReadyAPI(conopt, gmo, NULL, NULL);
  conCallSolver(conopt);
  conFree(&conopt);
  
}
