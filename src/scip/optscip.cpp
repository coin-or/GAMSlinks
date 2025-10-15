/** Copyright (C) GAMS Development and others
  * All Rights Reserved.
  * This code is published under the Eclipse Public License.
  *
  * @file optscip.cpp
  * @author Stefan Vigerske
 */

#include "GamsLinksConfig.h"
#include "GamsOptionsSpecWriter.hpp"

#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/pub_paramset.h"
#include "GamsScip.hpp"
#ifdef GAMS_BUILD
#include "lpiswitch.h"
#endif

#include <map>
#include <iostream>
#include <fstream>

static
int ScipLongintToInt(
   SCIP_Longint              value
)
{
   if( value >=  SCIP_LONGINT_MAX )
      return INT_MAX;
   if( value <= -SCIP_LONGINT_MAX )
      return -INT_MAX;
   // if (value > INT_MAX) {
   //    std::cout << "Warning, chop long int value " << value << " to max integer = " << INT_MAX << std::endl;
   //    return INT_MAX;
   // }
   // if (value < -INT_MAX) {
   //    std::cout << "Warning, chop long int value " << value << " to min integer = " << -INT_MAX << std::endl;
   //    return -INT_MAX;
   // }
   return (int)value;
}

static
bool ScipParamCompare(SCIP_PARAM* a, SCIP_PARAM* b)
{
   /* move advanced parameters to end */
   if( SCIPparamIsAdvanced(a) == SCIPparamIsAdvanced(b) )
   {
#if 0
      int nslasha = 0;
      for( const char* c = SCIPparamGetName(a); *c ; ++c )
         if( *c == '/' )
            ++nslasha;
      int nslashb = 0;
      for( const char* c = SCIPparamGetName(b); *c ; ++c )
         if( *c == '/' )
            ++nslashb;

      if( nslasha == nslashb )
         return strcasecmp(SCIPparamGetName(a), SCIPparamGetName(b)) < 0;
      return nslasha < nslashb;
#else
      return strcasecmp(SCIPparamGetName(a), SCIPparamGetName(b)) < 0;
#endif
   }
   return SCIPparamIsAdvanced(b);
}

