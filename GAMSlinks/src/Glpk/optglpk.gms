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
      writemps               create MPS file for problem
      startalg               LP solver for root node
      scaling                scaling method
      pricing                pricing method
      tol_dual               dual feasibility tolerance
      tol_primal             primal feasibility tolerance
      tol_integer            integer feasibility tolerance
      backtracking           backtracking heuristic
      cuts                   generation of cuts for root problem
* GAMS options
      reslim                 resource limit
      iterlim                iteration limit
      optcr                  relative stopping tolerance
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
    optdata(g,o,t,f) /
general.(
  writemps        .s.(def '')
  startalg        .s.(def primal)
  scaling         .s.(def equilibrium)
  pricing         .s.(def textbook)
  tol_dual        .r.(def 1e-7)
  tol_primal      .r.(def 1e-7)
  tol_integer     .r.(def 1e-5)
  backtracking    .s.(def bestprojection)
  cuts            .b.(def 0)
* GAMS options
  reslim          .r.(def 1000)
  iterlim         .i.(def 10000)
  optcr           .r.(def 0.1)
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
) /
$onempty
  oe(o,e) /
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
  cuts.(          0                do not generate cuts
                  1                generate cuts for the initial LP relaxation )
 /
$offempty
 im  immediates recognized  / EolFlag , ReadFile, Message, NoBounds /
 immediate(o,im)   / NoBounds.NoBounds, ReadFile.ReadFile /
 hidden(o)         / NoBounds, ReadFile /
 odefault(o)       / reslim     'GAMS reslim'
                     iterlim    'GAMS iterlim' 
                     optcr      'GAMS optcr' /
$onempty
 oep(o) enum options for documentation only / cuts /;
$offempty
