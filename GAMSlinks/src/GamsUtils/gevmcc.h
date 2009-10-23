/* Copyright (C) GAMS Development 2009
   All Rights Reserved.
   This code is published under the Common Public License.

   $Id$

   Author:  Lutz Westermann
*/

/* gevmcc.h
 * Header file for C-style interface to the GEV library
 * generated by apiwrapper
 */

#if ! defined(_GEVCC_H_)
#     define  _GEVCC_H_

#define GEVAPIVERSION 2


enum gevWriteMode {
  doErr  = 0,
  doStat = 1,
  doLog  = 2  };
#define gevAlgFileType  "AlgFileType"  /* gevOptions */
#define gevGamsVersion  "GamsVersion"
#define gevLogOption    "LogOption"
#define gevIDEFlag      "IDEFlag"
#define gevSBBFlag      "SBBFlag"
#define gevDomLim       "DomLim"
#define gevIterLim      "IterLim"
#define gevNodeLim      "NodeLim"
#define gevSysOut       "SysOut"
#define gevInteger1     "Integer1"
#define gevInteger2     "Integer2"
#define gevInteger3     "Integer3"
#define gevInteger4     "Integer4"
#define gevInteger5     "Integer5"
#define gevPageWidth    "PageWidth"
#define gevPageSize     "PageSize"
#define gevOptCR        "OptCR"
#define gevOptCA        "OptCA"
#define gevResLim       "ResLim"
#define gevWorkSpace    "WorkSpace"
#define gevWorkFactor   "WorkFactor"
#define gevCutOff       "CutOff"
#define gevCheat        "Cheat"
#define gevTryInt       "TryInt"
#define gevReal1        "Real1"
#define gevReal2        "Real2"
#define gevReal3        "Real3"
#define gevReal4        "Real4"
#define gevReal5        "Real5"
#define gevHeapLimit    "HeapLimit"
#define gevNameCtrFile  "NameCtrFile"
#define gevNameLogFile  "NameLogFile"
#define gevNameStaFile  "NameStaFile"
#define gevNameScrDir   "NameScrDir"
#define gevNameGamsDate "NameGamsDate"
#define gevNameGamsTime "NameGamsTime"
#define gevNameSBBInfo  "NameSBBInfo"
#define gevNameSysDir   "NameSysDir"
#define gevNameWrkDir   "NameWrkDir"
#define gevNameCurDir   "NameCurDir"
#define gevLicense1     "License1"
#define gevLicense2     "License2"
#define gevLicense3     "License3"
#define gevLicense4     "License4"
#define gevLicense5     "License5"
#define gevNameScrExt   "NameScrExt"



#if defined(_WIN32)
# define GEV_CALLCONV __stdcall
#else
# define GEV_CALLCONV
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct gevRec;
typedef struct gevRec *gevHandle_t;

typedef int (*gevErrorCallback_t) (int ErrCount, const char *msg);

/* headers for "wrapper" routines implemented in C */
int gevGetReady  (char *msgBuf, int msgBufLen);
int gevGetReadyD (const char *dirName, char *msgBuf, int msgBufLen);
int gevGetReadyL (const char *libName, char *msgBuf, int msgBufLen);
int gevCreate    (gevHandle_t *pgev, char *msgBuf, int msgBufLen);
int gevCreateD   (gevHandle_t *pgev, const char *dirName, char *msgBuf, int msgBufLen);
int gevCreateL   (gevHandle_t *pgev, const char *libName, char *msgBuf, int msgBufLen);
int gevFree      (gevHandle_t *pgev);

int gevLibraryLoaded(void);
int gevLibraryUnload(void);

int  gevGetScreenIndicator   (void);
void gevSetScreenIndicator   (int scrind);
int  gevGetExceptionIndicator(void);
void gevSetExceptionIndicator(int excind);
int  gevGetExitIndicator     (void);
void gevSetExitIndicator     (int extind);
gevErrorCallback_t gevGetErrorCallback(void);
void gevSetErrorCallback(gevErrorCallback_t func);
int  gevGetAPIErrorCount     (void);
void gevSetAPIErrorCount     (int ecnt);

void gevErrorHandling(const char *msg);


