$setglobal PDFLINK none
$eolcom //
set g Cbc Option Groups /
        general        General Options
      /
    e / '-1', 0*100, '+n', primal, dual, barrier /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
      cuts                   switch for cut generation 
      roundingheuristic      rounding heuristic
      localsearch            local search
      strongbranching        strong branching
      integerpresolve        integer presolve
      findsos                find sos
      probing                probing cuts
      gomorycuts             Gomory cuts
      knapsackcuts           knapsack cuts
      oddholecuts            odd hole cuts
      cliquecuts             clique cuts
      flowcovercuts          flow cover cuts
      mircuts        		     mixed integer rounding cuts
      redsplitcuts           reduce and split cuts
      cutsonlyatroot         whether cuts are only generated at the root node
      startalg               LP solver for root node
      writemps               create MPS file for problem
* GAMS options
      reslim                 resource limit
      iterlim                iteration limit
      nodelim                node limit
      nodlim                 node limit
      optca                  absolute stopping tolerance
      cutoff                 cutoff for tree search
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
    optdata(g,o,t,f) /
general.(
  cuts                .i.(def 0, lo -1, up 1)
  roundingheuristic   .b.(def 1)
  localsearch         .b.(def 1)
  strongbranching     .b.(def 1)
  integerpresolve     .b.(def 1)
  findsos             .b.(def 1)
  probing             .b.(def 1)
  gomorycuts          .b.(def 1)
  knapsackcuts        .b.(def 1)
  oddholecuts         .b.(def 0)
  cliquecuts          .b.(def 1)
  flowcovercuts       .b.(def 1)
  mircuts             .b.(def 1)
  redsplitcuts        .b.(def 0)
  cutsonlyatroot      .b.(def 1)
  startalg            .s.(def primal)
  writemps            .s.(def '')
* GAMS options
  reslim          .r.(def 1000)
  iterlim         .i.(def 10000)
  nodelim         .i.(def maxint)
  nodlim          .i.(def maxint)
  optca           .r.(def 0)
  cutoff          .r.(def 0, lo mindouble)
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
) /
  oe(o,e) /
  cuts.(         '-1'     no cuts will be generated
                   0      automatic
                   1      cuts from all available cut classes will be generated )
  startalg.(      primal  primal simplex algorithm
                  dual    dual simplex algorithm )
 /
 im  immediates recognized  / EolFlag , ReadFile, Message, NoBounds /
 immediate(o,im)   / NoBounds.NoBounds, ReadFile.ReadFile /
 hidden(o)         / NoBounds, ReadFile /
 odefault(o)       / reslim     'GAMS reslim'
                     iterlim    'GAMS iterlim' 
                     nodelim    'GAMS nodlim' 
                     nodlim     'GAMS nodlim' 
                     optca      'GAMS optca' 
                     cutoff     'GAMS cutoff' /
