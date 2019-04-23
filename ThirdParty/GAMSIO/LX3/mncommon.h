/* mncommon.h
 * Declarations useful for using mathnew functions.
 *
 * This stuff is not global to mathnew,
 * merely useful in making calls to the mathnew functions.
 */

#if defined(functionValue)
# undef functionValue
#endif
#define functionValue   G2DMATHNEW_functionvalue

#if defined(gradientValue)
# undef gradientValue
#endif
#define gradientValue   G2DMATHNEW_gradientvalue

#if defined(hessianValue)
# undef hessianValue
#endif
#define hessianValue    G2DMATHNEW_hessianvalue

#if defined(nArgs)
# undef nArgs
#endif
#define nArgs           G2DMATHNEW_argumentnumbers

#if defined(derivRequest)
# undef derivRequest
#endif
#define derivRequest    G2DMATHNEW_derivativerequest

int mnRC;
double mnX[G2DMATHNEW_N];
