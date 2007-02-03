// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author:  Stefan Vigerske

#ifndef __CBCSTRATEGYGAMS_HPP__
#define __CBCSTRATEGYGAMS_HPP__

#include "GAMSlinksConfig.h"
#include "GamsModel.hpp"

#include "CbcModel.hpp"
#include "CbcStrategy.hpp"

/** The CbcStrategy to use for Gams/CoinCbc.
 * This CbcStrategy is copied in large portions from John Forrest's CbcStrategyDefault,
 * but it takes care of options set in a Cbc option file.
 */ 
class CbcStrategyGams : public CbcStrategy {
public:
  /** Default Constructor.
   * @param gm_ A GamsModel, needed to read options.
   */ 
  CbcStrategyGams(GamsModel& gm_);
  /** Copy constructor.
   */ 
  CbcStrategyGams(const CbcStrategyGams& rhs);
   
  /** Destructor.
   */ 
  ~CbcStrategyGams() { }
  
  /** Cloning method.
   */
  virtual CbcStrategy* clone() const;

  /** Setup for cut generators.
   */
  virtual void setupCutGenerators(CbcModel & model);
  /** Setup for heuristics.
   */
  virtual void setupHeuristics(CbcModel & model);
  /** Setup for printing stuff.
   */
  virtual void setupPrinting(CbcModel & model,int modelLogLevel);
  /** Setup for other stuff, e.g., preprocessing and strong branching.
   */
  virtual void setupOther(CbcModel & model);
  /** Creates C++ lines to get to current state.
   */
  virtual void generateCpp(FILE * fp);

private:
  CbcStrategyDefault & operator=(const CbcStrategyDefault&);
  
  GamsModel& gm;
};


#endif /*__CBCSTRATEGYGAMS_HPP__*/
