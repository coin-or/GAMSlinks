/* dict.h
 *
 * External declarations for dict routines.
 *
 */

/* status for DIMENSION/UEL increase: done */

#if ! defined(_DICT_H_)
#define _DICT_H_

#include "gamsglobals.h"

#define CIO_MAX_INDEX_DIM   GLOBAL_MAX_INDEX_DIM
#define CIO_UEL_IDENT_SIZE  GLOBAL_UEL_IDENT_SIZE

#if defined(_WIN32) && defined(DICTDLL)
# if defined(DD_EXPORTS)
#  define DD_API __declspec(dllexport)
# else
#  define DD_API __declspec(dllimport)
# endif
# define DD_CALLCONV __stdcall
#else
# define DD_API
# define DD_CALLCONV
#endif

enum {
  unknownSymType = 0,
  funcSymType,
  setSymType,
  acrSymType,
  parmSymType,
  varSymType,
  eqnSymType,
  aliasSymType = 127
};

#define GCD_INVALID_HANDLE   NULL

struct gcdHandle_;
typedef struct gcdHandle_ *gcdHandle_t;
struct dictRec;
typedef struct dictRec     dictRec_t;

#if defined(DICTSTATS)
typedef struct statsRec {
  int uelHashSize;
  int localSymHashSize;
  int numUelInserts;
  int numUelLookups;
  int numFailedUelLookups;
  int numUelProbes;
  int numLocalSymInserts;
  int numLocalSymLookups;
  int numFailedLocalSymLookups;
  int numLocalSymProbes;
  int numRowGets;
  int numRowProbes;
  int numColGets;
  int numColProbes;
} statsRec_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* gcdLoad call is deprecated, don't export it */
int
gcdLoad (dictRec_t **dict, const char *dictFileName, int version);
DD_API int DD_CALLCONV
gcdLoadEx (dictRec_t **dict, const char *dictFileName,
	   char *msgBuf, int msgBufLen);
DD_API void DD_CALLCONV
gcdFree (dictRec_t *dict);

DD_API int DD_CALLCONV
gcdNLSyms (dictRec_t *dict);
DD_API int DD_CALLCONV
gcdNUels (dictRec_t *dict);
DD_API int DD_CALLCONV
gcdNCols (dictRec_t *dict);
DD_API int DD_CALLCONV
gcdNRows (dictRec_t *dict);
DD_API int DD_CALLCONV
gcdNModList (dictRec_t *dict);

DD_API char * DD_CALLCONV
gcdUelLabel (dictRec_t *dict, int index, char *buf, int buflen,
	     char *quote);
DD_API int DD_CALLCONV
gcdUelIndex (dictRec_t *dict, const char *uelLabel);

#if ! defined(DICTSTATS)
void
gcdGetStats (dictRec_t *dict, void *stats);
void *
gcdGetStatsRec (void);
#else
void
gcdGetStats (dictRec_t *dict, statsRec_t *stats);
statsRec_t *
gcdGetStatsRec (void);
#endif

DD_API char * DD_CALLCONV
gcdPascalSymText  (dictRec_t *dict, int index, char *buf, int buflen, char *quotePas);
DD_API char * DD_CALLCONV
gcdPascalUelLabel (dictRec_t *dict, int index, char *buf, int buflen,
		   char *quotePas);
DD_API char * DD_CALLCONV
gcdPascalSetText  (dictRec_t *dict, int l, int k, char *buf, int buflen,
		   char *quotePas);

DD_API int DD_CALLCONV
gcdSymIndex (dictRec_t *dict, const char *name);
DD_API char * DD_CALLCONV
gcdSymName  (dictRec_t *dict, int index, char *buf, int buflen);
DD_API char * DD_CALLCONV
gcdSymText  (dictRec_t *dict, int index, char *buf, int buflen, char *quote);
DD_API int DD_CALLCONV
gcdSymDom   (dictRec_t *dict, int index, int *symDoms, int symDim);
DD_API int DD_CALLCONV
gcdSymEntries (dictRec_t *dict, int index);
DD_API int DD_CALLCONV
gcdSymDim   (dictRec_t *dict, int index);
DD_API int DD_CALLCONV
gcdSymType  (dictRec_t *dict, int index);
DD_API int DD_CALLCONV
gcdSymAlias (dictRec_t *dict, int index);
DD_API int DD_CALLCONV
gcdSymOffset (dictRec_t *dict, int index);

DD_API int DD_CALLCONV
gcdSetIndex (dictRec_t *dict, int lSym, int k);
DD_API char * DD_CALLCONV
gcdSetText  (dictRec_t *dict, int l, int k, char *buf, int buflen,
	     char *quote);

DD_API int DD_CALLCONV
gcdModList    (dictRec_t *dict, int maxSyms, int *modListSyms);
DD_API int DD_CALLCONV
gcdModListSym (dictRec_t *dict, int k);

DD_API int DD_CALLCONV
gcdRowUels  (dictRec_t *dict, int i, int *symIndex, int *indices, int *symDim);
DD_API int DD_CALLCONV
gcdRowIndex (dictRec_t *dict, int symIndex, int dim, int *uelIndices,
	     int prevIndex);

DD_API int DD_CALLCONV
gcdColUels (dictRec_t *dict, int j, int *symIndex, int *indices, int *symDim);
DD_API int DD_CALLCONV
gcdColIndex (dictRec_t *dict, int symIndex, int dim, int *uelIndices,
	     int prevIndex);

DD_API gcdHandle_t DD_CALLCONV
gcdFindFirstRow (dictRec_t *dict, int symIndex, int indexPos, int uelIndex,
		 int dim, int *rowIndex);
DD_API gcdHandle_t DD_CALLCONV
gcdFindFirstCol (dictRec_t *dict, int symIndex, int indexPos, int uelIndex,
		 int dim, int *colIndex);
DD_API void DD_CALLCONV
gcdFindClose (gcdHandle_t findHandle);
DD_API int DD_CALLCONV
gcdFindNextRow (gcdHandle_t findHandle, int *rowIndex);
DD_API int DD_CALLCONV
gcdFindNextCol (gcdHandle_t findHandle, int *colIndex);

DD_API gcdHandle_t DD_CALLCONV
gcdFindFirstRowCol (dictRec_t *dict, int symIndex, const int *uelIndices,
		   int *rcIndex);
DD_API int DD_CALLCONV
gcdFindNextRowCol (gcdHandle_t findHandle, int *rowIndex);

#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_DICT_H_) */
