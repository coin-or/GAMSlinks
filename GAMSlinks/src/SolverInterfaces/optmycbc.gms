$setglobal PDFLINK coin
$eolcom //
set g Cbc Option Groups /
        general        General Options
        lpoptions      LP Options
        mipgeneral     MIP Options
        mipcuts        MIP Options for Cutting Plane Generators
        mipheu         MIP Options for Heuristics
*        bch            MIP Options for the GAMS Branch Cut and Heuristic Facility
      /
    e / '-1', 0*100, primal, dual, barrier, default, off, on, auto,
        solow_halim, halim_solow, dantzig, steepest, partial, exact, change, sprint, equilibrium, geometric
        root, ifmove, forceon, forceonbut, forceonstrong, forceonbutstrong, onglobal, longon, longroot, endonly, long, longifmove, forcelongon, longendonly
        priorities, columnorder, binaryfirst, binarylast, length
        hybrid, fewest, depth, upfewest, downfewest, updepth, downdepth
        equal, equalall, sos, trysos
      /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
      reslim                 resource limit
      special                options passed unseen to CBC
      writemps               create MPS file for problem
*LP options
      iterlim                iteration limit
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
      passpresolve           how many passes to do in presolve
      randomseedclp          random seed for CLP
      tol_dual               dual feasibility tolerance
      tol_primal             primal feasibility tolerance
      tol_presolve           tolerance used in presolve
      startalg               LP solver for root node
*MIP options
      threads                number of threads to use (available on Unix variants only)
      strategy               switches on groups of features
      mipstart               whether it should be tried to use the initial variable levels as initial MIP solution
      tol_integer            tolerance for integrality
      sollim                 limit on number of solutions
      dumpsolutions          name of solutions index gdx file for writing alternate solutions
      maxsol                 maximal number of solutions to store during search
      strongbranching        strong branching
      trustpseudocosts       after howmany nodes we trust the pseudo costs
      coststrategy           how to use costs as priorities
      extravariables         group together variables with same cost
      multiplerootpasses     runs multiple copies of the solver at the root node 
      nodestrategy           how to select nodes
      preprocess             integer presolve
      printfrequency         frequency of status prints
      randomseedcbc          random seed for CBC
      loglevel               CBC loglevel
      increment              increment of cutoff when new incumbent
      solvefinal             final solve of MIP with fixed discrete variables
      solvetrace             name of trace file for solving information
      solvetracenodefreq     frequency in number of nodes for writing to solve trace file
      solvetracetimefreq     frequency in seconds for writing to solve trace file
      nodelim                node limit
      nodlim                 node limit
      optca                  absolute stopping tolerance
      optcr                  relative stopping tolerance
      cutoff                 cutoff for objective function value
      cutoffconstraint       whether to add a constraint from the objective function 
*MIP cuts options
      cutdepth               depth in tree at which cuts are applied
      cut_passes_root        number of cut passes at root node
      cut_passes_tree        number of cut passes at nodes in the tree
      cut_passes_slow        number of cut passes for slow cut generators
      cuts                   global switch for cutgenerators
      cliquecuts             Clique Cuts
      flowcovercuts          Flow Cover Cuts
      gomorycuts             Gomory Cuts
      gomorycuts2            Gomory Cuts 2nd implementation
      knapsackcuts           Knapsack Cover Cuts
      liftandprojectcuts     Lift and Project Cuts
      mircuts                Mixed Integer Rounding Cuts
      twomircuts             Two Phase Mixed Integer Rounding Cuts
      probingcuts            Probing Cuts
      reduceandsplitcuts     Reduce and Split Cuts
      reduceandsplitcuts2    Reduce and Split Cuts 2nd implementation
      residualcapacitycuts   Residual Capacity Cuts
      zerohalfcuts           Zero-Half Cuts
*MIP heuristics options
      heuristics             global switch for heuristics
      combinesolutions       combine solutions heuristic
      dins                   distance induced neighborhood search
      divingrandom           turns on random diving heuristic
      divingcoefficient      coefficient diving heuristic
      divingfractional       fractional diving heuristic
      divingguided           guided diving heuristic
      divinglinesearch       line search diving heuristic
      divingpseudocost       pseudo cost diving heuristic
      divingvectorlength     vector length diving heuristic
      feaspump               feasibility pump
      feaspump_passes        number of feasibility passes
      greedyheuristic        greedy heuristic
      localtreesearch        local tree search heuristic
      naiveheuristics        naive heuristics
      pivotandfix            pivot and fix heuristic
      randomizedrounding     randomized rounding heuristis
      rens                   relaxation enforced neighborhood search
      rins                   relaxed induced neighborhood search
      roundingheuristic      rounding heuristic
      vubheuristic           VUB heuristic
      proximitysearch        proximity search heuristic
