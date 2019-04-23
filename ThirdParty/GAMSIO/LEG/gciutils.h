#if ! defined(_GCIUTILS_H_)
#define       _GCIUTILS_H_

typedef struct gciWrMatDat {
  int recLen;			/* size of this record, in bytes */

  /* first come the required arrays:
   * gciWriteMatrix will barf if these aren't here */
  int    *rowType;
  int    *rowNB;
  double *rowRHS;
  double *rowLev;
  int    *colNZ;			/* column nonzero count */
  int    *colNB;			/* is nonbasic */
  double *colLB;			/* lower bound */
  double *colLev;			/* level */
  double *colUB;			/* upper bound */
  int    *nzRow;			/* nonzero row indices */
  double *nzJ;				/* nonzero values */

  /* now the optional arrays:
   * just set them to NULL if you don't have the data */
  double *rowDual;
  double *rowScale;
  int    *rowNZ;			/* row nonzero count */
  int    *rowMch;			/* matching column for row */
  int    *colType;			/* continous, binary, etc. */
  int    *colSOSS;			/* set membership */
  double *colPrio;			/* priority */
  double *colDual;
  double *colScale;
  int    *nzType;			/* nonzero type */

  int m;
  int n;
  int nnz;
} gciWrMatDat_t;

#if defined(__cplusplus)
extern "C" {
#endif

int
gciWriteMatrix (const char *matrixFileName, int fileType,
		int scaleOpt, int priorOpt,
		gciWrMatDat_t *pd);

int
gciWrMatDatInit (gciWrMatDat_t *pd);


#if defined(__cplusplus)
}
#endif

#endif /* ! defined(_GCIUTILS_H_) */
