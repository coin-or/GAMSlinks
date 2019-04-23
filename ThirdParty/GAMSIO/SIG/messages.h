/* messages.h
 * header for output routines
 */

#if ! defined(_MESSAGES_H_)
#     define  _MESSAGES_H_

#include <stdio.h>
#include <stdarg.h>

#define LOGMASK    0x1
#define STATUSMASK 0x2
#define LSTMASK    0x4
#define ALLMASK    LOGMASK|STATUSMASK

int
printMsg  (int mode, char *fmt, ...);

#endif /* ! defined(_MESSAGES_H_) */
