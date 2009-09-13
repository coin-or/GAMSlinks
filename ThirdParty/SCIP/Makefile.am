# Copyright (C) 2008 Stefan Vigerske
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id$

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST =

if COIN_BUILD_SCIP
  noinst_LTLIBRARIES = libcoinscip.la
else
  noinst_LTLIBRARIES =
endif

SCIPDIR = @SCIPDIR@

AM_CPPFLAGS = -I`$(CYGPATH_W) $(SCIPINCDIR)`

nodist_libcoinscip_la_SOURCES = \
      $(SCIPDIR)/src/scip/branch.c \
      $(SCIPDIR)/src/scip/bitencode.c \
			$(SCIPDIR)/src/scip/buffer.c \
			$(SCIPDIR)/src/scip/clock.c \
			$(SCIPDIR)/src/scip/conflict.c \
			$(SCIPDIR)/src/scip/cons.c \
			$(SCIPDIR)/src/scip/cutpool.c \
			$(SCIPDIR)/src/scip/debug.c \
			$(SCIPDIR)/src/scip/dialog.c \
			$(SCIPDIR)/src/scip/disp.c \
			$(SCIPDIR)/src/scip/event.c \
			$(SCIPDIR)/src/scip/fileio.c \
			$(SCIPDIR)/src/scip/heur.c \
			$(SCIPDIR)/src/scip/history.c \
			$(SCIPDIR)/src/scip/implics.c \
			$(SCIPDIR)/src/scip/interrupt.c \
			$(SCIPDIR)/src/scip/intervalarith.c \
			$(SCIPDIR)/src/scip/lp.c \
			$(SCIPDIR)/src/scip/mem.c \
      $(SCIPDIR)/src/scip/message.c \
			$(SCIPDIR)/src/scip/misc.c \
			$(SCIPDIR)/src/scip/nodesel.c \
			$(SCIPDIR)/src/scip/paramset.c \
			$(SCIPDIR)/src/scip/presol.c \
			$(SCIPDIR)/src/scip/pricestore.c \
			$(SCIPDIR)/src/scip/pricer.c \
			$(SCIPDIR)/src/scip/primal.c \
			$(SCIPDIR)/src/scip/prob.c \
			$(SCIPDIR)/src/scip/prop.c \
			$(SCIPDIR)/src/scip/reader.c \
			$(SCIPDIR)/src/scip/relax.c \
			$(SCIPDIR)/src/scip/retcode.c \
			$(SCIPDIR)/src/scip/scip.c \
			$(SCIPDIR)/src/scip/scipdefplugins.c \
			$(SCIPDIR)/src/scip/scipshell.c \
			$(SCIPDIR)/src/scip/sepa.c \
			$(SCIPDIR)/src/scip/sepastore.c \
			$(SCIPDIR)/src/scip/set.c \
			$(SCIPDIR)/src/scip/sol.c \
			$(SCIPDIR)/src/scip/solve.c \
			$(SCIPDIR)/src/scip/stat.c \
			$(SCIPDIR)/src/scip/tree.c \
			$(SCIPDIR)/src/scip/var.c \
			$(SCIPDIR)/src/scip/vbc.c \
			$(SCIPDIR)/src/scip/branch_allfullstrong.c \
			$(SCIPDIR)/src/scip/branch_fullstrong.c \
			$(SCIPDIR)/src/scip/branch_inference.c \
			$(SCIPDIR)/src/scip/branch_mostinf.c \
			$(SCIPDIR)/src/scip/branch_leastinf.c \
			$(SCIPDIR)/src/scip/branch_pscost.c \
			$(SCIPDIR)/src/scip/branch_random.c \
			$(SCIPDIR)/src/scip/branch_relpscost.c \
			$(SCIPDIR)/src/scip/cons_and.c \
			$(SCIPDIR)/src/scip/cons_binpack.c \
			$(SCIPDIR)/src/scip/cons_bounddisjunction.c \
			$(SCIPDIR)/src/scip/cons_conjunction.c \
			$(SCIPDIR)/src/scip/cons_eqknapsack.c \
			$(SCIPDIR)/src/scip/cons_integral.c \
			$(SCIPDIR)/src/scip/cons_invarknapsack.c \
			$(SCIPDIR)/src/scip/cons_knapsack.c \
			$(SCIPDIR)/src/scip/cons_linear.c \
			$(SCIPDIR)/src/scip/cons_logicor.c \
			$(SCIPDIR)/src/scip/cons_or.c \
			$(SCIPDIR)/src/scip/cons_setppc.c \
			$(SCIPDIR)/src/scip/cons_varbound.c \
			$(SCIPDIR)/src/scip/cons_xor.c \
			$(SCIPDIR)/src/scip/cons_countsols.c \
			$(SCIPDIR)/src/scip/cons_indicator.c \
			$(SCIPDIR)/src/scip/cons_sos1.c \
			$(SCIPDIR)/src/scip/cons_sos2.c \
			$(SCIPDIR)/src/scip/dialog_default.c \
			$(SCIPDIR)/src/scip/disp_default.c \
			$(SCIPDIR)/src/scip/heur_actconsdiving.c \
			$(SCIPDIR)/src/scip/heur_coefdiving.c \
			$(SCIPDIR)/src/scip/heur_crossover.c \
			$(SCIPDIR)/src/scip/heur_feaspump.c \
			$(SCIPDIR)/src/scip/heur_fixandinfer.c \
			$(SCIPDIR)/src/scip/heur_fracdiving.c \
			$(SCIPDIR)/src/scip/heur_guideddiving.c \
			$(SCIPDIR)/src/scip/heur_intdiving.c \
			$(SCIPDIR)/src/scip/heur_intshifting.c \
			$(SCIPDIR)/src/scip/heur_linesearchdiving.c \
			$(SCIPDIR)/src/scip/heur_localbranching.c \
			$(SCIPDIR)/src/scip/heur_mutation.c \
			$(SCIPDIR)/src/scip/heur_objpscostdiving.c \
			$(SCIPDIR)/src/scip/heur_octane.c \
			$(SCIPDIR)/src/scip/heur_oneopt.c \
			$(SCIPDIR)/src/scip/heur_pscostdiving.c \
			$(SCIPDIR)/src/scip/heur_rens.c \
			$(SCIPDIR)/src/scip/heur_rins.c \
			$(SCIPDIR)/src/scip/heur_rootsoldiving.c \
			$(SCIPDIR)/src/scip/heur_rounding.c \
			$(SCIPDIR)/src/scip/heur_shifting.c \
			$(SCIPDIR)/src/scip/heur_simplerounding.c \
			$(SCIPDIR)/src/scip/heur_veclendiving.c \
			$(SCIPDIR)/src/scip/heur_dins.c \
			$(SCIPDIR)/src/scip/nodesel_bfs.c \
			$(SCIPDIR)/src/scip/nodesel_dfs.c \
			$(SCIPDIR)/src/scip/nodesel_estimate.c \
			$(SCIPDIR)/src/scip/nodesel_hybridestim.c \
			$(SCIPDIR)/src/scip/nodesel_restartdfs.c \
			$(SCIPDIR)/src/scip/presol_dualfix.c \
			$(SCIPDIR)/src/scip/presol_implics.c \
			$(SCIPDIR)/src/scip/presol_inttobinary.c \
			$(SCIPDIR)/src/scip/presol_probing.c \
			$(SCIPDIR)/src/scip/presol_trivial.c \
			$(SCIPDIR)/src/scip/presol_boundshift.c \
			$(SCIPDIR)/src/scip/prop_pseudoobj.c \
			$(SCIPDIR)/src/scip/prop_rootredcost.c \
			$(SCIPDIR)/src/scip/reader_cnf.c \
			$(SCIPDIR)/src/scip/reader_fix.c \
			$(SCIPDIR)/src/scip/reader_lp.c \
			$(SCIPDIR)/src/scip/reader_mps.c \
			$(SCIPDIR)/src/scip/reader_sol.c \
			$(SCIPDIR)/src/scip/reader_zpl.c \
			$(SCIPDIR)/src/scip/reader_ccg.c \
			$(SCIPDIR)/src/scip/reader_cip.c \
			$(SCIPDIR)/src/scip/reader_opb.c \
			$(SCIPDIR)/src/scip/reader_ppm.c \
			$(SCIPDIR)/src/scip/reader_rlp.c \
			$(SCIPDIR)/src/scip/reader_sos.c \
			$(SCIPDIR)/src/scip/sepa_clique.c \
			$(SCIPDIR)/src/scip/sepa_cmir.c \
			$(SCIPDIR)/src/scip/sepa_flowcover.c \
			$(SCIPDIR)/src/scip/sepa_gomory.c \
			$(SCIPDIR)/src/scip/sepa_impliedbounds.c \
			$(SCIPDIR)/src/scip/sepa_intobj.c \
			$(SCIPDIR)/src/scip/sepa_redcost.c \
			$(SCIPDIR)/src/scip/sepa_strongcg.c \
			$(SCIPDIR)/src/scip/sepa_mcf.c \
			$(SCIPDIR)/src/scip/sepa_zerohalf.c \
			$(SCIPDIR)/src/tclique/tclique_branch.c \
			$(SCIPDIR)/src/tclique/tclique_coloring.c \
			$(SCIPDIR)/src/tclique/tclique_graph.c \
      $(SCIPDIR)/src/blockmemshell/memory.c

