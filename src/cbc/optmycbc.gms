$setglobal SEPARATOR " "
$setglobal STRINGQUOTE ""
$onempty
set e / 
  '0'
  '01first'
  '01last'
  '1'
  '10'
  '100'
  '1000'
  '10000'
  '2'
  '200'
  '3'
  '300'
  '4'
  '5'
  '6'
  '7'
  '8'
  '9'
  'aggregate'
  'auto'
  'automatic'
  'barrier'
  'before'
  'beforequick'
  'binaryfirst'
  'binarylast'
  'both'
  'bothaswell'
  'bothaswellroot'
  'bothinstead'
  'bothquick'
  'change'
  'cleanaswell'
  'cleanaswellroot'
  'cleaninstead'
  'columnorder'
  'conflict'
  'cpu'
  'dantzig'
  'delta'
  'deltastrong'
  'dense'
  'depth'
  'deterministic'
  'dj'
  'djbefore'
  'downdepth'
  'downfewest'
  'dual'
  'dynamic'
  'endboth'
  'endbothroot'
  'endclean'
  'endcleanroot'
  'endonly'
  'endonlyroot'
  'equal'
  'equalall'
  'equilibrium'
  'exact'
  'fewest'
  'file'
  'force'
  'forceandglobal'
  'forcelongon'
  'forceon'
  'forceonbut'
  'forceonbutstrong'
  'forceonglobal'
  'forceonstrong'
  'forcesos'
  'forcevariable'
  'gamma'
  'gammastrong'
  'generalforce'
  'geometric'
  'high'
  'hybrid'
  'idiot1'
  'idiot2'
  'idiot3'
  'idiot4'
  'idiot5'
  'idiot6'
  'idiot7'
  'ifmove'
  'intree'
  'length'
  'll'
  'long'
  'longendonly'
  'longifmove'
  'longon'
  'longroot'
  'lots'
  'low'
  'lx'
  'maybe'
  'more'
  'nonzero'
  'normal'
  'off'
  'off1'
  'off2'
  'often'
  'on'
  'on1'
  'on2'
  'onglobal'
  'onlyaswell'
  'onlyaswellroot'
  'onlyinstead'
  'onquick'
  'onstrong'
  'opportunistic'
  'orderhigh'
  'orderlow'
  'osl'
  'partial'
  'pedantzig'
  'pesteepest'
  'presolve'
  'primal'
  'priorities'
  'root'
  'rowsonly'
  'save'
  'simple'
  'singletons'
  'solow_halim'
  'sos'
  'sprint'
  'steepest'
  'stopaftersaving'
  'strategy'
  'strong'
  'strongroot'
  'trysos'
  'updepth'
  'upfewest'
  'usesolution'
  'uu'
  'ux'
  'variable'
  'wall'
/;
set f / def Default, lo Lower Bound, up Upper Bound, ref Reference /;
set t / I Integer, R Real, S String, B Binary /;
set g Option Groups /
  gr_General_Options   'General Options'
  gr_LP_Options   'LP Options'
  gr_MIP_Options   'MIP Options'
  gr_MIP_Options_for_Cutting_Plane_Generators   'MIP Options for Cutting Plane Generators'
  gr_MIP_Options_for_Primal_Heuristics   'MIP Options for Primal Heuristics'
