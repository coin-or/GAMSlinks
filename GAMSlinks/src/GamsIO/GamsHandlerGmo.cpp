// Copyright (C) GAMS Development 2008
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsHandlerGmo.cpp 618 2009-02-15 19:53:56Z stefan $
//
// Authors:  Stefan Vigerske

#include "GamsHandlerGmo.hpp"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
#error "don't have header file for stdlib"
#endif
#endif

GamsHandlerGmo::GamsHandlerGmo(gmoHandle_t gmo_)
: gmo(gmo_), sysdir(NULL), dictfile(NULL)
{ }

GamsHandlerGmo::~GamsHandlerGmo() {
	delete[] sysdir;
	delete[] dictfile;
}

void GamsHandlerGmo::print(PrintMask mask, const char* msg) const {
	switch (mask) {
		case GamsHandler::LogMask :
			gmoLogPChar(gmo, msg);
			break;
		case GamsHandler::StatusMask :
			gmoStatPChar(gmo, msg);
			break;
		case GamsHandler::AllMask:
		default:
			gmoLogStatPChar(gmo, msg);
			break;
	}
}

void GamsHandlerGmo::println(PrintMask mask, const char* msg) const {
	switch (mask) {
		case GamsHandler::LogMask :
			gmoLog(gmo, msg);
			break;
		case GamsHandler::StatusMask :
			gmoStat(gmo, msg);
			break;
		case GamsHandler::AllMask:
		default:
			gmoLogStat(gmo, msg);
			break;
	}
}

void GamsHandlerGmo::flush(PrintMask mask) const { }

void GamsHandlerGmo::translateToGamsSpaceX(const double* x_, double objval_, double* x) const {
	println(GamsHandler::AllMask, "call of unimplemented method");
	exit(EXIT_FAILURE);
}
void GamsHandlerGmo::translateToGamsSpaceLB(const double* lb_, double* lb) const {
	println(GamsHandler::AllMask, "call of unimplemented method");
	exit(EXIT_FAILURE);
}
void GamsHandlerGmo::translateToGamsSpaceUB(const double* ub_, double* ub) const {
	println(GamsHandler::AllMask, "call of unimplemented method");
	exit(EXIT_FAILURE);
}
void GamsHandlerGmo::translateFromGamsSpaceX(const double* x_, double* x) const {
	println(GamsHandler::AllMask, "call of unimplemented method");
	exit(EXIT_FAILURE);
}
bool GamsHandlerGmo::translateFromGamsSpaceCol(const int* indices_, int* indices, int nr) const {
	println(GamsHandler::AllMask, "call of unimplemented method");
	exit(EXIT_FAILURE);
}
int GamsHandlerGmo::translateToGamsSpaceCol(int colindex) const {
	return gmoGetjModel(gmo, colindex); // TODO: is this correct?
//	println(GamsHandler::AllMask, "call of unimplemented method");
//	exit(EXIT_FAILURE);
}
int GamsHandlerGmo::translateToGamsSpaceRow(int rowindex) const {
	return gmoGetiModel(gmo, rowindex); // TODO: is this correct?
//	println(GamsHandler::AllMask, "call of unimplemented method");
//	exit(EXIT_FAILURE);
}

double GamsHandlerGmo::getMInfinity() const {
	return gmoMinf(gmo);
}
double GamsHandlerGmo::getPInfinity() const {
	return gmoPinf(gmo);
}

int GamsHandlerGmo::getObjSense() const {
	return gmoSense(gmo)==Obj_Min ? 1 : -1;
}

int GamsHandlerGmo::getColCount() const {
	return gmoN(gmo);
}
int GamsHandlerGmo::getColCountGams() const {
	println(GamsHandler::AllMask, "call of unimplemented method");
	exit(EXIT_FAILURE);
}

int GamsHandlerGmo::getObjVariable() const {
	return gmoObjVar(gmo);
//	int varnr;
//	gmoOptI(gmo, I_ObjVar, &varnr);
//	return varnr;
}
int GamsHandlerGmo::getObjRow() const {
	return gmoObjEqu(gmo);
//	int rownr;
//	gmoOptI(gmo, I_ObjRow, &rownr);
//	return rownr;
}

const char* GamsHandlerGmo::getSystemDir() const {
	if (!sysdir) sysdir = new char[1024];
	gmoNameSysDir(gmo, sysdir);
//	gmoOptS(gmo, S_NameSysDir, sysdir);
	return sysdir;
}

bool GamsHandlerGmo::isDictionaryWritten() const {
	return gmoDictionary(gmo);
//	int havedict;
//	gmoOptI(gmo, I_DictFile, &havedict);
//	return havedict;
}
const char* GamsHandlerGmo::dictionaryFile() const {
	if (!dictfile) dictfile = new char[1024];
	gmoNameDict(gmo, dictfile);
//	gmoOptS(gmo, S_NameDict, dictfile);
	return dictfile;
}
int GamsHandlerGmo::dictionaryVersion() const {
	println(GamsHandler::AllMask, "call of unimplemented method");
	exit(EXIT_FAILURE);
}