if SCIPDEVVERSION
nodist_libcoinscip_la_SOURCES += \
			$(SCIPDIR)/src/scip/cons_quadratic.c \
			$(SCIPDIR)/src/scip/nlpi.c \
			$(SCIPDIR)/src/scip/nlpi_oracle.c \
			$(SCIPDIR)/src/scip/heur_nlp.c \
			$(SCIPDIR)/src/scip/heur_trivial.c \
			$(SCIPDIR)/src/scip/heur_trysol.c \
			$(SCIPDIR)/src/scip/reader_gms.c \
			$(SCIPDIR)/src/scip/reader_fzn.c
endif

#nodist_libcoinscip_la_SOURCES += \
#      $(SCIPDIR)/src/objscip/objbranchrule.cpp \
#			$(SCIPDIR)/src/objscip/objconshdlr.cpp \
#			$(SCIPDIR)/src/objscip/objdialog.cpp \
#			$(SCIPDIR)/src/objscip/objdisp.cpp \
#			$(SCIPDIR)/src/objscip/objeventhdlr.cpp \
#			$(SCIPDIR)/src/objscip/objheur.cpp \
#			$(SCIPDIR)/src/objscip/objmessagehdlr.cpp \
#			$(SCIPDIR)/src/objscip/objnodesel.cpp \
#			$(SCIPDIR)/src/objscip/objpresol.cpp \
#			$(SCIPDIR)/src/objscip/objpricer.cpp \
#			$(SCIPDIR)/src/objscip/objprobdata.cpp \
#			$(SCIPDIR)/src/objscip/objprop.cpp \
#			$(SCIPDIR)/src/objscip/objreader.cpp \
#			$(SCIPDIR)/src/objscip/objrelax.cpp \
#			$(SCIPDIR)/src/objscip/objsepa.cpp \
#			$(SCIPDIR)/src/objscip/objvardata.cpp