$ontext
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
$offtext
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
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
)
lpoptions.(
  iterlim              .i.(def 10000)
  idiotcrash           .i.(def -1, lo -1, up 999999)
  sprintcrash          .i.(def -1, lo -1, up 5000000)
  sifting              .i.(def -1, lo -1, up 5000000)
  crash                .s.(def off)
  maxfactor            .i.(def 200, lo 1, up 999999)
  crossover            .b.(def 1)
  dualpivot            .s.(def auto)
  primalpivot          .s.(def auto)
  perturbation         .b.(def 1)
  randomseedclp        .i.(def -1, lo -1)
  scaling              .s.(def auto)
  presolve             .b.(def 1)
  tol_dual             .r.(def 1e-7)
  tol_primal           .r.(def 1e-7)
  tol_presolve         .r.(def 1e-8)
  passpresolve         .i.(def 5, lo -200, up 100)
  startalg             .s.(def dual)
)
mipgeneral.(
  mipstart             .b.(def 0)
  strategy             .i.(def 1, lo 0, up 2)
  tol_integer          .r.(def 1e-6)
  sollim               .i.(def -1, lo -1, up 2147483647)
  dumpsolutions        .s.(def '')
  maxsol               .i.(def 100)
  strongbranching      .i.(def 5, lo 0, up 999999)
  trustpseudocosts     .i.(def 5, lo -1, up 2000000)
  coststrategy         .s.(def off)
  extravariables       .i.(def 0)
  multiplerootpasses   .i.(def 0, up 100000000)
  nodestrategy         .s.(def fewest)
  preprocess           .s.(def on)
  threads              .i.(def 1, lo 0)
  printfrequency       .i.(def 0)
  randomseedcbc        .i.(def -1, lo -1)
  loglevel             .i.(def 1)
  increment            .r.(def 0)
  nodelim              .i.(def maxint)
  nodlim               .i.(def maxint)
  optca                .r.(def 0)
  optcr                .r.(def 0.1)
  cutoff               .r.(def 0, lo mindouble)
  cutoffconstraint     .b.(def 0)
  solvefinal           .b.(def 1)
  solvetrace           .s.(def '')
  solvetracenodefreq   .i.(def 100)
  solvetracetimefreq   .r.(def 5)
)
mipcuts.(
  cutdepth             .i.(def -1, lo -1, up 999999)
  cut_passes_root      .i.(def -1, lo -9999999, up 9999999)
  cut_passes_tree      .i.(def 1, lo -9999999, up 9999999)
  cut_passes_slow      .i.(def 10, lo -1)
  cuts                 .s.(def on)
  cliquecuts           .s.(def ifmove)
  flowcovercuts        .s.(def ifmove)
  gomorycuts           .s.(def ifmove)
  gomorycuts2          .s.(def off)
  knapsackcuts         .s.(def ifmove)
  liftandprojectcuts   .s.(def off)
  mircuts        		   .s.(def ifmove)
  twomircuts           .s.(def root)
  probingcuts          .s.(def ifmove)
  reduceandsplitcuts   .s.(def off)
  reduceandsplitcuts2  .s.(def off)
  residualcapacitycuts .s.(def off)
  zerohalfcuts         .s.(def off)
)
mipheu.(
  heuristics           .b.(def 1)
  combinesolutions     .b.(def 1)
  dins                 .b.(def 0)
  divingrandom         .b.(def 0)
  divingcoefficient    .b.(def 1)
  divingfractional     .b.(def 0)
  divingguided         .b.(def 0)
  divinglinesearch     .b.(def 0)
  divingpseudocost     .b.(def 0)
  divingvectorlength   .b.(def 0)
  feaspump             .b.(def 1)
  feaspump_passes      .i.(def 20, lo 0, up 10000)
  greedyheuristic      .s.(def on)
  localtreesearch      .b.(def 0)
  naiveheuristics      .b.(def 0)
  pivotandfix          .b.(def 0)
  randomizedrounding   .b.(def 0)
  rens                 .b.(def 0)
  rins                 .b.(def 0)
  roundingheuristic    .b.(def 1)
  vubheuristic         .i.(lo -2, up 20)
  proximitysearch      .i.(def 0)
)
$ontext
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
$offtext
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
   strategy.(       0, 1, 2 )
   sollim.(         '-1')
   cutdepth.(       '-1')
   cuts.(           off, on, root, ifmove, forceon )
*   cliquecuts.(     off, on, root, ifmove, forceon )
*   flowcovercuts.(  off, on, root, ifmove, forceon )
*   gomorycuts.(     off, on, root, ifmove, forceon )
   gomorycuts2.(     off, on, root, ifmove, forceon, endonly, long, longroot, longifmove, forcelongon, longendonly )
*   knapsackcuts.(   off, on, root, ifmove, forceon )
*   liftandprojectcuts.( off, on, root, ifmove, forceon )
*   mircuts.(        off, on, root, ifmove, forceon )
*   twomircuts.(     off, on, root, ifmove, forceon )
   probingcuts.(    off, on, root, ifmove, forceon, forceonbut, forceonstrong, forceonbutstrong )
