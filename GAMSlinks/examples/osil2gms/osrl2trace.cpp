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
#include "OSiLReader.h"
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

void fillTraceRecord(tracerecord& record, OSInstance& instance, OSResult& result);
void writeTraceRecord(tracerecord& record, ostream& out);
string getModelType(OSInstance& instance);

int main(int argc, char** argv) {
  if(argc < 3 || argc>4) {
     cout << "usage: " << argv[0] << " <osilfile.osil> <osrlfile.osrl> [tracefile.trc]" << endl;
     return EXIT_FAILURE;
  }
  ifstream osilfile(argv[1]);
  if (!osilfile.good()) {
    cerr << "Cannot open file " << argv[1] << endl;
    return EXIT_FAILURE;
	}
  ifstream osrlfile(argv[2]);
  if(!osrlfile.good()) {
  	cerr << "Cannot open file " << argv[2] << endl;
  	return EXIT_FAILURE;
  }
  
  stringbuf osilstringbuf;
  osilfile.get(osilstringbuf, '\0');
  OSiLReader osilreader;
  OSInstance* osinstance = osilreader.readOSiL(osilstringbuf.str());
  
  stringbuf osrlstringbuf;
  osrlfile.get(osrlstringbuf, '\0');
	OSrLReader osrlreader;
	OSResult* osresult = osrlreader.readOSrL(osrlstringbuf.str());
	
	tracerecord record;
	
	fillTraceRecord(record, *osinstance, *osresult);
	
	if (argc==4) {
		ofstream tracefile(argv[3], ios_base::out | ios_base::app);
		if (!tracefile.good()) {
			cerr << "Cannot open file " << argv[3] << " for writing." << endl;
			return EXIT_FAILURE;
		}
		if (tracefile.tellp()==0) {
			tracefile << "* Trace Record Definition" << endl
			          << "* InputFileName,ModelType,SolverName,Direction,ModelStatus,SolverStatus,ObjectiveValue,SolverTime" << endl
			          ;
		}
		writeTraceRecord(record, tracefile);
		
	} else {
		
		writeTraceRecord(record, cout);
	}
	
	return EXIT_SUCCESS;
}

void fillTraceRecord(tracerecord& record, OSInstance& instance, OSResult& result) {
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
	
	if (result.getInstanceName().length() && instance.getInstanceName().length()) {
		if (result.getInstanceName().length() != instance.getInstanceName().length()) {
			cerr << "Error: Instance name in OSInstance an OSResult do not match." << endl;
			exit(EXIT_FAILURE);
		}
	}	
	if (result.getInstanceName().length())
		record.instancename = result.getInstanceName();
	
	record.modeltype = getModelType(instance);
	
	if (result.getServiceName().length()) {
		string::size_type pos = result.getServiceName().find("Solved with Coin Solver: ");
		if (pos == 0) {
			record.solvername = string(result.getServiceName(), 25);
		} else if (result.getServiceName().find("LINDO") != string::npos)
			record.solvername = "LindoSolver";
		else if (result.getServiceName().find("Knitro") != string::npos)
			record.solvername = "Knitro";
		else if (result.getServiceName().find("Bonmin") != string::npos)
			record.solvername = "Bonmin";
		else if (result.getServiceName().find("Couenne") != string::npos)
			record.solvername = "Couenne";
		else if (result.getServiceName().find("Ipopt") != string::npos)
			record.solvername = "Ipopt";
		else
			record.solvername = result.getServiceName();
	}
	
	if (instance.getObjectiveNumber())
		record.direction = instance.getObjectiveMaxOrMins()[0] == "min" ? 0 : 1;
	
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
	
	const char* timestr = result.resultHeader->time.c_str();
	char* endptr;
	record.solvertime = strtod(timestr, &endptr);
	if (endptr == timestr || *endptr != '\0') // error in conversion, or string is empty
		record.solvertime = 0;
	
//	cout << "time: " << result.resultHeader->time << endl;
	
}

void writeTraceRecord(tracerecord& record, ostream& out) {
	out
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

string getModelType(OSInstance& instance) {
	bool isnonlinear = instance.getNumberOfNonlinearExpressions() || instance.getNumberOfQuadraticTerms();
	bool isdiscrete = instance.getNumberOfBinaryVariables() || instance.getNumberOfIntegerVariables();
	
	//TODO recognize MIQCP, QCP, DNLP, CNS
	
	if (isnonlinear)
		if (isdiscrete)
			return "MINLP";
		else
			return "NLP";
	else
		if (isdiscrete)
			return "MIP";
		else
			return "LP";
	
	return "NA";
}
