// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsOsiHelper.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStartBasis.hpp"
#include "OsiSolverInterface.hpp"

#include "gmomcc.h"

bool gamsOsiLoadProblem(struct gmoRec* gmo, OsiSolverInterface& solver) {
	switch (gmoSense(gmo)) {
		case Obj_Min:
			solver.setObjSense(1.0);
			break;
		case Obj_Max:
			solver.setObjSense(-1.0);
			break;
		default:
			gmoLogStat(gmo, "Error: Unsupported objective sense.");
			return false;
	}
	
	// objective
	double* objcoeff = new double[gmoN(gmo)];
	gmoGetObjVector(gmo, objcoeff);
	solver.setDblParam(OsiObjOffset, -gmoObjConst(gmo)); // strange, but cbc seem to wanna have the constant with different sign
//	printf("obj constant: %g\n", gmoObjConst(gmo));

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
	for (int col = 0; col < gmoN(gmo); ++col) {
		colstarts[col+1] -= shift;
		int k = colstarts[col];
		while (k < colstarts[col+1]) {
			values[k] = values[k+shift];
			rowindexes[k] = rowindexes[k+shift];
			if (!values[k]) {
				++shift;
				--colstarts[col+1];
			} else {
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

	// right-hand-side and row sense
	double* rhs    = new double[gmoM(gmo)];
	int* rowtype   = new int[gmoM(gmo)];
	char* rowsense = new char[gmoM(gmo)];
	gmoGetRhs(gmo, rhs);
	gmoGetEquType(gmo, rowtype);
	for (int i = 0; i < gmoM(gmo); ++i)
		switch ((enum gmoEquType)rowtype[i]) {
			case equ_E: rowsense[i] = 'E'; break;
			case equ_G: rowsense[i] = 'G'; break;
			case equ_L: rowsense[i] = 'L'; break;
			case equ_N: rowsense[i] = 'N'; break;
			case equ_C:
				gmoLogStat(gmo, "Error: Conic constraints not supported by OSI.");
				return false;
			default:
				gmoLogStat(gmo, "Error: Unsupported row type.");
				return false;
		}
	double* rowrng = CoinCopyOfArrayOrZero((double*)NULL, gmoM(gmo));
	
//	printf("%d columns:\n", gmoN(gmo));
//	for (int i = 0; i < gmoN(gmo); ++i)
//		printf("lb %g\t ub %g\t obj %g\t colstart %d\n", varlow[i], varup[i], objcoeff[i], colstarts[i]);
//	printf("%d rows:\n", gmoM(gmo));
//	for (int i = 0; i < gmoM(gmo); ++i)
//		printf("rhs %g\t sense %c\t rng %g\n", rhs[i], rowsense[i], rowrng[i]);
//	printf("%d nonzero values:", nz);
//	for (int i = 0; i < nz; ++i)
//		printf("%d:%g ", rowindexes[i], values[i]);

	solver.loadProblem(gmoN(gmo), gmoM(gmo), colstarts, rowindexes, values, varlow, varup, objcoeff, rowsense, rhs, rowrng);  

	delete[] colstarts;
	delete[] rowindexes;
	delete[] values;
	delete[] nlflags;
	delete[] varlow;
	delete[] varup;
	delete[] objcoeff;
	delete[] rowtype;
	delete[] rowsense;
	delete[] rhs;
	delete[] rowrng;

	// tell solver which variables are discrete
	if (gmoNDisc(gmo)) {
		for (int i = 0; i < gmoN(gmo); ++i) {
			switch ((enum gmoVarType)gmoGetVarTypeOne(gmo, i)) {
				case var_B:  // binary
				case var_I:  // integer
				case var_SI: // semiinteger
					solver.setInteger(i);
					break;
				case var_X:  // probably this means continuous variable
				case var_S1: // in SOS1
				case var_S2: // in SOS2
				case var_SC: // semicontinuous
					break;
			}
		}
	}
	
	return true;
}


bool gamsOsiStoreSolution(struct gmoRec* gmo, const OsiSolverInterface& solver, bool swapRowStatus) {
	const double* colLevel  = solver.getColSolution();
	const double* colMargin = solver.getReducedCost(); 
	const double* rowLevel  = solver.getRowActivity();
	const double* rowMargin = solver.getRowPrice();		
	assert(colLevel);
	assert(rowLevel);

	int* colBasis = new int[solver.getNumCols()];
	int* rowBasis = new int[solver.getNumRows()];
	int* dummy    = CoinCopyOfArray((int*)NULL, CoinMax(gmoN(gmo), gmoM(gmo)), 0);
		
	if (solver.optimalBasisIsAvailable()) {
		solver.getBasisStatus(colBasis, rowBasis);

		// translate from OSI codes to GAMS codes
		for (int j = 0; j < gmoN(gmo); ++j) {
			// only for fully continuous variables we can give a reliable basis status
			if (gmoGetVarTypeOne(gmo, j) != var_X)
				colBasis[j] = Bstat_Super;
			else switch (colBasis[j]) {
				case 3: colBasis[j] = (fabs(colLevel[j] - gmoGetVarLowerOne(gmo, j)) > 1e-6) ? Bstat_Super : Bstat_Lower; break; // change to super if value is not on bound as it should be
				case 2: colBasis[j] = (fabs(colLevel[j] - gmoGetVarUpperOne(gmo, j)) > 1e-6) ? Bstat_Super : Bstat_Upper; break;
				case 1: colBasis[j] = Bstat_Basic; break;
				case 0: colBasis[j] = Bstat_Super; break;
				default: gmoLogStat(gmo, "Column basis status unknown!"); return false;
			}
		}
		for (int i = 0; i < gmoM(gmo); ++i) { 
			switch (rowBasis[i]) {
				case 2: rowBasis[i] = swapRowStatus ? Bstat_Upper : Bstat_Lower; break;
				case 3: rowBasis[i] = swapRowStatus ? Bstat_Lower : Bstat_Upper; break;
				case 1: rowBasis[i] = Bstat_Basic; break;
				case 0: rowBasis[i] = Bstat_Super; break;
				default: gmoLogStat(gmo, "Row basis status unknown!"); return false;
			}
		}
	} else {
		const CoinWarmStartBasis* wsb = dynamic_cast<const CoinWarmStartBasis*>(solver.getWarmStart());
		if (wsb) {
			for (int j = 0; j < gmoN(gmo); ++j)
				if (gmoGetVarTypeOne(gmo, j) != var_X)
					colBasis[j] = Bstat_Super;
				else switch (wsb->getStructStatus(j)) {
					case CoinWarmStartBasis::atLowerBound: colBasis[j] = (fabs(colLevel[j] - gmoGetVarLowerOne(gmo, j)) > 1e-6) ? Bstat_Super : Bstat_Lower; break; // change to super if value is not on bound as it should be
					case CoinWarmStartBasis::atUpperBound: colBasis[j] = (fabs(colLevel[j] - gmoGetVarUpperOne(gmo, j)) > 1e-6) ? Bstat_Super : Bstat_Upper; break;
					case CoinWarmStartBasis::basic:        colBasis[j] = Bstat_Basic; break;
					case CoinWarmStartBasis::isFree:       colBasis[j] = Bstat_Super; break;
					default: gmoLogStat(gmo, "Column basis status unknown!"); return false;
				}
			for (int j = 0; j < gmoM(gmo); ++j) {
				switch (wsb->getArtifStatus(j)) {
					case CoinWarmStartBasis::atLowerBound: rowBasis[j] = (!swapRowStatus) ? Bstat_Upper : Bstat_Lower; break; // for Cbc, the basis status seem to be flipped in CoinWarmStartBasis, but not in getBasisStatus
					case CoinWarmStartBasis::atUpperBound: rowBasis[j] = (!swapRowStatus) ? Bstat_Lower : Bstat_Upper; break;
					case CoinWarmStartBasis::basic:        rowBasis[j] = Bstat_Basic; break;
					case CoinWarmStartBasis::isFree:       rowBasis[j] = Bstat_Super; break;
					default: gmoLogStat(gmo, "Row basis status unknown!"); return false;
				}
			}
			delete wsb;
		} else {
			CoinFillN(colBasis, gmoN(gmo), (int)Bstat_Super);
			CoinFillN(rowBasis, gmoM(gmo), (int)Bstat_Super);
		}
	}
	
	gmoSetHeadnTail(gmo, HobjVal, solver.getObjValue());
	gmoSetSolution8(gmo, colLevel, colMargin, rowMargin, rowLevel, colBasis, dummy, rowBasis, dummy);

	delete[] colBasis;
	delete[] rowBasis;
	delete[] dummy;

	return true;
}
