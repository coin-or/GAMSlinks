#if ! defined(_BBINFO_H_)
#define       _BBINFO_H_

typedef struct SBBBoundRec {
  int    *colIdx;				/* in 1..n */
  double *lb;
  double *ub;
  int nBounds;
  int nextBound;
} SBBBoundRec_t;
typedef SBBBoundRec_t *SBBBoundHandle_t;

typedef enum {
  ISMLP = 0,
  SREC,
  RREC,
  ITNLIM,
  BOUNDS,
  DOMLIM,
  PCONOFF,
  PCNUM,
  PCMXITER,
  IVEC_SIZE
} iVecRecs_t;

typedef enum {
  RESLIM = 0,
  PCFRAC,
  PCMAX,
  PCRES,
  RVEC_SIZE
} rVecRecs_t;

#if defined(__cplusplus)
extern "C" {
#endif

int
cioSBBInfo (SBBBoundHandle_t *boundHandle,
	    int usrIVec[], int usrIVecLen,
	    double usrRVec[], int usrRVecLen);
int
cioSBBReadNextBound (SBBBoundHandle_t boundHandle,
		     int *colIdx, double *lb, double *ub);
int
cioSBBCloseBounds (SBBBoundHandle_t *boundHandle);
int
cioSBBRecInfo (int *recLen, int *numRecs);
int
cioSBBRestart (int recNum, int givePrimal, int giveDual, int giveBasis,
	       double discLB[], double discUB[], double xLev[],
	       double fMarg[],
	       int varStat[], int equStat[]);
int
cioSBBSave (int recNum, int givePrimal, int giveDual, int giveBasis,
	    double discLB[], double discUB[], double xLev[],
	    double fMarg[],
	    int varStat[], int equStat[]);

#if defined(__cplusplus)
}
#endif

#endif /* #if ! defined(_BBINFO_H_) */
