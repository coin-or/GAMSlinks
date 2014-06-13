// Copyright (C) GAMS Development and others 2008-2011
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include "GamsOS.hpp"

#include "OSConfig.h"
#include "OSiLWriter.h"
#include "OSrLWriter.h"
#include "OSErrorClass.h"
#include "OSSolverAgent.h"

#include "GAMSlinksConfig.h"
#include "GamsOptions.hpp"
#include "GamsOSxL.hpp"

#include "gmomcc.h"
#include "gevmcc.h"
#ifdef GAMS_BUILD
#include "palmcc.h"
#endif

#include "GamsCompatibility.h"

#include <cassert>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

int GamsOS::readyAPI(
   struct gmoRec*     gmo,                /**< GAMS modeling object */
   struct optRec*     opt                 /**< GAMS options object */
)
{
   this->gmo = gmo;
   assert(gmo != NULL);
   this->opt = opt;

   if( getGmoReady() || getGevReady() )
      return 1;

   gev = (gevRec*)gmoEnvironment(gmo);
   assert(gev != NULL);

#ifdef GAMS_BUILD
   struct palRec*        pal;                /**< GAMS audit and license object */
   char buffer[GMS_SSSIZE];

   if( !palCreate(&pal, buffer, sizeof(buffer)) )
      return 1;

#define PALPTR pal
#include "coinlibdCL4svn.h" 
   palGetAuditLine(pal,buffer);
   gevLogStat(gev, "");
   gevLogStat(gev, buffer);
   gevStatAudit(gev, buffer);

   palFree(&pal);
#endif

   gevLogStatPChar(gev, "\nCOIN-OR Optimization Services (OS Library " OS_VERSION ")\nwritten by H. Gassmann, J. Ma, and K. Martin\n\n");

   return 0;
}

