// Copyright (C) GAMS Development 2007-2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsBCH.cpp 447 2008-05-28 19:29:35Z stefan $
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
#include "dict.h"
#include "bch.h"
}

GamsBCH::GamsBCH(GamsHandler& gams_, GamsDictionary& gamsdict, GamsOptions& opt)
: gams(gams_), dict(gamsdict.dict)
{ init();
  setupParameters(opt);
}

GamsBCH::GamsBCH(GamsHandler& gams_, GamsDictionary& gamsdict)
: gams(gams_), dict(gamsdict.dict)
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
  if (!dict) {
  	gams.print(GamsHandler::AllMask, "GamsBCH: Do not have GAMS dictionary. Initialization failed.\n");
  	return;
  }
	
	node_x=new double[gams.getColCountGams()];
	node_lb=new double[gams.getColCountGams()];
	node_ub=new double[gams.getColCountGams()];
	incumbent=new double[gams.getColCountGams()];
	global_lb=new double[gams.getColCountGams()];
	global_ub=new double[gams.getColCountGams()];
	
  ncalls = 0;
  ncuts = 0;
  nsols = 0;
  loglevel = 1;
  have_incumbent=false;
  new_incumbent=false;
	
  bchSlvSetInf(gams.getPInfinity(), gams.getMInfinity());
  
  *userjobid=0;
  *cutcall=0;
  *heurcall=0;
  *incbcall=0;
  *incbicall=0;

  gams.print(GamsHandler::LogMask, "GamsBCH initialized.\n");
}

void GamsBCH::set_userjobid(const char* userjobid_) {
	strncpy(userjobid, userjobid_, sizeof(userjobid));
}

void GamsBCH::set_usergdxname(const char* usergdxname, const char* usergdxprefix /* = NULL */) {
  // remove ".gdx" suffix, if exists
  int len=strlen(usergdxname);
  if (len>4 && strncmp(usergdxname+(len-4), ".gdx", 4)==0)
  	len-=4;
  if (usergdxprefix)
  	snprintf(gdxname, sizeof(gdxname), "%s%.*s%s.gdx", usergdxprefix, len, usergdxname, userjobid);
  else
  	snprintf(gdxname, sizeof(gdxname), "%.*s%s.gdx", len, usergdxname, userjobid);
}

void GamsBCH::set_usergdxnameinc(const char* usergdxnameinc, const char* usergdxprefix /* = NULL */) {
	int len=strlen(usergdxnameinc);
  if (len>4 && strncmp(usergdxnameinc+(len-4), ".gdx", 4)==0)
  	len-=4;
  if (usergdxprefix)
  	snprintf(gdxnameinc, sizeof(gdxnameinc), "%s%.*s%s.gdx", usergdxprefix, len, usergdxnameinc, userjobid);
  else
  	snprintf(gdxnameinc, sizeof(gdxnameinc), "%.*s%s.gdx", len, usergdxnameinc, userjobid);
}

void GamsBCH::set_usergdxin(const char* usergdxin_, const char* usergdxprefix /* = NULL */) {
	int len=strlen(usergdxin_);
  if (len>4 && strncmp(usergdxin_+(len-4), ".gdx", 4)==0)
  	len-=4;
  if (usergdxprefix)
  	snprintf(usergdxin, sizeof(usergdxin), "%s%.*s%s.gdx", usergdxprefix, len, usergdxin_, userjobid);
  else
  	snprintf(usergdxin, sizeof(usergdxin), "%.*s%s.gdx", len, usergdxin_, userjobid);
}

void GamsBCH::set_usercutcall(const char* usercutcall) {
  if (*userjobid)
  	snprintf(cutcall, sizeof(cutcall), "%s --userjobid=%s", usercutcall, userjobid);
  else
  	strncpy(cutcall, usercutcall, sizeof(cutcall));
}


void GamsBCH::set_userheurcall(const char* userheurcall) {
  if (*userjobid)
  	snprintf(heurcall, sizeof(heurcall), "%s --userjobid=%s", userheurcall, userjobid);
  else
  	strncpy(heurcall, userheurcall, sizeof(heurcall));
}

