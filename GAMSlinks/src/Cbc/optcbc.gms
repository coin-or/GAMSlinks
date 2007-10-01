$setglobal PDFLINK coin
$eolcom //
set g Cbc Option Groups /
        general        General Options
        lpoptions      LP Options
        mipgeneral     MIP Options
        mipcuts        MIP Options for Cutting Plane Generators
        mipheu         MIP Options for Heuristics
      /
    e / '-1', 0*100, primal, dual, barrier, default, off, on, auto,
        solow_halim, halim_solow, dantzig, steepest, partial, exact, change, sprint, equilibrium, geometric
        root, ifmove, forceon, forceonbut, forceonstrong, forceonbutstrong
        priorities, columnorder, binaryfirst, binarylast, length
        hybrid, fewest, depth, upfewest, downfewest, updepth, downdepth
        equal, equalall, sos, trysos
      /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
      writemps               create MPS file for problem
      special                options passed unseen to CBC
*LP options
      idiotcrash             idiot crash
      sprintcrash            sprint crash
      sifting                synonym for sprint crash
      crash                  use crash method to get dual feasible
      maxfactor              maximum number of iterations between refactorizations
      crossover              crossover to simplex algorithm after barrier
      dualpivot              dual pivot choice algorithm
      primalpivot            primal pivot choice algorithm
      perturbation           perturbation of problem
      scaling                scaling method
      presolve               switch for initial presolve of LP
      tol_dual               dual feasibility tolerance
      tol_primal             primal feasibility tolerance
      tol_presolve           tolerance used in presolve
      startalg               LP solver for root node
*MIP options
      tol_integer            tolerance for integrality
      sollim                 limit on number of solutions
      strongbranching        strong branching
      trustpseudocosts       after howmany nodes we trust the pseudo costs
      cutdepth               depth in tree at which cuts are applied
      cut_passes_root        number of cut passes at root node
      cut_passes_tree        number of cut passes at nodes in the tree
      cuts                   global switch for cutgenerators
      cliquecuts             Clique Cuts
      flowcovercuts          Flow Cover Cuts
      gomorycuts             Gomory Cuts
      knapsackcuts           Knapsack Cover Cuts
      liftandprojectcuts     Lift and Project Cuts
      mircuts        		     Mixed Integer Rounding Cuts
      twomircuts             Two Phase Mixed Integer Rounding Cuts
      probingcuts            Probing Cuts
      reduceandsplitcuts     Reduce and Split Cuts
      residualcapacitycuts   Residual Capacity Cuts
      heuristics             global switch for heuristics
      combinesolutions       combine solutions heuristic
      feaspump               feasibility pump      
      feaspump_passes        number of feasibility passes
      greedyheuristic        greedy heuristic
      localtreesearch        local tree search heuristic
      rins                   relaxed induced neighborhood search
      roundingheuristic      rounding heuristic
      coststrategy           how to use costs as priorities
      nodestrategy           how to select nodes
      preprocess             integer presolve
      printfrequency         frequency of status prints
      increment              increment of cutoff when new incumbent
