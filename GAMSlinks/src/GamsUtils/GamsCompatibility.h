// Copyright (C) GAMS Development and others 2011
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsScip.cpp 976 2011-02-06 14:47:08Z stefan $
//
// Author: Stefan Vigerske

#ifndef GAMSCOMPATIBILITY_H_
#define GAMSCOMPATIBILITY_H_

#ifndef GMOAPIVERSION
#error "gmomcc.h need to be included before this file to have GMOAPIVERSION defined"
#endif

/* value for not available/applicable */
#if GMOAPIVERSION >= 7 && GMOAPIVERSION <= 8
#define GMS_SV_NA     gmoValNA(gmo)
#elif GMOAPIVERSION < 7
#define GMS_SV_NA     2.0E300
#endif

#if GMOAPIVERSION < 8
#define Hresused     HresUsed
#define Hdomused     HdomUsed
#define Hobjval      HobjVal
#endif

#if GMOAPIVERSION > 8
#define gmoWantQSet gmoUseQSet
#define gmoEvalObjFunc gmoEvalFuncObj

#define equ_E gmoequ_E
#define equ_G gmoequ_G
#define equ_L gmoequ_L
#define equ_N gmoequ_N
#define equ_X gmoequ_X
#define equ_C gmoequ_C

#define var_X  gmovar_X
#define var_B  gmovar_B
#define var_I  gmovar_I
#define var_S1 gmovar_S1
#define var_S2 gmovar_S2
#define var_SC gmovar_SC
#define var_SI gmovar_SI

#define order_ERR gmoorder_ERR
#define order_L   gmoorder_L
#define order_Q   gmoorder_Q
#define order_NL  gmoorder_NL

#define var_X_F   gmovar_X_F
#define var_X_N   gmovar_X_N
#define var_X_P   gmovar_X_P

#define Bstat_Lower gmoBstat_Lower
#define Bstat_Upper gmoBstat_Upper
#define Bstat_Basic gmoBstat_Basic
#define Bstat_Super gmoBstat_Super

#define Cstat_OK     gmoCstat_OK
#define Cstat_NonOpt gmoCstat_NonOpt
#define Cstat_Infeas gmoCstat_Infeas
#define Cstat_UnBnd  gmoCstat_UnBnd

#define ObjType_Var gmoObjType_Var
#define ObjType_Equ gmoObjType_Equ
#define ObjType_Fun gmoObjType_Fun

#define IFace_Processed gmoIFace_Processed
#define IFace_Raw       gmoIFace_Raw

#define Obj_Min gmoObj_Min
#define Obj_Max gmoObj_Max

#define SolveStat_Normal      gmoSolveStat_Normal
#define SolveStat_Iteration   gmoSolveStat_Iteration
#define SolveStat_Resource    gmoSolveStat_Resource
#define SolveStat_Solver      gmoSolveStat_Solver
#define SolveStat_EvalError   gmoSolveStat_EvalError
#define SolveStat_Capability  gmoSolveStat_Capability
#define SolveStat_License     gmoSolveStat_License
#define SolveStat_User        gmoSolveStat_User
#define SolveStat_SetupErr    gmoSolveStat_SetupErr
#define SolveStat_SolverErr   gmoSolveStat_SolverErr
#define SolveStat_InternalErr gmoSolveStat_InternalErr
#define SolveStat_Skipped     gmoSolveStat_Skipped
#define SolveStat_SystemErr   gmoSolveStat_SystemErr

#define ModelStat_OptimalGlobal        gmoModelStat_OptimalGlobal
#define ModelStat_OptimalLocal         gmoModelStat_OptimalLocal
#define ModelStat_Unbounded            gmoModelStat_Unbounded
#define ModelStat_InfeasibleGlobal     gmoModelStat_InfeasibleGlobal
#define ModelStat_InfeasibleLocal      gmoModelStat_InfeasibleLocal
#define ModelStat_InfeasibleIntermed   gmoModelStat_InfeasibleIntermed
#define ModelStat_NonOptimalIntermed   gmoModelStat_NonOptimalIntermed
#define ModelStat_Integer              gmoModelStat_Integer
#define ModelStat_NonIntegerIntermed   gmoModelStat_NonIntegerIntermed
#define ModelStat_IntegerInfeasible    gmoModelStat_IntegerInfeasible
#define ModelStat_LicenseError         gmoModelStat_LicenseError
#define ModelStat_ErrorUnknown         gmoModelStat_ErrorUnknown
#define ModelStat_ErrorNoSolution      gmoModelStat_ErrorNoSolution
#define ModelStat_NoSolutionReturned   gmoModelStat_NoSolutionReturned
#define ModelStat_SolvedUnique         gmoModelStat_SolvedUnique
#define ModelStat_Solved               gmoModelStat_Solved
#define ModelStat_SolvedSingular       gmoModelStat_SolvedSingular
#define ModelStat_UnboundedNoSolution  gmoModelStat_UnboundedNoSolution
#define ModelStat_InfeasibleNoSolution gmoModelStat_InfeasibleNoSolution

#define Hiterused  gmoHiterused
#define Hresused   gmoHresused
#define Hobjval    gmoHobjval
#define Hdomused   gmoHdomused
#define Hsolution  gmoHsolution
#define Tmipnod    gmoTmipnod
#define Tninf      gmoTninf
#define Tnopt      gmoTnopt
#define Tmipbest   gmoTmipbest
#define Trescalc   gmoTrescalc
#define Tresderiv  gmoTresderiv
#define Tresin     gmoTresin
#define Tresout    gmoTresout
#define Tsinf      gmoTsinf
#define Tforcesave gmoTforcesave
#define Trobj      gmoTrobj

#define EvalRCOK     gmoEvalRCOK
#define EvalRCFUNC   gmoEvalRCFUNC
#define EvalRCGRAD   gmoEvalRCGRAD
#define EvalRCHESS   gmoEvalRCHESS
#define EvalRCSYSTEM gmoEvalRCSYSTEM

#define Proc_none   gmoProc_none
#define Proc_lp     gmoProc_lp
#define Proc_mip    gmoProc_mip
#define Proc_rmip   gmoProc_rmip
#define Proc_nlp    gmoProc_nlp
#define Proc_mcp    gmoProc_mcp
#define Proc_mpec   gmoProc_mpec
#define Proc_rmpec  gmoProc_rmpec
#define Proc_cns    gmoProc_cns
#define Proc_dnlp   gmoProc_dnlp
#define Proc_rminlp gmoProc_rminlp
#define Proc_minlp  gmoProc_minlp
#define Proc_qcp    gmoProc_qcp
#define Proc_miqcp  gmoProc_miqcp
#define Proc_rmiqcp gmoProc_rmiqcp
#define Proc_emp    gmoProc_emp
#endif

#endif /* GAMSCOMPATIBILITY_H_ */
