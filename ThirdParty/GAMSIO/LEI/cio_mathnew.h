
#if !defined(_P3___mathnew___H)
#define _P3___mathnew___H

  typedef SYSTEM_byte CIOMATHNEW_tgamsfunc; /* Anonymous */ enum{CIOMATHNEW_fnmapval,CIOMATHNEW_fnceil,CIOMATHNEW_fnfloor,CIOMATHNEW_fnround,
    CIOMATHNEW_fnmod,CIOMATHNEW_fntrunc,CIOMATHNEW_fnsign,CIOMATHNEW_fnmin,
    CIOMATHNEW_fnmax,CIOMATHNEW_fnsqr,CIOMATHNEW_fnexp,CIOMATHNEW_fnlog,
    CIOMATHNEW_fnlog10,CIOMATHNEW_fnsqrt,CIOMATHNEW_fnabs,CIOMATHNEW_fncos,
    CIOMATHNEW_fnsin,CIOMATHNEW_fnarctan,CIOMATHNEW_fnerrf,CIOMATHNEW_fndunfm,
    CIOMATHNEW_fndnorm,CIOMATHNEW_fnpower,CIOMATHNEW_fnjdate,CIOMATHNEW_fnjtime,
    CIOMATHNEW_fnjstart,CIOMATHNEW_fnjnow,CIOMATHNEW_fnerror,CIOMATHNEW_fngyear,
    CIOMATHNEW_fngmonth,CIOMATHNEW_fngday,CIOMATHNEW_fngdow,CIOMATHNEW_fngleap,
    CIOMATHNEW_fnghour,CIOMATHNEW_fngminute,CIOMATHNEW_fngsecond,
    CIOMATHNEW_fncurseed,CIOMATHNEW_fntimest,CIOMATHNEW_fntimeco,
    CIOMATHNEW_fntimeex,CIOMATHNEW_fntimecl,CIOMATHNEW_fnfrac,CIOMATHNEW_fnerrorl,
    CIOMATHNEW_fnheaps,CIOMATHNEW_fnfact,CIOMATHNEW_fnunfmi,CIOMATHNEW_fnpi,
    CIOMATHNEW_fnncpf,CIOMATHNEW_fnncpcm,CIOMATHNEW_fnentropy,CIOMATHNEW_fnsigmoid,
    CIOMATHNEW_fnlog2,CIOMATHNEW_fnboolnot,CIOMATHNEW_fnbooland,
    CIOMATHNEW_fnboolor,CIOMATHNEW_fnboolxor,CIOMATHNEW_fnboolimp,
    CIOMATHNEW_fnbooleqv,CIOMATHNEW_fnrelopeq,CIOMATHNEW_fnrelopgt,
    CIOMATHNEW_fnrelopge,CIOMATHNEW_fnreloplt,CIOMATHNEW_fnrelople,
    CIOMATHNEW_fnrelopne,CIOMATHNEW_fnifthen,CIOMATHNEW_fnrpower,
    CIOMATHNEW_fnedist,CIOMATHNEW_fndiv,CIOMATHNEW_fndiv0,CIOMATHNEW_fnsllog10,
    CIOMATHNEW_fnsqlog10,CIOMATHNEW_fnslexp,CIOMATHNEW_fnsqexp,CIOMATHNEW_fnslrec,
    CIOMATHNEW_fnsqrec,CIOMATHNEW_fncvpower,CIOMATHNEW_fnvcpower,
    CIOMATHNEW_fncentropy,CIOMATHNEW_fngmillisec,CIOMATHNEW_fnmaxerror,
    CIOMATHNEW_fntimeel,CIOMATHNEW_fngamma,CIOMATHNEW_fnloggamma,CIOMATHNEW_fnbeta,
    CIOMATHNEW_fnlogbeta,CIOMATHNEW_fngammareg,CIOMATHNEW_fnbetareg,
    CIOMATHNEW_fnsinh,CIOMATHNEW_fncosh,CIOMATHNEW_fntanh,CIOMATHNEW_fnmathlastrc,
    CIOMATHNEW_fnmathlastec,CIOMATHNEW_fnmathoval,CIOMATHNEW_fnsignpower,
    CIOMATHNEW_fnhandle,CIOMATHNEW_fnncpvusin,CIOMATHNEW_fnncpvupow,
    CIOMATHNEW_fnbinomial,CIOMATHNEW_fnrehandle,CIOMATHNEW_fngamsver,
    CIOMATHNEW_fndelhandle,CIOMATHNEW_fntan,CIOMATHNEW_fnarccos,
    CIOMATHNEW_fnarcsin,CIOMATHNEW_fnarctan2,CIOMATHNEW_fnsleep,CIOMATHNEW_fnheapf,
    CIOMATHNEW_fncohandle,CIOMATHNEW_fngamsrel,CIOMATHNEW_fnpoly,
    CIOMATHNEW_fnlicensestatus,CIOMATHNEW_fnlicenselevel,CIOMATHNEW_fnheaplimit,
    CIOMATHNEW_fndummy};
  extern _P3SET_127 CIOMATHNEW_cmexonlyfuncs;
  extern _P3SET_127 CIOMATHNEW_cmexreadwrite;
