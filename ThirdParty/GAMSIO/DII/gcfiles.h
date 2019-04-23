/*
 * gcfiles.h
 *
 * types, defines, and headers used to make the
 * C I/O Library machine and compiler-independent.
 *
 * these deal with the file system
 */

#if ! defined(_GCFILES_H_)
#define       _GCFILES_H_

#include "gclib.h"

#define FP(X)   ((X)->fp)

typedef struct _GC_file {
  char    *modeFlags;		/* for use in fopen call */
  char    *name;		/* for use in fopen call */
  char    *path, *base, *ext;	/* components of name */
  FILE    *fp;
  GC_BOOLEAN isOpen;		/* kept in case somebody messes with *fp */
  int     mode;			/* read, write, or append */
  int     format;		/* binary or text */
} GC_file;

#if defined(__cplusplus)
extern "C" {
#endif

char *GC_getPathName (const char *absName);
char *GC_getBaseName (const char *absName);
char *GC_getExtension (const char *absName);
GC_file *GC_finit (const char *name, char *path, char *base, char *ext);
void GC_fileFree (GC_file *GC_fp);
GC_BOOLEAN GC_fopen (int mode, int format, GC_file *GC_fp);
int GC_fclose (GC_file *GC_fp);
int GC_rename (GC_file *GC_fromfp, GC_file *GC_tofp);
void GC_fdump (GC_file *GC_fp, FILE *fp);

#if defined(__cplusplus)
}
#endif

#endif /* defined(_GCFILES_H_) */