void GamsBCH::set_userincbcall(const char* userincbcall) {
  if (*userjobid)
  	snprintf(incbcall, sizeof(incbcall), "%s --userjobid=%s", userincbcall, userjobid);
  else
  	strncpy(incbcall, userincbcall, sizeof(incbcall));
}

void GamsBCH::set_userincbicall(const char* userincbicall) {
  if (*userjobid)
  	snprintf(incbicall, sizeof(incbicall), "%s --userjobid=%s", userincbicall, userjobid);
  else
  	strncpy(incbicall, userincbicall, sizeof(incbicall));
}

void GamsBCH::setupParameters(GamsOptions& opt) {
  char buffer[256];
  char gdxprefix[256];
  if (opt.isDefined("usergdxprefix"))
  	opt.getString("usergdxprefix", gdxprefix);
  else *gdxprefix=0;
  
  if (opt.isDefined("userjobid")) {
  	opt.getString("userjobid", buffer);
  	set_userjobid(buffer);
  }
  
  opt.getString("usergdxname", buffer);
  set_usergdxname(buffer, gdxprefix);
  
  opt.getString("usergdxnameinc", buffer);
  set_usergdxnameinc(buffer, gdxprefix);
  
  opt.getString("usergdxin", buffer);
  set_usergdxin(buffer, gdxprefix);
  
  set_userkeep(opt.getBool("userkeep"));

  if (opt.isKnown("usercutcall")) {
  	opt.getString("usercutcall", buffer);
  	set_usercutcall(buffer);

  	set_usercutfreq(opt.getInteger("usercutfreq"));
  	set_usercutinterval(opt.getInteger("usercutinterval"));
  	set_usercutmult(opt.getInteger("usercutmult"));
  	set_usercutfirst(opt.getInteger("usercutfirst"));
  	set_usercutnewint(opt.getBool("usercutnewint"));
  }

  if (opt.isKnown("userheurcall")) {
  	opt.getString("userheurcall", buffer);
  	set_userheurcall(buffer);

  	set_userheurfreq(opt.getInteger("userheurfreq"));
  	set_userheurinterval(opt.getInteger("userheurinterval"));
  	set_userheurmult(opt.getInteger("userheurmult"));
  	set_userheurfirst(opt.getInteger("userheurfirst"));
  	set_userheurnewint(opt.getBool("userheurnewint"));
  	set_userheurobjfirst(opt.getInteger("userheurobjfirst"));
  }
  
  if (opt.isKnown("userincbcall")) {
  	opt.getString("userincbcall", buffer);
  	set_userincbcall(buffer);
  }
  
  if (opt.isKnown("userincbicall")) {
  	opt.getString("userincbicall", buffer);
  	set_userincbicall(buffer);
  }
}

void GamsBCH::printParameters() const {
	char buffer[1050];

	sprintf(buffer, "userkeep: %d\n", (int)userkeep); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "heurfreq: %d\n", heurfreq); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "heurinterval: %d\n", heurinterval); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "heurmult: %d\n", heurmult); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "heurfirst: %d\n", heurfirst); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "heurnewint: %d\n", (int)heurnewint); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "heurobjfirst: %d\n", heurobjfirst); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "heurcall: %s\n", heurcall); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "cutcall: %s\n", cutcall); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "cutfreq: %d\n", cutfreq); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "cutinterval: %d\n", cutinterval); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "cutmult: %d\n", cutmult); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "cutfirst: %d\n", cutfirst); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "cutnewint: %d\n", (int)cutnewint); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "incbcall: %s\n", incbcall); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "incbicall: %s\n", incbicall); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "userjobid: %s\n", userjobid); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "gdxname: %s\n", gdxname); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "gdxnameinc: %s\n", gdxnameinc); gams.print(GamsHandler::LogMask, buffer);
	sprintf(buffer, "usergdxin: %s\n", usergdxin); gams.print(GamsHandler::LogMask, buffer);
	
	gams.flush(GamsHandler::AllMask);
}