* GAMS options
      reslim                 resource limit
      iterlim                iteration limit
      nodelim                node limit
      nodlim                 node limit
      optca                  absolute stopping tolerance
      optcr                  relative stopping tolerance
      cutoff                 cutoff for objective function value
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
optdata(g,o,t,f) /
general.(
  writemps             .s.(def '')
  special              .s.(def '')
* GAMS options
  reslim          .r.(def 1000)
  iterlim         .i.(def 10000)
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
)
lpoptions.(
  idiotcrash           .i.(def -1, lo -1, up 999999)
  sprintcrash          .i.(def -1, lo -1, up 5000000)
  sifting              .i.(def -1, lo -1, up 5000000)
  crash                .s.(def off)
  maxfactor            .i.(def 200, lo 1, up 999999)
  crossover            .b.(def 1)
  dualpivot            .s.(def auto)
  primalpivot          .s.(def auto)
  perturbation         .b.(def 1)
  scaling              .s.(def auto)
  presolve             .b.(def 1)
  tol_dual             .r.(def 1e-7)
  tol_primal           .r.(def 1e-7)
  tol_presolve         .r.(def 1e-8)
  startalg             .s.(def dual)
)
mipgeneral.(
  tol_integer          .r.(def 1e-6)
  sollim               .i.(def -1, lo -1, up 2147483647)
  strongbranching      .i.(def 5, lo 0, up 999999)
  trustpseudocosts     .i.(def 5, lo -1, up 2000000)
  coststrategy         .s.(def off)
  nodestrategy         .s.(def fewest)
  preprocess           .s.(def on)
  printfrequency       .i.(def 0)
  increment            .r.(def 0)
  nodelim         .i.(def maxint)
  nodlim          .i.(def maxint)
  optca           .r.(def 0)
  optcr           .r.(def 0.1)
  cutoff          .r.(def 0, lo mindouble)
)
mipcuts.(
  cutdepth             .i.(def -1, lo -1, up 999999)
  cut_passes_root      .i.(def -1, lo -999999, up 999999)
  cut_passes_tree      .i.(def 1, lo -999999, up 999999)
  cuts                 .s.(def on)
  cliquecuts           .s.(def ifmove)
  flowcovercuts        .s.(def ifmove)
  gomorycuts           .s.(def ifmove)
  knapsackcuts         .s.(def ifmove)
  liftandprojectcuts   .s.(def off)
  mircuts        		   .s.(def ifmove)
  twomircuts           .s.(def root)
  probingcuts          .s.(def ifmove)
  reduceandsplitcuts   .s.(def off)
  residualcapacitycuts .s.(def off)
)
mipheu.(
  heuristics           .b.(def 1)
  combinesolutions     .b.(def 1)
  feaspump             .b.(def 1)
  feaspump_passes      .i.(def 20, lo 0, up 10000)
  greedyheuristic      .s.(def on)
  localtreesearch      .b.(def 0)
  rins                 .b.(def 0)
  roundingheuristic    .b.(def 1)
)
/
  oe(o,e) /
   idiotcrash.(     '-1', 0 )
   sprintcrash.(    '-1', 0 )
   crash.(          off    do not create basis by crash method
                    on     create basis to get dual feasible
                    solow_halim use crash variant due to Solow and Halim
                    halim_solow use crash variant due to Solow and Halim with JJ Forrest modification )
   crossover.(      0, 1 )
   dualpivot.(      auto   Variant of Steepest which decides on each iteration based on factorization
                    dantzig Dantzig method
                    steepest Steepest choice method
                    partial Variant of Steepest which scan only a subset )
   primalpivot.(    auto   Variant of exact devex
                    exact  Exact devex method
                    dantzig Dantzig method
                    partial Variant of exact devex which scan only a subset
                    steepest Steepest choice method
                    change initially does Dantzig until the factorization becomes denser
                    sprint Sprint method )
   perturbation.(   0, 1 )
   scaling.(        off    no scaling
                    equilibrium equilibrium based scaling method
                    geometric geometric based scaling method
                    auto   automatic choice of scaling method )
   presolve.(       0, 1 )
   startalg.(       primal  primal simplex algorithm
                    dual    dual simplex algorithm
                    barrier primal dual predictor corrector algorithm )
   cutdepth.(       '-1')
   cuts.(           off, on, root, ifmove, forceon )
*   cliquecuts.(     off, on, root, ifmove, forceon )
*   flowcovercuts.(  off, on, root, ifmove, forceon )
*   gomorycuts.(     off, on, root, ifmove, forceon )
*   knapsackcuts.(   off, on, root, ifmove, forceon )
*   liftandprojectcuts.( off, on, root, ifmove, forceon )
*   mircuts.(        off, on, root, ifmove, forceon )
*   twomircuts.(     off, on, root, ifmove, forceon )
*   probingcuts.(    off, on, root, ifmove, forceon, forceonbut, forceonstrong, forceonbutstrong )
   probingcuts.(    forceonbut, forceonstrong, forceonbutstrong )
*   reduceandsplitcuts.( off, on, root, ifmove, forceon )
*   residualcapacitycuts.( off, on, root, ifmove, forceon )
   greedyheuristic.( off, on, root )
   coststrategy.(   off, priorities, columnorder, binaryfirst, binarylast, length )
   nodestrategy.(   hybrid, fewest, depth, upfewest, downfewest, updepth, downdepth )
   preprocess.(     off, on, equal, equalall, sos, trysos )
   printfrequency.(  0 )
 /
 im  immediates recognized  / EolFlag , ReadFile, Message, NoBounds /
 immediate(o,im)   / NoBounds.NoBounds, ReadFile.ReadFile /
 hidden(o)         / NoBounds, ReadFile /
 odefault(o)       / reslim     'GAMS reslim'
                     iterlim    'GAMS iterlim' 
                     nodelim    'GAMS nodlim' 
                     nodlim     'GAMS nodlim' 
                     optca      'GAMS optca' 
                     optcr      'GAMS optcr'
                     cutoff     'GAMS cutoff'
                     increment  'GAMS cheat'
                   /
 oep(o) / crossover, perturbation, presolve,
    heuristics, combinesolutions, feaspump, localtreesearch, rins, roundingheuristic /;
