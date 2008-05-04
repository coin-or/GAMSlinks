// Copyright (C) GAMS Development 2007-2008
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
//  cbinfo.incbcall   = (char *) malloc(1024*sizeof(char));
//  cbinfo.incbicall  = (char *) malloc(1024*sizeof(char));
	
  ncalls=0;
  have_incumbent=false;
  new_incumbent=false;
	
  bchSlvSetInf(gams.getPInfinity(), gams.getMInfinity());
  
  *userjobid=0;

  gams.print(GamsHandler::StatusMask, "GamsBCH initialized.\n");
}

void GamsBCH::set_userjobid(const char* userjobid_) {
	strncpy(userjobid, userjobid_, sizeof(userjobid));
}

void GamsBCH::set_usergdxname(char* usergdxname, const char* usergdxprefix /* = NULL */) {
  // remove ".gdx" suffix, if exists
  int len=strlen(usergdxname);
  if (len>4 && strncmp(usergdxname+(len-4), ".gdx", 4)==0)
  	usergdxname[len-4]=0;
  if (usergdxprefix)
  	snprintf(gdxname, sizeof(gdxname), "%s%s%s.gdx", usergdxprefix, usergdxname, userjobid);
  else
  	snprintf(gdxname, sizeof(gdxname), "%s%s.gdx", usergdxname, userjobid);
}

void GamsBCH::set_usergdxnameinc(char* usergdxnameinc, const char* usergdxprefix /* = NULL */) {
	int len=strlen(usergdxnameinc);
  if (len>4 && strncmp(usergdxnameinc+(len-4), ".gdx", 4)==0)
  	usergdxnameinc[len-4]=0;
  if (usergdxprefix)
  	snprintf(gdxnameinc, sizeof(gdxnameinc), "%s%s%s.gdx", usergdxprefix, usergdxnameinc, userjobid);
  else
  	snprintf(gdxnameinc, sizeof(gdxnameinc), "%s%s.gdx", usergdxnameinc, userjobid);
}

