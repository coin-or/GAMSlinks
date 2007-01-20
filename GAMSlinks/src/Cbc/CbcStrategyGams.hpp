// Copyright (C) GAMS Development 2007
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Author:  Stefan Vigerske
//
// This CbcStrategy is copied in large portions from John Forrest's CbcStrategyDefault.

#ifndef __CBCSTRATEGYGAMS_HPP__
#define __CBCSTRATEGYGAMS_HPP__

#include "GAMSlinksConfig.h"
#include "GamsModel.hpp"

#include "CbcModel.hpp"
#include "CbcStrategy.hpp"

class CbcStrategyGams : public CbcStrategy {
public:
  // Default Constructor 
  CbcStrategyGams(GamsModel& gm_);
  // Copy constructor 
  CbcStrategyGams(const CbcStrategyGams& rhs);
   
  // Destructor 
  ~CbcStrategyGams() { }
  
  /// Clone
  virtual CbcStrategy* clone() const;

  /// Setup cut generators
  virtual void setupCutGenerators(CbcModel & model);
  /// Setup heuristics
  virtual void setupHeuristics(CbcModel & model);
  /// Do printing stuff
  virtual void setupPrinting(CbcModel & model,int modelLogLevel);
  /// Other stuff: preprocessing and strong branching
  virtual void setupOther(CbcModel & model);
  /// Create C++ lines to get to current state
  virtual void generateCpp(FILE * fp);

private:
  // Illegal Assignment operator 
  CbcStrategyDefault & operator=(const CbcStrategyDefault&);
  
  // Data
  GamsModel& gm;
};


#endif /*__CBCSTRATEGYGAMS_HPP__*/
