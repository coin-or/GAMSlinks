// Copyright (C) GAMS Development and others 2009
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: GamsBonmin.hpp 652 2009-04-11 17:14:51Z stefan $
//
// this is an adapted version of BonCouenneSetup to use GAMS instead of ASL

#ifndef GamsCouenneSetup_H
#define GamsCouenneSetup_H
#include "BonBabSetupBase.hpp"
//#include "CbcFeasibilityBase.hpp"

class CouenneCutGenerator;
class GamsMINLP;

class GamsCouenneSetup : public BabSetupBase {
private:
    /// hold a reference to Couenne cut generator to delete it at
  /// last. The alternative would be to clone it every time the
  /// CouenneSolverInterface is cloned (that is, at each call of
  /// Optimality-based bound tightening).
  CouenneCutGenerator *CouennePtr_;
  
  struct gmoRec* gmo;
  
public:
  /** Default constructor*/
  GamsCouenneSetup(struct gmoRec* gmo_) :
  BabSetupBase(), CouennePtr_(NULL), gmo(gmo_)
  { }

  /** Copy constructor.*/
  GamsCouenneSetup(const GamsCouenneSetup& other):
  BabSetupBase(other), CouennePtr_(other.CouennePtr_), gmo(gmo_)
  { }
  
  /** virtual copy constructor.*/
  virtual BabSetupBase * clone() const {
    return new GamsCouenneSetup(*this);
  }
  
  /// destructor
  virtual ~GamsCouenneSetup();

  bool InitializeCouenne(SmartPtr<GamsMINLP> minlp, CouenneProblem* problem);

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
  inline void setDoubleParameter (const DoubleParameter &p, const double val)
  {doubleParam_ [p] = val;}

  /// modify parameter (used for MaxTime)
	inline double getDoubleParameter (const DoubleParameter &p) const
	{return doubleParam_ [p];}
};

#endif
