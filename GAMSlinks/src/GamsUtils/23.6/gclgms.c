/* global constants (symbol dimensions, symbol layout, etc.)
 * that might ultimately come from global
 * created: Steve Dirkse, July 2007
 */

#include "gclgms.h"

const char *gmsGdxTypeText[GMS_DT_MAX] =
  {"Set","Parameter","Variable","Equation","Alias"};
const char *gmsVarTypeText[GMS_VARTYPE_MAX] = {"Unknown","Binary","Integer","Positive","Negative","Free","Sos1","Sos2","Semicont","Semiint"};
const char *gmsValTypeText[GMS_VAL_MAX] = {".l",".m",".lo",".ub",".scale"};
const char *gmsSVText[GMS_SVIDX_MAX] = {"UNdef","NA","+Inf","-Inf","Eps","0","AcroN"};

const double gmsDefRecVar[GMS_VARTYPE_MAX][GMS_VAL_MAX] = {
 /* .l   .m           .lo         .ub  .scale */
  { 0.0, 0.0,         0.0,         0.0, 1.0},    /* unknown */
  { 0.0, 0.0,         0.0,         1.0, 1.0},    /* binary */
  { 0.0, 0.0,         0.0,       100.0, 1.0},    /* integer */
  { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* positive */
  { 0.0, 0.0, GMS_SV_MINF,         0.0, 1.0},    /* negative */
  { 0.0, 0.0, GMS_SV_MINF, GMS_SV_PINF, 1.0},    /* free */
  { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* sos1 */
  { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* sos2 */
  { 0.0, 0.0,         1.0, GMS_SV_PINF, 1.0},    /* semicont */
  { 0.0, 0.0,         1.0,       100.0, 1.0}     /* semiint */
};

const double gmsDefRecEqu[GMS_EQUTYPE_MAX][GMS_VAL_MAX] = {
 /* .l   .m           .lo         .ub  .scale */
  { 0.0, 0.0,         0.0,         0.0, 1.0},    /* =e= */
  { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* =g= */
  { 0.0, 0.0, GMS_SV_MINF,         0.0, 1.0},    /* =l= */
  { 0.0, 0.0, GMS_SV_MINF, GMS_SV_PINF, 1.0},    /* =n= */
  { 0.0, 0.0,         0.0,         0.0, 1.0},    /* =x= */
  { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0}     /* =c= */
};