int GamsOS::callSolver()
{
   assert(gmo != NULL);
   assert(gev != NULL);

   gmoMinfSet(gmo, -OSDBL_MAX);
   gmoPinfSet(gmo,  OSDBL_MAX);
   gmoObjReformSet(gmo, 1);
   gmoObjStyleSet(gmo, gmoObjType_Fun);
   gmoIndexBaseSet(gmo, 0);

   bool calledbyconvert = (gevGetIntOpt(gev, gevInteger5) == 273);

   if( (gmoGetVarTypeCnt(gmo, gmovar_S1) || gmoGetVarTypeCnt(gmo, gmovar_S2)) && !calledbyconvert )
   {
      gevLogStat(gev, "Error: Special ordered sets not supported by OS.");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 1;
   }

   if( gmoGetEquTypeCnt(gmo, gmoequ_C) || gmoGetEquTypeCnt(gmo, gmoequ_X) || gmoGetEquTypeCnt(gmo, gmoequ_B) )
   {
      gevLogStat(gev, "Error: Conic constraints, logic equations, and external functions not supported by OS.");
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      return 1;
   }

   char buffer[2*GMS_SSSIZE];

   // setup options object, read options file, if given
   GamsOptions gamsopt(gev, opt);
   if( opt == NULL )
   {
      char* optfilename = NULL;
      if( gmoOptFile(gmo) > 0 )
      {
         gmoNameOptFile(gmo, buffer);
         optfilename = buffer;
      }

      // solvername is usually os or myos, but if we are called from convert, then we want to read the convert option definitions file
      const char* solvername = NULL;
      if( calledbyconvert )
         solvername = "convert";
      else
#ifdef GAMS_BUILD
         solvername = "os";
#else
         solvername = "myos";
#endif
      gamsopt.readOptionsFile(solvername, optfilename);
   }

   // setup OSInstance
   GamsOSxL gmoosxl(gmo);
   if( !gmoosxl.createOSInstance() )
   {
      gevLogStat(gev, "Creation of OSInstance failed.");
      /* let's assume that this happened because of some capability issue */
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      gmoSolveStatSet(gmo, gmoSolveStat_Capability);
      return 0;
   }

   OSInstance* osinstance = gmoosxl.takeOverOSInstance();
   assert(osinstance != NULL);

   // write OSInstance, if desired
   if( calledbyconvert || gamsopt.isDefined("writeosil") )
   {
      OSiLWriter osilwriter;
      char osilfilename[GMS_SSSIZE];
      std::ofstream osilfile(gamsopt.getString(calledbyconvert ? "OSIL" : "writeosil", osilfilename));
      if( !osilfile.good() )
      {
         sprintf(buffer, "Error opening file %s for writing of instance in OSiL.", osilfilename);
         gevLogStat(gev, buffer);
      }
      else
      {
         std::string osil(osilwriter.writeOSiL(osinstance));
         if( !contriveSOS(osil) )
         {
            gevLogStat(gev, "Error contriving SOS information into OSiL string.");
         }
         sprintf(buffer, "Writing instance in OSiL to %s.", osilfilename);
         gevLogStat(gev, buffer);
         osilfile << osil;
      }
   }

   // setup options string; read from file, if given
   std::string osol;
   if( !calledbyconvert && gamsopt.isDefined("readosol") )
   {
      char osolfilename[GMS_SSSIZE];
      std::ifstream osolfile(gamsopt.getString("readosol", osolfilename));
      if( !osolfile.good() )
      {
         sprintf(buffer, "Error opening file %s for reading solver options in OSoL.", osolfilename);
         gevLogStat(gev, buffer);
      }
      else
      {
         sprintf(buffer, "Reading solver options in OSoL from %s.", osolfilename);
         gevLogStat(gev, buffer);
         std::getline(osolfile, osol, '\0');
      }
   }
   else
   {
      // just give an "empty" xml file
      osol = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <osol xmlns=\"os.optimizationservices.org\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"os.optimizationservices.org http://www.optimizationservices.org/schemas/OSoL.xsd\"></osol>";
   }

   if( !calledbyconvert && gamsopt.isDefined("service") )
   {
      if( !remoteSolve(osinstance, osol, gamsopt) )
      {
         gevLogStatPChar(gev, "Error in remote solve.\n");
         delete osinstance;
         return 1;
      }
   }
   else
   {
      if( !calledbyconvert )
         gevLogStatPChar(gev, "\nNo Optimization Services server specified (option 'service').\nSkipping remote solve.\n");
      gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
      gmoSolveStatSet(gmo, gmoSolveStat_Normal);
   }

   delete osinstance;

   return 0;
}

