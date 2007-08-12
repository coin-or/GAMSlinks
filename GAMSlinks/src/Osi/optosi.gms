$setglobal PDFLINK coin
$eolcom //
set g Glpk Option Groups /
        general        General Options
      /
    e / 0*100,
        primal, dual, clp, cbc, glpk, volume, dylp, symphony /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
      solver                 which solver we should use
      writemps               create MPS file for problem
      startalg               LP solver for root node or for LP
      scaling                scaling
      tol_dual               dual feasibility tolerance
      tol_primal             primal feasibility tolerance
      presolve               LP presolver
* GAMS options
*      reslim                 resource limit
      iterlim                iteration limit
*      optcr                  relative stopping tolerance
*      cutoff                 Cutoff for objective function value
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
    optdata(g,o,t,f) /
general.(
  solver          .s.(def '')
  writemps        .s.(def '')
  startalg        .s.(def primal)
  scaling         .b.(def 1)
  tol_dual        .r.(def 1e-7)
  tol_primal      .r.(def 1e-7)
  presolve        .b.(def 1)
* GAMS options
*  reslim          .r.(def 1000)
  iterlim         .i.(def 10000)
*  optcr           .r.(def 0.1)
*  cutoff          .r.(def 0, lo mindouble)
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
) /
$onempty
  oe(o,e) /
  startalg.(      primal           use the primal simplex algorithm for the root node
                  dual             use the dual simplex algorithm for the root node )
 /
$offempty
 im  immediates recognized  / EolFlag , ReadFile, Message, NoBounds /
 immediate(o,im)   / NoBounds.NoBounds, ReadFile.ReadFile /
 hidden(o)         / NoBounds, ReadFile /
 odefault(o)       / iterlim    'GAMS iterlim' 
*                     reslim     'GAMS reslim'
*                     optcr      'GAMS optcr'
*                     cutoff     'GAMS cutoff'
                   /
$onempty
 oep(o) enum options for documentation only / /;
$offempty
