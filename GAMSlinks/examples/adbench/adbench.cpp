// Copyright (C) 2008 GAMS Development and others
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
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

#ifdef COIN_HAS_OS
#include "Smag2OSiL.hpp"
#include "OSInstance.h"
#else
class OSInstance;
#endif

enum EvalRoutine { ObjFunc=0, ObjGrad, ObjGradProd, ConFunc, Jacobi, LagHess, LagHessVector, LastRoutine };
string EvalRoutineName[LastRoutine] = { "f(x)", "f'(x)", "f'(x)*d", "c(x)", "c'(x)", "L''(x,l)", "L''(x,l)*v" }; 

typedef struct {
	EvalRoutine routine;
	int nrcalls;
	
	int g2d_nrsuccess;
	double g2d_acctime;
	
	int cppad_nrsuccess;
	double cppad_acctime;	
} timing_info;

double tolerance = 1e-8;

double relError(const double& a, const double& b) {
	double refval = fabs(a);
	if (fabs(b)>refval) refval = fabs(b);
	if (refval<0.1) return fabs(a-b);
	return fabs(a-b)/refval;
}

void runRoutine(smagHandle_t prob, OSInstance* osinstance, timing_info& timing);

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
	
	OSInstance* osinstance = NULL;
#ifdef COIN_HAS_OS
	Smag2OSiL smag2osil(prob);
	smag2osil.keep_original_instr = true;
	if (smag2osil.createOSInstance()) {
		osinstance = smag2osil.osinstance;
		if (smagColCount(prob)!=osinstance->getVariableNumber()) {
			cerr << "Number of variables in OSInstance and in SMAG problem different. SMAG: " << smagColCount(prob) << "\t OS: " << osinstance->getVariableNumber() << endl;
			osinstance = NULL;
		} else if (smagRowCount(prob)!=osinstance->getConstraintNumber()) {
			cerr << "Number of rows in OSInstance and in SMAG problem different. SMAG: " << smagRowCount(prob) << "\t OS: " << osinstance->getConstraintNumber() << endl;
			osinstance = NULL;
		}
	} else {
		cerr << "Creation of OSInstance failed." << endl;
	}
#endif
	
	double starttime = smagGetCPUTime(prob);
	bool have_hessian = !smagHessInit(prob);
	double endtime = smagGetCPUTime(prob);
	double g2d_hessinit_time = endtime - starttime;
  if (!have_hessian)
		smagStdOutputPrint(prob, SMAG_ALLMASK, "Failed to initialize Hessian structure in SMAG.\n");

#ifdef COIN_HAS_OS
  starttime = smagGetCPUTime(prob);
	if (!osinstance->initForAlgDiff()) {
		cerr << "Initialization of CppAD failed." << endl;
		osinstance = NULL;
	} else	{ // make one gradient evaluation of a nonlinear function to get more of CppAD initialized
		if (smagObjColCountNL(prob))
			osinstance->calculateObjectiveFunctionGradient(prob->colLev, -1, true);
		else
			osinstance->calculateAllConstraintFunctionGradients(prob->colLev, NULL, NULL, true, 1);
	}
	endtime = smagGetCPUTime(prob);
#endif

	cout << "Nr. cols: " << smagColCount(prob) << endl
		<< "Nr. rows: " << smagRowCount(prob) << endl
		<< "Nr. nonzeros: " << smagNZCount (prob) << endl;
	if (have_hessian)
		cout << "Nr. nonlinear nonzeros: " << prob->hesData->lowTriNZ << endl;

  if (have_hessian)
  	cout << "G2D Hessian initialization time: " << g2d_hessinit_time << endl;
  if (osinstance)
    cout << "CppAD initialization time: " << endtime - starttime << endl;
	cout.flush();

  srand48(1);
  
  cout << setw(10) << "Routine" << ' '
  	<< setw(10) << "Calls" << ' '
  	<< setw(10) << "Success" << ' '
  	<< setw(10) << "Acc. time" << ' '
  	<< setw(10) << "Avg. time" << ' '
#ifdef COIN_HAS_OS
  	<< setw(10) << "Success" << ' '
  	<< setw(10) << "Acc. time" << ' '
  	<< setw(10) << "Avg. time" << ' '