static
void printPluginTables(
   SCIP* scip
   )
{
   /* benders skipped so far as only default */

   /* branching rules */
   {
      std::ofstream outfile("branchrules.md");
      std::cout << "Writing branchrules.md" << std::endl;
      outfile << std::endl;
      outfile << "| branching rule | priority | maxdepth | maxbounddist | description |" << std::endl;
      outfile << "|:---------------|---------:|---------:|-------------:|:------------|" << std::endl;

      int nbranchrules = SCIPgetNBranchrules(scip);
      SCIP_BRANCHRULE** sorted;
      SCIP_CALL_ABORT( SCIPduplicateBufferArray(scip, &sorted, SCIPgetBranchrules(scip), nbranchrules) );
      SCIPsortPtr((void**)sorted, SCIPbranchruleComp, nbranchrules);
      for( int i = 0; i < nbranchrules; ++i )
      {
         outfile << "| \\ref SCIP_gr_branching_" << SCIPbranchruleGetName(sorted[i]) << " \"" << SCIPbranchruleGetName(sorted[i]) << '"';
         outfile << " | " << SCIPbranchruleGetPriority(sorted[i]);
         outfile << " | " << SCIPbranchruleGetMaxdepth(sorted[i]);
         outfile << " | " << 100.0 * SCIPbranchruleGetMaxbounddist(sorted[i]);
         outfile << " | " << SCIPbranchruleGetDesc(sorted[i]);
         outfile << " |" << std::endl;
      }
      SCIPfreeBufferArray(scip, &sorted);
   }

   /* compression methods skipped so far as reopt not available */

   /* conflict handler */
   {
      SCIP_CONFLICTHDLR** conflicthdlrs = SCIPgetConflicthdlrs(scip);
      SCIP_CONFLICTHDLR** sorted;
      int nconflicthdlrs = SCIPgetNConflicthdlrs(scip);
      SCIP_CALL_ABORT( SCIPduplicateBufferArray(scip, &sorted, conflicthdlrs, nconflicthdlrs) );
      SCIPsortPtr((void**)sorted, SCIPconflicthdlrComp, nconflicthdlrs);

      std::ofstream outfile("conflicthdlrs.md");
      std::cout << "Writing conflicthdlrs.md" << std::endl;
      outfile << std::endl;
      outfile << "| conflict handler | priority | description |" << std::endl;
      outfile << "|:-----------------|---------:|:------------|" << std::endl;

      for( int i = 0; i < nconflicthdlrs; ++i )
      {
         outfile << "| \\ref SCIP_gr_conflict_" << SCIPconflicthdlrGetName(sorted[i]) << " \"" << SCIPconflicthdlrGetName(sorted[i]) << '"';
         outfile << " | " << SCIPconflicthdlrGetPriority(sorted[i]);
         outfile << " | " << SCIPconflicthdlrGetDesc(sorted[i]);
         outfile << " |" << std::endl;
      }
      SCIPfreeBufferArray(scip, &sorted);
   }

   /* constraint handler */
   {
      std::ofstream outfile("conshdlrs.md");
      std::cout << "Writing conshdlrs.md" << std::endl;
      outfile << std::endl;
      outfile << "| constraint handler | checkprio | enfoprio | sepaprio | sepafreq | propfreq | eagerfreq | presolvetimings | description |" << std::endl;
      outfile << "|:-------------------|----------:|---------:|---------:|---------:|---------:|----------:|:---------------:|:------------|" << std::endl;
      SCIP_CONSHDLR** conshdlrs = SCIPgetConshdlrs(scip);
      for( int i = 0; i < SCIPgetNConshdlrs(scip); ++i )
      {
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "benders") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "benderslp") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "cardinality") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "conjunction") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "countsols") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "cumulative") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "disjunction") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "linking") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "pseudoboolean") == 0 )
            continue;
         if( strcmp(SCIPconshdlrGetName(conshdlrs[i]), "superindicator") == 0 )
            continue;

         outfile << "| \\ref SCIP_gr_constraints_" << SCIPconshdlrGetName(conshdlrs[i]) << " \"" << SCIPconshdlrGetName(conshdlrs[i]) << '"';
         outfile << " | " << SCIPconshdlrGetCheckPriority(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetEnfoPriority(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetSepaPriority(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetSepaFreq(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetPropFreq(conshdlrs[i]);
         outfile << " | " << SCIPconshdlrGetEagerFreq(conshdlrs[i]);
         if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) == SCIP_PRESOLTIMING_ALWAYS )
            outfile << " | always";
         else
         {
            outfile << " |";
            if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) & SCIP_PRESOLTIMING_FAST )
               outfile << " fast";
            if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) & SCIP_PRESOLTIMING_MEDIUM )
               outfile << " medium";
            if( SCIPconshdlrGetPresolTiming(conshdlrs[i]) & SCIP_PRESOLTIMING_EXHAUSTIVE )
               outfile << " exhaustive";
         }
         outfile << " | " << SCIPconshdlrGetDesc(conshdlrs[i]);
         outfile << " |" << std::endl;
      }
   }

   /* cut selectors */
   {
      std::ofstream outfile("cutsels.md");
      std::cout << "Writing cutsels.md" << std::endl;
      outfile << std::endl;
      outfile << "| cut selector | priority| description |" << std::endl;
      outfile << "|:-------------|--------:|:------------|" << std::endl;
      SCIP_CUTSEL** cutsels = SCIPgetCutsels(scip);
      for( int i = 0; i < SCIPgetNCutsels(scip); ++i )
      {
         outfile << "| \\ref SCIP_gr_cutselection_" << SCIPcutselGetName(cutsels[i]) << " \"" << SCIPcutselGetName(cutsels[i]) << '"';
         outfile << " | " << SCIPcutselGetPriority(cutsels[i]);
         outfile << " | " << SCIPcutselGetDesc(cutsels[i]);
         outfile << " |" << std::endl;
      }
   }

   /* display columns */
   {
      SCIP_DISP** disps = SCIPgetDisps(scip);
      int ndisps = SCIPgetNDisps(scip);

      std::ofstream outfile("disps.md");
      std::cout << "Writing disps.md" << std::endl;
      outfile << std::endl;
      outfile << "| display column | header | position | width | priority | status | description |" << std::endl;
      outfile << "|:---------------|:-------|---------:|------:|---------:|-------:|:------------|" << std::endl;
      for( int i = 0; i < ndisps; ++i )
      {
         if( strstr(SCIPdispGetName(disps[i]), "conc") == SCIPdispGetName(disps[i]) )
            continue;
         outfile << "| \\ref SCIP_gr_display_" << SCIPdispGetName(disps[i]) << " \"" << SCIPdispGetName(disps[i]) << '"';
         outfile << " | " << SCIPdispGetHeader(disps[i]);
         outfile << " | " << SCIPdispGetPosition(disps[i]);
         outfile << " | " << SCIPdispGetWidth(disps[i]);
         outfile << " | " << SCIPdispGetPriority(disps[i]);
         switch( SCIPdispGetStatus(disps[i]) )
         {
         case SCIP_DISPSTATUS_OFF:
            outfile << " | off";
            break;
         case SCIP_DISPSTATUS_AUTO:
            outfile << " | auto";
            break;
         case SCIP_DISPSTATUS_ON:
            outfile << " | on";
            break;
         default:
            outfile << " | ?";
            break;
         }
         outfile << " | ";

         /* escape "|" */
         char* descr = strdup(SCIPdispGetDesc(disps[i]));
         char* desc = descr;
         char* rest = desc;
         while( (rest = strstr(desc, "|")) != NULL )
         {
            *rest = '\0';
            outfile << desc << "\\|";
            desc = rest+1;
         }
         outfile << desc;
         free(descr);

         outfile << " |" << std::endl;
      }
   }

   /* expression handler skipped so far as no interesting info and not all come with parameters */

   /* primal heuristics */
   {
      std::ofstream outfile("heurs.md");
      std::cout << "Writing heurs.md" << std::endl;
      outfile << std::endl;
      outfile << "| primal heuristic | type | priority | freq | freqoffset | description |" << std::endl;
      outfile << "|:-----------------|:----:|---------:|-----:|-----------:|:------------|" << std::endl;

      int nheurs = SCIPgetNHeurs(scip);
      SCIP_HEUR** heurs = SCIPgetHeurs(scip);
      for( int h = 0; h < nheurs; ++h )
      {
         outfile << "| \\ref SCIP_gr_heuristics_" << SCIPheurGetName(heurs[h]) << " \"" << SCIPheurGetName(heurs[h]) << '"';
         outfile << " | " << SCIPheurGetDispchar(heurs[h]);
         outfile << " | " << SCIPheurGetPriority(heurs[h]);
         outfile << " | " << SCIPheurGetFreq(heurs[h]);
         outfile << " | " << SCIPheurGetFreqofs(heurs[h]);
         outfile << " | " << SCIPheurGetDesc(heurs[h]);
         outfile << " |" << std::endl;
      }
   }

   /* nonlinear handler */
   {
      SCIP_CONSHDLR* conshdlr;
      SCIP_NLHDLR** nlhdlrs;
      int nnlhdlrs;
      int i;

      conshdlr = SCIPfindConshdlr(scip, "nonlinear");
      assert(conshdlr != NULL);

      nlhdlrs = SCIPgetNlhdlrsNonlinear(conshdlr);
      nnlhdlrs = SCIPgetNNlhdlrsNonlinear(conshdlr);

      std::ofstream outfile("nlhdlrs.md");
      std::cout << "Writing nlhdlrs.md" << std::endl;
      outfile << std::endl;
      outfile << "| nonlinear handler | enabled | detect priority | enforce priority | description |" << std::endl;
      outfile << "|:------------------|:-------:|----------------:|-----------------:|:------------|" << std::endl;
      for( i = 0; i < nnlhdlrs; ++i )
      {
         outfile << "| \\ref SCIP_gr_nlhdlr_" << SCIPnlhdlrGetName(nlhdlrs[i]) << " \"" << SCIPnlhdlrGetName(nlhdlrs[i]) << '"';
         outfile << " | " << SCIPnlhdlrIsEnabled(nlhdlrs[i]);
         outfile << " | " << SCIPnlhdlrGetDetectPriority(nlhdlrs[i]);
         outfile << " | " << SCIPnlhdlrGetEnfoPriority(nlhdlrs[i]);
         outfile << " | " << SCIPnlhdlrGetDesc(nlhdlrs[i]);
         outfile << " |" << std::endl;
      }
   }

   /* nlpis skipped so far, as only one with GAMS */

   /* node selectors */
   {
      SCIP_NODESEL** nodesels;
      int nnodesels;
      int i;

      nodesels = SCIPgetNodesels(scip);
      nnodesels = SCIPgetNNodesels(scip);

      std::ofstream outfile("nodesels.md");
      std::cout << "Writing nodesels.md" << std::endl;
      outfile << std::endl;
      outfile << "| node selector | standard priority | memsave priority | description |" << std::endl;
      outfile << "|:--------------|------------------:|-----------------:|:------------|" << std::endl;
      for( i = 0; i < nnodesels; ++i )
      {
         outfile << "| \\ref SCIP_gr_nodeselection_" << SCIPnodeselGetName(nodesels[i]) << " \"" << SCIPnodeselGetName(nodesels[i]) << '"';
         outfile << " | " << SCIPnodeselGetStdPriority(nodesels[i]);
         outfile << " | " << SCIPnodeselGetMemsavePriority(nodesels[i]);
         outfile << " | " << SCIPnodeselGetDesc(nodesels[i]);
         outfile << " |" << std::endl;
      }
   }

   /* presolvers */
   {
      SCIP_PRESOL** presols;
      int npresols;
      int i;

      presols = SCIPgetPresols(scip);
      npresols = SCIPgetNPresols(scip);

      std::ofstream outfile("presols.md");
      std::cout << "Writing presols.md" << std::endl;
      outfile << std::endl;
      outfile << "| presolver | priority | timing | maxrounds | description |" << std::endl;
      outfile << "|:----------|---------:|:------:|----------:|:------------|" << std::endl;
      for( i = 0; i < npresols; ++i )
      {
         outfile << "| \\ref SCIP_gr_presolving_" << SCIPpresolGetName(presols[i]) << " \"" << SCIPpresolGetName(presols[i]) << '"';
         outfile << " | " << SCIPpresolGetPriority(presols[i]);
         if( SCIPpresolGetTiming(presols[i]) == SCIP_PRESOLTIMING_ALWAYS )
            outfile << " | always";
         else
         {
            outfile << " |";
            if( SCIPpresolGetTiming(presols[i]) & SCIP_PRESOLTIMING_FAST )
               outfile << " fast";
            if( SCIPpresolGetTiming(presols[i]) & SCIP_PRESOLTIMING_MEDIUM )
               outfile << " medium";
            if( SCIPpresolGetTiming(presols[i]) & SCIP_PRESOLTIMING_EXHAUSTIVE )
               outfile << " exhaustive";
         }
         outfile << " | " << SCIPpresolGetMaxrounds(presols[i]);
         outfile << " | " << SCIPpresolGetDesc(presols[i]);
         outfile << " |" << std::endl;
      }
   }

   /* pricers skipped as none */

   /* propagators */
   {
      SCIP_PROP** props;
      int nprops;
      int i;

      props = SCIPgetProps(scip);
      nprops = SCIPgetNProps(scip);

      std::ofstream outfile("props.md");
      std::cout << "Writing props.md" << std::endl;
      outfile << std::endl;
      outfile << "| propagator | propprio | freq | presolveprio | presolvetiming | description |" << std::endl;
      outfile << "|:-----------|---------:|-----:|-------------:|:--------------:|:------------|" << std::endl;
      for( i = 0; i < nprops; ++i )
      {
         outfile << "| \\ref SCIP_gr_propagating_" << SCIPpropGetName(props[i]) << " \"" << SCIPpropGetName(props[i]) << '"';
         outfile << " | " << SCIPpropGetPriority(props[i]) << (SCIPpropIsDelayed(props[i]) ? 'd' : ' ');
         outfile << " | " << SCIPpropGetFreq(props[i]);
         outfile << " | " << SCIPpropGetPresolPriority(props[i]);
         if( SCIPpropGetPresolTiming(props[i]) == SCIP_PRESOLTIMING_ALWAYS )
            outfile << " | always";
         else
         {
            outfile << " |";
            if( SCIPpropGetPresolTiming(props[i]) & SCIP_PRESOLTIMING_FAST )
               outfile << " fast";
            if( SCIPpropGetPresolTiming(props[i]) & SCIP_PRESOLTIMING_MEDIUM )
               outfile << " medium";
            if( SCIPpropGetPresolTiming(props[i]) & SCIP_PRESOLTIMING_EXHAUSTIVE )
               outfile << " exhaustive";
         }
         outfile << " | " << SCIPpropGetDesc(props[i]);
         outfile << " |" << std::endl;
      }
   }

   /* readers skipped as not so relevant in GAMS */

   /* relaxators skipped as none */

   /* separators */
   {
      SCIP_SEPA** sepas;
      int nsepas;
      int i;

      sepas = SCIPgetSepas(scip);
      nsepas = SCIPgetNSepas(scip);

      std::ofstream outfile("sepas.md");
      std::cout << "Writing sepas.md" << std::endl;
      outfile << std::endl;
      outfile << "| separator | priority | freq | bounddist | description |" << std::endl;
      outfile << "|:----------|---------:|-----:|----------:|:------------|" << std::endl;
      for( i = 0; i < nsepas; ++i )
      {
         outfile << "| \\ref SCIP_gr_separating_" << SCIPsepaGetName(sepas[i]) << " \"" << SCIPsepaGetName(sepas[i]) << '"';
         outfile << " | " <<  SCIPsepaGetPriority(sepas[i]) << (SCIPsepaIsDelayed(sepas[i]) ? 'd' : ' ');
         outfile << " | " << SCIPsepaGetFreq(sepas[i]);
         outfile << " | " << SCIPsepaGetMaxbounddist(sepas[i]);
         outfile << " | " << SCIPsepaGetDesc(sepas[i]);
         outfile << " |" << std::endl;
      }
   }
}