GamsBCH::Cut::Cut()
: lb(0.), ub(0.), nnz(-1), indices(NULL), coeff(NULL)
{ }

GamsBCH::Cut::~Cut() {
	delete[] indices;
	delete[] coeff;
}

void GamsBCH::setNodeSolution(const double* x_, double objval_, const double* lb_, const double* ub_) {
	gams.translateToGamsSpaceX(x_, objval_, node_x);
	gams.translateToGamsSpaceLB(lb_, node_lb);
	gams.translateToGamsSpaceUB(ub_, node_ub);	
}

void GamsBCH::setGlobalBounds(const double* lb_, const double* ub_) {
	gams.translateToGamsSpaceLB(lb_, global_lb);
	gams.translateToGamsSpaceUB(ub_, global_ub);	
}

bool GamsBCH::setIncumbentSolution(const double* x_, double objval_) {
	if (!have_incumbent || (objval_!=incumbent_value)) {
		gams.translateToGamsSpaceX(x_, objval_, incumbent);
		have_incumbent=true;
		new_incumbent=true;
		incumbent_value=objval_;
		
		if (*incbcall || *incbicall)
			if (reportIncumbent()==2) // return false if an incumbent checker rejects the solution
				return false;

//		char buffer[255];
//		snprintf(buffer, 255, "GamsBCH: updated incumbent solution to objvalue %g\n", objval_);
//		gams.print(GamsHandler::LogMask, buffer);
	}
	return true;
}

int GamsBCH::reportIncumbent() {
	if (!new_incumbent) return 1;
	
  if (bchWriteSol(gdxnameinc, dict, gams.getColCountGams(), global_lb, incumbent, global_ub, NULL)) {
    gams.print(GamsHandler::AllMask, "Could not write incumbent solution to GDX file.\n");
    return 0;
  }
  new_incumbent=false;
  
  if (*incbicall) { // incumbent reporter
  	char buffer[1024];
  	snprintf(buffer, 1024, "%s --ncalls %d", incbicall, ncalls++);
  	int rcode = bchRunGAMS(buffer, usergdxin, userkeep, userjobid);
  	
  	if (rcode > 0) {
  		gams.print(GamsHandler::AllMask, "Could not spawn GAMS incumbent reporter.\n");
  		return -2;
  	} else if (rcode < 0) { // not necessarily a failure; just an 'abort' in the incumbent report model
  		if (loglevel) {
  			sprintf(buffer, "GAMS incumbent reported failed with return code %d.\n", rcode);
  			gams.print(GamsHandler::LogMask, buffer);
  		}
      return -2;
    }
  }
  
  if (*incbcall) { // incumbent checker
  	char buffer[1024];
  	snprintf(buffer, 1024, "%s --ncalls %d", incbcall, ncalls++);
  	int rcode = bchRunGAMS(buffer, usergdxin, userkeep, userjobid);
  	
  	if (rcode > 0) {
  		gams.print(GamsHandler::AllMask, "Could not spawn GAMS incumbent checker.\n");
  		return -3;
  	} else if (rcode < 0) { // acceptance of incumbent
  		if (loglevel)
  			gams.print(GamsHandler::AllMask, "GAMS incumbent checker accepts solution.\n");
      return 1;
    } else { // rejection of incumbent
  		if (loglevel)
  			gams.print(GamsHandler::AllMask, "GAMS incumbent checker rejects solution.\n");
    	return 2;
    }
  }
  
  return 1;
}

bool GamsBCH::doCuts() {
	double bestint=have_incumbent ? incumbent_value : (gams.getObjSense()==1 ? gams.getPInfinity() : gams.getMInfinity());
	return bchQueryCuts(cutfreq, cutinterval, cutmult,
                    cutfirst, bestint, have_incumbent, gams.getObjSense(),
                    cutnewint);	
}

