int bchWriteSol(const char *fname, dictRec_t *dict, int n, double *lb, double *x, double *ub, double *rc);
int bchReadSolGDX(char* fname, dictRec_t *dict, int n, double *x);
int bchRunGAMS(const char *gcall, const char *gdxFN, const int keep, const char *jobid);
int bchQueryHeuristic(const int heurcnt, const int heurint, const int heurmult, const int firstcnt,
                      const int objcnt,const double bestbnd, const double bestint, const double curobj,
                      const int intfeas, const int objsen, const int dointheur);
int bchQueryCuts(const int cutcnt, const int cutint, const int cutmult, const int firstcnt,
                 const double bestint, const int intfeas, const int objsen, const int dointcut);
int bchGetCutCtrlInfo(char* fname, int *ncuts, int *nnz, int *symNnz);
int bchGetCutMatrix(const char* fname, dictRec_t *dict, const int n, const int ncuts,
                    int *symNnz, double *rhs, int* sense, int *cutbeg, int *cutind, double *cutval);
void bchSlvSetInf(const double pinf, const double minf);
void bchSlvGetInf(double *pinf, double *minf);




