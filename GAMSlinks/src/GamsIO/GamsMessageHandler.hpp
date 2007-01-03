// Copyright (C) GAMS Development 2006
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors: Michael Bussieck, Stefan Vigerske

#ifndef GamsMessageHandler_H
#define GamsMessageHandler_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinMessageHandler.hpp"
#include "GamsModel.hpp"

class GamsMessageHandler : public CoinMessageHandler {

public:
  
  GamsMessageHandler();
  int print() ;
  void setGamsModel(GamsModel *GMptr);
  inline void setRemoveLBlanks(int rm) { rmlblanks_ = rm; }

private:

  GamsModel *GMptr_;
  int rmlblanks_;
};

#endif // GamsMessageHandler_H
