#include "optcc.h"

#if defined(__cplusplus)
extern "C" {
#endif
/* Wrapper function headers */
int    optDefined(optHandle_t oh, const char *optname);
int    optDRecent(optHandle_t oh, const char *optname);
int    optGetStrI(optHandle_t oh, const char *optname);
double optGetStrD(optHandle_t oh, const char *optname);
char  *optGetStrS(optHandle_t oh, const char *optname, char *sval);
void   optSetStrB(optHandle_t oh, const char *optname, int ival);
void   optSetStrI(optHandle_t oh, const char *optname, int ival);
void   optSetStrD(optHandle_t oh, const char *optname, double dval);
void   optSetStrS(optHandle_t oh, const char *optname, char *sval);
#if defined(__cplusplus)
}
#endif

#define optDefined_writemps(oh)                                                     optDefined(oh,"writemps")
#define optDRecent_writemps(oh)                                                     optDRecent(oh,"writemps")
#define optGetStrS_writemps(oh,sval)                                                optGetStrS(oh,"writemps",sval)
#define optSetStrS_writemps(oh,sval)                                                optSetStrS(oh,"writemps",sval)
#define optDefined_reslim(oh)                                                       optDefined(oh,"reslim")
#define optDRecent_reslim(oh)                                                       optDRecent(oh,"reslim")
#define optGetStrD_reslim(oh)                                                       optGetStrD(oh,"reslim")
#define optSetStrD_reslim(oh,dval)                                                  optSetStrD(oh,"reslim",dval)
#define optDefined_iterlim(oh)                                                      optDefined(oh,"iterlim")
#define optDRecent_iterlim(oh)                                                      optDRecent(oh,"iterlim")
#define optGetStrI_iterlim(oh)                                                      optGetStrI(oh,"iterlim")
#define optSetStrI_iterlim(oh,ival)                                                 optSetStrI(oh,"iterlim",ival)
#define optDefined_optcr(oh)                                                        optDefined(oh,"optcr")
#define optDRecent_optcr(oh)                                                        optDRecent(oh,"optcr")
#define optGetStrD_optcr(oh)                                                        optGetStrD(oh,"optcr")
#define optSetStrD_optcr(oh,dval)                                                   optSetStrD(oh,"optcr",dval)
#define optDefined_nobounds(oh)                                                     optDefined(oh,"nobounds")
#define optDRecent_nobounds(oh)                                                     optDRecent(oh,"nobounds")
#define optGetStrB_nobounds(oh)                                                     optGetStrI(oh,"nobounds")
#define optGetStrI_nobounds(oh)                                                     optGetStrI(oh,"nobounds")
#define optSetStrB_nobounds(oh,ival)                                                optSetStrB(oh,"nobounds",ival)
#define optSetStrI_nobounds(oh,ival)                                                optSetStrI(oh,"nobounds",ival)
#define optDefined_readfile(oh)                                                     optDefined(oh,"readfile")
#define optDRecent_readfile(oh)                                                     optDRecent(oh,"readfile")
#define optGetStrS_readfile(oh,sval)                                                optGetStrS(oh,"readfile",sval)
#define optSetStrS_readfile(oh,sval)                                                optSetStrS(oh,"readfile",sval)
