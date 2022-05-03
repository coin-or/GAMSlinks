// Copyright (C) GAMS Development and others 2022
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifndef _WIN32
#include <libgen.h>
#include <unistd.h>
/*#include <sys/wait.h>*/
#else
#include <io.h>
#define F_OK 0
#define access _access
#define popen _popen
#define pclose _pclose
#endif

#include "convert_nl.h"

#include "gmomcc.h"
#include "gevmcc.h"
#include "cfgmcc.h"
#include "optcc.h"

typedef struct
{
   gmoHandle_t gmo;
   gevHandle_t gev;

   /* name of nl file with arbitrary extension */
   char filename[GMS_SSSIZE + 30];
   /* length of nl filename without extension */
   int  stublen;
   char solver[GMS_SSSIZE];

} amplsolver;

static
int processOptions(
   amplsolver* as
   )
{
   optHandle_t opt;
   cfgHandle_t cfg;
   char deffile[2*GMS_SSSIZE+20];
   char buffer[1000];
   int rc = 1;

   /* get the Option File Handling set up */
   if( !optCreate(&opt, buffer, sizeof(buffer)) )
   {
      gevLogStatPChar(as->gev, "\n*** Could not create optionfile handle: ");
      gevLogStat(as->gev, buffer);
      return 1;
   }

   // get definition file name from cfg object
   cfg = (cfgHandle_t)gevGetALGX(as->gev);
   assert(cfg != NULL);
   gevGetCurrentSolver(as->gev, as->gmo, buffer);
   deffile[0] = '\0';
   cfgDefFileName(cfg, buffer, deffile);
   if( deffile[0] != '/' && deffile[1] != ':' )
   {
      // if deffile is not absolute path, then prefix with sysdir
      gevGetStrOpt(as->gev, gevNameSysDir, buffer);
      strcat(buffer, deffile);
      strcpy(deffile, buffer);
   }
   if( optReadDefinition(opt, deffile) )
   {
      int itype;
      for( int i = 1; i <= optMessageCount(opt); ++i )
      {
         optGetMessage(opt, i, buffer, &itype);
         if( itype <= optMsgFileLeave || itype == optMsgUserError )
            gevLogStat(as->gev, buffer);
      }
      optClearMessages(opt);
      optEchoSet(opt, 0);
      optFree(&opt);
      goto TERMINATE;
   }
   optEOLOnlySet(opt, 1);

   // read user options file
   if( gmoOptFile(as->gmo) == 0 )
   {
      gevLogStatPChar(as->gev, "No amplsolver options file given. Don't know which solver to run.\n");
      goto TERMINATE;
   }

   gmoNameOptFile(as->gmo, buffer);

   /* read option file */
   optEchoSet(opt, 1);
   optReadParameterFile(opt, buffer);
   if( optMessageCount(opt) )
   {
      int itype;
      for( int i = 1; i <= optMessageCount(opt); ++i )
      {
         optGetMessage(opt, i, buffer, &itype);
         if( itype <= optMsgFileLeave || itype == optMsgUserError )
            gevLogStat(as->gev, buffer);
      }
      optClearMessages(opt);
      optEchoSet(opt, 0);
   }
   else
   {
      optEchoSet(opt, 0);
   }

   if( !optGetDefinedStr(opt, "solver") )
   {
      gevLogStatPChar(as->gev, "Option solver not specified in options file. Don't know which solver to run.\n");
      goto TERMINATE;
   }

   optGetStrStr(opt, "solver", as->solver);

   if( optGetDefinedStr(opt, "options") )
   {
      char options[GMS_SSSIZE];
      char solvername[GMS_SSSIZE];
      char envname[GMS_SSSIZE+10];

      optGetStrStr(opt, "solvername", solvername);
      if( *solvername == '\0' )
      {
#ifndef _WIN32
         char* bname;
         bname = basename(as->solver);
         sprintf(envname, "%s_options", bname);
#else
         char fname[GMS_SSSIZE];
         _splitpath(as->solver, NULL, NULL, fname, NULL);
         sprintf(envname, "%s_options", fname);
#endif
      }
      else
      {
         sprintf(envname, "%s_options", solvername);
      }

      optGetStrStr(opt, "options", options);

      /* printf("Setting %s\n", envstr); */
#ifndef _WIN32
      if( setenv(envname, options, 1) != 0 )
#else
      if( _putenv_s(envname, options) != 0 )
#endif
         gevLogStatPChar(as->gev, "Warning: Failed to pass solver options.\n");
   }

   rc = 0;

 TERMINATE:
   optFree(&opt);

   return rc;
}

