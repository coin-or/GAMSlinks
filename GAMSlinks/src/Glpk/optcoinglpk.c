#include <stdio.h>
#include "optcc.h"
#include "optcoinglpk.h"

int optDefined(optHandle_t oh, const char *optname) {
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType;

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    return isDefined;
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s", optname);
    return 0;
  }
}

int optDRecent(optHandle_t oh, const char *optname) {
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType;

  if (optFindStr (oh, optname, &i, &refNum)) {
    optGetInfoNr (oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    return isDefinedRecent;
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s", optname);
    return 0;
  }
}

int optGetStrI(optHandle_t oh, const char *optname) {
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char sval[255], oname[255];

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    if (dataType != optDataInteger) {
      fprintf(stderr,"*** Internal Error. Option %s is not an integer (is %d)\n", optname, dataType);
      return 0;
    }
    optGetValuesNr(oh, i, oname, &ival, &dval, sval);
    return ival;
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s\n", optname);
    return 0;
  }
}

double optGetStrD(optHandle_t oh, const char *optname) {
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char sval[255], oname[255];

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    if (dataType != optDataDouble) {
      fprintf(stderr,"*** Internal Error. Option %s is not a double (is %d)\n", optname, dataType);
      return 0.0;
    }
    optGetValuesNr(oh, i, oname, &ival, &dval, sval);
    return dval;
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s\n", optname);
    return 0.0;
  }
}

char *optGetStrS(optHandle_t oh, const char *optname, char *sval) {
  int i, refNum, isDefined, isDefinedRecent, dataType, optType, subType, ival;
  double dval;
  char oname[255];

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    if (dataType != optDataString) {
      fprintf(stderr,"*** Internal Error. Option %s is not a string (is %d)\n", optname, dataType);
      return sval;
    }
    optGetValuesNr(oh, i, oname, &ival, &dval, sval);
    return sval;
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s\n", optname);
    return sval;
  }
}

void optSetStrB(optHandle_t oh, const char *optname, int ival) {
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    if (dataType != optDataInteger) {
      fprintf(stderr,"*** Internal Error. Option %s is not an integer (is %d)\n", optname, dataType);
      return;
    }
    /* must knock nonzero values to 1, or optobj complains */
    optSetValuesNr(oh, i, ival ? 1 : 0, 0.0, "");
    for (i=1; i<=optMessageCount(oh); i++) {
      optGetMessage(oh, i, stmp, &k);
      if (k==optMsgUserError || k<optMsgFileEnter) fprintf(stderr,"%d: %s\n", k, stmp);
    }
    optClearMessages(oh);
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s\n", optname);
  }
}

void optSetStrI(optHandle_t oh, const char *optname, int ival) {
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    if (dataType != optDataInteger) {
      fprintf(stderr,"*** Internal Error. Option %s is not an integer (is %d)\n", optname, dataType);
      return;
    }
    optSetValuesNr(oh, i, ival, 0.0, "");
    for (i=1; i<=optMessageCount(oh); i++) {
      optGetMessage(oh, i, stmp, &k);
      if (k==optMsgUserError || k<optMsgFileEnter) fprintf(stderr,"%d: %s\n", k, stmp);
    }
    optClearMessages(oh);
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s\n", optname);
  }
}

void optSetStrD(optHandle_t oh, const char *optname, double dval) {
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    if (dataType != optDataDouble) {
      fprintf(stderr,"*** Internal Error. Option %s is not a double (is %d)\n", optname, dataType);
      return;
    }
    optSetValuesNr(oh, i, 0, dval, "");
    for (i=1; i<=optMessageCount(oh); i++) {
      optGetMessage(oh, i, stmp, &k);
      if (k==optMsgUserError || k<optMsgFileEnter) fprintf(stderr,"%d: %s\n", k, stmp);
    }
    optClearMessages(oh);
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s\n", optname);
  }
}

void optSetStrS(optHandle_t oh, const char *optname, char *sval) {
  int i, k, refNum, isDefined, isDefinedRecent, dataType, optType, subType;
  char stmp[255];

  if (optFindStr(oh, optname, &i, &refNum)) {
    optGetInfoNr(oh, i, &isDefined, &isDefinedRecent, &refNum,
                 &dataType, &optType, &subType);
    if (dataType != optDataString) {
      fprintf(stderr,"*** Internal Error. Option %s is not a string (is %d)\n", optname, dataType);
      return;
    }
    optSetValuesNr(oh, i, 0, 0.0, sval);
    for (i=1; i<=optMessageCount(oh); i++) {
      optGetMessage(oh, i, stmp, &k);
      if (k==optMsgUserError || k<optMsgFileEnter) fprintf(stderr,"%d: %s\n", k, stmp);
    }
    optClearMessages(oh);
  }
  else {
    fprintf(stderr,"*** Internal Error. Unknown option %s\n", optname);
  }
}
