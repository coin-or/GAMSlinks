# COIN-OR/GAMSlinks

## Introduction

This project is dedicated to the development of links between [GAMS](http://www.gams.com) (**G**eneral **A**lgebraic **M**odeling **S**ystem) and some solvers,
including some of the [COIN-OR](https://www.coin-or.org) open source solvers.
The links are written in C++ and are released as open source code under the Eclipse Public Licence (EPL) 2.0.
The COIN-OR project leader for GAMSlinks is [Stefan Vigerske](http://www.gams.com/~svigerske) (GAMS Software GmbH).

Currently the following links are available:

  * [Bonmin](https://github.com/coin-or/Bonmin): Basic Open-source Nonlinear Mixed-Integer Programming
  * [Couenne](https://github.com/coin-or/Couenne): Convex Over and Under Envelopes for Nonlinear Estimation
  * [Cbc](https://github.com/coin-or/Cbc): COIN-OR Branch and Cut code
  * [Ipopt](https://github.com/coin-or/Ipopt): Interior Point Optimizer
  * [Osi](https://github.com/coin-or/Osi): Interface to LP and MIP solvers with an Open Solver Interface, currently CPLEX, Gurobi, Mosek, and Xpress
  * [SCIP](https://www.scipopt.org): Solving Constraint Integer Programs
  * [SoPlex](https://soplex.zib.de): Sequential object-oriented simPlex

Note that most of these solver links are also distributed with any regular GAMS distribution.

Note that one still requires a **licensed GAMS base system** to use the solvers via GAMS.

## Download / Installation

The links should work under Linux, macOS, and Windows.
The build under Windows can be difficult.

The latest version is available in the master branch. The stable branches do no longer receive updates.
One can obtain GAMSlinks via git by typing
```
git clone https://github.com/coin-or/GAMSlinks.git
```

The main installation steps are:

 1. Install a GAMS system, if not yet present.
 2. Install solvers.
 3. Call configure, make, and make install.
 4. Update GAMS configuration.

### 1. Installation of a GAMS system

A GAMS system is needed to build and use the GAMS solver interfaces.
It can be downloaded from https://www.gams.com/latest.
This page also provides a mechanism to obtain a GAMS demo license.

### 2. Install solvers

To build the interface to a certain solver, this solver needs to be installed in the system first.
Check with the solvers instructions on how to build and install.

### 3. Call configure, make, and make install

Call
 1. configure
 2. make
 3. make install

This should setup the Makefiles, compile all solver interfaces and install
- libraries of the solver links in `<prefix>/lib` (`<prefix>/bin` on Windows),
- a `gamsconfig.yaml` file in `<prefix>/share/gamslinks`,
- solver options definition files in `<prefix>/share/gamslinks`, and
- solver options documentation in `<prefix>/share/doc/gamslinks`.

If a GAMS system is found in the search path, it will automatically be found by the GAMSlinks configure script.
Alternatively one has to provide a path with the `--with-gams` option of configure.

The configure call also supports the VPATH feature, so one can compile the code in a different place than the one where the source code is located.

### 4. Update GAMS configuration

After installation of the solvers, GAMS needs to be made aware of them.

For that, copy the prepared GAMS configuration file `<prefix>/share/gamslinks/gamsconfig.yaml` into the GAMS system directory or another
[location where GAMS is looking for this file](https://www.gams.com/latest/docs/UG_STANDARD_LOCATIONS.html).
If you already have a gamsconfig.yaml, then append the content of `<prefix>/share/gamslinks/gamsconfig.yaml`.

## Usage

The build solver links should be usable in GAMS via the option `SOLVER=<SolverName>`, e.g., use `SOLVER=CBC` to call the installed GAMS/CBC link.
Alternatively, an option statement can be added to the GAMS code (before the solve statement), e.g., to use Ipopt as an NLP solver, use
```
  Option NLP = Ipopt;
```
Note, that entries in gamsconfig.yaml overwrite solvers with the same name that come with the GAMS system.

For more information we refer to the [GAMS documentation](http://www.gams.com/latest/docs).

## Testing

The directory `test` contains scripts that test Cbc, Ipopt, Bonmin, Couenne, OsiXyz, SoPlex, and SCIP on some models from the GAMS test and model libraries.
