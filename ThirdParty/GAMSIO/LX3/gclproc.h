/* gclproc.h
 * written on 01/13/06 12:03:32 by global/ver22
 *
 * model types (procs?) used by CMEX
 */

#if ! defined(_GCLPROC_H_)
#     define  _GCLPROC_H_

#define PROC_NONE 0
#define PROC_LP 1
#define PROC_MIP 2
#define PROC_RMIP 3
#define PROC_NLP 4
#define PROC_MCP 5
#define PROC_MPEC 6
#define PROC_RMPEC 7
#define PROC_CNS 8
#define PROC_DNLP 9
#define PROC_RMINLP 10
#define PROC_MINLP 11
#define PROC_QCP 12
#define PROC_MIQCP 13
#define PROC_RMIQCP 14
#define PROC_DIM 15

extern const int          gclProcHide[PROC_DIM];
extern const int          gclProcRelaxed[PROC_DIM];
extern const char * const gclProcName[PROC_DIM];
extern const char * const gclProcText[PROC_DIM];

#endif /* if ! defined(_GCLPROC_H_) */
