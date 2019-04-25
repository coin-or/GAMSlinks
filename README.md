# COIN-OR/GAMSlinks

## Introduction

This project is dedicated to the development of links between [GAMS](http://www.gams.com) ( **G**eneral **A**lgebraic **M**odeling **S**ystem) and open source solvers.
The links are written in C++ and are released as open source code under the Eclipse Public Licence (EPL) 1.0.
The COIN-OR project leader for GAMSlinks is [Stefan Vigerske](http://www.gams.com/~stefan) (GAMS Software GmbH).

Currently the following links are available:

  * [https://projects.coin-or.org/Bonmin Bonmin]: Basic Open-source Nonlinear Mixed-Integer Programming
  * [https://projects.coin-or.org/Couenne Couenne] Convex Over and Under Envelopes for Nonlinear Estimation
  * [https://github.com/coin-or/Cbc Cbc]: Coin Branch and Cut code
  * [https://github.com/coin-or/Ipopt Ipopt]: Interior Point Optimizer
  * [https://github.com/coin-or/Osi Osi]: Interface to LP and MIP solvers with an Open Solver Interface, currently CPLEX, Gurobi, Mosek, and Xpress
  * [https://projects.coin-or.org/OS OS]: Optimization Services
  * [https://scip.zib.de SCIP]: Solving Constraint Integer Programs
  * [https://soplex.zib.de SoPlex]: Sequential object-oriented simPlex

Note, that these solver links are also distributed with any regular GAMS distribution.

Note, that one still requires a **licensed GAMS base system** to use the solvers via GAMS.

## Download / Installation ==

The links should work under Linux and Mac OS X (both 64 bit), and maybe Windows.

The latest version is available in trunk or master. The stable branches do not receive much updates.
You can obtain GAMSlinks/trunk via subversion by typing
```
svn checkout https://projects.coin-or.org/svn/GAMSlinks/trunk GAMSlinks
```
See also the [COIN-OR documentation for the download of source code](https://projects.coin-or.org/BuildTools/wiki/user-download) and [current issues](https://projects.coin-or.org/BuildTools/wiki/current-issues) for building COIN-OR code.
Especially the build under Windows is difficult and not suggested.

The main installation steps are:

 1. Install a GAMS system, if you do not have one.
 2. Download third-party software.
 3. Call configure, make, and make install.
 4. Install the GAMS/COIN-OR links in your GAMS system by calling make gams-install.

### 1. Installation of a GAMS system

A GAMS system is needed to build the GAMS solver interfaces.

You can download a GAMS demonstration system at http://www.gams.com/latest.
The demonstrations limits are:
 * Number of constraints: 300
 * Number of variables: 300
 * Number of nonzero elements: 2000 (of which 1000 nonlinear)
 * Number of discrete variables: 50 (including semi continuous, semi integer and member of SOS-Sets)

### 2. Download third-party software.

Several solvers require third-party software, e.g., for linear algebra or matrix ordering.
Also to build interfaces for SoPlex and SCIP, these solvers need to be made available.
Go through the directories in directory ThirdParty/ to check which third-party software you need
and make the source code available. For some packages, a `get.*` script to download the code is
available. For others, follow the instructions in INSTALL.

### 3. Configure and make COIN-OR/GAMS links

We suggest to compile the code in a different place than the one where the source code is located using the VPATH feature of the build system.

Now you can use the GAMSlinks build system.
That is, you just call
 1. configure
 2. make
 3. make install

This should setup the Makefiles, compile all COIN-OR packages and install binaries for the COIN-OR/GAMSlinks in the subdirectory bin.

If a GAMS system is found in the search path, it will automatically be found by the GAMSlinks configure script.
Alternatively you can provide a path with the `--with-gams` option of configure.

### 4. Installation of solvers in your GAMS system

By calling "make gams-install", the GAMS system is made aware of the build solver interfaces.

To use a solver under GAMS, use `SOLVER=MY<SolverName>`, e.g., use `SOLVER=MYCBC` to call your build of the GAMS/Cbc link.
The prefix "`MY`" has been added to distinguish the solvers from those that are already included in the GAMS distribution.

## Usage / Documentation

After installing the COIN-OR solvers in your GAMS system, you can use the COIN-OR solvers as every other GAMS solver.
For example, to use Ipopt as an NLP solver, you can use the following statement (before the solve statement) inside your GAMS program to specify using Ipopt:
```
  Option NLP = MyIpopt;
```
Another way is to give the argument `NLP=MyIpopt` to your gams call.

For more information we refer to the [GAMS documentation](http://www.gams.com/latest/docs).

For a documentation of the GAMS/COIN-OR links and their parameters please see the [GAMS solver manuals](http://www.gams.com/latest/docs/S_MAIN.html).

## Testing

The directory `GAMSlinks/test` contains scripts that test Cbc, Ipopt, Bonmin, Couenne, Osi, SoPlex, and SCIP on some models from the GAMS test and model libraries.

## Project Links

  * [http://www.coin-or.org COIN-OR Initiative]

  * [http://www.gams.com GAMS]

  * [http://www.coin-or.org/Doxygen/GAMSlinks Documentation of GAMSlinks classes]

  * [http://list.coin-or.org/mailman/listinfo/gamslinks GAMSlinks mailing list]
