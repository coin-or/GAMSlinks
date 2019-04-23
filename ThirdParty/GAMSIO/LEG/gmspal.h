#if ! defined(_GMSPAL_H_)
#define       _GMSPAL_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define auditSetLine          X_auditsetline
#define auditSetLineLib       X_auditsetlinelib
#define auditGetLine          X_auditgetline
#define licenseRegisterGAMS   X_licenseregistergams
#define licenseRegisterSystem X_licenseregistersystem
#define licenseCheck          X_licensecheck
#define licenseGetMessage     X_licensegetmessage
#define licenseCheckSubSys    X_licensechecksubsys
#define licenseQueryOption    X_licensequeryoption

#define GMSPALPOFFSET           17
#define GMSPALDEMOMAXROW       300
#define GMSPALDEMOMAXCOL       300
#define GMSPALDEMOMAXNZ       2000
#define GMSPALDEMOMAXNLNZ     1000
#define GMSPALDEMOMAXDISC       50
#define GMSPALDEMOGLOBALMAXROW  10
#define GMSPALDEMOGLOBALMAXCOL  10

/* Audit stuff */
void auditSetLine            (char *auditline);
void auditSetLineLib         (char *auditline);
char *auditGetLine           (char *auditline, int len_auditline);

/* License stuff */
void licenseRegisterGAMS     (int linenr, char *liceline);
void licenseRegisterSystem   (int subsysnum, char *subsyscode, int checksum, int demotype);
int  licenseCheck            (int m, int n, int nz, int nlnz, int ndisc);
int  licenseGetMessage       (char *msg, int len_msg);
int  licenseCheckSubSys      (int subsysnum, char *subsyscode);
int  licenseQueryOption      (char *slv, char *opt, int *ival);

#if defined(__cplusplus)
}
#endif

#endif /* if ! defined(_GMSPAL_H_) */
