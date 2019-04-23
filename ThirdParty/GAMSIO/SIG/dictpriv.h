/* dictpriv.h
 *
 * Steve Dirkse, GAMS Development Corp, Dec 1997
 * Internal declarations for dict routines.
 * We don't hide this from you, but if you do anything that
 * depends on the stuff in this file, you are on your own.
 * I recommend you don't look at it.
 *
 */

/* status for DIMENSION/UEL increase: done */

#if ! defined(_DICTPRIV_H_)
#define _DICTPRIV_H_

#include "dict.h"

/* use malloc from stdlib */
#define DICTMALLOC(PTR,TYPE,N) \
 ((PTR) = (TYPE *)malloc((N)*sizeof(TYPE)))
#define DICTFREE(X) do { \
  if (NULL != (X)) { free(X);(X) = NULL;} \
 } while (0)

typedef struct stringRec {
  char *s;
  int index;			/* [1 .. ???] */
} stringRec_t;

typedef struct localSymRec {
  int  *domList;		/* set localSyms, [0..dimension-1] */
  char *name;
  char *text;
  int  entries;
  int  offset;			/* to first entry in row/column or UEL list */
  int  aliasTo;			/* 0 for non-aliases, o/w index into localSyms */
  char dimension;
  char type;
  char textQuote;		/* ' ' for no quote */
} localSymRec_t;

struct gcdHandle_ {
  int symType;
  dictRec_t *dict;
  int symIndex;
  int indexPos;
  int uelIndex;
  int uelIndices[CIO_MAX_INDEX_DIM];
  int dim;
  int curr;			/* index in [0..numRows/numCols) returned by
				 * last successful search, or -1 if failure */
  int last;			/* index of first row/col in next eqn/var */
};

struct dictRec {
  /* the dictionary heaps do not support free(),
   * so we keep a pointer to the start of the "heap block",
   * and the first free byte and end of each heap
   */
  char *dHeap;			/* space for all dictionary mallocs */
  char *dcFree;			/* allocate from here */
  char *dcEnd;			/* first byte past the end */
  char *diFree;			/* allocate from here */
  char *diEnd;			/* first byte past the end */
  char *dpFree;			/* allocate from here */
  char *dpEnd;			/* first byte past the end */
  char *fileName;		/* of dictionary file */
  char **uelLabels;
  char *uelQuotes;
  char ***setText;
  char **setTextQuotes;
  localSymRec_t *localSyms;
  int *modListSyms;
  int **uelList;		/* uelList[i][0] = num UEL's in set i */
				/* uelList[i][1..k] = UEL indices */
  int **rowNames;		/* rowNames[i][0] = index in localSyms */
				/* rowNames[i][k] = UEL index for k'th
				 * subscript of row i */
  int **colNames;		/* colNames[j][0] = index in localSyms */
				/* colNames[j][k] = UEL index for k'th
				 * subscript of column j */
  stringRec_t **uelHashTable;
  stringRec_t **localSymHashTable;
#if defined(DICTSTATS)
  statsRec_t *stats;
#else
  void *stats;
#endif
  int numGlobalSyms;
  int numLocalSyms;
  int numIndexSets;
  int numUels;
  int numIndexEntries;
  int numRows;
  int numCols;
  int hasQuote;			/* strings in file are quoted */
  int dictFileOpt;
  int numAcronyms;
  int numModListSyms;
  int uelHashSize;
  int localSymHashSize;
  /* info from version 1 of dict file */
  int version;			/* version of dictionary file read */
  int lrgdim;			/* max dim for any r/c */
  int lrgindx;			/* max subblock for any r/c */
  int lrgblk;			/* max (dim * size) for any r/c */
  int ttlblk;			/* total entries in index space */
  /* added with version 2 of dict file */
  int numUelChars;		/* exclusive of terminating nulls */
  int numSymNameChars;		/* ditto */
  int numSymTextChars;		/* ditto */
  int numSymIndices;
  int numSetTextChars;		/* ditto */
  int numRowUels;
  int numColUels;
  int isRead;
};

int *
gcdImalloc (dictRec_t *dict, unsigned int n);
void *
gcdPmalloc (dictRec_t *dict, unsigned int n);
char *
gcdCmalloc (dictRec_t *dict, unsigned int n);
void
gcdMsgCopy (const char *msg, char *msgBuf, int msgBufLen);
int
gcdstricmp(const char *s1, const char *s2);


#endif /* ! defined(_DICTPRIV_H_) */
