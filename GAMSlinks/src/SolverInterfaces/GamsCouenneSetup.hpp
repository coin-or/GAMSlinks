// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// this is an adapted version of BonCouenneSetup to use GAMS instead of ASL

#ifndef GamsCouenneSetup_H
#define GamsCouenneSetup_H
#include "BonBabSetupBase.hpp"
//#include "CbcFeasibilityBase.hpp"

class CouenneCutGenerator;
class CouenneProblem;
class expression;
class GamsMINLP;

class GamsCouenneSetup : public Bonmin::BabSetupBase {
private:
    /// hold a reference to Couenne cut generator to delete it at
  /// last. The alternative would be to clone it every time the
  /// CouenneSolverInterface is cloned (that is, at each call of
  /// Optimality-based bound tightening).
  CouenneCutGenerator *CouennePtr_;
  
  struct gmoRec* gmo;
  
  bool setupProblem(CouenneProblem* prob);
  expression* parseGamsInstructions(CouenneProblem* prob, int codelen, int* opcodes, int* fields, int constantlen, double* constants);

public:
  /** Default constructor*/
  GamsCouenneSetup(struct gmoRec* gmo_) :
  Bonmin::BabSetupBase(), CouennePtr_(NULL), gmo(gmo_)
  { }

  /** virtual copy constructor.*/
  virtual Bonmin::BabSetupBase* clone() const {
    return new GamsCouenneSetup(*this);
  }
  
  /// destructor
  virtual ~GamsCouenneSetup();

  bool InitializeCouenne(Ipopt::SmartPtr<GamsMINLP> minlp);

  /** register the options */
  virtual void registerOptions();
  /** Register all Couenne options.*/
  static void registerAllOptions(Ipopt::SmartPtr<Bonmin::RegisteredOptions> roptions);
  
  /** Get the basic options if don't already have them.*/
  virtual void readOptionsFile(){}

  /// return pointer to cut generator (used to get pointer to problem)
  CouenneCutGenerator *couennePtr () const
  {return CouennePtr_;}

  /// add cut generators
  void addMilpCutGenerators();

  /// modify parameter (used for MaxTime)
  inline void setDoubleParameter(const Bonmin::BabSetupBase::DoubleParameter& p, const double val)
  { doubleParam_[p] = val; }

  /// modify parameter (used for MaxTime)
	inline double getDoubleParameter(const Bonmin::BabSetupBase::DoubleParameter& p) const
	{ return doubleParam_ [p]; }
};

#endif