int main(int argc, char** argv)
{
   GamsScip* gamsscip;
   SCIP* scip;

#ifdef GAMS_BUILD
   SCIPlpiSwitchSetDefaultSolver();
#endif
   gamsscip = new GamsScip();
   gamsscip->setupSCIP();
   scip = gamsscip->scip;

   static std::map<std::string, std::string> categname;
   categname[" gams"] = "GAMS interface specific options";
   categname["branching"] = "Branching";
   categname["cutselection"] = "Cut Selection";
   categname["conflict"] = "Conflict analysis";
   categname["constraints"] = "Constraints";
   categname["decomposition"] = "Decomposition";
   categname["display"] = "Output";
   categname["estimation"] = "Restarts and Tree Size Estimation";
   categname["expr"] = "Arithemtic Expressions";
   categname["heuristics"] = "Heuristics";
   categname["history"] = "History";
   categname["limits"] = "Limits";
   categname["lp"] = "LP";
   categname["memory"] = "Memory";
   categname["misc"] = "Miscellaneous";
   categname["nlhdlr"] = "Nonlinear Handler";
   categname["nlp"] = "Nonlinear Programming Relaxation";
   categname["nlpi"] = "Nonlinear Programming Solver interfaces";
   categname["nodeselection"] = "Node Selection";
   categname["numerics"] = "Tolerances";
   categname["presolving"] = "Presolving";
   categname["propagating"] = "Domain Propagation";
   categname["randomization"] = "Randomization";
   categname["separating"] = "Separation";
   categname["solvingphases"] = "Solving Phases";
   categname["table"] = "Solve Statistic Tables";
   categname["timing"] = "Timing";

   SCIP_PARAM** params = SCIPgetParams(scip);
   int nparams = SCIPgetNParams(scip);
   SCIP_PARAM* param;

   std::map<std::string, std::list<SCIP_PARAM*> > paramsort;

   GamsOption::Type opttype;
   GamsOption::Value defaultval, minval, maxval;
   GamsOption::EnumVals enumval;
   std::string tmpstr;
   std::string descr;
   std::string longdescr;
   std::string defaultdescr;
   std::string category;

   GamsOptions gmsopt("SCIP");
   gmsopt.setSeparator("=");
   gmsopt.setStringQuote("\"");
   gmsopt.setEolChars("#");

   for( int i = 0; i < nparams; ++i )
   {
      param = params[i];
      const char* paramname = SCIPparamGetName(param);

      if( strcmp(paramname, "numerics/infinity") == 0 )
         continue;

      // options have no effect
      if( strcmp(paramname, "propagating/symmetry/symfixnonbinaryvars") == 0 ||
          strcmp(paramname, "propagating/symmetry/addconsstiming") == 0 ||
          strcmp(paramname, "propagating/symmetry/ofsymcomptiming") == 0 )
         continue;

      if( strstr(paramname, "constraints/benders")       == paramname ||
          strstr(paramname, "constraints/cardinality")   == paramname ||
          strstr(paramname, "constraints/conjunction")   == paramname ||
          strstr(paramname, "constraints/countsols")     == paramname ||
          strstr(paramname, "constraints/cumulative")    == paramname ||
          strstr(paramname, "constraints/disjunction")   == paramname ||
          strstr(paramname, "constraints/linking")       == paramname ||
          strstr(paramname, "constraints/pseudoboolean") == paramname ||
          strstr(paramname, "constraints/superindicator")== paramname ||
          strstr(paramname, "table/benders")             == paramname ||
          strstr(paramname, "decomposition/applybenders") == paramname ||
          strstr(paramname, "decomposition/benderslabels") == paramname ||
          strstr(paramname, "nlp/solver") == paramname
         )
         continue;

      const char* catend = strchr(paramname, '/');
      if( catend != NULL )
         category = std::string(paramname, 0, catend - paramname);
      else
         category = "";

      if( category == "benders" ||
          category == "compression" ||  //for reoptimization
          category == "concurrent" ||
          category == "reading" ||
          category == "reoptimization" ||
          category == "parallel" ||
          category == "pricing" ||
          category == "visual" ||
          category == "write"
        )
         continue;

      if( category == "gams" )
         category = " gams";

      paramsort[category].push_back(param);
   }

   for( std::map<std::string, std::list<SCIP_PARAM*> >::iterator it_categ(paramsort.begin()); it_categ != paramsort.end(); ++it_categ )
   {
      if( !categname.count(it_categ->first) )
      {
         std::cerr << "Error: Do not have name for SCIP option category " << it_categ->first << std::endl;
         return -1;
      }

      it_categ->second.sort(ScipParamCompare);
      bool hadadvanced = false;
      for( std::list<SCIP_PARAM*>::iterator it_param(it_categ->second.begin()); it_param != it_categ->second.end(); ++it_param)
      {
         param = *it_param;
         enumval.clear();
         switch( SCIPparamGetType(param) )
         {
            case SCIP_PARAMTYPE_BOOL:
            {
               opttype = GamsOption::Type::BOOL;
               minval = false;
               maxval = true;
               defaultval = (bool)SCIPparamGetBoolDefault(param);
               break;
            }

            case SCIP_PARAMTYPE_INT:
            {
               opttype = GamsOption::Type::INTEGER;
               minval = SCIPparamGetIntMin(param);
               maxval = SCIPparamGetIntMax(param);
               defaultval = SCIPparamGetIntDefault(param);
               break;
            }

            case SCIP_PARAMTYPE_LONGINT:
            {
               opttype = GamsOption::Type::INTEGER;
               minval = ScipLongintToInt(SCIPparamGetLongintMin(param));
               maxval = ScipLongintToInt(SCIPparamGetLongintMax(param));
               defaultval = ScipLongintToInt(SCIPparamGetLongintDefault(param));
               break;
            }

            case SCIP_PARAMTYPE_REAL:
            {
               opttype = GamsOption::Type::REAL;
               minval = SCIPparamGetRealMin(param);
               maxval = SCIPparamGetRealMax(param);
               defaultval = SCIPparamGetRealDefault(param);
               if( SCIPisInfinity(scip, -minval.realval) )
                  minval = -DBL_MAX;
               if( SCIPisInfinity(scip,  maxval.realval) )
                  maxval =  DBL_MAX;
               if( SCIPisInfinity(scip, ABS(defaultval.realval)) )
                  defaultval = (defaultval.realval < 0 ? -1.0 : 1.0) * DBL_MAX;
               break;
            }

            case SCIP_PARAMTYPE_CHAR:
               opttype = GamsOption::Type::CHAR;
               defaultval = SCIPparamGetCharDefault(param);
               if( SCIPparamGetCharAllowedValues(param) != NULL )
                  for( char* c = SCIPparamGetCharAllowedValues(param); *c != '\0'; ++c )
                     enumval.append(*c);
               break;

            case SCIP_PARAMTYPE_STRING:
               opttype = GamsOption::Type::STRING;
               defaultval = SCIPparamGetStringDefault(param);
               break;

            default:
               std::cerr << "Skip option " << SCIPparamGetName(param) << " of unknown type." << std::endl;
               continue;
         }
         descr = SCIPparamGetDesc(param);
         defaultdescr.clear();

         if( strcmp(SCIPparamGetName(param), "limits/time") == 0 )
         {
            defaultval = 1e10;
#ifdef GAMS_BUILD
            defaultdescr = "\\ref GAMSAOreslim \"GAMS reslim\"";
#else
            defaultdescr = "GAMS reslim";
#endif
         }
         else if( strcmp(SCIPparamGetName(param), "limits/gap") == 0 )
         {
            defaultval = 1e-4;
#ifdef GAMS_BUILD
            defaultdescr = "\\ref GAMSAOoptcr \"GAMS optcr\"";
#else
            defaultdescr = "GAMS optcr";
#endif
         }
         else if( strcmp(SCIPparamGetName(param), "limits/absgap") == 0 )
         {
            defaultval = 0.0;
#ifdef GAMS_BUILD
            defaultdescr = "\\ref GAMSAOoptca \"GAMS optca\"";
#else
            defaultdescr = "GAMS optca";
#endif
         }
         else if( strcmp(SCIPparamGetName(param), "limits/memory") == 0 )
#ifdef GAMS_BUILD
            defaultdescr = "\\ref GAMSAOworkspace \"GAMS workspace\"";
#else
            defaultdescr = "GAMS workspace";
#endif
         else if( strcmp(SCIPparamGetName(param), "limits/nodes") == 0 )
#ifdef GAMS_BUILD
            defaultdescr = "\\ref GAMSAOnodlim \"GAMS nodlim\", if > 0, otherwise -1";
#else
            defaultdescr = "GAMS nodlim, if > 0, otherwise -1";
#endif
         else if( strcmp(SCIPparamGetName(param), "lp/solver") == 0 )
         {
            defaultdescr = "cplex, if licensed, otherwise soplex";
            descr = "LP solver to use (clp, cplex, highs, soplex)";
         }
         else if( strcmp(SCIPparamGetName(param), "lp/threads") == 0 )
#ifdef GAMS_BUILD
            defaultdescr = "\\ref GAMSAOthreads \"GAMS threads\"";
#else
            defaultdescr = "GAMS threads";
#endif
         else if( strcmp(SCIPparamGetName(param), "presolving/milp/threads") == 0 )
#ifdef GAMS_BUILD
            defaultdescr = "\\ref GAMSAOthreads \"GAMS threads\"";
#else
            defaultdescr = "GAMS threads";
#endif
         else if( strcmp(SCIPparamGetName(param), "misc/printreason") == 0 )
            defaultval = false;
         else if( strcmp(SCIPparamGetName(param), "display/lpavgiterations/active") == 0 )
            defaultdescr = "1 (0 for Windows without IDE)";
         else if( strcmp(SCIPparamGetName(param), "display/maxdepth/active") == 0 )
            defaultdescr = "1 (0 for Windows without IDE)";
         else if( strcmp(SCIPparamGetName(param), "display/time/active") == 0 )
            defaultdescr = "1 (2 for Windows without IDE)";
         else if( strcmp(SCIPparamGetName(param), "display/width") == 0 )
            defaultdescr = "143 (80 for Windows without IDE)";
#ifdef GAMS_BUILD
         else if( strcmp(SCIPparamGetName(param), "gams/mipstart") == 0 )
            descr += ", see also section \\ref SCIP_PARTIALSOL";
#endif
         else if( strcmp(SCIPparamGetName(param), "misc/usesymmetry") == 0 )
            descr.erase(descr.find("See type_symmetry.h."));
         else if( strcmp(SCIPparamGetName(param), "constraints/nonlinear/linearizeheursol") == 0 )
            defaultdescr = "i if QCP or NLP, o otherwise";
         else if( strcmp(SCIPparamGetName(param), "nlpi/ipopt/linear_solver") == 0 )
            defaultdescr = "ma27, if IpoptH licensed, mumps otherwise";
         else if( strcmp(SCIPparamGetName(param), "nlpi/ipopt/linear_system_scaling") == 0 )
            defaultdescr = "mc19, if IpoptH licensed, empty otherwise";

         if( !hadadvanced && SCIPparamIsAdvanced(param) )
            hadadvanced = true;

         longdescr = "";
         std::string::size_type nlpos = descr.find("\n");
         if( nlpos != std::string::npos )
         {
            longdescr = std::string(descr, nlpos+1);
            descr = std::string(descr, 0, nlpos);
         }

         tmpstr = SCIPparamGetName(param);

         char* lastslash = strrchr(const_cast<char*>(SCIPparamGetName(param)), '/');
         if( lastslash == NULL )
         {
            category = "";
         }
         else
         {
            category = std::string(SCIPparamGetName(param), 0, lastslash - SCIPparamGetName(param));
            if( category.find("gams") == 0 )
               category = " " + category;
         }
         gmsopt.setGroup(category);

         GamsOption& gopt = gmsopt.collect(SCIPparamGetName(param), descr, longdescr,
            opttype, defaultval, minval, maxval, true, true, enumval, defaultdescr);
         gopt.advanced = SCIPparamIsAdvanced(param);
      }
   }
   gmsopt.finalize();

   // gmsopt.writeDef();
#ifdef GAMS_BUILD
   gmsopt.writeDoxygen(true);
   printPluginTables(scip);
#else
   gmsopt.writeMarkdown();
#endif

   delete gamsscip;
}
