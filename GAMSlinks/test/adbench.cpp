// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsScip.cpp 432 2008-05-10 14:50:55Z stefan $
//
// Author: Stefan Vigerske

#include "GAMSlinksConfig.h"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

#ifdef HAVE_CSTDIO
#include <cstdio>
#else
#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "don't have header file for stdio"
#endif
#endif

#ifdef HAVE_CSTRING
#include <cstring>
#else
#ifdef HAVE_STRING_H
#include <string.h>
#else
#error "don't have header file for string"
#endif
#endif

#include <string>
#include <iostream>
#include <iomanip>
using namespace std;

#include "smag.h"
#include "SmagExtra.h"

enum EvalRoutine { ObjFunc=0, ObjGrad, ObjGradProd, ConFunc, Jacobi, LagHess, LagHessVector, LastRoutine };
string EvalRoutineName[LastRoutine] = { "f(x)", "f'(x)", "f'(x)*d", "c(x)", "c'(x)", "L''(x,l)", "L''(x,l)*v" }; 

typedef struct {
	EvalRoutine routine;
	int nrcalls;
	
	int nrsuccess;
	double acctime;
	
} timing_info;

void runRoutine(smagHandle_t prob, timing_info& timing);

int main(int argc, char** argv) {
#if defined(_MSC_VER)
  /* Prevents hanging "Application Error, Click OK" Windows in case something bad happens */
  { UINT oldMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); }
#endif
	if (argc==1) {
		fprintf(stderr, "usage: %s <control_file_name>\nexiting ...\n",  argv[0]);
		exit(EXIT_FAILURE);
	}

	smagHandle_t prob = smagInit(argv[1]);
  if (!prob) {
  	fprintf(stderr, "Error reading control file %s\nexiting ...\n", argv[1]);
		return EXIT_FAILURE;
  }

  char buffer[512];
  prob->logFlush = 1; // flush output more often to avoid funny look
  if (smagStdOutputStart(prob, SMAG_STATUS_OVERWRITE_IFDUMMY, buffer, sizeof(buffer)))
  	fprintf(stderr, "Warning: Error opening GAMS output files .. continuing anyhow\t%s\n", buffer);

  
  smagReadModelStats (prob);
  smagSetObjFlavor(prob, OBJ_FUNCTION);
	smagSetSqueezeFreeRows (prob, 1);	/* don't show me =n= rows */
	smagReadModel (prob);
	
	double starttime = smagGetCPUTime(prob);
	bool have_hessian = !smagHessInit(prob);
	double endtime = smagGetCPUTime(prob);
  if (!have_hessian)
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to initialize Hessian structure.\n");

	cout << "Nr. cols: " << smagColCount(prob) << endl
		<< "Nr. rows: " << smagRowCount(prob) << endl
		<< "Nr. nonzeros: " << smagNZCount (prob) << endl
		<< "Nr. nonlinear nonzeros: " << prob->hesData->lowTriNZ << endl;

  cout << "Hessian initialization time: " << endtime-starttime << endl;
	cout.flush();

  srand48(1);
  
  cout << setw(10) << "Routine" << ' '
  	<< setw(10) << "Calls" << ' '
  	<< setw(10) << "Success" << ' '
  	<< setw(10) << "Acc. time" << ' '
  	<< setw(10) << "Avg. time" << ' '
  	<< endl;
  
	for (int i=0; i<LastRoutine; ++i) {
		if (!have_hessian && EvalRoutine(i)==LagHess) break;
		timing_info timing;
		timing.nrcalls = (i>=LagHess ? 100 : 1000);
		timing.routine = EvalRoutine(i);
		
		cout << setw(10) << EvalRoutineName[i] << ' ';
		runRoutine(prob, timing);
		
		cout.precision(8);
		cout << setw(10) << timing.nrcalls << ' '
			<< setw(10) << timing.nrsuccess << ' '
			<< setw(10) << timing.acctime << ' '
			<< setw(10) << timing.acctime / timing.nrsuccess << ' '
			<< endl;
		cout.flush();
	}
  
  
	return 0;
}