/;
set o Solver and Link Options with one-line description /
  'reslim'  "maximum seconds"
  'clocktype'  "type of clock for time measurement"
  'special'  "options passed unseen to CBC"
  'writemps'  "create MPS file for problem"
  'iterlim'  "Maximum number of iterations before stopping"
  'idiotCrash'  "Whether to try idiot crash"
  'sprintCrash'  "Whether to try sprint crash"
  'crash'  "Whether to create basis for problem"
  'factorization'  "Which factorization to use"
  'denseThreshold'  "Threshold for using dense factorization"
  'smallFactorization'  "Threshold for using small factorization"
  'sparseFactor'  "Whether factorization treated as sparse"
  'biasLU'  "Whether factorization biased towards U"
  'maxFactor'  "Maximum number of iterations between refactorizations"
  'crossover'  "Whether to get a basic solution with the simplex algorithm after the barrier algorithm finished"
  'dualPivot'  "Dual pivot choice algorithm"
  'primalPivot'  "Primal pivot choice algorithm"
  'psi'  "Two-dimension pricing factor for Positive Edge criterion"
  'perturbation'  "Whether to perturb the problem"
  'scaling'  "Whether to scale problem"
  'presolve'  "Whether to presolve problem"
  'passPresolve'  "How many passes in presolve"
  'substitution'  "How long a column to substitute for in presolve"
  'randomSeedClp'  "Random seed for Clp"
  'tol_primal'  "For a feasible solution no primal infeasibility, i.e., constraint violation, may exceed this value"
  'tol_dual'  "For an optimal solution no dual infeasibility may exceed this value"
  'tol_presolve'  "Tolerance to use in presolve"
  'startalg'  "LP solver for root node"
  'primalWeight'  "Initially algorithm acts as if it costs this much to be infeasible"
  'autoScale'  "Whether to scale objective, rhs and bounds of problem if they look odd (experimental)"
  'bscale'  "Whether to scale in barrier (and ordering speed)"
  'gamma'  "Whether to regularize barrier"
  'KKT'  "Whether to use KKT factorization in barrier"
  'threads'  "Number of threads to try and use"
  'parallelmode'  "whether to run opportunistic or deterministic"
  'strategy'  "Switches on groups of features"
  'mipstart'  "whether it should be tried to use the initial variable levels as initial MIP solution"
  'tol_integer'  "For a feasible solution no integer variable may be more than this away from an integer value"
  'sollim'  "Maximum number of feasible solutions to get"
  'dumpsolutions'  "name of solutions index gdx file for writing alternate solutions"
  'dumpsolutionsmerged'  "name of gdx file for writing all alternate solutions"
  'maxsol'  "Maximum number of solutions to save"
  'strongBranching'  "Number of variables to look at in strong branching"
  'trustPseudoCosts'  "Number of branches before we trust pseudocosts"
  'expensiveStrong'  "Whether to do even more strong branching"
  'OrbitalBranching'  "Whether to try orbital branching"
  'costStrategy'  "How to use costs for branching priorities"
  'extraVariables'  "Allow creation of extra integer variables"
  'multipleRootPasses'  "Do multiple root passes to collect cuts and solutions"
  'nodeStrategy'  "What strategy to use to select the next node from the branch and cut tree"
  'infeasibilityWeight'  "Each integer infeasibility is expected to cost this much"
  'preprocess'  "Whether to use integer preprocessing"
  'fixOnDj'  "Try heuristic based on fixing variables with reduced costs greater than this"
  'printfrequency'  "frequency of status prints"
  'randomSeedCbc'  "Random seed for Cbc"
  'loglevel'  "amount of output printed by CBC"
  'increment'  "A valid solution must be at least this much better than last integer solution"
  'solvefinal'  "final solve of MIP with fixed discrete variables"
  'solvetrace'  "name of trace file for solving information"
  'solvetracenodefreq'  "frequency in number of nodes for writing to solve trace file"
  'solvetracetimefreq'  "frequency in seconds for writing to solve trace file"
  'nodlim'  "node limit"
  'optca'  "Stop when gap between best possible and best less than this"
  'optcr'  "Stop when gap between best possible and best known is less than this fraction of larger of two"
  'cutoff'  "Bound on the objective value for all solutions"
  'cutoffConstraint'  "Whether to use cutoff as constraint"
  'sosPrioritize'  "How to deal with SOS priorities"
  'cutDepth'  "Depth in tree at which to do cuts"
  'cut_passes_root'  "Number of rounds that cut generators are applied in the root node"
  'cut_passes_tree'  "Number of rounds that cut generators are applied in the tree"
  'cut_passes_slow'  "Maximum number of rounds for slower cut generators"
  'cutLength'  "Length of a cut"
  'cuts'  "Switches all cut generators on or off"
  'cliqueCuts'  "Whether to use Clique cuts"
  'conflictcuts'  "Conflict Cuts"
  'flowCoverCuts'  "Whether to use Flow Cover cuts"
  'gomoryCuts'  "Whether to use Gomory cuts"
  'gomorycuts2'  "Whether to use alternative Gomory cuts"
  'knapsackCuts'  "Whether to use Knapsack cuts"
  'liftAndProjectCuts'  "Whether to use Lift and Project cuts"
  'mirCuts'  "Whether to use Mixed Integer Rounding cuts"
  'twoMirCuts'  "Whether to use Two phase Mixed Integer Rounding cuts"
  'probingCuts'  "Whether to use Probing cuts"
  'reduceAndSplitCuts'  "Whether to use Reduce-and-Split cuts"
  'reduceAndSplitCuts2'  "Whether to use Reduce-and-Split cuts - style 2"
  'residualCapacityCuts'  "Whether to use Residual Capacity cuts"
  'zeroHalfCuts'  "Whether to use zero half cuts"
  'lagomoryCuts'  "Whether to use Lagrangean Gomory cuts"
  'latwomirCuts'  "Whether to use Lagrangean TwoMir cuts"
  'heuristics'  "Switches most primal heuristics on or off"
  'hOptions'  "Heuristic options"
  'combineSolutions'  "Whether to use combine solution heuristic"
  'combine2Solutions'  "Whether to use crossover solution heuristic"
  'Dins'  "Whether to try Distance Induced Neighborhood Search"
  'DivingRandom'  "Whether to try Diving heuristics"
  'DivingCoefficient'  "Whether to try Coefficient diving heuristic"
  'DivingFractional'  "Whether to try Fractional diving heuristic"
  'DivingGuided'  "Whether to try Guided diving heuristic"
  'DivingLineSearch'  "Whether to try Linesearch diving heuristic"
  'DivingPseudoCost'  "Whether to try Pseudocost diving heuristic"
  'DivingVectorLength'  "Whether to try Vectorlength diving heuristic"
  'diveSolves'  "Diving solve option"
  'feaspump'  "Whether to try the Feasibility Pump heuristic"
  'feaspump_passes'  "How many passes to do in the Feasibility Pump heuristic"
  'feaspump_artcost'  "Costs &ge; this treated as artificials in feasibility pump"
  'feaspump_fracbab'  "Fraction in feasibility pump"
  'feaspump_cutoff'  "Fake cutoff for use in feasibility pump"
  'feaspump_increment'  "Fake increment for use in feasibility pump"
  'greedyHeuristic'  "Whether to use a greedy heuristic"
  'localTreeSearch'  "Whether to use local tree search when a solution is found"
  'naiveHeuristics'  "Whether to try some stupid heuristic"
  'pivotAndFix'  "Whether to try Pivot and Fix heuristic"
  'randomizedRounding'  "Whether to try randomized rounding heuristic"
  'Rens'  "Whether to try Relaxation Enforced Neighborhood Search"
  'Rins'  "Whether to try Relaxed Induced Neighborhood Search"
  'roundingHeuristic'  "Whether to use simple (but effective) Rounding heuristic"
  'vubheuristic'  "Type of VUB heuristic"
  'proximitySearch'  "Whether to do proximity search heuristic"
  'dwHeuristic'  "Whether to try Dantzig Wolfe heuristic"
  'pivotAndComplement'  "Whether to try Pivot and Complement heuristic"
  'VndVariableNeighborhoodSearch'  "Whether to try Variable Neighborhood Search"