bool GamsOS::remoteSolve(
   OSInstance*           osinstance,         /**< Optimization Services instance */
   std::string&          osol,               /**< Optimization Services options string in OSoL format */
   GamsOptions&          gamsopt             /**< GAMS options object */
)
{
   char buffer[GMS_SSSIZE];

   if( gamsopt.isDefined("solver") )
   {
      // if a solver option was specified put that into osol
      gamsopt.getString("solver", buffer);

      std::string::size_type iStringpos = osol.find("</general");
      std::string solverInput;
      if( iStringpos == std::string::npos )
      {
         iStringpos = osol.find("</osol");
         assert(iStringpos != std::string::npos);
         solverInput = "<general><solverToInvoke>";
         solverInput += buffer;
         solverInput += "</solverToInvoke></general>";
      }
      else
      {
         solverInput = "<solverToInvoke>";
         solverInput += buffer;
         solverInput += "</solverToInvoke>";
      }
      osol.insert(iStringpos, solverInput);
   }

   try
   {
      OSSolverAgent agent(gamsopt.getString("service", buffer));

#if 1
      std::string osrl = agent.solve(OSiLWriter().writeOSiL(osinstance), osol);
      if( !processResult(&osrl, NULL, gamsopt) )
         return false;

#else
      if( !gamsopt.getString("service_method", buffer) )
      {
         gevLogStat(gev, "Error reading value of parameter service_method");
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         return false;
      }

      if( strncmp(buffer, "solve", 5) == 0 )
      {
         std::string osrl = agent.solve(OSiLWriter().writeOSiL(osinstance), osol);
         if( !processResult(&osrl, NULL, gamsopt) )
            return false;
      }
      else if( strncmp(buffer, "getJobID", 8) == 0 )
      {
         std::string jobid = agent.getJobID(osol);
         sprintf(buffer, "OS Job ID: %s", jobid.c_str());
         gevLog(gev, buffer);
         gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
         gmoSolveStatSet(gmo, gmoSolveStat_Normal);
      }
      else if( strncmp(buffer, "knock", 5) == 0 )
      {
         std::string ospl;
         if( gamsopt.isDefined("readospl") )
         {
            char osplfilename[GMS_SSSIZE];
            std::ifstream osplfile(gamsopt.getString("readospl", osplfilename));
            if( !osplfile.good() )
            {
               snprintf(buffer, sizeof(buffer), "Error opening OSpL file %s for reading knock request.", osplfilename);
               gevLogStat(gev, buffer);
               gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
               gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
               return false;
            }
            else
            {
               snprintf(buffer, sizeof(buffer), "Reading knock request from OSpL file %s.", osplfilename);
               gevLog(gev, buffer);
               std::getline(osplfile, ospl, '\0');
            }
         }
         else
         {
            ospl="<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<ospl xmlns=\"os.optimizationservices.org\" "
               "xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" "
               "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
               "xsi:schemaLocation=\"os.optimizationservices.org http://www.optimizationservices.org/schemas/OSpL.xsd\">"
               "<processHeader>"
               "<request action=\"getAll\"/>"
               "</processHeader>"
               "<processData/>"
               "</ospl>";
         }
         ospl = agent.knock(ospl, osol);

         if( gamsopt.isDefined("writeospl") )
         {
            char osplfilename[GMS_SSSIZE];
            std::ofstream osplfile(gamsopt.getString("writeospl", osplfilename));
            if( !osplfile.good() )
            {
               snprintf(buffer, sizeof(buffer), "Error opening file %s for writing knock result in OSpL.", osplfilename);
               gevLogStat(gev, buffer);
               gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
               gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
               return false;
            }
            else
            {
               snprintf(buffer, sizeof(buffer), "Writing knock result in OSpL to %s.", osplfilename);
               gevLog(gev, buffer);
               osplfile << ospl;
            }
         }
         else
         {
            gevLogStat(gev, "Answer from knock:");
            gevLogStatPChar(gev, ospl.c_str());
            gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
            gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         }
      }
      else if( strncmp(buffer, "kill", 4) == 0 )
      {
         std::string ospl = agent.kill(osol);

         if( gamsopt.isDefined("writeospl") )
         {
            char osplfilename[GMS_SSSIZE];
            std::ofstream osplfile(gamsopt.getString("writeospl", osplfilename));
            if( !osplfile.good() )
            {
               snprintf(buffer, sizeof(buffer), "Error opening file %s for writing kill result in OSpL.", osplfilename);
               gevLogStat(gev, buffer);
               gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
               gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
               return false;
            }
            else
            {
               snprintf(buffer, sizeof(buffer), "Writing kill result in OSpL to %s.", osplfilename);
               gevLog(gev, buffer);
               osplfile << ospl;
            }
         }
         else
         {
            gevLogStat(gev, "Answer from kill:");
            gevLogStatPChar(gev, ospl.c_str());
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         }
      }
      else if( strncmp(buffer, "send", 4) == 0 )
      {
         if( agent.send(OSiLWriter().writeOSiL(osinstance), osol) )
         {
            gevLogStat(gev, "Problem instance successfully send to OS service.");
            gmoModelStatSet(gmo, gmoModelStat_NoSolutionReturned);
            gmoSolveStatSet(gmo, gmoSolveStat_Normal);
         }
         else
         {
            gevLogStat(gev, "There was an error sending the problem instance to the OS service.");
            gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
            gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         }
      }
      else if( strncmp(buffer, "retrieve", 8) == 0 )
      {
         std::string osrl = agent.retrieve(osol);
         if( !processResult(&osrl, NULL, gamsopt) )
            return false;
      }
      else
      {
         char buffer2[2*GMS_SSSIZE];
         snprintf(buffer2, sizeof(buffer2), "Error: OS service method %s not known.", buffer);
         gevLogStat(gev, buffer2);
         gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
         gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
         return false;
      }
#endif

   }
   catch( ErrorClass& error )
   {
      gevLogStat(gev, "Error handling the OS service. Error message: ");
      gevLogStatPChar(gev, error.errormsg.c_str());
      gmoModelStatSet(gmo, gmoModelStat_ErrorNoSolution);
      gmoSolveStatSet(gmo, gmoSolveStat_SystemErr);
      return false;
   }

   return true;
}