#add CLP as linear solver
nodist_libcoinscip_la_SOURCES += $(SCIPDIR)/src/scip/lpi_clp.cpp
if SCIPDEVVERSION
#add IPOPT as nonlinear solver
nodist_libcoinscip_la_SOURCES += $(SCIPDIR)/src/scip/nlpi_ipopt.cpp
endif

COINUTILSSRCDIR=$(srcdir)/../../CoinUtils
COINUTILSOBJDIR=../../CoinUtils
CLPSRCDIR=$(srcdir)/../../Clp
CLPOBJDIR=../../Clp
IPOPTSRCDIR=$(srcdir)/../../Ipopt
IPOPTOBJDIR=../../Ipopt

AM_CPPFLAGS += \
 -I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
 -I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(CLPSRCDIR)/src` \
 -I`$(CYGPATH_W) $(CLPOBJDIR)/inc` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Interfaces` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Common` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/LinAlg` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/Algorithm` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/CGPenalty` \
 -I`$(CYGPATH_W) $(IPOPTSRCDIR)/src/contrib/LinearSolverLoader` \
 -I`$(CYGPATH_W) $(IPOPTOBJDIR)/inc`


DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)`

libcoinscip_la_LDFLAGS = $(LT_LDFLAGS)

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES = $(MAKEOKFILE)

include ../../BuildTools/Makemain.inc