#if defined(GEV_MAIN)    /* we must define some things only once */
# define GEV_FUNCPTR(NAME)  NAME##_t NAME = NULL
#else
# define GEV_FUNCPTR(NAME)  extern NAME##_t NAME
#endif

/* function typedefs and pointer definitions */

typedef void (GEV_CALLCONV *Tgevlswrite_t) (const char *msg, int mode, void *usrmem);

/* Prototypes for Dummy Functions */
int  GEV_CALLCONV d_gevInitEnvironmentLegacy (gevHandle_t pgev, const char *cntrfile);
void  GEV_CALLCONV d_gevRegisterWriteCallback (gevHandle_t pgev, Tgevlswrite_t lsw, int logenabled, void *usrmem);
void  GEV_CALLCONV d_gevCompleteEnvironment (gevHandle_t pgev, void *pA, void *ivec, void *rvec, void *svec);
int  GEV_CALLCONV d_gevSwitchLogStat (gevHandle_t pgev, int lo, const char *logfn, int logappend, const char *statfn, Tgevlswrite_t lsw, void *usrmem, void **lshandle);
int  GEV_CALLCONV d_gevRestoreLogStat (gevHandle_t pgev, void **lshandle);
void  GEV_CALLCONV d_gevLog (gevHandle_t pgev, const char *s);
void  GEV_CALLCONV d_gevLogPChar (gevHandle_t pgev, const char *p);
void  GEV_CALLCONV d_gevStat (gevHandle_t pgev, const char *s);
void  GEV_CALLCONV d_gevStatPChar (gevHandle_t pgev, const char *p);
void  GEV_CALLCONV d_gevStatAudit (gevHandle_t pgev, const char *s);
void  GEV_CALLCONV d_gevStatCon (gevHandle_t pgev);
void  GEV_CALLCONV d_gevStatCoff (gevHandle_t pgev);
void  GEV_CALLCONV d_gevStatEOF (gevHandle_t pgev);
void  GEV_CALLCONV d_gevStatSysout (gevHandle_t pgev);
void  GEV_CALLCONV d_gevStatAddE (gevHandle_t pgev, int num, const char *s);
void  GEV_CALLCONV d_gevStatAddV (gevHandle_t pgev, int num, const char *s);
void  GEV_CALLCONV d_gevStatAddJ (gevHandle_t pgev, int num1, int num2, const char *s);
void  GEV_CALLCONV d_gevStatEject (gevHandle_t pgev);
void  GEV_CALLCONV d_gevStatEdit (gevHandle_t pgev, const char C);
void  GEV_CALLCONV d_gevStatC (gevHandle_t pgev, const char *s);
void  GEV_CALLCONV d_gevStatE (gevHandle_t pgev, const char *s1, int num, const char *s2);
void  GEV_CALLCONV d_gevStatV (gevHandle_t pgev, const char *s1, int num, const char *s2);
void  GEV_CALLCONV d_gevStatT (gevHandle_t pgev);
void  GEV_CALLCONV d_gevStatA (gevHandle_t pgev, const char *s);
void  GEV_CALLCONV d_gevStatB (gevHandle_t pgev, const char *s);
void  GEV_CALLCONV d_gevLogStat (gevHandle_t pgev, const char *s);
void  GEV_CALLCONV d_gevLogStatPChar (gevHandle_t pgev, const char *p);
int  GEV_CALLCONV d_gevLicenseCheck (gevHandle_t pgev, int m, int n, int nz, int nlnz, int ndisc);
int  GEV_CALLCONV d_gevLicenseCheckSubSys (gevHandle_t pgev, char *msg, const char *Lcode);
int  GEV_CALLCONV d_gevLicenseCheckSubInternal (gevHandle_t pgev, char *msg, int subsysnum, const char *Lcode);
int  GEV_CALLCONV d_gevLicenseQueryOption (gevHandle_t pgev, const char *cstr, const char *ostr, int *oval);
void  GEV_CALLCONV d_gevLicenseRegisterSystem (gevHandle_t pgev, int nsubsys, const char *codes, int checksum, int isglobal);
int  GEV_CALLCONV d_gevLicenseGetMessage (gevHandle_t pgev, char *msg);
int  GEV_CALLCONV d_gevGetExecName (gevHandle_t pgev, const char *algid, char *execname);
int  GEV_CALLCONV d_gevGetSlvLibInfo (gevHandle_t pgev, const char *algid, char *libname, char *prefix, int *ifversion);
int  GEV_CALLCONV d_gevCapabilityCheck (gevHandle_t pgev, int modeltype, const char *algid, int *check);
char * GEV_CALLCONV d_gevGetAlg (gevHandle_t pgev, int modeltype, char *buf);
char * GEV_CALLCONV d_gevGetAlgDefault (gevHandle_t pgev, int modeltype, char *buf);
double  GEV_CALLCONV d_gevTimeDiff (gevHandle_t pgev);
double  GEV_CALLCONV d_gevTimeDiffStart (gevHandle_t pgev);
void  GEV_CALLCONV d_gevTerminateInstall (gevHandle_t pgev);
void  GEV_CALLCONV d_gevTerminateSet (gevHandle_t pgev, void *intr, void *ehdler);
int  GEV_CALLCONV d_gevTerminateGet (gevHandle_t pgev);
int  GEV_CALLCONV d_gevGetIntOpt (gevHandle_t pgev, const char *optName);
double  GEV_CALLCONV d_gevGetDblOpt (gevHandle_t pgev, const char *optName);
char * GEV_CALLCONV d_gevGetStrOpt (gevHandle_t pgev, const char *optName, char *buf);
void  GEV_CALLCONV d_gevSetIntOpt (gevHandle_t pgev, const char *optName, int ival);
void  GEV_CALLCONV d_gevSetDblOpt (gevHandle_t pgev, const char *optName, double rval);
void  GEV_CALLCONV d_gevSetStrOpt (gevHandle_t pgev, const char *optName, const char *sval);
void * GEV_CALLCONV d_gevGetOptHandle (gevHandle_t pgev);
void  GEV_CALLCONV d_gevSynchronizeOpt (gevHandle_t pgev, void *optr);
char * GEV_CALLCONV d_gevGetScratchName (gevHandle_t pgev, const char *filename, char *buf);
int  GEV_CALLCONV d_gevDumpControlLegacy (gevHandle_t pgev, const char *cfin, const char *cfout, void *gmoptr);
int  GEV_CALLCONV d_gevWriteSbbInfoLegacy (gevHandle_t pgev, const char *sbbfile, const char *rfile, int bounds, void *gmoptr);
int  GEV_CALLCONV d_gevDumpMatrixLegacy (gevHandle_t pgev, const char *mfout, void *gmoptr);
void GEV_CALLCONV d_gevSysDirSet (gevHandle_t pgev, const char *x);
void GEV_CALLCONV d_gevSkipIOLegacySet (gevHandle_t pgev, const int x);

