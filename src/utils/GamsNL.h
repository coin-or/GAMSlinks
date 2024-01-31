// Copyright (C) GAMS Development and others
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Author: Stefan Vigerske

#ifndef GAMSNL_H_
#define GAMSNL_H_

#include "def.h"
#include "GamsNLinstr.h"

typedef enum
{
   gamsnl_osil,
   gamsnl_ampl,
   gamsnl_gurobi
} gamsnl_mode;

typedef enum
{
   gamsnl_opvar,
   gamsnl_opconst,
   gamsnl_opsum,
   gamsnl_opprod,
   gamsnl_opmin,
   gamsnl_opmax,
   gamsnl_opand,
   gamsnl_opor,
   gamsnl_opsub,
   gamsnl_opdiv,
   gamsnl_opnegate,
   gamsnl_opfunc
} gamsnl_opcode;

typedef struct gamsnl_node_s gamsnl_node;
struct gamsnl_node_s
{
   gamsnl_opcode op;

   GamsFuncCode  func;
   int           varidx;
   double        coef;

   gamsnl_node** args;
   int           nargs;
   int           argssize;
};

extern
RETURN gamsnlCreate(
   gamsnl_node**  n,
   gamsnl_opcode  op
   );

extern
RETURN gamsnlCreateVar(
   gamsnl_node**  n,
   int            varidx
   );

extern
void gamsnlFree(
   gamsnl_node**  n
   );

extern
RETURN gamsnlCopy(
   gamsnl_node**  target,
   gamsnl_node*   src
   );

/* should be used for sum, product, min, and max only */
extern
RETURN gamsnlAddArg(
   gamsnl_node*   n,
   gamsnl_node*   arg
   );

/* should be used for sum, product, min, and max only */
extern
RETURN gamsnlAddArgFront(
   gamsnl_node*   n,
   gamsnl_node*   arg
   );

extern
void gamsnlPrint(
   gamsnl_node*   n
   );

extern
RETURN gamsnlParseGamsInstructions(
   gamsnl_node**   nl,                 /**< buffer to store created root node */
   struct gmoRec*  gmo,                /**< GMO */
   int             codelen,            /**< length of GAMS instructions */
   int*            opcodes,            /**< opcodes of GAMS instructions */
   int*            fields,             /**< fields of GAMS instructions */
   double*         constants,          /**< GAMS constants pool */
   double          factor,             /**< extra factor to multiply expression with */
   double          constant,           /**< extra constrant to add to expression (after applying factor) */
   gamsnl_mode     mode                /**< for which purpose the nl is created */
);

#endif /* GAMSNL_H_ */
