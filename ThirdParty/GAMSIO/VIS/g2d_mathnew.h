
#if !defined(_P3___mathnew___H)
#define _P3___mathnew___H

  typedef SYSTEM_byte G2DMATHNEW_tgamsfunc; /* Anonymous */ enum{G2DMATHNEW_fnmapval,G2DMATHNEW_fnceil,G2DMATHNEW_fnfloor,G2DMATHNEW_fnround,
    G2DMATHNEW_fnmod,G2DMATHNEW_fntrunc,G2DMATHNEW_fnsign,G2DMATHNEW_fnmin,
    G2DMATHNEW_fnmax,G2DMATHNEW_fnsqr,G2DMATHNEW_fnexp,G2DMATHNEW_fnlog,
    G2DMATHNEW_fnlog10,G2DMATHNEW_fnsqrt,G2DMATHNEW_fnabs,G2DMATHNEW_fncos,
    G2DMATHNEW_fnsin,G2DMATHNEW_fnarctan,G2DMATHNEW_fnerrf,G2DMATHNEW_fndunfm,
    G2DMATHNEW_fndnorm,G2DMATHNEW_fnpower,G2DMATHNEW_fnjdate,G2DMATHNEW_fnjtime,
    G2DMATHNEW_fnjstart,G2DMATHNEW_fnjnow,G2DMATHNEW_fnerror,G2DMATHNEW_fngyear,
    G2DMATHNEW_fngmonth,G2DMATHNEW_fngday,G2DMATHNEW_fngdow,G2DMATHNEW_fngleap,
    G2DMATHNEW_fnghour,G2DMATHNEW_fngminute,G2DMATHNEW_fngsecond,
    G2DMATHNEW_fncurseed,G2DMATHNEW_fntimest,G2DMATHNEW_fntimeco,
    G2DMATHNEW_fntimeex,G2DMATHNEW_fntimecl,G2DMATHNEW_fnfrac,G2DMATHNEW_fnerrorl,
    G2DMATHNEW_fnheaps,G2DMATHNEW_fnfact,G2DMATHNEW_fnunfmi,G2DMATHNEW_fnpi,
    G2DMATHNEW_fnncpf,G2DMATHNEW_fnncpcm,G2DMATHNEW_fnentropy,G2DMATHNEW_fnsigmoid,
    G2DMATHNEW_fnlog2,G2DMATHNEW_fnboolnot,G2DMATHNEW_fnbooland,
    G2DMATHNEW_fnboolor,G2DMATHNEW_fnboolxor,G2DMATHNEW_fnboolimp,
    G2DMATHNEW_fnbooleqv,G2DMATHNEW_fnrelopeq,G2DMATHNEW_fnrelopgt,
    G2DMATHNEW_fnrelopge,G2DMATHNEW_fnreloplt,G2DMATHNEW_fnrelople,
    G2DMATHNEW_fnrelopne,G2DMATHNEW_fnifthen,G2DMATHNEW_fnrpower,
    G2DMATHNEW_fnedist,G2DMATHNEW_fndiv,G2DMATHNEW_fndiv0,G2DMATHNEW_fnsllog10,
    G2DMATHNEW_fnsqlog10,G2DMATHNEW_fnslexp,G2DMATHNEW_fnsqexp,G2DMATHNEW_fnslrec,
    G2DMATHNEW_fnsqrec,G2DMATHNEW_fncvpower,G2DMATHNEW_fnvcpower,
    G2DMATHNEW_fncentropy,G2DMATHNEW_fngmillisec,G2DMATHNEW_fnmaxerror,
    G2DMATHNEW_fntimeel,G2DMATHNEW_fngamma,G2DMATHNEW_fnloggamma,G2DMATHNEW_fnbeta,
    G2DMATHNEW_fnlogbeta,G2DMATHNEW_fngammareg,G2DMATHNEW_fnbetareg,
    G2DMATHNEW_fnsinh,G2DMATHNEW_fncosh,G2DMATHNEW_fntanh,G2DMATHNEW_fnmathlastrc,
    G2DMATHNEW_fnmathlastec,G2DMATHNEW_fnmathoval,G2DMATHNEW_fnsignpower,
    G2DMATHNEW_fnhandle,G2DMATHNEW_fnncpvusin,G2DMATHNEW_fnncpvupow,
    G2DMATHNEW_fnbinomial,G2DMATHNEW_fnrehandle,G2DMATHNEW_fngamsver,
    G2DMATHNEW_fndelhandle,G2DMATHNEW_fntan,G2DMATHNEW_fnarccos,
    G2DMATHNEW_fnarcsin,G2DMATHNEW_fnarctan2,G2DMATHNEW_fnsleep,G2DMATHNEW_fnheapf,
    G2DMATHNEW_fncohandle,G2DMATHNEW_fngamsrel,G2DMATHNEW_fnpoly,
    G2DMATHNEW_fnlicensestatus,G2DMATHNEW_fnlicenselevel,G2DMATHNEW_fnheaplimit,
    G2DMATHNEW_fndummy};
  extern _P3SET_127 G2DMATHNEW_cmexonlyfuncs;
  extern _P3SET_127 G2DMATHNEW_cmexreadwrite;