#endif
  	<< endl;
  
	for (int i=0; i<LastRoutine; ++i) {
		if (EvalRoutine(i)==LagHess) break;
		if (EvalRoutine(i)==ObjGradProd || EvalRoutine(i)==LagHessVector) continue;
		if (!have_hessian && EvalRoutine(i)==LagHess) break;
		timing_info timing;
		timing.nrcalls = (i>=LagHess ? 10 : 100);
		timing.routine = EvalRoutine(i);
		
		cout << setw(10) << EvalRoutineName[i] << ' ';
		runRoutine(prob, osinstance, timing);
		
		cout.precision(8);
		cout << setw(10) << timing.nrcalls << ' '
			<< setw(10) << timing.g2d_nrsuccess << ' '
			<< setw(10) << timing.g2d_acctime << ' '
			<< setw(10) << timing.g2d_acctime / timing.g2d_nrsuccess << ' '
#ifdef COIN_HAS_OS
			<< setw(10) << timing.cppad_nrsuccess << ' '
			<< setw(10) << timing.cppad_acctime << ' '
			<< setw(10) << timing.cppad_acctime / timing.cppad_nrsuccess << ' '
#endif
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

void runRoutine(smagHandle_t prob, OSInstance* osinstance, timing_info& timing) {
	timing.g2d_nrsuccess = 0;
	timing.g2d_acctime = 0.;
	timing.cppad_nrsuccess = 0;
	timing.cppad_acctime = 0.;
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
	int i1;

	for (int call=0; call<timing.nrcalls; ++call) {
		setRandom(x, smagColCount(prob), prob->colLB, prob->colUB, prob->inf);
		
		switch (timing.routine) {
			case ObjFunc: {
				double smagval = prob->inf;
				starttime = smagGetCPUTime(prob);
				if (!smagEvalObjFunc(prob, x, &smagval)) {
					endtime = smagGetCPUTime(prob);
					timing.g2d_acctime += endtime - starttime;
					++timing.g2d_nrsuccess;
				}
#ifdef COIN_HAS_OS
				if (osinstance) {
					double osval = prob->inf;
					starttime = smagGetCPUTime(prob);
					osval = osinstance->calculateAllObjectiveFunctionValues(x, true)[0];
					endtime = smagGetCPUTime(prob);
					timing.cppad_acctime += endtime - starttime;
					++timing.cppad_nrsuccess;

					if (smagval<prob->inf && osval < prob->inf && relError(smagval,osval)>tolerance)
						cout << "  Call " << call << ": Objective values reported by SMAG and OS/CppAD differ by more than " << tolerance << ": SMAG: " << smagval << "\t OS: " << osval << endl;
				}
#endif
			} break;
			
			case ObjGrad: {
//				for (int i=0; i<smagColCount(prob); ++i) cout << x[i] << '\t'; cout << endl;
				int smagsuccess;
				starttime = smagGetCPUTime(prob);
				smagsuccess = !smagEvalObjGrad(prob, x, &d1);
				endtime = smagGetCPUTime(prob);
				if (smagsuccess) {
					timing.g2d_acctime += endtime - starttime;
					++timing.g2d_nrsuccess;
				}
#ifdef COIN_HAS_OS
				double* osval = NULL;
				if (osinstance) {
					starttime = smagGetCPUTime(prob);
					osval = osinstance->calculateObjectiveFunctionGradient(x, -1, true);
					endtime = smagGetCPUTime(prob);
					timing.cppad_acctime += endtime - starttime;
					++timing.cppad_nrsuccess;

					if (osval && smagsuccess)
						for (smagObjGradRec_t* objGrad = prob->objGrad; objGrad; objGrad = objGrad->next) {
//							clog << "at " << objGrad->j << ": " << objGrad->dfdj << '\t' << osval[objGrad->j] << endl;
							if (relError(objGrad->dfdj, osval[objGrad->j])>tolerance)
								cout << "  Call " << call << ": Objective gradient coefficient " << objGrad->j << " reported by SMAG and OS/CppAD differ by more than " << tolerance << ": SMAG: " << objGrad->dfdj << "\t OS: " << osval[objGrad->j] << endl;
						}
				}
#endif
			} break;

			case ObjGradProd: {
				starttime = smagGetCPUTime(prob);
				if (!smagEvalObjGradProd(prob, x, d, &d1, &d2)) {
					endtime = smagGetCPUTime(prob);
					timing.g2d_acctime += endtime - starttime;
					++timing.g2d_nrsuccess;
				}
			} break;

			case ConFunc: {
				int smagsuccess;
				starttime = smagGetCPUTime(prob);
				smagsuccess = !smagEvalConFunc(prob, x, c); 
				endtime = smagGetCPUTime(prob);
				if (smagsuccess) {
					timing.g2d_acctime += endtime - starttime;
					++timing.g2d_nrsuccess;
				} else {
//					std::cerr << "SMAG constraint evaluation failed." << std::endl;
				}
#ifdef COIN_HAS_OS
				double* osval = NULL;
				if (osinstance) {
					starttime = smagGetCPUTime(prob);
					osval = osinstance->calculateAllConstraintFunctionValues(x, true);
					endtime = smagGetCPUTime(prob);
					timing.cppad_acctime += endtime - starttime;
					++timing.cppad_nrsuccess;

					if (osval && smagsuccess)
						for (int i=0; i<smagRowCount(prob); ++i)
							if (relError(c[i], osval[i])>tolerance)
								cout << "  Call " << call << ": Constraint " << i << " values reported by SMAG and OS/CppAD differ by more than " << tolerance << ": SMAG: " << c[i] << "\t OS: " << osval[i] << endl;
				}
#endif
			} break;

			case Jacobi: {
				int smagsuccess; 
				starttime = smagGetCPUTime(prob);
				smagsuccess = !smagEvalConGrad(prob, x);
				endtime = smagGetCPUTime(prob);
				if (smagsuccess) {
					timing.g2d_acctime += endtime - starttime;
					++timing.g2d_nrsuccess;
				}
#ifdef COIN_HAS_OS
				SparseJacobianMatrix* osjac = NULL;
				if (osinstance) {
					starttime = smagGetCPUTime(prob);
					osjac = osinstance->calculateAllConstraintFunctionGradients(x, NULL, NULL, true, 1);
					endtime = smagGetCPUTime(prob);
					timing.cppad_acctime += endtime - starttime;
					++timing.cppad_nrsuccess;

					if (osjac && smagsuccess)
						for (int c=0; c<smagRowCount(prob); ++c) {
							// this does not check for entries that SMAG reports and are missing in OS/CppAD
							for (int matidx = osjac->starts[c]; matidx < osjac->starts[c+1]; ++matidx) {
								smagConGradRec_t* conGrad = NULL;
//								if (osjac->starts[c+1]-osjac->starts[c+1]<=5)
//									cout << "x at " << osjac->indexes[matidx] << ": " << x[osjac->indexes[matidx]] << endl;
								for (conGrad = prob->conGrad[c]; conGrad; conGrad = conGrad->next)
									if (conGrad->j == osjac->indexes[matidx]) break;
								if (!conGrad) {
									if (fabs(osjac->values[matidx])>tolerance)
										cout << "  Call " << call << ": Constraint " << c << " derivative w.r.t. var. " << osjac->indexes[matidx] << " does not exist (=0) in SMAG but " << osjac->values[matidx] << " in OS/CppAD" << endl;
								} else if (relError(osjac->values[matidx], conGrad->dcdj)>tolerance)
										cout << "  Call " << call << ": Constraint " << c << " derivative w.r.t. var. " << conGrad->j  << " reported by SMAG and OS/CppAD differ by more than " << tolerance << ": SMAG: " << conGrad->dcdj << "\t OS: " << osjac->values[matidx] << endl;
							}
							
//							smagConGradRec_t* conGrad = prob->conGrad[c];
//							while(conGrad && matidx<osjac->starts[c+1]) {
//								clog << c << ": " << osjac->indexes[matidx] << '\t' << conGrad->j << endl;
//								while (matidx < osjac->starts[c+1] && osjac->indexes[matidx] < conGrad->j) {
//									if (osjac->values[matidx])
//										cout << "  Call " << call << ": Constraint " << c << " derivative w.r.t. var. " << osjac->indexes[matidx] << " is 0 in SMAG but " << osjac->values[matidx] << " in OS/CppAD" << endl;
//									++matidx;
//								}
//								while (conGrad && conGrad->j < osjac->indexes[matidx]) {
//									if (conGrad->dcdj)
//										cout << "  Call " << call << ": Constraint " << c << " derivative w.r.t. var. " << conGrad->j << " is " << conGrad->dcdj << " in SMAG but 0 in OS/CppAD" << endl;
//									conGrad = conGrad->next;
//								}
//								if (!conGrad)
//									cout << "  Call " << call << ": Constraint " << c << " gradient: SMAG gives less values than OS/CppAD" << endl;
//								else if (matidx >= osjac->starts[c+1])
//									cout << "  Call " << call << ": Constraint " << c << " gradient: OS/CppAD gives less values than SMAG" << endl;
//								else {
//									if (relError(osjac->values[matidx], conGrad->dcdj)>tolerance)
//										cout << "  Call " << call << ": Constraint " << c << " derivative w.r.t. var. " << conGrad->dcdj  << " reported by SMAG and OS/CppAD differ by more than " << tolerance << ": SMAG: " << conGrad->dcdj << "\t OS: " << osjac->values[matidx] << endl;
//									++matidx;
//									conGrad = conGrad->next;
//								}
//							}
						}
				}
#endif
			} break;
			
			case LagHess: {
				starttime = smagGetCPUTime(prob);
				smagEvalLagHess(prob, x, 1., c, h, prob->hesData->lowTriNZ, &i1);
				if (!i1) {
					endtime = smagGetCPUTime(prob);
					timing.g2d_acctime += endtime - starttime;
					++timing.g2d_nrsuccess;
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