bool GamsBCH::generateCuts(std::vector<Cut>& cuts) {
	cuts.clear();

	char buffer[1024];
	if (loglevel) {
		sprintf(buffer, "GamsBCH call %d: ", ncalls);
		gams.print(GamsHandler::LogMask, buffer);
	}
	
//	printf("node relax. opt.val.: %g\n", node_x[gams.getObjVariable()]);
//	printf("node relax. solution: ");
//	for (int i=0; i<gams.getColCountGams(); ++i) printf("%g ", node_x[i]);
//	printf("\n");
	if (bchWriteSol(gdxname, dict, gams.getColCountGams(), node_lb, node_x, node_ub, NULL)) {
		gams.print(GamsHandler::AllMask, "Could not write node solution to GDX file.\n");
		return false;
	}
	
	if (reportIncumbent()==-1) // abort if writing the incumbent failed 
		return false;
//	if (new_incumbent) {
//    if (bchWriteSol(gdxnameinc, dict, gams.getColCountGams(), global_lb, incumbent, global_ub, NULL)) {
//      gams.print(GamsHandler::AllMask, "Could not write incumbent solution to GDX file.\n");
//      return false;
//    }
//    new_incumbent=false;
//	}

//	gm.PrintOut(GamsModel::LogMask, "GamsBCH: Spawn GAMS cutgenerator.");
	snprintf(buffer, 1024, "%s --ncalls %d", cutcall, ncalls++);
	int rcode = bchRunGAMS(buffer, usergdxin, userkeep, userjobid);
	
	if (rcode > 0) {
		gams.print(GamsHandler::AllMask, "Could not spawn GAMS cutgenerator.\n");
		return false;
	} else if (rcode < 0) { // not necessarily a failure; just an 'abort' in the cutgenerator model
		if (loglevel) {
			sprintf(buffer, "Cut generator failed with return code %d.\n", rcode);
			gams.print(GamsHandler::LogMask, buffer);
		}
    return false;
  }
	
	int ncuts = -1; // indicate that we want the number of cuts
  if (bchGetCutCtrlInfo(usergdxin, &ncuts, NULL, NULL)) {
		gams.print(GamsHandler::AllMask, "Could not extract NUMCUTS from GDX file.\n");
		return false;
  }
  
  if (ncuts==0) {
  	if (loglevel)
  		gams.print(GamsHandler::LogMask, "No cuts found.\n");
		return true;
  }
  
  int nnz = 0;
  int* cutNnz = new int[ncuts];
  if (bchGetCutCtrlInfo(usergdxin, &ncuts, &nnz, cutNnz)) {
    gams.print(GamsHandler::AllMask, "Could not extract ***_C nonzeros from GDX file.\n");
    delete[] cutNnz;
    return false;
  }
  if (loglevel) {
  	sprintf(buffer, "Got %d %s.\n", ncuts, ncuts==1 ? "cut" : "cuts");
  	gams.print(GamsHandler::LogMask, buffer);
  }
	
	this->ncuts += ncuts;

  int* cutsense  = new int[ncuts];
  int* cutbeg    = new int[ncuts+1];
  int* cutind    = new int[nnz];
  double* cutrhs = new double[ncuts];
  double* cutval = new double[nnz];
  
  if (bchGetCutMatrix(usergdxin, dict, gams.getColCountGams(), ncuts, cutNnz, cutrhs, cutsense, cutbeg, cutind, cutval)) {
    gams.print(GamsHandler::AllMask, "GamsBCH: Could not extract ***_C cutMatrix from GDX file.\n");
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
//  printf("\ncut sense: ");
//  for (int i=0; i<ncuts; ++i)
//  	printf("%d ", cutsense[i]);
//  printf("\ncut nnz: ");
//  for (int i=0; i<ncuts; ++i)
//  	printf("%d ", cutNnz[i]);
//  printf("\n");

  cuts.resize(ncuts);
  
  for (int i=0; i<ncuts; ++i) {
  	Cut& cut(cuts[i]);
    cut.nnz = cutbeg[i+1]-cutbeg[i];
    switch (cutsense[i]) {
    case 1: // <=
    	cut.lb=gams.getMInfinity();
    	cut.ub=cutrhs[i];
      break;
    case 2: // ==
    	cut.lb=cutrhs[i];
    	cut.ub=cutrhs[i];
      break;
    case 3: // >=
    	cut.lb=cutrhs[i];
    	cut.ub=gams.getPInfinity();
      break;
    default: // free row
    	cut.lb=gams.getMInfinity();
    	cut.ub=gams.getPInfinity();
      break;
    }
  	cut.indices=CoinCopyOfArray(cutind+cutbeg[i], cut.nnz);
  	cut.coeff=CoinCopyOfArray(cutval+cutbeg[i], cut.nnz);
  	if (!gams.translateFromGamsSpaceCol(cut.indices, cut.indices, cut.nnz)) {
			gams.print(GamsHandler::AllMask, "GamsBCH: Cut including optimization variable not allowed.\n");
			return false; // TODO: should only skip this cut and hope that others are better
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
	double bestint=have_incumbent ? incumbent_value : (gams.getObjSense()==1 ? gams.getPInfinity() : gams.getMInfinity());
	return bchQueryHeuristic(heurfreq, heurinterval, heurmult,
	                         heurfirst, heurobjfirst, bestbnd,
	                         bestint, curobj, have_incumbent, gams.getObjSense(), heurnewint);
}

bool GamsBCH::runHeuristic(double* x, double& objvalue) {
	char buffer[1024];

	if (loglevel) {
		sprintf(buffer, "GamsBCH call %d: ", ncalls);
		gams.print(GamsHandler::LogMask, buffer);
	}

	if (bchWriteSol(gdxname, dict, gams.getColCountGams(), node_lb, node_x, node_ub, NULL)) {
		gams.print(GamsHandler::AllMask, "Could not write node solution to GDX file.\n");
		return false;
	}
	
	if (!reportIncumbent()) // abort if writing the incumbent failed
		return false;
//	if (new_incumbent) {
//    if (bchWriteSol(gdxnameinc, dict, gams.getColCountGams(), global_lb, incumbent, global_ub, NULL)) {
//      gams.print(GamsHandler::AllMask, "Could not write incumbent solution to GDX file.\n");
//      return false;
//    }
//    new_incumbent=false;
//	}

	snprintf(buffer, 1024, "%s --ncalls %d", heurcall, ncalls++);
	int rcode = bchRunGAMS(buffer, usergdxin, userkeep, userjobid);
	
	if (rcode > 0) {
		gams.print(GamsHandler::AllMask, "Could not spawn GAMS cutgenerator.\n");
		return false;
	} else if (rcode < 0) { // not necessarily a failure; just an 'abort' in the heuristic model
		if (loglevel) {
			sprintf(buffer, "Heuristic failed with return code %d.\n", rcode);
			gams.print(GamsHandler::LogMask, buffer);
		}
    return false;
  }
	
	// initialize storage with global lower bound
	double* newx=	CoinCopyOfArray(global_lb, gams.getColCountGams());
	if (bchReadSolGDX(usergdxin, dict, gams.getColCountGams(), newx)) {
		gams.print(GamsHandler::AllMask, "Could not read solution GDX file.");
		delete[] newx;
		return false;
	}
	
	++nsols;
	if (!have_incumbent || gams.getObjSense()*(incumbent_value-newx[gams.getObjVariable()])>1e-6) {
		gams.translateFromGamsSpaceX(newx, x);
		objvalue=newx[gams.getObjVariable()];

		if (loglevel) {
			sprintf(buffer, "Got new incumbent solution with objective value %g.\n", objvalue);
			gams.print(GamsHandler::AllMask, buffer);
		}

		delete[] newx;
		return true;
	} else {
		if (loglevel) {
			sprintf(buffer, "Got nonimproving solution with objective value %g.\n", newx[gams.getObjVariable()]);
			gams.print(GamsHandler::AllMask, buffer);
		}

		delete[] newx;
		return false;
	}
	
}
