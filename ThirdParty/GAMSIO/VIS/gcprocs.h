/*
 * gcprocs.h
 *
 * types, defines, and headers used to make the
 * C I/O Library machine and compiler-independent.
 *
 * these deal with process start-up routines
 */

#if ! defined(_GCPROCS_H_)
#define       _GCPROCS_H_

#include "gclib.h"

#if defined(__cplusplus)
extern "C" {
#endif

int GC_gsshell (char *cmdline, int *rc);
int GC_gsspawnv  (char *pathname, char **argv, int *rc);
int GC_gsspawnvp (char *filename, char **argv, int *rc);
int GC_shell (char *cmdline);
int GC_spawnv  (char *pathname, char **argv);
int GC_spawnvp (char *filename, char **argv);

#if defined(__cplusplus)
}
#endif

#endif /* defined(_GCPROCS_H_) */
