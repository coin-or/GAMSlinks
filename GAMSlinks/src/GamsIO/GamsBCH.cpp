// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include "GamsBCH.hpp"
#include "CoinHelperFunctions.hpp"

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
#ifdef HAVE_CASSERT
#include <cassert>
#else
#ifdef HAVE_ASSERT_H
#include <assert.h>
#else
#error "don't have header file for assert"
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

extern "C" {
#include "iolib.h"
#include "dict.h"
#include "bch.h"
}

GamsBCH::GamsBCH(GamsModel& gm_, GamsOptions& opt_)
: gm(gm_), opt(opt_), dict(gm_.dict)
{ init();
}

GamsBCH::~GamsBCH() {
	delete[] node_x;
	delete[] node_lb;
	delete[] node_ub;
	delete[] incumbent;
	delete[] global_lb;
	delete[] global_ub;
}

void GamsBCH::init() {
  char buffer[256];
	
	node_x=new double[iolib.ncols];
	node_lb=new double[iolib.ncols];
	node_ub=new double[iolib.ncols];
	incumbent=new double[iolib.ncols];
	global_lb=new double[iolib.ncols];
	global_ub=new double[iolib.ncols];
//  cbinfo.heurcall   = (char *) malloc(1024*sizeof(char));
//  cbinfo.incbcall   = (char *) malloc(1024*sizeof(char));
//  cbinfo.incbicall  = (char *) malloc(1024*sizeof(char));
	
  char gdxprefix[256];
  if (opt.isDefined("usergdxprefix"))
  	opt.getString("usergdxprefix", gdxprefix);
  else *gdxprefix=0;
  
  if (opt.isDefined("userjobid"))
  	opt.getString("userjobid", userjobid);
  else *userjobid=0;
  
  opt.getString("usergdxname", buffer);
  // remove ".gdx" suffix, if exists
  int len=strlen(buffer);
  if (len>4 && strncmp(buffer+(len-4), ".gdx", 4)==0)
  	buffer[len-4]=0;
  snprintf(gdxname, 1024, "%s%s%s.gdx", gdxprefix, buffer, userjobid);
  
  opt.getString("usergdxnameinc", buffer);
  len=strlen(buffer);
  if (len>4 && strncmp(buffer+(len-4), ".gdx", 4)==0)
  	buffer[len-4]=0;
  snprintf(gdxnameinc, 1024, "%s%s%s.gdx", gdxprefix, buffer, userjobid);
  
  opt.getString("usergdxin", buffer);
  len=strlen(buffer);
  if (len>4 && strncmp(buffer+(len-4), ".gdx", 4)==0)
  	buffer[len-4]=0;
  snprintf(usergdxin, 1024, "%s%s%s.gdx", gdxprefix, buffer, userjobid);
  
  opt.getString("usercutcall", cutcall);
  len=strlen(cutcall);
  if (*userjobid)
  	snprintf(cutcall+len, 1024-len, " --userjobid=%s", userjobid);

  userkeep=opt.getBool("userkeep");

  cutfreq=opt.getInteger("usercutfreq");
  cutinterval=opt.getInteger("usercutinterval");
  cutmult=opt.getInteger("usercutmult");
  cutfirst=opt.getInteger("usercutfirst");
  cutnewint=opt.getBool("usercutnewint");

  opt.getString("userheurcall", heurcall);
  len=strlen(heurcall);
  if (*userjobid)
  	snprintf(heurcall+len, 1024-len, " --userjobid=%s", userjobid);

  heurfreq=opt.getInteger("userheurfreq");
  heurinterval=opt.getInteger("userheurinterval");
  heurmult=opt.getInteger("userheurmult");
  heurfirst=opt.getInteger("userheurfirst");
  heurnewint=opt.getInteger("userheurnewint");
  heurobjfirst=opt.getInteger("userheurobjfirst");
	
  ncalls=0;
  have_incumbent=false;
  new_incumbent=false;
	
  bchSlvSetInf(gm.getPInfinity(), gm.getMInfinity());
  setGlobalBounds(gm.ColLb(), gm.ColUb());

  printf("GamsBCH initialized.\n");
}

void GamsBCH::translateToGamsSpaceX(const double* x_, double objval_, double* x) {
	if (gm.isReform_) {
		assert(iolib.ncols == gm.nCols()+1);
		CoinDisjointCopyN(x_, iolib.iobvar, x);
		CoinDisjointCopy(x_+iolib.iobvar, x_+gm.nCols(), x+iolib.iobvar+1);
		x[iolib.iobvar]=objval_;
	} else {
		assert(iolib.ncols == gm.nCols());
		CoinDisjointCopyN(x_, iolib.ncols, x);
	}
}

void GamsBCH::translateFromGamsSpaceX(const double* x_, double* x) {
	if (gm.isReform_) {
		assert(iolib.ncols == gm.nCols()+1);
		CoinDisjointCopyN(x_, iolib.iobvar, x);
		CoinDisjointCopy(x_+iolib.iobvar+1, x_+iolib.ncols, x+iolib.iobvar);
	} else {
		assert(iolib.ncols == gm.nCols());
		CoinDisjointCopyN(x_, iolib.ncols, x);
	}
}