#define G2DMATHNEW_N  6
typedef struct mathNewCommon {
  double functionvalue;
  double gradientvalue[G2DMATHNEW_N];
  double hessianvalue[G2DMATHNEW_N*G2DMATHNEW_N];
  int    exceptcode;
  int    returncode;
  int    oldstyle;
  int    nargs;
  int    derivrequest;
} mathNewCommon_t;

  cnstdef {G2DMATHNEW_am = 6};
  typedef SYSTEM_double G2DMATHNEW_tf;
  typedef SYSTEM_uint8 _sub_0MATHNEW;
  typedef SYSTEM_double G2DMATHNEW_tx[6];
  typedef SYSTEM_uint8 _sub_1MATHNEW;
  typedef SYSTEM_double G2DMATHNEW_tg[6];
  typedef SYSTEM_uint8 _sub_2MATHNEW;
  typedef SYSTEM_uint8 _sub_4MATHNEW;
  typedef SYSTEM_double _arr_3MATHNEW[6];
  typedef _arr_3MATHNEW G2DMATHNEW_th[6];
  extern SYSTEM_double G2DMATHNEW_math_huge;
  extern SYSTEM_double G2DMATHNEW_oo_math_huge;
  extern SYSTEM_double G2DMATHNEW_ln_math_huge;
  extern SYSTEM_double G2DMATHNEW_invgamma_math_huge;
  extern SYSTEM_double G2DMATHNEW_sqrt_math_huge;
  extern SYSTEM_double G2DMATHNEW_oosqrt_math_huge;
  extern SYSTEM_double G2DMATHNEW_ooroot23_math_huge;
  extern SYSTEM_double G2DMATHNEW_cuberoot_math_huge;
  extern SYSTEM_double G2DMATHNEW_oocuberoot_math_huge;
  extern SYSTEM_double G2DMATHNEW_fourthroot_math_huge;
  extern SYSTEM_double G2DMATHNEW_oofourthroot_math_huge;
  typedef SYSTEM_byte G2DMATHNEW_treturncode; /* Anonymous */ enum{G2DMATHNEW_rcok,G2DMATHNEW_rcfunc,G2DMATHNEW_rcgrad,G2DMATHNEW_rchess,
    G2DMATHNEW_rcsystem};
  typedef SYSTEM_byte G2DMATHNEW_texceptcode; /* Anonymous */ enum{G2DMATHNEW_ecok,G2DMATHNEW_ecdomain,G2DMATHNEW_ecsingular,
    G2DMATHNEW_ecoverflow,G2DMATHNEW_ecsigloss};
  typedef _P3STR_7 _arr_5MATHNEW[5];
  extern _arr_5MATHNEW G2DMATHNEW_returnstr;
  typedef _P3STR_15 _arr_6MATHNEW[5];
  extern _arr_6MATHNEW G2DMATHNEW_exceptstr;
/*   extern G2DMATHNEW_tf G2DMATHNEW_functionvalue;  */
/*   extern G2DMATHNEW_tg G2DMATHNEW_gradientvalue;  */
/*   extern G2DMATHNEW_th G2DMATHNEW_hessianvalue;  */
  extern SYSTEM_shortstring G2DMATHNEW_exceptmsg;
/*   extern G2DMATHNEW_texceptcode G2DMATHNEW_exceptcode;  */
/*   extern G2DMATHNEW_treturncode G2DMATHNEW_returncode;  */
/*   extern SYSTEM_integer G2DMATHNEW_oldstyle;  */
/*   extern SYSTEM_integer G2DMATHNEW_derivativerequest;  */
/*   extern SYSTEM_integer G2DMATHNEW_argumentnumbers;  */

Function(SYSTEM_integer ) G2DMATHNEW_funceval(
  SYSTEM_integer *ifunc,
  SYSTEM_double *x);

Function(SYSTEM_char *) G2DMATHNEW_makemsg(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  G2DMATHNEW_treturncode returncode,
  G2DMATHNEW_texceptcode exceptcode,
  SYSTEM_char *msg);

Procedure G2DMATHNEW_cmexstuff(
  G2DMATHNEW_tgamsfunc nr,
  SYSTEM_char *fid,
  SYSTEM_integer *minarg,
  SYSTEM_integer *maxarg,
  SYSTEM_integer *deriv,
  SYSTEM_integer *endogs,
  SYSTEM_integer *equnever);

Function(SYSTEM_integer ) G2DMATHNEW_funcsmooth(
  SYSTEM_integer nr);

Function(SYSTEM_char *) G2DMATHNEW_funcname(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  SYSTEM_integer nr);

Function(SYSTEM_integer ) G2DMATHNEW_funcminargs(
  SYSTEM_integer nr);

Function(SYSTEM_integer ) G2DMATHNEW_funcmaxargs(
  SYSTEM_integer nr);

Function(SYSTEM_integer ) G2DMATHNEW_funcargtype(
  SYSTEM_integer nr,
  SYSTEM_integer i);

Function(SYSTEM_boolean ) G2DMATHNEW_funcequnever(
  SYSTEM_integer nr);

Function(SYSTEM_char *) G2DMATHNEW_functf(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  SYSTEM_integer nr);

Function(SYSTEM_char *) G2DMATHNEW_functs(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  SYSTEM_integer nr);

Function(SYSTEM_double ) G2DMATHNEW_funcgetinf(void);

Procedure G2DMATHNEW_funcsetinf(
  SYSTEM_double inf);

Function(SYSTEM_integer ) G2DMATHNEW_funcinterval(
  SYSTEM_integer *ifunc,
  SYSTEM_double *xmin,
  SYSTEM_double *xmax,
  SYSTEM_double *functionmin,
  SYSTEM_double *functionmax,
  SYSTEM_double *gradientmin,
  SYSTEM_double *gradientmax);
  
Prototype Function(SYSTEM_integer ) (*G2DMATHNEW_tlogerror)(
  SYSTEM_integer retcode,
  SYSTEM_integer exccode,
  SYSTEM_char *msg);


/* extern void _Init_Module_mathnew(void);  */
/* extern void _Final_Module_mathnew(void);  */

#endif
