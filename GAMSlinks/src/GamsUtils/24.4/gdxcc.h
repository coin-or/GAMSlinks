/* gdxcc.h
 * Header file for C-style interface to the GDX library
 * generated by apiwrapper for GAMS Version 24.4.1
 */

#if ! defined(_GDXCC_H_)
#     define  _GDXCC_H_

#define GDXAPIVERSION 7


#if defined(_WIN32) && defined(__GNUC__)
# include <stdio.h>
#endif

#include "gclgms.h"

#if defined(_WIN32)
# define GDX_CALLCONV __stdcall
#else
# define GDX_CALLCONV
#endif

#if defined(_WIN32)
typedef __int64 INT64;
#elif defined(__LP64__) || defined(__axu__) || defined(_FCGLU_LP64_)
typedef signed long int INT64;
#else
typedef signed long long int INT64;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct gdxRec;
typedef struct gdxRec *gdxHandle_t;

typedef int (*gdxErrorCallback_t) (int ErrCount, const char *msg);

/* headers for "wrapper" routines implemented in C */
int gdxGetReady  (char *msgBuf, int msgBufLen);
int gdxGetReadyD (const char *dirName, char *msgBuf, int msgBufLen);
int gdxGetReadyL (const char *libName, char *msgBuf, int msgBufLen);
int gdxCreate    (gdxHandle_t *pgdx, char *msgBuf, int msgBufLen);
int gdxCreateD   (gdxHandle_t *pgdx, const char *dirName, char *msgBuf, int msgBufLen);
int gdxCreateL   (gdxHandle_t *pgdx, const char *libName, char *msgBuf, int msgBufLen);
int gdxFree      (gdxHandle_t *pgdx);

int gdxLibraryLoaded(void);
int gdxLibraryUnload(void);

int  gdxGetScreenIndicator   (void);
void gdxSetScreenIndicator   (int scrind);
int  gdxGetExceptionIndicator(void);
void gdxSetExceptionIndicator(int excind);
int  gdxGetExitIndicator     (void);
void gdxSetExitIndicator     (int extind);
gdxErrorCallback_t gdxGetErrorCallback(void);
void gdxSetErrorCallback(gdxErrorCallback_t func);
int  gdxGetAPIErrorCount     (void);
void gdxSetAPIErrorCount     (int ecnt);

void gdxErrorHandling(const char *msg);
void gdxInitMutexes(void);
void gdxFiniMutexes(void);


#if defined(GDX_MAIN)    /* we must define some things only once */
# define GDX_FUNCPTR(NAME)  NAME##_t NAME = NULL
#else
# define GDX_FUNCPTR(NAME)  extern NAME##_t NAME
#endif

/* function typedefs and pointer definitions */

typedef void (GDX_CALLCONV *TDataStoreProc_t) (const int Indx[], const double Vals[]);
typedef int (GDX_CALLCONV *TDataStoreFiltProc_t) (const int Indx[], const double Vals[], void *Uptr);
typedef void (GDX_CALLCONV *TDomainIndexProc_t) (int RawIndex, int MappedIndex, void *Uptr);

