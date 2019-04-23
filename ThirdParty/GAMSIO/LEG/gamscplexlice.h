#if ! defined(_GAMSCPLEXLICE_H_)
#define       _GAMSCPLEXLICE_H_

typedef enum licenseInit {
  demo,
  zeroUses,
  GAMS,
  ILOG,
  CL,
  none
} licenseInit_t;

int
gamscplexlice (int m,			/* rows */
	       int n,			/* columns */
	       int nnz,			/* nonzeroes */
	       int nl_nnz,		/* nonlinear nonzeros */
	       int disc_n,		/* discrete variables */
	       int skipBasic,		/* if true, skip basic (first) test */
	       licenseInit_t *initType,
	       int *nUses,		/* simultaneous uses */
	       int *isMIP,
	       int *isBarrier,
	       int *isQuadratic,
	       int *nParallel,
	       int *isFloating);

#endif /* if ! defined(_GAMSCPLEXLICE_H_) */
