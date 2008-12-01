// Copyright (C) 2008 GAMS Development
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author: Stefan Vigerske

#include <cstdlib>
#include <iostream>
#include <fstream>
//#include <string>
//#include <list>

using namespace std;

//#include "OSInstance.h"
#include "OSrLReader.h"
//#include "OSCommonUtil.h"

typedef struct {
	string instancename;
	string modeltype;
	string solvername;
	int direction; /* 0 for min, 1 for max */
	int model_status;
	int solver_status;
	double obj_value;
	double solvertime;	
	
} tracerecord;

void fillTraceRecord(tracerecord& record, OSResult& result);
void writeTraceRecord(tracerecord& record);

int main(int argc, char** argv) {
  if(argc != 2) {
     cout << "usage: " << argv[0] << " <osrlfile.osrl>" << endl;
     return EXIT_FAILURE;
  }
  ifstream osrlfile(argv[1]);
  if(!osrlfile.good()) {
     cout << "Cannot open file " << argv[1] << endl;
     return EXIT_FAILURE;
  }
  
  stringbuf osrlstringbuf;
  osrlfile.get(osrlstringbuf, '\0');
//  osrlstringbuf << '\0';
//  cout << osrlstringbuf.str();
	
	OSrLReader reader;
	OSResult* osresult = reader.readOSrL(osrlstringbuf.str());
	
	tracerecord record;
	
	fillTraceRecord(record, *osresult);
	
	writeTraceRecord(record);
	
	return EXIT_SUCCESS;
}

void fillTraceRecord(tracerecord& record, OSResult& result) {
	record.instancename = "NA";
	record.modeltype = "NA";
	record.solvername = "NA";
	record.direction = 0;
	record.model_status = 13;
	record.solver_status = 13;
	record.obj_value = 0;
	record.solvertime = 0;
	
	if (result.resultHeader == NULL) {
		record.solver_status = 10; // error solver failure
		record.model_status = 13; // error no solution
		return;
	}
	
	if (result.getInstanceName().length())
		record.instancename = result.getInstanceName();
	if (result.getServiceName().length())
		record.solvername = result.getServiceName();
	
	if (result.getGeneralStatusType()=="error") {
		record.solver_status = 10; // error solver failure
		record.model_status = 13; // error no solution
		return;
	}
	
	record.solver_status=1; // normal completion
	
	if (result.getSolutionNumber()==0) {
		record.model_status=14; // no solution returned
	} else if (result.getSolutionStatusType(0)=="unbounded") {
		record.model_status=3;	// unbounded
	} else if (result.getSolutionStatusType(0)=="globallyOptimal") {
		record.model_status=1;	// optimal
	} else if (result.getSolutionStatusType(0)=="locallyOptimal") {
		record.model_status=2;	// locally optimal
	} else if (result.getSolutionStatusType(0)=="optimal") {
		record.model_status=2;	// locally optimal
	} else if (result.getSolutionStatusType(0)=="bestSoFar") {
		record.model_status=7;	// intermediate nonoptimal (or should we report integer solution if integer var.?)
	} else if (result.getSolutionStatusType(0)=="feasible") {
		record.model_status=3;	// intermediate nonoptimal
	} else if (result.getSolutionStatusType(0)=="infeasible") {
		record.model_status=4;	// infeasible
	} else if (result.getSolutionStatusType(0)=="stoppedByLimit") {
		record.solver_status=2; // interation interrupt (just a guess)
		record.model_status=6;	// intermediate infeasible
	} else if (result.getSolutionStatusType(0)=="unsure") {
		record.model_status=6;	// intermediate infeasible
	} else if (result.getSolutionStatusType(0)=="error") {
		record.model_status=12;	// error unknown
	} else if (result.getSolutionStatusType(0)=="other") {
		record.model_status=6;	// intermediate infeasible
	} else {
		record.model_status=12;	// error unknown
	}

	if (result.getSolutionNumber()) {
		OptimizationSolution* sol=result.resultData->optimization->solution[0];
		
		if (sol->objectives && sol->objectives->values && sol->objectives->values->obj[0])
			record.obj_value = sol->objectives->values->obj[0]->value;
	}
	
//	cout << "time: " << result.resultHeader->time << endl;
	
}

void writeTraceRecord(tracerecord& record) {
	cout
		<< record.instancename << ','
		<< record.modeltype << ','
		<< record.solvername << ','
		<< record.direction << ','
		<< record.model_status << ','
		<< record.solver_status << ','
		<< record.obj_value << ','
		<< record.solvertime
		<< endl;
}
