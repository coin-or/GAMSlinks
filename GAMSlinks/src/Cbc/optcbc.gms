$setglobal PDFLINK coin
$eolcom //
set g Cbc Option Groups /
        general        General Options
        lpoptions      LP Options
        mipgeneral     MIP Options
        mipcuts        MIP Options for Cutting Plane Generators
        mipheu         MIP Options for Heuristics
        bch            MIP Options for the GAMS Branch Cut and Heuristic Facility
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
      names                  specifies whether variable and equation names should be given to CBC 
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
      mipstart               whether it should be tried to use the initial variable levels as initial MIP solution
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
      usercutcall      The GAMS command line to call the cut generator
      usercutfirst     Calls the cut generator for the first n nodes
      usercutfreq      Determines the frequency of the cut generator model calls
      usercutinterval  Determines the interval when to apply the multiplier for the frequency of the cut generator model calls
      usercutmult      Determines the multiplier for the frequency of the cut generator model calls
      usercutnewint    Calls the cut generator if the solver found a new integer feasible solution
      usergdxin        The name of the GDX file read back into CBC
      usergdxname      The name of the GDX file exported from the solver with the solution at the node
      usergdxnameinc   The name of the GDX file exported from the solver with the incumbent solution
      usergdxprefix   'Prefixes usergdxin, usergdxname, and usergdxnameinc'
      userheurcall     The GAMS command line to call the heuristic
      userheurfirst    Calls the heuristic for the first n nodes
      userheurfreq     Determines the frequency of the heuristic model calls
      userheurinterval Determines the interval when to apply the multiplier for the frequency of the heuristic model calls
      userheurmult     Determines the multiplier for the frequency of the heuristic model calls
      userheurnewint   Calls the heuristic if the solver found a new integer feasible solution
      userheurobjfirst 'Calls the heuristic if the LP value of the node is closer to the best bound than the current incumbent'
*      userincbcall     The GAMS command line to call the incumbent checking program
*      userincbicall    The GAMS command line to call the incumbent reporting program
      userjobid        'Postfixes gdxname, gdxnameinc, and gdxin'
      userkeep         Calls gamskeep instead of gams
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
  names                .b.(def 0)
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
  mipstart             .b.(def 0)
  tol_integer          .r.(def 1e-6)
  sollim               .i.(def -1, lo -1, up 2147483647)
  strongbranching      .i.(def 5, lo 0, up 999999)
  trustpseudocosts     .i.(def 5, lo -1, up 2000000)
  coststrategy         .s.(def off)
  nodestrategy         .s.(def fewest)
  preprocess           .s.(def on)
  printfrequency       .i.(def 0)
  increment            .r.(def 0)
  nodelim              .i.(def maxint)
  nodlim               .i.(def maxint)
  optca                .r.(def 0)
  optcr                .r.(def 0.1)
  cutoff               .r.(def 0, lo mindouble)
)
mipcuts.(
  cutdepth             .i.(def -1, lo -1, up 999999)
  cut_passes_root      .i.(lo -999999, up 999999)
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
bch.(
            usercutcall      .s.(def '')
            usercutfirst     .i.(def 10, lo 0, up maxint)
            usercutfreq      .i.(def 10, lo 0, up maxint)
            usercutinterval  .i.(def 100, lo 0, up maxint)
            usercutmult      .i.(def 2, lo 0, up maxint)
            usercutnewint    .b.(def 0)
            usergdxin        .s.(def 'bchin.gdx')
            usergdxname      .s.(def 'bchout.gdx')
            usergdxnameinc   .s.(def 'bchout_i.gdx')
            usergdxprefix    .s.(def '')
            userheurcall     .s.(def '')
            userheurfirst    .i.(def 10, lo 0, up maxint)
            userheurfreq     .i.(def 10, lo 0, up maxint)
            userheurinterval .i.(def 100, lo 0, up maxint)
            userheurmult     .i.(def 2, lo 0, up maxint)
            userheurnewint   .b.(def 0)
            userheurobjfirst .i.(def 0, lo 0, up maxint)
*            userincbcall     .s.(def '')
*            userincbicall    .s.(def '')
            userkeep         .b.(def 0)
            userjobid        .s.(def '')
       )
/
  oe(o,e) /
   idiotcrash.(     '-1', 0 )
   sprintcrash.(    '-1', 0 )
   crash.(          off    
                    on     
                    solow_halim 
                    halim_solow )
   crossover.(      0, 1 )
   dualpivot.(      auto   
                    dantzig 
                    steepest 
                    partial  )
   primalpivot.(    auto   
                    exact  
                    dantzig 
                    partial 
                    steepest 
                    change 
*JJF: "I don't think sprint option works"; so we do not advertise it
*                    sprint 
                    )
   perturbation.(   0, 1 )
   scaling.(        off    
                    equilibrium 
                    geometric 
                    auto   )
   presolve.(       0, 1 )
   startalg.(       primal  
                    dual    
                    barrier  )
   mipstart.(       0, 1 )
   sollim.(         '-1')
   cutdepth.(       '-1')
   cuts.(           off, on, root, ifmove, forceon )
*   cliquecuts.(     off, on, root, ifmove, forceon )
*   flowcovercuts.(  off, on, root, ifmove, forceon )
*   gomorycuts.(     off, on, root, ifmove, forceon )
*   knapsackcuts.(   off, on, root, ifmove, forceon )
*   liftandprojectcuts.( off, on, root, ifmove, forceon )
*   mircuts.(        off, on, root, ifmove, forceon )
*   twomircuts.(     off, on, root, ifmove, forceon )
   probingcuts.(    off, on, root, ifmove, forceon, forceonbut, forceonstrong, forceonbutstrong )
*   probingcuts.(    forceonbut, forceonstrong, forceonbutstrong )
*   reduceandsplitcuts.( off, on, root, ifmove, forceon )
*   residualcapacitycuts.( off, on, root, ifmove, forceon )
   heuristics.(     0, 1 )
   combinesolutions.( 0, 1 )
   feaspump.(       0, 1 )
   greedyheuristic.( off, on, root )
   localtreesearch.( 0, 1 )
   rins.(           0, 1 )
   roundingheuristic.( 0, 1 )
   coststrategy.(   off, priorities, columnorder, binaryfirst, binarylast, length )
   nodestrategy.(   hybrid, fewest, depth, upfewest, downfewest, updepth, downdepth )
   preprocess.(     off, on, equal, equalall, sos, trysos )
   printfrequency.(  0 )
   names.(           0, 1)
   usercutnewint.(   0, 1)
   userheurnewint.(   0, 1)
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
                     cut_passes_root '100 passes if the MIP has less than 500 columns, 100 passes (but stop if the drop in the objective function value is small) if it has less than 5000 columns, and 20 passes otherwise.'
                   /
 oep(o) / mipstart, crossover, perturbation, presolve, names, printfrequency,
    heuristics, combinesolutions, feaspump, localtreesearch, rins, roundingheuristic,
    usercutnewint, userheurnewint /;