void GamsBCH::translateToGamsSpaceLB(const double* lb_, double* lb) {
	if (gm.isReform_) {
		assert(iolib.ncols == gm.nCols()+1);
		CoinDisjointCopyN(lb_, iolib.iobvar, lb);
		CoinDisjointCopy(lb_+iolib.iobvar, lb_+gm.nCols(), lb+iolib.iobvar+1);
		lb[iolib.iobvar]=iolib.usrminf;
	} else {
		assert(iolib.ncols == gm.nCols());
		CoinDisjointCopyN(lb_, iolib.ncols, lb);
	}	
}

void GamsBCH::translateToGamsSpaceUB(const double* ub_, double* ub) {
	if (gm.isReform_) {
		assert(iolib.ncols == gm.nCols()+1);
		CoinDisjointCopyN(ub_, iolib.iobvar, ub);
		CoinDisjointCopy(ub_+iolib.iobvar, ub_+gm.nCols(), ub+iolib.iobvar+1);
		ub[iolib.iobvar]=iolib.usrpinf;
	} else {
		assert(iolib.ncols == gm.nCols());
		CoinDisjointCopyN(ub_, iolib.ncols, ub);
	}	
}

GamsBCH::Cut::Cut()
: lb(iolib.usrminf), ub(iolib.usrpinf), nnz(-1), indices(NULL), coeff(NULL)
{ }

GamsBCH::Cut::~Cut() {
	delete[] indices;
	delete[] coeff;
}

void GamsBCH::setNodeSolution(const double* x_, double objval_, const double* lb_, const double* ub_) {
	translateToGamsSpaceX(x_, objval_, node_x);
	translateToGamsSpaceLB(lb_, node_lb);
	translateToGamsSpaceUB(ub_, node_ub);	
}

void GamsBCH::setGlobalBounds(const double* lb_, const double* ub_) {
	translateToGamsSpaceLB(lb_, global_lb);
	translateToGamsSpaceUB(ub_, global_ub);	
}

void GamsBCH::setIncumbentSolution(const double* x_, double objval_) {
	if (!have_incumbent || (objval_!=incumbent[iolib.iobvar])) {
		translateToGamsSpaceX(x_, objval_, incumbent);
		have_incumbent=true;
		new_incumbent=true;

		char buffer[255];
		snprintf(buffer, 255, "GamsBCH: updated incumbent solution to objvalue %g", objval_);
		gm.PrintOut(GamsModel::LogMask, buffer);
	}
}

bool GamsBCH::doCuts() {
	double bestint=have_incumbent ? incumbent[iolib.iobvar] : (gm.ObjSense()==1 ? iolib.usrpinf : iolib.usrminf);
	return bchQueryCuts(cutfreq, cutinterval, cutmult,
                    cutfirst, bestint, have_incumbent, (int)gm.ObjSense(),
                    cutnewint);	
}