/* Prototypes for Dummy Functions */
int  GDX_CALLCONV d_gdxAcronymAdd (gdxHandle_t pgdx, const char *AName, const char *Txt, int AIndx);
int  GDX_CALLCONV d_gdxAcronymCount (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxAcronymGetInfo (gdxHandle_t pgdx, int N, char *AName, char *Txt, int *AIndx);
int  GDX_CALLCONV d_gdxAcronymGetMapping (gdxHandle_t pgdx, int N, int *orgIndx, int *newIndx, int *autoIndex);
int  GDX_CALLCONV d_gdxAcronymIndex (gdxHandle_t pgdx, double V);
int  GDX_CALLCONV d_gdxAcronymName (gdxHandle_t pgdx, double V, char *AName);
int  GDX_CALLCONV d_gdxAcronymNextNr (gdxHandle_t pgdx, int NV);
int  GDX_CALLCONV d_gdxAcronymSetInfo (gdxHandle_t pgdx, int N, const char *AName, const char *Txt, int AIndx);
double  GDX_CALLCONV d_gdxAcronymValue (gdxHandle_t pgdx, int AIndx);
int  GDX_CALLCONV d_gdxAddAlias (gdxHandle_t pgdx, const char *Id1, const char *Id2);
int  GDX_CALLCONV d_gdxAddSetText (gdxHandle_t pgdx, const char *Txt, int *TxtNr);
int  GDX_CALLCONV d_gdxAutoConvert (gdxHandle_t pgdx, int NV);
int  GDX_CALLCONV d_gdxClose (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxDataErrorCount (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxDataErrorRecord (gdxHandle_t pgdx, int RecNr, int KeyInt[], double Values[]);
int  GDX_CALLCONV d_gdxDataReadDone (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxDataReadFilteredStart (gdxHandle_t pgdx, int SyNr, const int FilterAction[], int *NrRecs);
int  GDX_CALLCONV d_gdxDataReadMap (gdxHandle_t pgdx, int RecNr, int KeyInt[], double Values[], int *DimFrst);
int  GDX_CALLCONV d_gdxDataReadMapStart (gdxHandle_t pgdx, int SyNr, int *NrRecs);
int  GDX_CALLCONV d_gdxDataReadRaw (gdxHandle_t pgdx, int KeyInt[], double Values[], int *DimFrst);
int  GDX_CALLCONV d_gdxDataReadRawFast (gdxHandle_t pgdx, int SyNr, TDataStoreProc_t DP, int *NrRecs);
int  GDX_CALLCONV d_gdxDataReadRawFastFilt (gdxHandle_t pgdx, int SyNr, const char *UelFilterStr[], TDataStoreFiltProc_t DP);
int  GDX_CALLCONV d_gdxDataReadRawStart (gdxHandle_t pgdx, int SyNr, int *NrRecs);
int  GDX_CALLCONV d_gdxDataReadSlice (gdxHandle_t pgdx, const char *UelFilterStr[], int *Dimen, TDataStoreProc_t DP);
int  GDX_CALLCONV d_gdxDataReadSliceStart (gdxHandle_t pgdx, int SyNr, int ElemCounts[]);
int  GDX_CALLCONV d_gdxDataReadStr (gdxHandle_t pgdx, char *KeyStr[], double Values[], int *DimFrst);
int  GDX_CALLCONV d_gdxDataReadStrStart (gdxHandle_t pgdx, int SyNr, int *NrRecs);
int  GDX_CALLCONV d_gdxDataSliceUELS (gdxHandle_t pgdx, const int SliceKeyInt[], char *KeyStr[]);
int  GDX_CALLCONV d_gdxDataWriteDone (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxDataWriteMap (gdxHandle_t pgdx, const int KeyInt[], const double Values[]);
int  GDX_CALLCONV d_gdxDataWriteMapStart (gdxHandle_t pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
int  GDX_CALLCONV d_gdxDataWriteRaw (gdxHandle_t pgdx, const int KeyInt[], const double Values[]);
int  GDX_CALLCONV d_gdxDataWriteRawStart (gdxHandle_t pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
int  GDX_CALLCONV d_gdxDataWriteStr (gdxHandle_t pgdx, const char *KeyStr[], const double Values[]);
int  GDX_CALLCONV d_gdxDataWriteStrStart (gdxHandle_t pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
int  GDX_CALLCONV d_gdxGetDLLVersion (gdxHandle_t pgdx, char *V);
int  GDX_CALLCONV d_gdxErrorCount (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxErrorStr (gdxHandle_t pgdx, int ErrNr, char *ErrMsg);
int  GDX_CALLCONV d_gdxFileInfo (gdxHandle_t pgdx, int *FileVer, int *ComprLev);
int  GDX_CALLCONV d_gdxFileVersion (gdxHandle_t pgdx, char *FileStr, char *ProduceStr);
int  GDX_CALLCONV d_gdxFilterExists (gdxHandle_t pgdx, int FilterNr);
int  GDX_CALLCONV d_gdxFilterRegister (gdxHandle_t pgdx, int UelMap);
int  GDX_CALLCONV d_gdxFilterRegisterDone (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxFilterRegisterStart (gdxHandle_t pgdx, int FilterNr);
int  GDX_CALLCONV d_gdxFindSymbol (gdxHandle_t pgdx, const char *SyId, int *SyNr);
int  GDX_CALLCONV d_gdxGetElemText (gdxHandle_t pgdx, int TxtNr, char *Txt, int *Node);
int  GDX_CALLCONV d_gdxGetLastError (gdxHandle_t pgdx);
INT64  GDX_CALLCONV d_gdxGetMemoryUsed (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxGetSpecialValues (gdxHandle_t pgdx, double AVals[]);
int  GDX_CALLCONV d_gdxGetUEL (gdxHandle_t pgdx, int UelNr, char *Uel);
int  GDX_CALLCONV d_gdxMapValue (gdxHandle_t pgdx, double D, int *sv);
int  GDX_CALLCONV d_gdxOpenAppend (gdxHandle_t pgdx, const char *FileName, const char *Producer, int *ErrNr);
int  GDX_CALLCONV d_gdxOpenRead (gdxHandle_t pgdx, const char *FileName, int *ErrNr);
int  GDX_CALLCONV d_gdxOpenWrite (gdxHandle_t pgdx, const char *FileName, const char *Producer, int *ErrNr);
int  GDX_CALLCONV d_gdxOpenWriteEx (gdxHandle_t pgdx, const char *FileName, const char *Producer, int Compr, int *ErrNr);
int  GDX_CALLCONV d_gdxResetSpecialValues (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxSetHasText (gdxHandle_t pgdx, int SyNr);
int  GDX_CALLCONV d_gdxSetReadSpecialValues (gdxHandle_t pgdx, const double AVals[]);
int  GDX_CALLCONV d_gdxSetSpecialValues (gdxHandle_t pgdx, const double AVals[]);
int  GDX_CALLCONV d_gdxSetTextNodeNr (gdxHandle_t pgdx, int TxtNr, int Node);
int  GDX_CALLCONV d_gdxSetTraceLevel (gdxHandle_t pgdx, int N, const char *s);
int  GDX_CALLCONV d_gdxSymbIndxMaxLength (gdxHandle_t pgdx, int SyNr, int LengthInfo[]);
int  GDX_CALLCONV d_gdxSymbMaxLength (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxSymbolAddComment (gdxHandle_t pgdx, int SyNr, const char *Txt);
int  GDX_CALLCONV d_gdxSymbolGetComment (gdxHandle_t pgdx, int SyNr, int N, char *Txt);
int  GDX_CALLCONV d_gdxSymbolGetDomain (gdxHandle_t pgdx, int SyNr, int DomainSyNrs[]);
int  GDX_CALLCONV d_gdxSymbolGetDomainX (gdxHandle_t pgdx, int SyNr, char *DomainIDs[]);
int  GDX_CALLCONV d_gdxSymbolDim (gdxHandle_t pgdx, int SyNr);
int  GDX_CALLCONV d_gdxSymbolInfo (gdxHandle_t pgdx, int SyNr, char *SyId, int *Dimen, int *Typ);
int  GDX_CALLCONV d_gdxSymbolInfoX (gdxHandle_t pgdx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt);
int  GDX_CALLCONV d_gdxSymbolSetDomain (gdxHandle_t pgdx, const char *DomainIDs[]);
int  GDX_CALLCONV d_gdxSymbolSetDomainX (gdxHandle_t pgdx, int SyNr, const char *DomainIDs[]);
int  GDX_CALLCONV d_gdxSystemInfo (gdxHandle_t pgdx, int *SyCnt, int *UelCnt);
int  GDX_CALLCONV d_gdxUELMaxLength (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxUELRegisterDone (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxUELRegisterMap (gdxHandle_t pgdx, int UMap, const char *Uel);
int  GDX_CALLCONV d_gdxUELRegisterMapStart (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxUELRegisterRaw (gdxHandle_t pgdx, const char *Uel);
int  GDX_CALLCONV d_gdxUELRegisterRawStart (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxUELRegisterStr (gdxHandle_t pgdx, const char *Uel, int *UelNr);
int  GDX_CALLCONV d_gdxUELRegisterStrStart (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxUMFindUEL (gdxHandle_t pgdx, const char *Uel, int *UelNr, int *UelMap);
int  GDX_CALLCONV d_gdxUMUelGet (gdxHandle_t pgdx, int UelNr, char *Uel, int *UelMap);
int  GDX_CALLCONV d_gdxUMUelInfo (gdxHandle_t pgdx, int *UelCnt, int *HighMap);
int  GDX_CALLCONV d_gdxGetDomainElements (gdxHandle_t pgdx, int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int *NrElem, void *Uptr);
int  GDX_CALLCONV d_gdxCurrentDim (gdxHandle_t pgdx);
int  GDX_CALLCONV d_gdxRenameUEL (gdxHandle_t pgdx, const char *OldName, const char *NewName);

typedef int  (GDX_CALLCONV *gdxAcronymAdd_t) (gdxHandle_t pgdx, const char *AName, const char *Txt, int AIndx);
GDX_FUNCPTR(gdxAcronymAdd);
typedef int  (GDX_CALLCONV *gdxAcronymCount_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxAcronymCount);
typedef int  (GDX_CALLCONV *gdxAcronymGetInfo_t) (gdxHandle_t pgdx, int N, char *AName, char *Txt, int *AIndx);
GDX_FUNCPTR(gdxAcronymGetInfo);
typedef int  (GDX_CALLCONV *gdxAcronymGetMapping_t) (gdxHandle_t pgdx, int N, int *orgIndx, int *newIndx, int *autoIndex);
GDX_FUNCPTR(gdxAcronymGetMapping);
typedef int  (GDX_CALLCONV *gdxAcronymIndex_t) (gdxHandle_t pgdx, double V);
GDX_FUNCPTR(gdxAcronymIndex);
typedef int  (GDX_CALLCONV *gdxAcronymName_t) (gdxHandle_t pgdx, double V, char *AName);
GDX_FUNCPTR(gdxAcronymName);
typedef int  (GDX_CALLCONV *gdxAcronymNextNr_t) (gdxHandle_t pgdx, int NV);
GDX_FUNCPTR(gdxAcronymNextNr);
typedef int  (GDX_CALLCONV *gdxAcronymSetInfo_t) (gdxHandle_t pgdx, int N, const char *AName, const char *Txt, int AIndx);
GDX_FUNCPTR(gdxAcronymSetInfo);
typedef double  (GDX_CALLCONV *gdxAcronymValue_t) (gdxHandle_t pgdx, int AIndx);
GDX_FUNCPTR(gdxAcronymValue);
typedef int  (GDX_CALLCONV *gdxAddAlias_t) (gdxHandle_t pgdx, const char *Id1, const char *Id2);
GDX_FUNCPTR(gdxAddAlias);
typedef int  (GDX_CALLCONV *gdxAddSetText_t) (gdxHandle_t pgdx, const char *Txt, int *TxtNr);
GDX_FUNCPTR(gdxAddSetText);
typedef int  (GDX_CALLCONV *gdxAutoConvert_t) (gdxHandle_t pgdx, int NV);
GDX_FUNCPTR(gdxAutoConvert);
typedef int  (GDX_CALLCONV *gdxClose_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxClose);
typedef int  (GDX_CALLCONV *gdxDataErrorCount_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxDataErrorCount);
typedef int  (GDX_CALLCONV *gdxDataErrorRecord_t) (gdxHandle_t pgdx, int RecNr, int KeyInt[], double Values[]);
GDX_FUNCPTR(gdxDataErrorRecord);
typedef int  (GDX_CALLCONV *gdxDataReadDone_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxDataReadDone);
typedef int  (GDX_CALLCONV *gdxDataReadFilteredStart_t) (gdxHandle_t pgdx, int SyNr, const int FilterAction[], int *NrRecs);
GDX_FUNCPTR(gdxDataReadFilteredStart);
typedef int  (GDX_CALLCONV *gdxDataReadMap_t) (gdxHandle_t pgdx, int RecNr, int KeyInt[], double Values[], int *DimFrst);
GDX_FUNCPTR(gdxDataReadMap);
typedef int  (GDX_CALLCONV *gdxDataReadMapStart_t) (gdxHandle_t pgdx, int SyNr, int *NrRecs);
GDX_FUNCPTR(gdxDataReadMapStart);
typedef int  (GDX_CALLCONV *gdxDataReadRaw_t) (gdxHandle_t pgdx, int KeyInt[], double Values[], int *DimFrst);
GDX_FUNCPTR(gdxDataReadRaw);
typedef int  (GDX_CALLCONV *gdxDataReadRawFast_t) (gdxHandle_t pgdx, int SyNr, TDataStoreProc_t DP, int *NrRecs);
GDX_FUNCPTR(gdxDataReadRawFast);
typedef int  (GDX_CALLCONV *gdxDataReadRawFastFilt_t) (gdxHandle_t pgdx, int SyNr, const char *UelFilterStr[], TDataStoreFiltProc_t DP);
GDX_FUNCPTR(gdxDataReadRawFastFilt);
typedef int  (GDX_CALLCONV *gdxDataReadRawStart_t) (gdxHandle_t pgdx, int SyNr, int *NrRecs);
GDX_FUNCPTR(gdxDataReadRawStart);
typedef int  (GDX_CALLCONV *gdxDataReadSlice_t) (gdxHandle_t pgdx, const char *UelFilterStr[], int *Dimen, TDataStoreProc_t DP);
GDX_FUNCPTR(gdxDataReadSlice);
typedef int  (GDX_CALLCONV *gdxDataReadSliceStart_t) (gdxHandle_t pgdx, int SyNr, int ElemCounts[]);
GDX_FUNCPTR(gdxDataReadSliceStart);
typedef int  (GDX_CALLCONV *gdxDataReadStr_t) (gdxHandle_t pgdx, char *KeyStr[], double Values[], int *DimFrst);
GDX_FUNCPTR(gdxDataReadStr);
typedef int  (GDX_CALLCONV *gdxDataReadStrStart_t) (gdxHandle_t pgdx, int SyNr, int *NrRecs);
GDX_FUNCPTR(gdxDataReadStrStart);
typedef int  (GDX_CALLCONV *gdxDataSliceUELS_t) (gdxHandle_t pgdx, const int SliceKeyInt[], char *KeyStr[]);
GDX_FUNCPTR(gdxDataSliceUELS);
typedef int  (GDX_CALLCONV *gdxDataWriteDone_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxDataWriteDone);
typedef int  (GDX_CALLCONV *gdxDataWriteMap_t) (gdxHandle_t pgdx, const int KeyInt[], const double Values[]);
GDX_FUNCPTR(gdxDataWriteMap);
typedef int  (GDX_CALLCONV *gdxDataWriteMapStart_t) (gdxHandle_t pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
GDX_FUNCPTR(gdxDataWriteMapStart);
typedef int  (GDX_CALLCONV *gdxDataWriteRaw_t) (gdxHandle_t pgdx, const int KeyInt[], const double Values[]);
GDX_FUNCPTR(gdxDataWriteRaw);
typedef int  (GDX_CALLCONV *gdxDataWriteRawStart_t) (gdxHandle_t pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
GDX_FUNCPTR(gdxDataWriteRawStart);
typedef int  (GDX_CALLCONV *gdxDataWriteStr_t) (gdxHandle_t pgdx, const char *KeyStr[], const double Values[]);
GDX_FUNCPTR(gdxDataWriteStr);
typedef int  (GDX_CALLCONV *gdxDataWriteStrStart_t) (gdxHandle_t pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
GDX_FUNCPTR(gdxDataWriteStrStart);
typedef int  (GDX_CALLCONV *gdxGetDLLVersion_t) (gdxHandle_t pgdx, char *V);
GDX_FUNCPTR(gdxGetDLLVersion);
typedef int  (GDX_CALLCONV *gdxErrorCount_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxErrorCount);
typedef int  (GDX_CALLCONV *gdxErrorStr_t) (gdxHandle_t pgdx, int ErrNr, char *ErrMsg);
GDX_FUNCPTR(gdxErrorStr);
typedef int  (GDX_CALLCONV *gdxFileInfo_t) (gdxHandle_t pgdx, int *FileVer, int *ComprLev);
GDX_FUNCPTR(gdxFileInfo);
typedef int  (GDX_CALLCONV *gdxFileVersion_t) (gdxHandle_t pgdx, char *FileStr, char *ProduceStr);
GDX_FUNCPTR(gdxFileVersion);
typedef int  (GDX_CALLCONV *gdxFilterExists_t) (gdxHandle_t pgdx, int FilterNr);
GDX_FUNCPTR(gdxFilterExists);
typedef int  (GDX_CALLCONV *gdxFilterRegister_t) (gdxHandle_t pgdx, int UelMap);
GDX_FUNCPTR(gdxFilterRegister);
typedef int  (GDX_CALLCONV *gdxFilterRegisterDone_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxFilterRegisterDone);
typedef int  (GDX_CALLCONV *gdxFilterRegisterStart_t) (gdxHandle_t pgdx, int FilterNr);
GDX_FUNCPTR(gdxFilterRegisterStart);
typedef int  (GDX_CALLCONV *gdxFindSymbol_t) (gdxHandle_t pgdx, const char *SyId, int *SyNr);
GDX_FUNCPTR(gdxFindSymbol);
typedef int  (GDX_CALLCONV *gdxGetElemText_t) (gdxHandle_t pgdx, int TxtNr, char *Txt, int *Node);
GDX_FUNCPTR(gdxGetElemText);
typedef int  (GDX_CALLCONV *gdxGetLastError_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxGetLastError);
typedef INT64  (GDX_CALLCONV *gdxGetMemoryUsed_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxGetMemoryUsed);
typedef int  (GDX_CALLCONV *gdxGetSpecialValues_t) (gdxHandle_t pgdx, double AVals[]);
GDX_FUNCPTR(gdxGetSpecialValues);
typedef int  (GDX_CALLCONV *gdxGetUEL_t) (gdxHandle_t pgdx, int UelNr, char *Uel);
GDX_FUNCPTR(gdxGetUEL);
typedef int  (GDX_CALLCONV *gdxMapValue_t) (gdxHandle_t pgdx, double D, int *sv);
GDX_FUNCPTR(gdxMapValue);
typedef int  (GDX_CALLCONV *gdxOpenAppend_t) (gdxHandle_t pgdx, const char *FileName, const char *Producer, int *ErrNr);
GDX_FUNCPTR(gdxOpenAppend);
typedef int  (GDX_CALLCONV *gdxOpenRead_t) (gdxHandle_t pgdx, const char *FileName, int *ErrNr);
GDX_FUNCPTR(gdxOpenRead);
typedef int  (GDX_CALLCONV *gdxOpenWrite_t) (gdxHandle_t pgdx, const char *FileName, const char *Producer, int *ErrNr);
GDX_FUNCPTR(gdxOpenWrite);
typedef int  (GDX_CALLCONV *gdxOpenWriteEx_t) (gdxHandle_t pgdx, const char *FileName, const char *Producer, int Compr, int *ErrNr);
GDX_FUNCPTR(gdxOpenWriteEx);
typedef int  (GDX_CALLCONV *gdxResetSpecialValues_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxResetSpecialValues);
typedef int  (GDX_CALLCONV *gdxSetHasText_t) (gdxHandle_t pgdx, int SyNr);
GDX_FUNCPTR(gdxSetHasText);
typedef int  (GDX_CALLCONV *gdxSetReadSpecialValues_t) (gdxHandle_t pgdx, const double AVals[]);
GDX_FUNCPTR(gdxSetReadSpecialValues);
typedef int  (GDX_CALLCONV *gdxSetSpecialValues_t) (gdxHandle_t pgdx, const double AVals[]);
GDX_FUNCPTR(gdxSetSpecialValues);
typedef int  (GDX_CALLCONV *gdxSetTextNodeNr_t) (gdxHandle_t pgdx, int TxtNr, int Node);
GDX_FUNCPTR(gdxSetTextNodeNr);
typedef int  (GDX_CALLCONV *gdxSetTraceLevel_t) (gdxHandle_t pgdx, int N, const char *s);
GDX_FUNCPTR(gdxSetTraceLevel);
typedef int  (GDX_CALLCONV *gdxSymbIndxMaxLength_t) (gdxHandle_t pgdx, int SyNr, int LengthInfo[]);
GDX_FUNCPTR(gdxSymbIndxMaxLength);
typedef int  (GDX_CALLCONV *gdxSymbMaxLength_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxSymbMaxLength);
typedef int  (GDX_CALLCONV *gdxSymbolAddComment_t) (gdxHandle_t pgdx, int SyNr, const char *Txt);
GDX_FUNCPTR(gdxSymbolAddComment);
typedef int  (GDX_CALLCONV *gdxSymbolGetComment_t) (gdxHandle_t pgdx, int SyNr, int N, char *Txt);
GDX_FUNCPTR(gdxSymbolGetComment);
typedef int  (GDX_CALLCONV *gdxSymbolGetDomain_t) (gdxHandle_t pgdx, int SyNr, int DomainSyNrs[]);
GDX_FUNCPTR(gdxSymbolGetDomain);
typedef int  (GDX_CALLCONV *gdxSymbolGetDomainX_t) (gdxHandle_t pgdx, int SyNr, char *DomainIDs[]);
GDX_FUNCPTR(gdxSymbolGetDomainX);
typedef int  (GDX_CALLCONV *gdxSymbolDim_t) (gdxHandle_t pgdx, int SyNr);
GDX_FUNCPTR(gdxSymbolDim);
typedef int  (GDX_CALLCONV *gdxSymbolInfo_t) (gdxHandle_t pgdx, int SyNr, char *SyId, int *Dimen, int *Typ);
GDX_FUNCPTR(gdxSymbolInfo);
typedef int  (GDX_CALLCONV *gdxSymbolInfoX_t) (gdxHandle_t pgdx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt);
GDX_FUNCPTR(gdxSymbolInfoX);
typedef int  (GDX_CALLCONV *gdxSymbolSetDomain_t) (gdxHandle_t pgdx, const char *DomainIDs[]);
GDX_FUNCPTR(gdxSymbolSetDomain);
typedef int  (GDX_CALLCONV *gdxSymbolSetDomainX_t) (gdxHandle_t pgdx, int SyNr, const char *DomainIDs[]);
GDX_FUNCPTR(gdxSymbolSetDomainX);
typedef int  (GDX_CALLCONV *gdxSystemInfo_t) (gdxHandle_t pgdx, int *SyCnt, int *UelCnt);
GDX_FUNCPTR(gdxSystemInfo);
typedef int  (GDX_CALLCONV *gdxUELMaxLength_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxUELMaxLength);
typedef int  (GDX_CALLCONV *gdxUELRegisterDone_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxUELRegisterDone);
typedef int  (GDX_CALLCONV *gdxUELRegisterMap_t) (gdxHandle_t pgdx, int UMap, const char *Uel);
GDX_FUNCPTR(gdxUELRegisterMap);
typedef int  (GDX_CALLCONV *gdxUELRegisterMapStart_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxUELRegisterMapStart);
typedef int  (GDX_CALLCONV *gdxUELRegisterRaw_t) (gdxHandle_t pgdx, const char *Uel);
GDX_FUNCPTR(gdxUELRegisterRaw);
typedef int  (GDX_CALLCONV *gdxUELRegisterRawStart_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxUELRegisterRawStart);
typedef int  (GDX_CALLCONV *gdxUELRegisterStr_t) (gdxHandle_t pgdx, const char *Uel, int *UelNr);
GDX_FUNCPTR(gdxUELRegisterStr);
typedef int  (GDX_CALLCONV *gdxUELRegisterStrStart_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxUELRegisterStrStart);
typedef int  (GDX_CALLCONV *gdxUMFindUEL_t) (gdxHandle_t pgdx, const char *Uel, int *UelNr, int *UelMap);
GDX_FUNCPTR(gdxUMFindUEL);
typedef int  (GDX_CALLCONV *gdxUMUelGet_t) (gdxHandle_t pgdx, int UelNr, char *Uel, int *UelMap);
GDX_FUNCPTR(gdxUMUelGet);
typedef int  (GDX_CALLCONV *gdxUMUelInfo_t) (gdxHandle_t pgdx, int *UelCnt, int *HighMap);
GDX_FUNCPTR(gdxUMUelInfo);
typedef int  (GDX_CALLCONV *gdxGetDomainElements_t) (gdxHandle_t pgdx, int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int *NrElem, void *Uptr);
GDX_FUNCPTR(gdxGetDomainElements);
typedef int  (GDX_CALLCONV *gdxCurrentDim_t) (gdxHandle_t pgdx);
GDX_FUNCPTR(gdxCurrentDim);
typedef int  (GDX_CALLCONV *gdxRenameUEL_t) (gdxHandle_t pgdx, const char *OldName, const char *NewName);
GDX_FUNCPTR(gdxRenameUEL);
#if defined(__cplusplus)
}
#endif
#endif /* #if ! defined(_GDXCC_H_) */