void GamsBCH::set_usergdxin(char* usergdxin_, const char* usergdxprefix /* = NULL */) {
	int len=strlen(usergdxin_);
  if (len>4 && strncmp(usergdxin_+(len-4), ".gdx", 4)==0)
  	usergdxin_[len-4]=0;
  if (usergdxprefix)
  	snprintf(usergdxin, sizeof(usergdxin), "%s%s%s.gdx", usergdxprefix, usergdxin_, userjobid);
  else
  	snprintf(usergdxin, sizeof(usergdxin), "%s%s.gdx", usergdxin_, userjobid);
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

  opt.getString("usercutcall", buffer);
  set_usercutcall(buffer);

  set_usercutfreq(opt.getInteger("usercutfreq"));
  set_usercutinterval(opt.getInteger("usercutinterval"));
  set_usercutmult(opt.getInteger("usercutmult"));
  set_usercutfirst(opt.getInteger("usercutfirst"));
  set_usercutnewint(opt.getBool("usercutnewint"));

  opt.getString("userheurcall", buffer);
  set_userheurcall(buffer);

  set_userheurfreq(opt.getInteger("userheurfreq"));
  set_userheurinterval(opt.getInteger("userheurinterval"));
  set_userheurmult(opt.getInteger("userheurmult"));
  set_userheurfirst(opt.getInteger("userheurfirst"));
  set_userheurnewint(opt.getBool("userheurnewint"));
  set_userheurobjfirst(opt.getInteger("userheurobjfirst"));
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

void GamsBCH::setIncumbentSolution(const double* x_, double objval_) {
	if (!have_incumbent || (objval_!=incumbent_value)) {
		gams.translateToGamsSpaceX(x_, objval_, incumbent);
		have_incumbent=true;
		new_incumbent=true;
		incumbent_value=objval_;

		char buffer[255];
		snprintf(buffer, 255, "GamsBCH: updated incumbent solution to objvalue %g\n", objval_);
		gams.print(GamsHandler::LogMask, buffer);
	}
}

bool GamsBCH::doCuts() {
	double bestint=have_incumbent ? incumbent_value : (gams.getObjSense()==1 ? gams.getPInfinity() : gams.getMInfinity());
	return bchQueryCuts(cutfreq, cutinterval, cutmult,
                    cutfirst, bestint, have_incumbent, gams.getObjSense(),
                    cutnewint);	
}

bool GamsBCH::generateCuts(std::vector<Cut>& cuts) {
	cuts.clear();
	
//	printf("node relax. opt.val.: %g\n", node_x[iolib.iobvar]);
//	printf("node relax. solution: ");
//	for (int i=0; i<iolib.ncols; ++i) printf("%g ", node_x[i]);
//	printf("\n");
	if (bchWriteSol(gdxname, dict, gams.getColCountGams(), node_lb, node_x, node_ub, NULL)) {
		gams.print(GamsHandler::AllMask, "GamsBCH: Could not write node solution to GDX file.\n");
		return false;
	}
	
	if (new_incumbent) {
    if (bchWriteSol(gdxnameinc, dict, gams.getColCountGams(), global_lb, incumbent, global_ub, NULL)) {
      gams.print(GamsHandler::AllMask, "GamsBCH: Could not write incumbent solution to GDX file.\n");
      return false;
    }
    new_incumbent=false;
	}

//	gm.PrintOut(GamsModel::LogMask, "GamsBCH: Spawn GAMS cutgenerator.");
	char command[1024];
	snprintf(command, 1024, "%s --ncalls %d", cutcall, ncalls++);
	int rcode = bchRunGAMS(command, usergdxin, userkeep, userjobid);
	
	if (rcode > 0) {
		gams.print(GamsHandler::AllMask, "GamsBCH: Could not spawn GAMS cutgenerator.\n");
		return false;
	} else if (rcode < 0) {
//		gm.PrintOut(GamsModel::AllMask, "GamsBCH: GAMS cutgenerator returned with negative return code.");
		// what is the error in this case?
    return false;
  }
	
	int ncuts =- 1; // indicate that we want the number of cuts
  if (bchGetCutCtrlInfo(usergdxin, &ncuts, NULL, NULL)) {
		gams.print(GamsHandler::AllMask, "GamsBCH: Could not extract NUMCUTS from GDX file.\n");
		return false;
  }
  
  if (ncuts==0) {
		gams.print(GamsHandler::LogMask, "GamsBCH: No cuts found.\n");
		return true;
  }
  
  int nnz;
  int* cutNnz = new int[ncuts];
  if (bchGetCutCtrlInfo(usergdxin, &ncuts, &nnz, cutNnz)) {
    gams.print(GamsHandler::AllMask, "GamsBCH: Could not extract ***_C nonzeros from GDX file.\n");
    delete[] cutNnz;
    return false;
  }
  sprintf(command, "GamsBCH call %d: Got %d cuts with %d nonzeros.\n", ncalls, ncuts, nnz);
	gams.print(GamsHandler::LogMask, command);

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
	if (bchWriteSol(gdxname, dict, gams.getColCountGams(), node_lb, node_x, node_ub, NULL)) {
		gams.print(GamsHandler::AllMask, "GamsBCH: Could not write node solution to GDX file.\n");
		return false;
	}
	
	if (new_incumbent) {
    if (bchWriteSol(gdxnameinc, dict, gams.getColCountGams(), global_lb, incumbent, global_ub, NULL)) {
      gams.print(GamsHandler::AllMask, "GamsBCH: Could not write incumbent solution to GDX file.\n");
      return false;
    }
    new_incumbent=false;
	}

	char command[1024];
	snprintf(command, 1024, "%s --ncalls %d", heurcall, ncalls++);
	int rcode = bchRunGAMS(command, usergdxin, userkeep, userjobid);
	
	if (rcode > 0) {
		gams.print(GamsHandler::AllMask, "GamsBCH: Could not spawn GAMS cutgenerator.\n");
		return false;
	} else if (rcode < 0) {
		// what is the error in this case?
    return false;
  }
	
	// initialize storage with global lower bound
	double* newx=	CoinCopyOfArray(global_lb, gams.getColCountGams());
	if (bchReadSolGDX(usergdxin, dict, gams.getColCountGams(), newx)) {
		gams.print(GamsHandler::AllMask, "GamsBCH: Could not read solution GDX file.");
		delete[] newx;
		return false;
	}
	
	if (!have_incumbent || gams.getObjSense()*(incumbent_value-newx[gams.getObjVariable()])>1e-6) {
		gams.translateFromGamsSpaceX(newx, x);
		objvalue=newx[gams.getObjVariable()];

	  sprintf(command, "GamsBCH call %d: Got new incumbent with objvalue %g.\n", ncalls, objvalue);
		gams.print(GamsHandler::AllMask, command);

		delete[] newx;
		return true;
	} else {
		delete[] newx;
		return false;
	}
	
}