static
void writeNL(
   amplsolver* as
   )
{
   convertWriteNLopts writeopts;

   /* get the problem into a normal form */
   gmoObjStyleSet(as->gmo, gmoObjType_Fun);
   gmoObjReformSet(as->gmo, 1);
   gmoIndexBaseSet(as->gmo, 0);
   gmoSetNRowPerm(as->gmo); /* hide =N= rows */

   gevGetStrOpt(as->gev, gevNameScrDir, as->filename);
   strcat(as->filename, "prob.nl");
   as->stublen = strlen(as->filename) - 3;

   writeopts.filename = as->filename;
   writeopts.binary = 1;

   if( convertWriteNL(as->gmo, writeopts) == RETURN_ERROR )
   {
      gmoSolveStatSet(as->gmo, gmoSolveStat_Capability);
      gmoModelStatSet(as->gmo, gmoModelStat_ErrorNoSolution);
   }
}

#if 0
static
int runSolver(
   amplsolver* as
   )
{
   pid_t pid;
   pid_t pid2;
   int wstatus;

   pid = fork();
   if( pid < 0 )
   {
      gevLogStat(as->gev, "Could not fork().\n");
      return 1;
   }

   if( pid == 0 )
   {
      /* child process */
      int rc;

      as->filename[as->stublen] = '\0';  /* pass filename without .nl extension */
      /* printf("Exec %s %s %s -AMPL\n", as->solver, as->solver, as->filename); */
      rc = execlp(as->solver, as->solver, as->filename, "-AMPL");
      fprintf(stderr, "Failed to execute %s %s %s -AMPL, rc=%d\n", as->solver, as->solver, as->filename, rc);
      _exit(127);
   }

   pid2 = waitpid(pid, &wstatus, 0);
   if( pid2 == -1 )
   {
      gevLogStatPChar(as->gev, "Failure in waitpid().\n");
      return 1;
   }
   if( pid != pid2 )
   {
      gevLogStatPChar(as->gev, "Unexpected PID returned by waitpid().\n");
      return 1;
   }

   if( !WIFEXITED(wstatus) )
   {
      gevLogStatPChar(as->gev, "Abnormal termination of AMPL solver process.\n");
      return 1;
   }

   if( WEXITSTATUS(wstatus) != 0 )
   {
      char buf[200];
      sprintf(buf, "Warning: AMPL solver process terminated with exit code %d\n", WEXITSTATUS(wstatus));
      gevLogStatPChar(as->gev, buf);
   }

   return 0;
}
#endif

static
int runSolver(
   amplsolver* as
   )
{
   char command[2*GMS_SSSIZE + 100];
   char buf[GMS_SSSIZE];
   FILE* stream;

   as->filename[as->stublen] = '\0';  /* pass filename without .nl extension */

#ifndef _WIN32
   if( snprintf(command, sizeof(command), "\"%s\" \"%s\" -AMPL 2>&1", as->solver, as->filename) >= sizeof(command) )
#else
   /* windows doesn't seem to like quotes around the executable name */
   if( snprintf(command, sizeof(command), "%s \"%s\" -AMPL 2>&1", as->solver, as->filename) >= sizeof(command) )
#endif
   {
      /* with GAMS' limit on GMS_SSSIZE for option values and scratch dirname, this shouldn't happen */
      gevLogStatPChar(as->gev, "Solver name or nl filename too long.\n");
      return 1;
   }

   stream = popen(command, "r");
   if( stream == NULL )
   {
      gevLogStatPChar(as->gev, "Failed to start AMPL solver (popen() failed)).\n");
      return 1;
   }

   while( fgets(buf, GMS_SSSIZE, stream) != NULL )
      gevLogPChar(as->gev, buf);

   pclose(stream);

   gevLogPChar(as->gev, "\n");

   return 0;
}