inline double randomNumber(double lb, double ub, double& infty) {
	if (lb<=-infty) lb = (ub <= -1000 ? ub-1 : -1000.);
	if (ub>= infty) ub = (lb >= 1000. ? lb+1 :  1000.);
	return lb + drand48() * (ub-lb);
}

inline void setRandom(double* x, int n, double* lb, double* ub, double& infty) {
	while (n) {
		*x = randomNumber(*lb, *ub, infty);
		--n; ++x; ++lb; ++ub;
	}	
}

void runRoutine(smagHandle_t prob, timing_info& timing) {
	timing.nrsuccess = 0;
	timing.acctime = 0.;
	double* x = new double[smagColCount(prob)]; // every routine needs an x
	double* d = NULL;
	double* c = NULL;
	double* h = NULL;
	
	switch (timing.routine) {
		case ObjGradProd:
			d = new double[smagColCount(prob)];
			for (int i=0; i<smagColCount(prob); ++i) d[i] = -1+2*drand48();
			break;
		case LagHess:
			h = new double[prob->hesData->lowTriNZ];
			c = new double[smagRowCount(prob)];
			for (int i=0; i<smagRowCount(prob); ++i) c[i] = -1+2*drand48();
			break;
		case ConFunc:
			c = new double[smagRowCount(prob)];
			
	};

	double starttime, endtime;
	double d1, d2;
	int i1, i2;

	for (int call=timing.nrcalls; call; --call) {
		setRandom(x, smagColCount(prob), prob->colLB, prob->colUB, prob->inf);
		
		
		switch (timing.routine) {
			case ObjFunc: {
				starttime = smagGetCPUTime(prob);
				if (!smagEvalObjFunc(prob, x, &d1)) {
					endtime = smagGetCPUTime(prob);
					timing.acctime += endtime - starttime;
					++timing.nrsuccess;
				}
			} break;
			
			case ObjGrad: {
				starttime = smagGetCPUTime(prob);
				if (!smagEvalObjGrad(prob, x, &d1)) {
					endtime = smagGetCPUTime(prob);
					timing.acctime += endtime - starttime;
					++timing.nrsuccess;
				}
			} break;

			case ObjGradProd: {
				starttime = smagGetCPUTime(prob);
				if (!smagEvalObjGradProd(prob, x, d, &d1, &d2)) {
					endtime = smagGetCPUTime(prob);
					timing.acctime += endtime - starttime;
					++timing.nrsuccess;
				}
			} break;

			case ConFunc: {
				starttime = smagGetCPUTime(prob);
				if (!smagEvalConFunc(prob, x, c)) {
					endtime = smagGetCPUTime(prob);
					timing.acctime += endtime - starttime;
					++timing.nrsuccess;
				}
			} break;

			case Jacobi: {
				starttime = smagGetCPUTime(prob);
				if (!smagEvalConGrad(prob, x)) {
					endtime = smagGetCPUTime(prob);
					timing.acctime += endtime - starttime;
					++timing.nrsuccess;
				}
			} break;
			
			case LagHess: {
				starttime = smagGetCPUTime(prob);
				smagEvalLagHess(prob, x, c[0], c, h, prob->hesData->lowTriNZ, &i1);
				if (!i1) {
					endtime = smagGetCPUTime(prob);
					timing.acctime += endtime - starttime;
					++timing.nrsuccess;
				}				
			} break;
			
			case LagHessVector:

			default: {
				cerr << "Error: Timing of evaluation routine " << EvalRoutineName[timing.routine] << " not implemented." << endl;
				return;
			}
			
		}
		
		
	}
	
	delete[] x;
	delete[] d;
	delete[] c;
	delete[] h;
}