/;
$onembedded
set optdata(g,o,t,f) /
  gr_General_Options.'reslim'.R.(def 1e+100, lo -1, ref 56)
  gr_General_Options.'clocktype'.S.(def 'wall', ref -1)
  gr_General_Options.'special'.S.(def '', ref -1)
  gr_General_Options.'writemps'.S.(def '', ref -1)
  gr_LP_Options.'iterlim'.I.(def maxint, ref 104)
  gr_LP_Options.'idiotCrash'.I.(def -1, lo -1, ref 106)
  gr_LP_Options.'sprintCrash'.I.(def -1, lo -1, ref 107)
  gr_LP_Options.'crash'.S.(def 'off', ref 209)
  gr_LP_Options.'factorization'.S.(def 'normal', ref 222)
  gr_LP_Options.'denseThreshold'.I.(def -1, lo -1, up 10000, ref 172)
  gr_LP_Options.'smallFactorization'.I.(def -1, lo -1, up 10000, ref 177)
  gr_LP_Options.'sparseFactor'.B.(def 1, ref 206)
  gr_LP_Options.'biasLU'.S.(def 'LX', ref 210)
  gr_LP_Options.'maxFactor'.I.(def 200, lo 1, ref 102)
  gr_LP_Options.'crossover'.S.(def 'on', ref 218)
  gr_LP_Options.'dualPivot'.S.(def 'automatic', ref 202)
  gr_LP_Options.'primalPivot'.S.(def 'automatic', ref 207)
  gr_LP_Options.'psi'.R.(def -0.5, lo -1.1, up 1.1, ref 9)
  gr_LP_Options.'perturbation'.B.(def 1, ref 211)
  gr_LP_Options.'scaling'.S.(def 'automatic', ref 203)
  gr_LP_Options.'presolve'.S.(def 'on', ref 208)
  gr_LP_Options.'passPresolve'.I.(def 5, lo -200, up 100, ref 105)
  gr_LP_Options.'substitution'.I.(def 3, up 10000, ref 113)
  gr_LP_Options.'randomSeedClp'.I.(def 1234567, ref 119)
  gr_LP_Options.'tol_primal'.R.(def 1e-07, lo 1e-20, ref 1)
  gr_LP_Options.'tol_dual'.R.(def 1e-07, lo 1e-20, ref 2)
  gr_LP_Options.'tol_presolve'.R.(def 1e-08, lo 1e-20, ref 83)
  gr_LP_Options.'startalg'.S.(def 'dual', ref -1)
  gr_LP_Options.'primalWeight'.R.(def 1e+10, lo 1e-20, ref 5)
  gr_LP_Options.'autoScale'.B.(def 0, ref 213)
  gr_LP_Options.'bscale'.S.(def 'off', ref 216)
  gr_LP_Options.'gamma'.S.(def 'off', ref 217)
  gr_LP_Options.'KKT'.B.(def 0, ref 215)
  gr_MIP_Options.'threads'.I.(def 1, lo 1, up 99, ref 169)
  gr_MIP_Options.'parallelmode'.S.(def 'deterministic', ref -1)
  gr_MIP_Options.'strategy'.I.(def 1, up 2, ref 176)
  gr_MIP_Options.'mipstart'.B.(def 0, ref -1)
  gr_MIP_Options.'tol_integer'.R.(def 1e-07, lo 1e-20, up 0.5, ref 53)
  gr_MIP_Options.'sollim'.I.(def maxint, lo 1, ref 160)
  gr_MIP_Options.'dumpsolutions'.S.(def '', ref -1)
  gr_MIP_Options.'dumpsolutionsmerged'.S.(def '', ref -1)
  gr_MIP_Options.'maxsol'.I.(def 100, ref 182)
  gr_MIP_Options.'strongBranching'.I.(def 5, ref 151)
  gr_MIP_Options.'trustPseudoCosts'.I.(def 10, lo -3, ref 154)
  gr_MIP_Options.'expensiveStrong'.I.(def 0, ref 185)
  gr_MIP_Options.'OrbitalBranching'.S.(def 'off', ref 349)
  gr_MIP_Options.'costStrategy'.S.(def 'off', ref 312)
  gr_MIP_Options.'extraVariables'.I.(def 0, lo minint, ref 186)
  gr_MIP_Options.'multipleRootPasses'.I.(def 0, ref 184)
  gr_MIP_Options.'nodeStrategy'.S.(def 'fewest', ref 301)
  gr_MIP_Options.'infeasibilityWeight'.R.(def 0, ref 51)
  gr_MIP_Options.'preprocess'.S.(def 'sos', ref 316)
  gr_MIP_Options.'fixOnDj'.R.(def -1, lo mindouble, ref 81)
  gr_MIP_Options.'printfrequency'.I.(def 0, ref -1)
  gr_MIP_Options.'randomSeedCbc'.I.(def -1, lo -1, ref 183)
  gr_MIP_Options.'loglevel'.I.(def 1, ref -1)
  gr_MIP_Options.'increment'.R.(def 1e-05, lo mindouble, ref 54)
  gr_MIP_Options.'solvefinal'.B.(def 1, ref -1)
  gr_MIP_Options.'solvetrace'.S.(def '', ref -1)
  gr_MIP_Options.'solvetracenodefreq'.I.(def 100, ref -1)
  gr_MIP_Options.'solvetracetimefreq'.R.(def 5, ref -1)
  gr_MIP_Options.'nodlim'.I.(def maxint, lo -1, ref 153)
  gr_MIP_Options.'optca'.R.(def 1e-10, ref 55)
  gr_MIP_Options.'optcr'.R.(def 0, ref 57)
  gr_MIP_Options.'cutoff'.R.(def 1e+100, lo mindouble, ref 52)
  gr_MIP_Options.'cutoffConstraint'.S.(def 'off', ref 347)
  gr_MIP_Options.'sosPrioritize'.S.(def 'off', ref 351)
  gr_MIP_Options_for_Cutting_Plane_Generators.'cutDepth'.I.(def -1, lo -1, ref 152)
  gr_MIP_Options_for_Cutting_Plane_Generators.'cut_passes_root'.I.(def 20, lo minint, ref 170)
  gr_MIP_Options_for_Cutting_Plane_Generators.'cut_passes_tree'.I.(def 10, lo minint, ref 168)
  gr_MIP_Options_for_Cutting_Plane_Generators.'cut_passes_slow'.I.(def 10, lo -1, ref 187)
  gr_MIP_Options_for_Cutting_Plane_Generators.'cutLength'.I.(def -1, lo -1, ref 179)
  gr_MIP_Options_for_Cutting_Plane_Generators.'cuts'.S.(def 'on', ref 303)
  gr_MIP_Options_for_Cutting_Plane_Generators.'cliqueCuts'.S.(def 'ifmove', ref 311)
  gr_MIP_Options_for_Cutting_Plane_Generators.'conflictcuts'.B.(def 0, ref -1)
  gr_MIP_Options_for_Cutting_Plane_Generators.'flowCoverCuts'.S.(def 'ifmove', ref 313)
  gr_MIP_Options_for_Cutting_Plane_Generators.'gomoryCuts'.S.(def 'ifmove', ref 305)
  gr_MIP_Options_for_Cutting_Plane_Generators.'gomorycuts2'.S.(def 'off', ref 346)
  gr_MIP_Options_for_Cutting_Plane_Generators.'knapsackCuts'.S.(def 'ifmove', ref 307)
  gr_MIP_Options_for_Cutting_Plane_Generators.'liftAndProjectCuts'.S.(def 'off', ref 323)
  gr_MIP_Options_for_Cutting_Plane_Generators.'mirCuts'.S.(def 'ifmove', ref 314)
  gr_MIP_Options_for_Cutting_Plane_Generators.'twoMirCuts'.S.(def 'root', ref 315)
  gr_MIP_Options_for_Cutting_Plane_Generators.'probingCuts'.S.(def 'ifmove', ref 306)
  gr_MIP_Options_for_Cutting_Plane_Generators.'reduceAndSplitCuts'.S.(def 'off', ref 308)
  gr_MIP_Options_for_Cutting_Plane_Generators.'reduceAndSplitCuts2'.S.(def 'off', ref 345)
  gr_MIP_Options_for_Cutting_Plane_Generators.'residualCapacityCuts'.S.(def 'off', ref 325)
  gr_MIP_Options_for_Cutting_Plane_Generators.'zeroHalfCuts'.S.(def 'ifmove', ref 338)
  gr_MIP_Options_for_Cutting_Plane_Generators.'lagomoryCuts'.S.(def 'off', ref 343)
  gr_MIP_Options_for_Cutting_Plane_Generators.'latwomirCuts'.S.(def 'off', ref 344)
  gr_MIP_Options_for_Primal_Heuristics.'heuristics'.B.(def 1, ref 304)
  gr_MIP_Options_for_Primal_Heuristics.'hOptions'.I.(def 0, lo minint, ref 178)
  gr_MIP_Options_for_Primal_Heuristics.'combineSolutions'.S.(def 'off', ref 319)
  gr_MIP_Options_for_Primal_Heuristics.'combine2Solutions'.S.(def 'off', ref 340)
  gr_MIP_Options_for_Primal_Heuristics.'Dins'.S.(def 'off', ref 334)
  gr_MIP_Options_for_Primal_Heuristics.'DivingRandom'.S.(def 'off', ref 327)
  gr_MIP_Options_for_Primal_Heuristics.'DivingCoefficient'.S.(def 'off', ref 328)
  gr_MIP_Options_for_Primal_Heuristics.'DivingFractional'.S.(def 'off', ref 329)
  gr_MIP_Options_for_Primal_Heuristics.'DivingGuided'.S.(def 'off', ref 330)
  gr_MIP_Options_for_Primal_Heuristics.'DivingLineSearch'.S.(def 'off', ref 331)
  gr_MIP_Options_for_Primal_Heuristics.'DivingPseudoCost'.S.(def 'off', ref 332)
  gr_MIP_Options_for_Primal_Heuristics.'DivingVectorLength'.S.(def 'off', ref 333)
  gr_MIP_Options_for_Primal_Heuristics.'diveSolves'.I.(def 100, lo -1, up 200000, ref 175)
  gr_MIP_Options_for_Primal_Heuristics.'feaspump'.S.(def 'on', ref 317)
  gr_MIP_Options_for_Primal_Heuristics.'feaspump_passes'.I.(def 20, up 10000, ref 159)
  gr_MIP_Options_for_Primal_Heuristics.'feaspump_artcost'.R.(def 0, ref 87)
  gr_MIP_Options_for_Primal_Heuristics.'feaspump_fracbab'.R.(def 0.5, lo 1e-05, up 1.1, ref 89)
  gr_MIP_Options_for_Primal_Heuristics.'feaspump_cutoff'.R.(def 0, lo mindouble, ref 86)
  gr_MIP_Options_for_Primal_Heuristics.'feaspump_increment'.R.(def 0, lo mindouble, ref 85)
  gr_MIP_Options_for_Primal_Heuristics.'greedyHeuristic'.S.(def 'on', ref 318)
  gr_MIP_Options_for_Primal_Heuristics.'localTreeSearch'.B.(def 0, ref 321)
  gr_MIP_Options_for_Primal_Heuristics.'naiveHeuristics'.S.(def 'off', ref 337)
  gr_MIP_Options_for_Primal_Heuristics.'pivotAndFix'.S.(def 'off', ref 335)
  gr_MIP_Options_for_Primal_Heuristics.'randomizedRounding'.S.(def 'off', ref 336)
  gr_MIP_Options_for_Primal_Heuristics.'Rens'.S.(def 'off', ref 326)
  gr_MIP_Options_for_Primal_Heuristics.'Rins'.S.(def 'off', ref 324)
  gr_MIP_Options_for_Primal_Heuristics.'roundingHeuristic'.S.(def 'on', ref 309)
  gr_MIP_Options_for_Primal_Heuristics.'vubheuristic'.I.(def -1, lo -2, up 20, ref 171)
  gr_MIP_Options_for_Primal_Heuristics.'proximitySearch'.S.(def 'off', ref 320)
  gr_MIP_Options_for_Primal_Heuristics.'dwHeuristic'.S.(def 'off', ref 348)
  gr_MIP_Options_for_Primal_Heuristics.'pivotAndComplement'.S.(def 'off', ref 341)
  gr_MIP_Options_for_Primal_Heuristics.'VndVariableNeighborhoodSearch'.S.(def 'off', ref 342)