typedef int  (GEV_CALLCONV *gevInitEnvironmentLegacy_t) (gevHandle_t pgev, const char *cntrfile);
GEV_FUNCPTR(gevInitEnvironmentLegacy);
typedef void  (GEV_CALLCONV *gevRegisterWriteCallback_t) (gevHandle_t pgev, Tgevlswrite_t lsw, int logenabled, void *usrmem);
GEV_FUNCPTR(gevRegisterWriteCallback);
typedef void  (GEV_CALLCONV *gevCompleteEnvironment_t) (gevHandle_t pgev, void *pA, void *ivec, void *rvec, void *svec);
GEV_FUNCPTR(gevCompleteEnvironment);
typedef int  (GEV_CALLCONV *gevSwitchLogStat_t) (gevHandle_t pgev, int lo, const char *logfn, int logappend, const char *statfn, Tgevlswrite_t lsw, void *usrmem, void **lshandle);
GEV_FUNCPTR(gevSwitchLogStat);
typedef int  (GEV_CALLCONV *gevRestoreLogStat_t) (gevHandle_t pgev, void **lshandle);
GEV_FUNCPTR(gevRestoreLogStat);
typedef void  (GEV_CALLCONV *gevLog_t) (gevHandle_t pgev, const char *s);
GEV_FUNCPTR(gevLog);
typedef void  (GEV_CALLCONV *gevLogPChar_t) (gevHandle_t pgev, const char *p);
GEV_FUNCPTR(gevLogPChar);
typedef void  (GEV_CALLCONV *gevStat_t) (gevHandle_t pgev, const char *s);
GEV_FUNCPTR(gevStat);
typedef void  (GEV_CALLCONV *gevStatPChar_t) (gevHandle_t pgev, const char *p);
GEV_FUNCPTR(gevStatPChar);
typedef void  (GEV_CALLCONV *gevStatAudit_t) (gevHandle_t pgev, const char *s);
GEV_FUNCPTR(gevStatAudit);
typedef void  (GEV_CALLCONV *gevStatCon_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevStatCon);
typedef void  (GEV_CALLCONV *gevStatCoff_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevStatCoff);
typedef void  (GEV_CALLCONV *gevStatEOF_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevStatEOF);
typedef void  (GEV_CALLCONV *gevStatSysout_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevStatSysout);
typedef void  (GEV_CALLCONV *gevStatAddE_t) (gevHandle_t pgev, int num, const char *s);
GEV_FUNCPTR(gevStatAddE);
typedef void  (GEV_CALLCONV *gevStatAddV_t) (gevHandle_t pgev, int num, const char *s);
GEV_FUNCPTR(gevStatAddV);
typedef void  (GEV_CALLCONV *gevStatAddJ_t) (gevHandle_t pgev, int num1, int num2, const char *s);
GEV_FUNCPTR(gevStatAddJ);
typedef void  (GEV_CALLCONV *gevStatEject_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevStatEject);
typedef void  (GEV_CALLCONV *gevStatEdit_t) (gevHandle_t pgev, const char C);
GEV_FUNCPTR(gevStatEdit);
typedef void  (GEV_CALLCONV *gevStatC_t) (gevHandle_t pgev, const char *s);
GEV_FUNCPTR(gevStatC);
typedef void  (GEV_CALLCONV *gevStatE_t) (gevHandle_t pgev, const char *s1, int num, const char *s2);
GEV_FUNCPTR(gevStatE);
typedef void  (GEV_CALLCONV *gevStatV_t) (gevHandle_t pgev, const char *s1, int num, const char *s2);
GEV_FUNCPTR(gevStatV);
typedef void  (GEV_CALLCONV *gevStatT_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevStatT);
typedef void  (GEV_CALLCONV *gevStatA_t) (gevHandle_t pgev, const char *s);
GEV_FUNCPTR(gevStatA);
typedef void  (GEV_CALLCONV *gevStatB_t) (gevHandle_t pgev, const char *s);
GEV_FUNCPTR(gevStatB);
typedef void  (GEV_CALLCONV *gevLogStat_t) (gevHandle_t pgev, const char *s);
GEV_FUNCPTR(gevLogStat);
typedef void  (GEV_CALLCONV *gevLogStatPChar_t) (gevHandle_t pgev, const char *p);
GEV_FUNCPTR(gevLogStatPChar);
typedef int  (GEV_CALLCONV *gevLicenseCheck_t) (gevHandle_t pgev, int m, int n, int nz, int nlnz, int ndisc);
GEV_FUNCPTR(gevLicenseCheck);
typedef int  (GEV_CALLCONV *gevLicenseCheckSubSys_t) (gevHandle_t pgev, char *msg, const char *Lcode);
GEV_FUNCPTR(gevLicenseCheckSubSys);
typedef int  (GEV_CALLCONV *gevLicenseCheckSubInternal_t) (gevHandle_t pgev, char *msg, int subsysnum, const char *Lcode);
GEV_FUNCPTR(gevLicenseCheckSubInternal);
typedef int  (GEV_CALLCONV *gevLicenseQueryOption_t) (gevHandle_t pgev, const char *cstr, const char *ostr, int *oval);
GEV_FUNCPTR(gevLicenseQueryOption);
typedef void  (GEV_CALLCONV *gevLicenseRegisterSystem_t) (gevHandle_t pgev, int nsubsys, const char *codes, int checksum, int isglobal);
GEV_FUNCPTR(gevLicenseRegisterSystem);
typedef int  (GEV_CALLCONV *gevLicenseGetMessage_t) (gevHandle_t pgev, char *msg);
GEV_FUNCPTR(gevLicenseGetMessage);
typedef int  (GEV_CALLCONV *gevGetExecName_t) (gevHandle_t pgev, const char *algid, char *execname);
GEV_FUNCPTR(gevGetExecName);
typedef int  (GEV_CALLCONV *gevGetSlvLibInfo_t) (gevHandle_t pgev, const char *algid, char *libname, char *prefix, int *ifversion);
GEV_FUNCPTR(gevGetSlvLibInfo);
typedef int  (GEV_CALLCONV *gevCapabilityCheck_t) (gevHandle_t pgev, int modeltype, const char *algid, int *check);
GEV_FUNCPTR(gevCapabilityCheck);
typedef char * (GEV_CALLCONV *gevGetAlg_t) (gevHandle_t pgev, int modeltype, char *buf);
GEV_FUNCPTR(gevGetAlg);
typedef char * (GEV_CALLCONV *gevGetAlgDefault_t) (gevHandle_t pgev, int modeltype, char *buf);
GEV_FUNCPTR(gevGetAlgDefault);
typedef double  (GEV_CALLCONV *gevTimeDiff_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevTimeDiff);
typedef double  (GEV_CALLCONV *gevTimeDiffStart_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevTimeDiffStart);
typedef void  (GEV_CALLCONV *gevTerminateInstall_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevTerminateInstall);
typedef void  (GEV_CALLCONV *gevTerminateSet_t) (gevHandle_t pgev, void *intr, void *ehdler);
GEV_FUNCPTR(gevTerminateSet);
typedef int  (GEV_CALLCONV *gevTerminateGet_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevTerminateGet);
typedef int  (GEV_CALLCONV *gevGetIntOpt_t) (gevHandle_t pgev, const char *optName);
GEV_FUNCPTR(gevGetIntOpt);
typedef double  (GEV_CALLCONV *gevGetDblOpt_t) (gevHandle_t pgev, const char *optName);
GEV_FUNCPTR(gevGetDblOpt);
typedef char * (GEV_CALLCONV *gevGetStrOpt_t) (gevHandle_t pgev, const char *optName, char *buf);
GEV_FUNCPTR(gevGetStrOpt);
typedef void  (GEV_CALLCONV *gevSetIntOpt_t) (gevHandle_t pgev, const char *optName, int ival);
GEV_FUNCPTR(gevSetIntOpt);
typedef void  (GEV_CALLCONV *gevSetDblOpt_t) (gevHandle_t pgev, const char *optName, double rval);
GEV_FUNCPTR(gevSetDblOpt);
typedef void  (GEV_CALLCONV *gevSetStrOpt_t) (gevHandle_t pgev, const char *optName, const char *sval);
GEV_FUNCPTR(gevSetStrOpt);
typedef void * (GEV_CALLCONV *gevGetOptHandle_t) (gevHandle_t pgev);
GEV_FUNCPTR(gevGetOptHandle);
typedef void  (GEV_CALLCONV *gevSynchronizeOpt_t) (gevHandle_t pgev, void *optr);
GEV_FUNCPTR(gevSynchronizeOpt);
typedef char * (GEV_CALLCONV *gevGetScratchName_t) (gevHandle_t pgev, const char *filename, char *buf);
GEV_FUNCPTR(gevGetScratchName);
typedef int  (GEV_CALLCONV *gevDumpControlLegacy_t) (gevHandle_t pgev, const char *cfin, const char *cfout, void *gmoptr);
GEV_FUNCPTR(gevDumpControlLegacy);
typedef int  (GEV_CALLCONV *gevWriteSbbInfoLegacy_t) (gevHandle_t pgev, const char *sbbfile, const char *rfile, int bounds, void *gmoptr);
GEV_FUNCPTR(gevWriteSbbInfoLegacy);
typedef int  (GEV_CALLCONV *gevDumpMatrixLegacy_t) (gevHandle_t pgev, const char *mfout, void *gmoptr);
GEV_FUNCPTR(gevDumpMatrixLegacy);
typedef void (GEV_CALLCONV *gevSysDirSet_t) (gevHandle_t pgev, const char *x);
GEV_FUNCPTR(gevSysDirSet);
typedef void (GEV_CALLCONV *gevSkipIOLegacySet_t) (gevHandle_t pgev, const int x);
GEV_FUNCPTR(gevSkipIOLegacySet);
#if defined(__cplusplus)
}
#endif
#endif /* #if ! defined(_GEVCC_H_) */