#define GAMSSOLVER_ID amp
#include "GamsEntryPoints_tpl.c"

void ampInitialize(void)
{
   gmoInitMutexes();
   gevInitMutexes();
   optInitMutexes();
   cfgInitMutexes();
}

void ampFinalize(void)
{
   gmoFiniMutexes();
   gevFiniMutexes();
   optFiniMutexes();
   cfgFiniMutexes();
}

DllExport int STDCALL ampCreate(
   void**   Cptr,
   char*    msgBuf,
   int      msgBufLen
)
{
   assert(Cptr != NULL);
   assert(msgBufLen > 0);
   assert(msgBuf != NULL);

   *Cptr = calloc(1, sizeof(amplsolver));

   msgBuf[0] = 0;

   if( !gmoGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !gevGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !optGetReady(msgBuf, msgBufLen) )
      return 1;

   if( !cfgGetReady(msgBuf, msgBufLen) )
      return 1;

   return 1;
}

DllExport void STDCALL ampFree(
   void** Cptr
)
{
   amplsolver* as;

   assert(Cptr != NULL);
   assert(*Cptr != NULL);

   as = (amplsolver*) *Cptr;
   free(as);

   *Cptr = NULL;

   gmoLibraryUnload();
   gevLibraryUnload();
   cfgLibraryUnload();
   optLibraryUnload();
}

DllExport int STDCALL ampReadyAPI(
   void*       Cptr,
   gmoHandle_t Gptr
)
{
   amplsolver* as;

   assert(Cptr != NULL);
   assert(Gptr != NULL);

   char msg[256];
   if( !gmoGetReady(msg, sizeof(msg)) )
      return 1;
   if( !gevGetReady(msg, sizeof(msg)) )
      return 1;

   as = (amplsolver*) Cptr;
   as->gmo = Gptr;
   as->gev = (gevHandle_t) gmoEnvironment(as->gmo);

   return 0;
}

DllExport int STDCALL ampCallSolver(
   void* Cptr
)
{
   amplsolver* as;

   as = (amplsolver*) Cptr;
   assert(as->gmo != NULL);
   assert(as->gev != NULL);

   gmoModelStatSet(as->gmo, gmoModelStat_NoSolutionReturned);
   gmoSolveStatSet(as->gmo, gmoSolveStat_SystemErr);

   if( processOptions(as) )
      return 0;

   writeNL(as);

   if( gmoSolveStat(as->gmo) == gmoSolveStat_Capability )
      return 0;

   gevTimeSetStart(as->gev);

   if( runSolver(as) )
      return 0;

   gmoSetHeadnTail(as->gmo, gmoHresused, gevTimeDiffStart(as->gev));

   strcpy(as->filename + as->stublen, ".sol");
   convertReadAmplSol(as->gmo, as->filename);

   /* remove temporary files */
   if( !gevGetIntOpt(as->gev, gevKeep) )
   {
      strcpy(as->filename + as->stublen, ".nl");
      if( remove(as->filename) != 0 )
         fprintf(stderr, "Could not remove temporary file %s\n", as->filename);

      strcpy(as->filename + as->stublen, ".sol");
      if( access(as->filename, F_OK) == 0 && remove(as->filename) != 0 )
         fprintf(stderr, "Could not remove temporary file %s\n", as->filename);

      if( gmoDict(as->gmo) != NULL )
      {
         strcpy(as->filename + as->stublen, ".col");
         if( remove(as->filename) != 0 )
            fprintf(stderr, "Could not remove temporary file %s\n", as->filename);

         strcpy(as->filename + as->stublen, ".row");
         if( remove(as->filename) != 0 )
            fprintf(stderr, "Could not remove temporary file %s\n", as->filename);
      }
   }

   return 0;
}