/;
set oe(o,e) /
  'clocktype'.'cpu'  "CPU clock"
  'clocktype'.'wall'  "Wall clock"
  'crash'.'off'
  'crash'.'on'
  'crash'.'solow_halim'
  'crash'.'lots'
  'crash'.'idiot1'
  'crash'.'idiot2'
  'crash'.'idiot3'
  'crash'.'idiot4'
  'crash'.'idiot5'
  'crash'.'idiot6'
  'crash'.'idiot7'
  'factorization'.'normal'
  'factorization'.'dense'
  'factorization'.'simple'
  'factorization'.'osl'
  'biasLU'.'UU'
  'biasLU'.'UX'
  'biasLU'.'LX'
  'biasLU'.'LL'
  'crossover'.'on'
  'crossover'.'off'
  'crossover'.'presolve'
  'crossover'.'0'  "Same as off. This is a deprecated setting."
  'crossover'.'1'  "Same as on. This is a deprecated setting."
  'dualPivot'.'automatic'
  'dualPivot'.'dantzig'
  'dualPivot'.'partial'
  'dualPivot'.'steepest'
  'dualPivot'.'PEsteepest'
  'dualPivot'.'PEdantzig'
  'dualPivot'.'auto'  "Same as automatic. This is a deprecated setting."
  'primalPivot'.'automatic'
  'primalPivot'.'exact'
  'primalPivot'.'dantzig'
  'primalPivot'.'partial'
  'primalPivot'.'steepest'
  'primalPivot'.'change'
  'primalPivot'.'sprint'
  'primalPivot'.'PEsteepest'
  'primalPivot'.'PEdantzig'
  'primalPivot'.'auto'  "Same as automatic. This is a deprecated setting."
  'scaling'.'off'
  'scaling'.'equilibrium'
  'scaling'.'geometric'
  'scaling'.'automatic'
  'scaling'.'dynamic'
  'scaling'.'rowsonly'
  'scaling'.'auto'  "Same as automatic. This is a deprecated setting."
  'presolve'.'on'
  'presolve'.'off'
  'presolve'.'more'
  'presolve'.'0'  "Same as off. This is a deprecated setting."
  'presolve'.'1'  "Same as on. This is a deprecated setting."
  'startalg'.'primal'  "Primal Simplex algorithm"
  'startalg'.'dual'  "Dual Simplex algorithm"
  'startalg'.'barrier'  "Primal-dual predictor-corrector algorithm"
  'bscale'.'off'
  'bscale'.'on'
  'bscale'.'off1'
  'bscale'.'on1'
  'bscale'.'off2'
  'bscale'.'on2'
  'gamma'.'off'
  'gamma'.'on'
  'gamma'.'gamma'
  'gamma'.'delta'
  'gamma'.'onstrong'
  'gamma'.'gammastrong'
  'gamma'.'deltastrong'
  'parallelmode'.'opportunistic'
  'parallelmode'.'deterministic'
  'OrbitalBranching'.'off'
  'OrbitalBranching'.'on'
  'OrbitalBranching'.'strong'
  'OrbitalBranching'.'force'
  'OrbitalBranching'.'simple'
  'costStrategy'.'off'
  'costStrategy'.'priorities'
  'costStrategy'.'columnOrder'
  'costStrategy'.'01first'
  'costStrategy'.'binaryfirst'  "This is a deprecated setting. Please use 01first."
  'costStrategy'.'01last'
  'costStrategy'.'binarylast'  "This is a deprecated setting. Please use 01last."
  'costStrategy'.'length'
  'costStrategy'.'singletons'
  'costStrategy'.'nonzero'
  'costStrategy'.'generalForce'
  'nodeStrategy'.'hybrid'
  'nodeStrategy'.'fewest'
  'nodeStrategy'.'depth'
  'nodeStrategy'.'upfewest'
  'nodeStrategy'.'downfewest'
  'nodeStrategy'.'updepth'
  'nodeStrategy'.'downdepth'
  'preprocess'.'off'
  'preprocess'.'on'
  'preprocess'.'save'
  'preprocess'.'equal'
  'preprocess'.'sos'
  'preprocess'.'trysos'
  'preprocess'.'equalall'
  'preprocess'.'strategy'
  'preprocess'.'aggregate'
  'preprocess'.'forcesos'
  'preprocess'.'stopaftersaving'
  'cutoffConstraint'.'off'
  'cutoffConstraint'.'on'
  'cutoffConstraint'.'variable'
  'cutoffConstraint'.'forcevariable'
  'cutoffConstraint'.'conflict'
  'cutoffConstraint'.'0'  "Same as off. This is a deprecated setting."
  'cutoffConstraint'.'1'  "Same as on. This is a deprecated setting."
  'sosPrioritize'.'off'
  'sosPrioritize'.'high'
  'sosPrioritize'.'low'
  'sosPrioritize'.'orderhigh'
  'sosPrioritize'.'orderlow'
  'cuts'.'off'
  'cuts'.'on'
  'cuts'.'root'
  'cuts'.'ifmove'
  'cuts'.'forceOn'
  'cliqueCuts'.'off'
  'cliqueCuts'.'on'
  'cliqueCuts'.'root'
  'cliqueCuts'.'ifmove'
  'cliqueCuts'.'forceOn'
  'cliqueCuts'.'onglobal'
  'flowCoverCuts'.'off'
  'flowCoverCuts'.'on'
  'flowCoverCuts'.'root'
  'flowCoverCuts'.'ifmove'
  'flowCoverCuts'.'forceOn'
  'flowCoverCuts'.'onglobal'
  'gomoryCuts'.'off'
  'gomoryCuts'.'on'
  'gomoryCuts'.'root'
  'gomoryCuts'.'ifmove'
  'gomoryCuts'.'forceOn'
  'gomoryCuts'.'onglobal'
  'gomoryCuts'.'forceandglobal'
  'gomoryCuts'.'forceLongOn'
  'gomoryCuts'.'long'
  'gomorycuts2'.'off'
  'gomorycuts2'.'on'
  'gomorycuts2'.'root'
  'gomorycuts2'.'ifmove'
  'gomorycuts2'.'forceOn'
  'gomorycuts2'.'endonly'
  'gomorycuts2'.'long'
  'gomorycuts2'.'longroot'
  'gomorycuts2'.'longifmove'
  'gomorycuts2'.'forceLongOn'
  'gomorycuts2'.'longendonly'
  'knapsackCuts'.'off'
  'knapsackCuts'.'on'
  'knapsackCuts'.'root'
  'knapsackCuts'.'ifmove'
  'knapsackCuts'.'forceOn'
  'knapsackCuts'.'onglobal'
  'knapsackCuts'.'forceandglobal'
  'liftAndProjectCuts'.'off'
  'liftAndProjectCuts'.'on'
  'liftAndProjectCuts'.'root'
  'liftAndProjectCuts'.'ifmove'
  'liftAndProjectCuts'.'forceOn'
  'mirCuts'.'off'
  'mirCuts'.'on'
  'mirCuts'.'root'
  'mirCuts'.'ifmove'
  'mirCuts'.'forceOn'
  'mirCuts'.'onglobal'
  'twoMirCuts'.'off'
  'twoMirCuts'.'on'
  'twoMirCuts'.'root'
  'twoMirCuts'.'ifmove'
  'twoMirCuts'.'forceOn'
  'twoMirCuts'.'onglobal'
  'twoMirCuts'.'forceandglobal'
  'twoMirCuts'.'forceLongOn'
  'probingCuts'.'off'
  'probingCuts'.'on'
  'probingCuts'.'root'
  'probingCuts'.'ifmove'
  'probingCuts'.'forceOn'
  'probingCuts'.'onglobal'
  'probingCuts'.'forceonglobal'
  'probingCuts'.'forceOnBut'
  'probingCuts'.'forceOnStrong'
  'probingCuts'.'forceOnButStrong'
  'probingCuts'.'strongRoot'
  'reduceAndSplitCuts'.'off'
  'reduceAndSplitCuts'.'on'
  'reduceAndSplitCuts'.'root'
  'reduceAndSplitCuts'.'ifmove'
  'reduceAndSplitCuts'.'forceOn'
  'reduceAndSplitCuts2'.'off'
  'reduceAndSplitCuts2'.'on'
  'reduceAndSplitCuts2'.'root'
  'reduceAndSplitCuts2'.'longOn'
  'reduceAndSplitCuts2'.'longRoot'
  'residualCapacityCuts'.'off'
  'residualCapacityCuts'.'on'
  'residualCapacityCuts'.'root'
  'residualCapacityCuts'.'ifmove'
  'residualCapacityCuts'.'forceOn'
  'zeroHalfCuts'.'off'
  'zeroHalfCuts'.'on'
  'zeroHalfCuts'.'root'
  'zeroHalfCuts'.'ifmove'
  'zeroHalfCuts'.'forceOn'
  'zeroHalfCuts'.'onglobal'
  'lagomoryCuts'.'off'
  'lagomoryCuts'.'endonlyroot'
  'lagomoryCuts'.'endcleanroot'
  'lagomoryCuts'.'root'
  'lagomoryCuts'.'endonly'
  'lagomoryCuts'.'endclean'
  'lagomoryCuts'.'endboth'
  'lagomoryCuts'.'onlyaswell'
  'lagomoryCuts'.'cleanaswell'
  'lagomoryCuts'.'bothaswell'
  'lagomoryCuts'.'onlyinstead'
  'lagomoryCuts'.'cleaninstead'
  'lagomoryCuts'.'bothinstead'
  'lagomoryCuts'.'onlyaswellroot'
  'lagomoryCuts'.'cleanaswellroot'
  'lagomoryCuts'.'bothaswellroot'
  'latwomirCuts'.'off'
  'latwomirCuts'.'endonlyroot'
  'latwomirCuts'.'endcleanroot'
  'latwomirCuts'.'endbothroot'
  'latwomirCuts'.'endonly'
  'latwomirCuts'.'endclean'
  'latwomirCuts'.'endboth'
  'latwomirCuts'.'onlyaswell'
  'latwomirCuts'.'cleanaswell'
  'latwomirCuts'.'bothaswell'
  'latwomirCuts'.'onlyinstead'
  'latwomirCuts'.'cleaninstead'
  'latwomirCuts'.'bothinstead'
  'combineSolutions'.'off'
  'combineSolutions'.'on'
  'combineSolutions'.'both'
  'combineSolutions'.'before'
  'combineSolutions'.'onquick'
  'combineSolutions'.'bothquick'
  'combineSolutions'.'beforequick'
  'combineSolutions'.'0'  "Same as off. This is a deprecated setting."
  'combineSolutions'.'1'  "Same as on. This is a deprecated setting."
  'combine2Solutions'.'off'
  'combine2Solutions'.'on'
  'combine2Solutions'.'both'
  'combine2Solutions'.'before'
  'Dins'.'off'
  'Dins'.'on'
  'Dins'.'both'
  'Dins'.'before'
  'Dins'.'often'
  'Dins'.'0'  "Same as off. This is a deprecated setting."
  'Dins'.'1'  "Same as on. This is a deprecated setting."
  'DivingRandom'.'off'
  'DivingRandom'.'on'
  'DivingRandom'.'both'
  'DivingRandom'.'before'
  'DivingRandom'.'0'  "Same as off. This is a deprecated setting."
  'DivingRandom'.'1'  "Same as on. This is a deprecated setting."
  'DivingCoefficient'.'off'
  'DivingCoefficient'.'on'
  'DivingCoefficient'.'both'
  'DivingCoefficient'.'before'
  'DivingCoefficient'.'0'  "Same as off. This is a deprecated setting."
  'DivingCoefficient'.'1'  "Same as on. This is a deprecated setting."
  'DivingFractional'.'off'
  'DivingFractional'.'on'
  'DivingFractional'.'both'
  'DivingFractional'.'before'
  'DivingFractional'.'0'  "Same as off. This is a deprecated setting."
  'DivingFractional'.'1'  "Same as on. This is a deprecated setting."
  'DivingGuided'.'off'
  'DivingGuided'.'on'
  'DivingGuided'.'both'
  'DivingGuided'.'before'
  'DivingGuided'.'0'  "Same as off. This is a deprecated setting."
  'DivingGuided'.'1'  "Same as on. This is a deprecated setting."
  'DivingLineSearch'.'off'
  'DivingLineSearch'.'on'
  'DivingLineSearch'.'both'
  'DivingLineSearch'.'before'
  'DivingLineSearch'.'0'  "Same as off. This is a deprecated setting."
  'DivingLineSearch'.'1'  "Same as on. This is a deprecated setting."
  'DivingPseudoCost'.'off'
  'DivingPseudoCost'.'on'
  'DivingPseudoCost'.'both'
  'DivingPseudoCost'.'before'
  'DivingPseudoCost'.'0'  "Same as off. This is a deprecated setting."
  'DivingPseudoCost'.'1'  "Same as on. This is a deprecated setting."
  'DivingVectorLength'.'off'
  'DivingVectorLength'.'on'
  'DivingVectorLength'.'both'
  'DivingVectorLength'.'before'
  'DivingVectorLength'.'0'  "Same as off. This is a deprecated setting."
  'DivingVectorLength'.'1'  "Same as on. This is a deprecated setting."
  'feaspump'.'off'
  'feaspump'.'on'
  'feaspump'.'both'
  'feaspump'.'before'
  'feaspump'.'0'  "Same as off. This is a deprecated setting."
  'feaspump'.'1'  "Same as on. This is a deprecated setting."
  'greedyHeuristic'.'off'
  'greedyHeuristic'.'on'
  'greedyHeuristic'.'both'
  'greedyHeuristic'.'before'
  'naiveHeuristics'.'off'
  'naiveHeuristics'.'on'
  'naiveHeuristics'.'both'
  'naiveHeuristics'.'before'
  'naiveHeuristics'.'0'  "Same as off. This is a deprecated setting."
  'naiveHeuristics'.'1'  "Same as on. This is a deprecated setting."
  'pivotAndFix'.'off'
  'pivotAndFix'.'on'
  'pivotAndFix'.'both'
  'pivotAndFix'.'before'
  'pivotAndFix'.'0'  "Same as off. This is a deprecated setting."
  'pivotAndFix'.'1'  "Same as on. This is a deprecated setting."
  'randomizedRounding'.'off'
  'randomizedRounding'.'on'
  'randomizedRounding'.'both'
  'randomizedRounding'.'before'
  'randomizedRounding'.'0'  "Same as off. This is a deprecated setting."
  'randomizedRounding'.'1'  "Same as on. This is a deprecated setting."
  'Rens'.'off'
  'Rens'.'on'
  'Rens'.'both'
  'Rens'.'before'
  'Rens'.'200'
  'Rens'.'1000'
  'Rens'.'10000'
  'Rens'.'dj'
  'Rens'.'djbefore'
  'Rens'.'usesolution'
  'Rens'.'0'  "Same as off. This is a deprecated setting."
  'Rens'.'1'  "Same as on. This is a deprecated setting."
  'Rins'.'off'
  'Rins'.'on'
  'Rins'.'both'
  'Rins'.'before'
  'Rins'.'often'
  'Rins'.'0'  "Same as off. This is a deprecated setting."
  'Rins'.'1'  "Same as on. This is a deprecated setting."
  'roundingHeuristic'.'off'
  'roundingHeuristic'.'on'
  'roundingHeuristic'.'both'
  'roundingHeuristic'.'before'
  'roundingHeuristic'.'0'  "Same as off. This is a deprecated setting."
  'roundingHeuristic'.'1'  "Same as on. This is a deprecated setting."
  'proximitySearch'.'off'
  'proximitySearch'.'on'
  'proximitySearch'.'both'
  'proximitySearch'.'before'
  'proximitySearch'.'10'
  'proximitySearch'.'100'
  'proximitySearch'.'300'
  'proximitySearch'.'0'  "Same as off. This is a deprecated setting."
  'proximitySearch'.'1'  "Same as on. This is a deprecated setting."
  'dwHeuristic'.'off'
  'dwHeuristic'.'on'
  'dwHeuristic'.'both'
  'dwHeuristic'.'before'
  'pivotAndComplement'.'off'
  'pivotAndComplement'.'on'
  'pivotAndComplement'.'both'
  'pivotAndComplement'.'before'
  'VndVariableNeighborhoodSearch'.'off'
  'VndVariableNeighborhoodSearch'.'on'
  'VndVariableNeighborhoodSearch'.'both'
  'VndVariableNeighborhoodSearch'.'before'
  'VndVariableNeighborhoodSearch'.'intree'