bool GamsBCH::generateCuts(std::vector<Cut>& cuts) {
	cuts.clear();
	
//	printf("node relax. solution: ");
//	for (int i=0; i<iolib.ncols; ++i) printf("%g ", node_x[i]);
//	printf("\n");
	if (bchWriteSol(gdxname, dict, iolib.ncols, node_lb, node_x, node_ub, NULL)) {
		gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not write node solution to GDX file.");
		return false;
	}
	
	if (new_incumbent) {
    if (bchWriteSol(gdxnameinc, dict, iolib.ncols, global_lb, incumbent, global_ub, NULL)) {
      gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not write incumbent solution to GDX file.");
      return false;
    }
    new_incumbent=false;
	}

//	gm.PrintOut(GamsModel::LogMask, "GamsBCH: Spawn GAMS cutgenerator.");
	char command[1024];
	snprintf(command, 1024, "%s --ncalls %d", cutcall, ncalls++);
	int rcode = bchRunGAMS(command, usergdxin, userkeep, userjobid);
	
	if (rcode > 0) {
		gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not spawn GAMS cutgenerator.");
		return false;
	} else if (rcode < 0) {
		// what is the error in this case?
    return false;
  }
	
	int ncuts =- 1; // indicate that we want the number of cuts
  if (bchGetCutCtrlInfo(usergdxin, &ncuts, NULL, NULL)) {
		gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not extract NUMCUTS from GDX file.");
		return false;
  }
  
  if (ncuts==0) {
		gm.PrintOut(GamsModel::LogMask, "GamsBCH: No cuts found.");
		return true;
  }
  
  int nnz;
  int* cutNnz = new int[ncuts];
  if (bchGetCutCtrlInfo(usergdxin, &ncuts, &nnz, cutNnz)) {
    gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not extract ***_C nonzeros from GDX file.");
    delete[] cutNnz;
    return false;
  }
  sprintf(command, "GamsBCH call %d: Got %d cuts with %d nonzeros.", ncalls, ncuts, nnz);
	gm.PrintOut(GamsModel::LogMask, command);

  int* cutsense  = new int[ncuts];
  int* cutbeg    = new int[ncuts+1];
  int* cutind    = new int[nnz];
  double* cutrhs = new double[ncuts];
  double* cutval = new double[nnz];
  
  if (bchGetCutMatrix(usergdxin, dict, iolib.ncols, ncuts, cutNnz, cutrhs, cutsense, cutbeg, cutind, cutval)) {
    gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not extract ***_C cutMatrix from GDX file.");
    delete[] cutsense;
    delete[] cutbeg;
    delete[] cutind;
    delete[] cutrhs;
    delete[] cutval;
    delete[] cutNnz;
    return false;
  }
  
//  printf("cut matrix: ");
//  for (int i=0; i<nnz; ++i)
//  	printf("%d:%g ", cutind[i], cutval[i]);
//  printf("\ncut rhs: ");
//  for (int i=0; i<ncuts; ++i)
//  	printf("%g ", cutrhs[i]);
//  printf("\n");

  cuts.resize(ncuts);
  
  for (int i=0; i<ncuts; ++i) {
  	Cut& cut(cuts[i]);
    cut.nnz = cutbeg[i+1]-cutbeg[i];
    switch (cutsense[i]) {
    case 1: // <=
    	cut.ub=cutrhs[i];
      break;
    case 2: // ==
    	cut.lb=cutrhs[i];
    	cut.ub=cutrhs[i];
      break;
    case 3: // >=
    	cut.lb=cutrhs[i];
      break;
    default: // free row
      break;
    }
  	cut.indices=CoinCopyOfArray(cutind+cutbeg[i], cut.nnz);
  	cut.coeff=CoinCopyOfArray(cutval+cutbeg[i], cut.nnz);
    if (gm.isReform_) {
    	for (int i=0; i<cut.nnz; ++i)
    		if (cut.indices[i]==iolib.iobvar) {
    			gm.PrintOut(GamsModel::AllMask, "GamsBCH: Cut including optimization variable not allowed.");
    			return false; // TODO: should only skip this cut and hope that others are better
    		} else if (cut.indices[i]>iolib.iobvar) { // decrement indices of variables behind objective variable
    			--cut.indices[i];
    		}
    }
  }

  delete[] cutsense;
  delete[] cutbeg;
  delete[] cutind;
  delete[] cutrhs;
  delete[] cutval;
  delete[] cutNnz;

  return true;
}

//curobj = objective value for the subproblem at the current node
//bestbnd = obj. value of best remaining node
bool GamsBCH::doHeuristic(double bestbnd, double curobj) {
	double bestint=have_incumbent ? incumbent[iolib.iobvar] : (gm.ObjSense()==1 ? iolib.usrpinf : iolib.usrminf);
	return bchQueryHeuristic(heurfreq, heurinterval, heurmult,
	                         heurfirst, heurobjfirst, bestbnd,
	                         bestint, curobj, have_incumbent, (int)gm.ObjSense(), heurnewint);
}

bool GamsBCH::runHeuristic(double* x, double& objvalue) {
	if (bchWriteSol(gdxname, dict, iolib.ncols, node_lb, node_x, node_ub, NULL)) {
		gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not write node solution to GDX file.");
		return false;
	}
	
	if (new_incumbent) {
    if (bchWriteSol(gdxnameinc, dict, iolib.ncols, global_lb, incumbent, global_ub, NULL)) {
      gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not write incumbent solution to GDX file.");
      return false;
    }
    new_incumbent=false;
	}

	char command[1024];
	snprintf(command, 1024, "%s --ncalls %d", heurcall, ncalls++);
	int rcode = bchRunGAMS(command, usergdxin, userkeep, userjobid);
	
	if (rcode > 0) {
		gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not spawn GAMS cutgenerator.");
		return false;
	} else if (rcode < 0) {
		// what is the error in this case?
    return false;
  }
	
	// initialize storage with global lower bound
	double* newx=	CoinCopyOfArray(global_lb, iolib.ncols);
	if (bchReadSolGDX(usergdxin, dict, iolib.ncols, newx)) {
		gm.PrintOut(GamsModel::AllMask, "GamsBCH: Could not read solution GDX file.");
		delete[] newx;
		return false;
	}
	
	if (!have_incumbent || gm.ObjSense()*(incumbent[iolib.iobvar]-newx[iolib.iobvar])>1e-6) {
		translateFromGamsSpaceX(newx, x);
		objvalue=newx[iolib.iobvar];

	  sprintf(command, "GamsBCH call %d: Got new incumbent with objvalue %g.", ncalls, objvalue);
		gm.PrintOut(GamsModel::AllMask, command);

		delete[] newx;
		return true;
	} else {
		delete[] newx;
		return false;
	}
	
}