#define CIOMATHNEW_N  6
typedef struct mathNewCommon {
  double functionvalue;
  double gradientvalue[CIOMATHNEW_N];
  double hessianvalue[CIOMATHNEW_N*CIOMATHNEW_N];
  int    exceptcode;
  int    returncode;
  int    oldstyle;
  int    nargs;
  int    derivrequest;
} mathNewCommon_t;

  cnstdef {CIOMATHNEW_am = 6};
  typedef SYSTEM_double CIOMATHNEW_tf;
  typedef SYSTEM_uint8 _sub_0MATHNEW;
  typedef SYSTEM_double CIOMATHNEW_tx[6];
  typedef SYSTEM_uint8 _sub_1MATHNEW;
  typedef SYSTEM_double CIOMATHNEW_tg[6];
  typedef SYSTEM_uint8 _sub_2MATHNEW;
  typedef SYSTEM_uint8 _sub_4MATHNEW;
  typedef SYSTEM_double _arr_3MATHNEW[6];
  typedef _arr_3MATHNEW CIOMATHNEW_th[6];
  extern SYSTEM_double CIOMATHNEW_math_huge;
  extern SYSTEM_double CIOMATHNEW_oo_math_huge;
  extern SYSTEM_double CIOMATHNEW_ln_math_huge;
  extern SYSTEM_double CIOMATHNEW_invgamma_math_huge;
  extern SYSTEM_double CIOMATHNEW_sqrt_math_huge;
  extern SYSTEM_double CIOMATHNEW_oosqrt_math_huge;
  extern SYSTEM_double CIOMATHNEW_ooroot23_math_huge;
  extern SYSTEM_double CIOMATHNEW_cuberoot_math_huge;
  extern SYSTEM_double CIOMATHNEW_oocuberoot_math_huge;
  extern SYSTEM_double CIOMATHNEW_fourthroot_math_huge;
  extern SYSTEM_double CIOMATHNEW_oofourthroot_math_huge;
  typedef SYSTEM_byte CIOMATHNEW_treturncode; /* Anonymous */ enum{CIOMATHNEW_rcok,CIOMATHNEW_rcfunc,CIOMATHNEW_rcgrad,CIOMATHNEW_rchess,
    CIOMATHNEW_rcsystem};
  typedef SYSTEM_byte CIOMATHNEW_texceptcode; /* Anonymous */ enum{CIOMATHNEW_ecok,CIOMATHNEW_ecdomain,CIOMATHNEW_ecsingular,
    CIOMATHNEW_ecoverflow,CIOMATHNEW_ecsigloss};
  typedef _P3STR_7 _arr_5MATHNEW[5];
  extern _arr_5MATHNEW CIOMATHNEW_returnstr;
  typedef _P3STR_15 _arr_6MATHNEW[5];
  extern _arr_6MATHNEW CIOMATHNEW_exceptstr;
/*   extern CIOMATHNEW_tf CIOMATHNEW_functionvalue;  */
/*   extern CIOMATHNEW_tg CIOMATHNEW_gradientvalue;  */
/*   extern CIOMATHNEW_th CIOMATHNEW_hessianvalue;  */
  extern SYSTEM_shortstring CIOMATHNEW_exceptmsg;
/*   extern CIOMATHNEW_texceptcode CIOMATHNEW_exceptcode;  */
/*   extern CIOMATHNEW_treturncode CIOMATHNEW_returncode;  */
/*   extern SYSTEM_integer CIOMATHNEW_oldstyle;  */
/*   extern SYSTEM_integer CIOMATHNEW_derivativerequest;  */
/*   extern SYSTEM_integer CIOMATHNEW_argumentnumbers;  */

Function(SYSTEM_integer ) CIOMATHNEW_funceval(
  SYSTEM_integer *ifunc,
  SYSTEM_double *x);

Function(SYSTEM_char *) CIOMATHNEW_makemsg(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  CIOMATHNEW_treturncode returncode,
  CIOMATHNEW_texceptcode exceptcode,
  SYSTEM_char *msg);

Procedure CIOMATHNEW_cmexstuff(
  CIOMATHNEW_tgamsfunc nr,
  SYSTEM_char *fid,
  SYSTEM_integer *minarg,
  SYSTEM_integer *maxarg,
  SYSTEM_integer *deriv,
  SYSTEM_integer *endogs,
  SYSTEM_integer *equnever);

Function(SYSTEM_integer ) CIOMATHNEW_funcsmooth(
  SYSTEM_integer nr);

Function(SYSTEM_char *) CIOMATHNEW_funcname(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  SYSTEM_integer nr);

Function(SYSTEM_integer ) CIOMATHNEW_funcminargs(
  SYSTEM_integer nr);

Function(SYSTEM_integer ) CIOMATHNEW_funcmaxargs(
  SYSTEM_integer nr);

Function(SYSTEM_integer ) CIOMATHNEW_funcargtype(
  SYSTEM_integer nr,
  SYSTEM_integer i);

Function(SYSTEM_boolean ) CIOMATHNEW_funcequnever(
  SYSTEM_integer nr);

Function(SYSTEM_char *) CIOMATHNEW_functf(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  SYSTEM_integer nr);

Function(SYSTEM_char *) CIOMATHNEW_functs(
  SYSTEM_char *result,
  SYSTEM_uint8 _len_ret,
  SYSTEM_integer nr);

Function(SYSTEM_double ) CIOMATHNEW_funcgetinf(void);

Procedure CIOMATHNEW_funcsetinf(
  SYSTEM_double inf);

Function(SYSTEM_integer ) CIOMATHNEW_funcinterval(
  SYSTEM_integer *ifunc,
  SYSTEM_double *xmin,
  SYSTEM_double *xmax,
  SYSTEM_double *functionmin,
  SYSTEM_double *functionmax,
  SYSTEM_double *gradientmin,
  SYSTEM_double *gradientmax);
  
Prototype Function(SYSTEM_integer ) (*CIOMATHNEW_tlogerror)(
  SYSTEM_integer retcode,
  SYSTEM_integer exccode,
  SYSTEM_char *msg);


/* extern void _Init_Module_mathnew(void);  */
/* extern void _Final_Module_mathnew(void);  */

#endif