/;
$onempty
set odefault(o) /
  'reslim' "GAMS reslim"
  'iterlim' "GAMS iterlim"
  'threads' "GAMS threads"
  'increment' "GAMS cheat"
  'nodlim' "GAMS nodlim"
  'optca' "GAMS optca"
  'optcr' "GAMS optcr"
  'cutoff' "GAMS cutoff"
  'cut_passes_root' "20 or 100"
/;
set os(o,*) synonyms  /
  'reslim'.'seconds'
  'iterlim'.'maxIterations'
  'sprintCrash'.'sifting'
  'randomSeedClp'.'randomSeed'
  'tol_primal'.'primalTolerance'
  'tol_dual'.'dualTolerance'
  'tol_presolve'.'preTolerance'
  'tol_integer'.'integerTolerance'
  'sollim'.'maxSolutions'
  'maxsol'.'maxSavedSolutions'
  'randomSeedCbc'.'randomCbcSeed'
  'nodlim'.'maxNodes'
  'nodlim'.'nodelim'
  'optca'.'allowableGap'
  'optcr'.'ratioGap'
  'cutoffConstraint'.'constraintfromCutoff'
  'cut_passes_root'.'passCuts'
  'cut_passes_tree'.'passTreeCuts'
  'cut_passes_slow'.'slowcutpasses'
  'cuts'.'cutsOnOff'
  'gomorycuts2'.'GMICuts'
  'mirCuts'.'mixedIntegerRoundingCuts'
  'reduceAndSplitCuts2'.'reduce2AndSplitCuts'
  'heuristics'.'heuristicsOnOff'
  'DivingRandom'.'DivingSome'
  'feaspump'.'feasibilityPump'
  'feaspump_passes'.'passFeasibilityPump'
  'feaspump_artcost'.'artificialCost'
  'feaspump_fracbab'.'fractionforBAB'
  'feaspump_cutoff'.'pumpCutoff'
  'feaspump_increment'.'pumpIncrement' /;
set im immediates recognized  / EolFlag , Message /;
set strlist(o)        / /;
set immediate(o,im)   / /;
set multilist(o,o)    / /;
set indicator(o)      / /;
set dotoption(o)      / /;
set dropdefaults(o)   / /;
set deprecated(*,*)   / /;
set oedepr(o,e)       / /;
set oep(o)            / /;
set olink(o)          / /;
$offempty