bool GamsOS::processResult(
   std::string*          osrl,               /**< optimization result as string in OSrL format */
   OSResult*             osresult,           /**< optimization result as object */
   GamsOptions&          gamsopt             /**< GAMS options object */
)
{
   assert(osrl != NULL || osresult != NULL);

   if( gamsopt.isDefined("writeosrl") )
   {
      char osrlfilename[GMS_SSSIZE];
      char buffer[2*GMS_SSSIZE];
      std::ofstream osrlfile(gamsopt.getString("writeosrl", osrlfilename));
      if( !osrlfile.good() )
      {
         snprintf(buffer, sizeof(buffer), "Error opening file %s for writing optimization results in OSrL.", osrlfilename);
         gevLogStat(gev, buffer);
      }
      else
      {
         snprintf(buffer, sizeof(buffer), "Writing result in OSrL to %s.", osrlfilename);
         gevLogStat(gev, buffer);
         if( osrl != NULL )
            osrlfile << *osrl;
         else
            osrlfile << OSrLWriter().writeOSrL(osresult);
      }
   }

   GamsOSxL osrl2gmo(gmo);
   if( osresult != NULL )
      osrl2gmo.writeSolution(*osresult);
   else
      osrl2gmo.writeSolution(*osrl);

   return true;
}

bool GamsOS::contriveSOS(std::string& osil)
{
   int numSos1;
   int numSos2;
   int nzSos;

   gmoGetSosCounts(gmo, &numSos1, &numSos2, &nzSos);

   if( nzSos == 0 )
      return true;

   int numSos;
   int* sostype;
   int* sosbeg;
   int* sosind;
   double* soswt;
   std::stringstream sosstr;

   numSos = numSos1 + numSos2;

   sostype = new int[numSos];
   sosbeg  = new int[numSos+1];
   sosind  = new int[nzSos];
   soswt   = new double[nzSos];

   gmoGetSosConstraints(gmo, sostype, sosbeg, sosind, soswt);

   sosstr << "<specialOrderedSets numberOfSOS=\"" << numSos << "\">";

   // TODO the SOS weights (soswt) are lost here; OSoL has a scheme for these, but we don't write out an OSoL here
   for( int i = 0; i < numSos; ++i )
   {
      sosstr << "<sos type=\"" << sostype[i] << "\" numberOfVar=\"" << sosbeg[i+1] - sosbeg[i] << "\">";
      for( int j = sosbeg[i]; j < sosbeg[i+1]; ++j )
         sosstr << "<var idx=\"" << sosind[j] << "\"/>";
      sosstr << "</sos>";

   }
   sosstr << "</specialOrderedSets>";

   size_t dataend = osil.rfind("</instanceData>");
   if( dataend == std::string::npos )
   {
      gevLogStat(gev, "Error: Could not find '</InstanceData>' in OSiL string.");
      return false;
   }

   osil.insert(dataend, sosstr.str());

   delete sostype;
   delete sosbeg;
   delete sosind;
   delete soswt;

   return true;
}

#define GAMSSOLVERC_ID         os_
#define GAMSSOLVERC_CLASS      GamsOS
#include "GamsSolverC_tpl.cpp"
