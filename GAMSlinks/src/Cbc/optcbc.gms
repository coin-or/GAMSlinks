$setglobal PDFLINK coin
$eolcom //
set g Cbc Option Groups /
        general        General Options
      /
    e / '-1', 0*200, primal, dual, default, off, on, auto,
        solow_halim, halim_solow, dantzig, steepest, partial, exact, change, sprint, equilibrium, geometric
      /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
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
*      roundingheuristic      rounding heuristic
*      localsearch            local search
*      strongbranching        strong branching
*      integerpresolve        integer presolve
*      findsos                find sos in integer presolve
*      cuts                   switch for cut generation 
*      probing                probing
*      gomorycuts             Gomory cuts
*      knapsackcuts           knapsack cover cuts
*      oddholecuts            odd hole cuts
*      cliquecuts             clique cuts
*      flowcovercuts          flow cover cuts
*      mircuts        		     mixed integer rounding cuts
*      redsplitcuts           reduce and split cuts
*      cutsonlyatroot         whether cuts are only generated at the root node
      startalg               LP solver for root node
      writemps               create MPS file for problem
      tol_dual               dual feasibility tolerance
      tol_primal             primal feasibility tolerance
      tol_integer            tolerance for integrality
      presolve               switch for initial presolve of LP
      printfrequency         print frequency
*      nodecompare            comparision method to determine tree search order
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
*  roundingheuristic   .b.(def 1)
*  localsearch         .b.(def 1)
*  strongbranching     .b.(def 1)
*  integerpresolve     .b.(def 1)
*  findsos             .b.(def 1)
*  cuts                .i.(def 0, lo -1, up 1)
*  probing             .b.(def 1)
*  gomorycuts          .b.(def 1)
*  knapsackcuts        .b.(def 1)
*  oddholecuts         .b.(def 0)
*  cliquecuts          .b.(def 1)
*  flowcovercuts       .b.(def 1)
*  mircuts             .b.(def 1)
*  redsplitcuts        .b.(def 0)
*  cutsonlyatroot      .b.(def 1)
  startalg            .s.(def dual)
  writemps            .s.(def '')
  tol_primal          .r.(def 1e-7)
  tol_dual            .r.(def 1e-7)
  tol_integer         .r.(def 1e-6)
  presolve            .b.(def 1)
  printfrequency      .i.(def 10)
*  nodecompare         .s.(def default)
* GAMS options
  reslim          .r.(def 1000)
  iterlim         .i.(def 10000)
  nodelim         .i.(def maxint)
  nodlim          .i.(def maxint)
  optca           .r.(def 0)
  optcr           .r.(def 0.1)
  cutoff          .r.(def 0, lo mindouble)
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
) /
  oe(o,e) /
   crash.(          off    do not create basis by crash method
                    on     create basis to get dual feasible
                    solow_halim use crash variant due to Solow and Halim
                    halim_solow use crash variant due to Solow and Halim with JJ Forrest modification )
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
   scaling.(        off    no scaling
                    equilibrium equilibrium based scaling method
                    geometric geometric based scaling method
                    auto   automatic choice of scaling method )
*  cuts.(           '-1'   no cuts will be generated
*                   0      automatic
*                   1      cuts from all available cut classes will be generated )
*  localsearch.(    0      local search heuristic is not used
*                   1      local search heuristic is used )
*  strongbranching.(0      strong branching is switched off
*                   1      strong branching is switched on )
*  integerpresolve.(0      don't do integer presolve
*                   1      do integer presolve )
*  findsos.(        0      don't try to find special ordered sets
*                   1      try to find special ordered sets )
*  probing.(        0      don't do probing
*                   1      do probing )
*  gomorycuts.(     0      don't add gomory cuts
*                   1      add gomory cuts )
*  knapsackcuts.(   0      don't add knapsack cover cuts
*                   1      add knapsack cover cuts )
*  oddholecuts.(    0      don't add odd hole cuts
*                   1      add odd hole cuts )
*  cliquecuts.(     0      don't add clique cuts
*                   1      add clique cuts )
*  flowcovercuts.(  0      don't add flow cover cuts
*                   1      add flow cover cuts )
*  mircuts.(        0      don't add mixed integer rounding cuts
*                   1      add mixed integer rounding cuts )
*  redsplitcuts.(   0      don't add reduce and split cuts
*                   1      add reduce and split cuts )
*  cutsonlyatroot.( 0      generate cuts always in the branch and bound
*                   1      generate cuts only at root node )
*  scaling.(        0      don't scale the problem
*                   1      scale the problem )
  presolve.(       0      don't do an initial presolve on the LP
                   1      do an initial presolve on the LP )
  startalg.(       primal  primal simplex algorithm
                   dual    dual simplex algorithm )
*  nodecompare.(    default, depth, objective )
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
                     cutoff     'GAMS cutoff' /
* oep(o) / localsearch, strongbranching, integerpresolve, findsos,
*    probing, gomorycuts, knapsackcuts, oddholecuts, cliquecuts,
*    flowcovercuts, mircuts, redsplitcuts, cutsonlyatroot, scaling, presolve /;
 oep(o) / presolve /;
    