#if ! defined(_GAMSXPRESSLICE_H_)
#define       _GAMSXPRESSLICE_H_

typedef enum XPlicenseInit {
  XPdemo,
  XPGAMS,
  XL,
  XPnone
} XPlicenseInit_t;

int
gamsxpresslice (int m,			/* rows */
	        int n,			/* columns */
	        int nnz,			/* nonzeroes */
	        int nl_nnz,		/* nonlinear nonzeros */
	        int disc_n,		/* discrete variables */
	        int skipBasic,		/* if true, skip basic (first) test */
	        XPlicenseInit_t *initType,
                char *licMsg, int licMsgSize);

#endif /* if ! defined(_GAMSXPRESSLICE_H_) */
