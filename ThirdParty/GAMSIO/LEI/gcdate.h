/* gcdate.h
 */

#if ! defined(_GCDATE_H_)
#     define  _GCDATE_H_

#  include "gclib.h"

#  if defined(__cplusplus)
extern "C" {
#  endif

int
gclgreg2jul (int day, int month, int year);

void
gclgettime(int *year, int *month, int *dow, int *day,
	   int *hour, int *min, int *sec);

#  if defined(__cplusplus)
}
#  endif

#endif /* ! defined(_GCDATE_H_) */