*   probingcuts.(    forceonbut, forceonstrong, forceonbutstrong )
*   reduceandsplitcuts.( off, on, root, ifmove, forceon )
   reduceandsplitcuts2.( off, on, root, longon, longroot )
*   residualcapacitycuts.( off, on, root, ifmove, forceon )
   zerohalfcuts.(    off, on, root, ifmove, forceon, onglobal )
   heuristics.(     0, 1 )
   combinesolutions.( 0, 1 )
   cutoffconstraint.( 0, 1 )
   dins.(           0, 1 )
   divingrandom.(   0, 1 )
   divingcoefficient.(0, 1 )
   divingfractional.(0, 1 )
   divingguided.(   0, 1 )
   divinglinesearch.( 0, 1 )
   divingpseudocost.( 0, 1 )
   divingvectorlength.( 0, 1 )
   feaspump.(       0, 1 )
   greedyheuristic.( off, on, root )
   localtreesearch.( 0, 1 )
   naiveheuristics.( 0, 1 )
   pivotandfix.(     0, 1 )
   randomizedrounding.( 0, 1 )
   rens.(           0, 1 )
   rins.(           0, 1 )
   roundingheuristic.( 0, 1 )
   proximitysearch.(0, 1)
   coststrategy.(   off, priorities, columnorder, binaryfirst, binarylast, length )
   nodestrategy.(   hybrid, fewest, depth, upfewest, downfewest, updepth, downdepth )
   preprocess.(     off, on, equal, equalall, sos, trysos )
   printfrequency.(  0 )
   solvefinal.(      0, 1)
*   usercutnewint.(   0, 1)
*   userheurnewint.(   0, 1)
 /

  publicoe(o,e) /
   idiotcrash.(     '-1'          Automatic
                      0           Off )
   sprintcrash.(    '-1'          Automatic
                      0           Off )
   crash.(          off           Off
                    on            On
                    solow_halim   SolowHalim
                    halim_solow   HalimSolow )
   dualpivot.(      auto          Automatic
                    dantzig       Dantzig
                    steepest      Steepest
                    partial       Partial )
   primalpivot.(    auto          Automatic
                    exact         Exact
                    dantzig       Dantzig
                    partial       Partial
                    steepest      Steepest
                    change        Change )
   scaling.(        off           Off
                    equilibrium   Equilibrium
                    geometric     Geometric
                    auto          Automatic )
   startalg.(       primal        Primal
                    dual          Dual
                    barrier       Barrier )
   strategy.(         0           EasyProblem
                      1           Default
                      2           DifficultProblem )
   sollim.(         '-1'          NoLimit )
   cutdepth.(       '-1'          Off )
   cuts.(           off           Off
                    on            On
                    root          Root
                    ifmove        IfMove
                    forceon       OnAll )
   probingcuts.(    off           Off
                    on            On
                    root          Root
                    ifmove        IfMove
                    forceon       ForceOn
                    forceonbut    ForceOnBut
                    forceonstrong ForceOnStrong
                    forceonbutstrong  ForceOnButAndStrong )
   reduceandsplitcuts2.( off      Off
                    on            On
                    root          Root
                    longon        LongOn
                    longroot      LongRoot )
   zerohalfcuts.(   off           Off
                    on            On
                    root          Root
                    ifmove        IfMove
                    forceon       OnAll
                    onglobal      OnGlobal )
   greedyheuristic.(off           Off
                    on            On
                    root          OnlyForRoot )
   coststrategy.(   off           Off
                    priorities    Priorities
                    columnorder   Columnorder
                    binaryfirst   BinaryFirst
                    binarylast    BinaryLast
                    length        Length )
   nodestrategy.(   hybrid        Hybrid
                    fewest        Fewest
                    depth         Depth
                    upfewest      UpFewest
                    downfewest    DownFewest
                    updepth       Updepth
                    downdepth     DownDepth )
   preprocess.(     off           Off
                    on            On
                    equal         Equal
                    equalall      EqualAll
                    sos           SOS
                    trysos        TrySOS )
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
                     threads    'GAMS threads'
                     cut_passes_root '100 passes if the MIP has less than 500 columns, 100 passes (but stop if the drop in the objective function value is small) if it has less than 5000 columns, and 20 passes otherwise.'
                   /
 oep(o) / mipstart, crossover, perturbation, presolve, printfrequency,
    heuristics, combinesolutions, dins, divingrandom, divingcoefficient, divingfractional, divingguided, divinglinesearch, divingpseudocost, divingvectorlength,
    feaspump, localtreesearch, naiveheuristics, pivotandfix, randomizedrounding, rens, rins, roundingheuristic, solvefinal, cutoffconstraint
*    ,usercutnewint, userheurnewint
    /;
