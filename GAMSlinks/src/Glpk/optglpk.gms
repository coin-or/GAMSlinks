$setglobal PDFLINK coin
$eolcom //
set g Glpk Option Groups /
        general        General Options
      /
    e / '-1', 0*100,
        primal, dual,
        off, equilibrium, mean, meanequilibrium,
        textbook, steepestedge,
        depthfirst, breadthfirst, bestprojection /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
      cuts                   generation of cuts for root problem
      gomorycuts             Gomorys mixed integer cuts
      cliquecuts             Clique cuts
      covercuts              Mixed cover cuts
      writemps               create MPS file for problem
      startalg               LP solver for root node
      scaling                scaling method
      pricing                pricing method
      tol_dual               dual feasibility tolerance
      tol_primal             primal feasibility tolerance
      tol_integer            integer feasibility tolerance
      backtracking           backtracking heuristic
      reslim_fixedrun        resource limit for solve with fixed discrete variables
* GAMS options
      reslim                 resource limit
      iterlim                iteration limit
      optcr                  relative stopping tolerance
*      cutoff                 Cutoff for objective function value
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
    optdata(g,o,t,f) /
general.(
  cuts            .i.(def -1, lo -1, up 1)
  gomorycuts      .b.(def 1)
  cliquecuts      .b.(def 1)
  covercuts       .b.(def 1)
  writemps        .s.(def '')
  startalg        .s.(def primal)
  scaling         .s.(def equilibrium)
  pricing         .s.(def textbook)
  tol_dual        .r.(def 1e-7)
  tol_primal      .r.(def 1e-7)
  tol_integer     .r.(def 1e-5)
  backtracking    .s.(def bestprojection)
  reslim_fixedrun .r.(def 1000)
* GAMS options
  reslim          .r.(def 1000)
  iterlim         .i.(def 10000, lo -1)
  optcr           .r.(def 0.1)
*  cutoff          .r.(def 0, lo mindouble)
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
) /
$onempty
  oe(o,e) /
  cuts.(          '-1'   no cuts will be generated
                  0      automatic
                  1      cuts from all available cut classes will be generated )
  startalg.(      primal           use the primal simplex algorithm for the root node
                  dual             use the dual simplex algorithm for the root node )
  scaling.(       off              no scaling
                  equilibrium      equilibrium scaling 
                  mean             geometric mean scaling
                  meanequilibrium  geometric mean scaling then equilibrium scaling )
  pricing.(       textbook         textbook pricing
                  steepestedge     steepest edge pricing )
  backtracking.(  depthfirst       depth first search
                  breadthfirst     breadth first search
                  bestprojection   using best projection heuristic )
 /
$offempty
 im  immediates recognized  / EolFlag , ReadFile, Message, NoBounds /
 immediate(o,im)   / NoBounds.NoBounds, ReadFile.ReadFile /
 hidden(o)         / NoBounds, ReadFile /
 odefault(o)       / reslim     'GAMS reslim'
                     iterlim    'GAMS iterlim' 
                     optcr      'GAMS optcr'
*                     cutoff     'GAMS cutoff'
                   /
$onempty
 oep(o) enum options for documentation only / gomorycuts, cliquecuts, covercuts /;
$offempty
