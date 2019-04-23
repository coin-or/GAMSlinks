unit gxdefs;
// Description:
//  used by gxfile.pas and any program needing
//  the constants and types for using the gdxio.dll

{$A+ use aligned records}
{$H- short strings}

interface

uses
   gmsspecs;

const
   DOMC_UNMAPPED = -2;        // Indicator for an unmapped index position
   DOMC_EXPAND   = -1;        // Indicator for a growing index position
   DOMC_STRICT   =  0;        // Indicator for a mapped index position

type
   PGXFile = pointer;  // Pointer to a GDX data structure

   TgdxSpecialValue = (
      sv_valund,       // 0: Undefined
      sv_valna,        // 1: Not Available
      sv_valpin,       // 2: Positive Infinity
      sv_valmin,       // 3: Negative Infinity
      sv_valeps,       // 4: Epsilon
      sv_normal,       // 5: Normal Value
      sv_acronym       // 6: Acronym base value
      );
// Brief:
//   Enumerated type for special values
// Description:
//   This enumerated type can be used in a Delphi program directly;
//   Programs using the DLL should use the integer values instead
//   This avoids passing enums

   TgdxDataType = (
      dt_set,          // 0: Set
      dt_par,          // 1: Parameter
      dt_var,          // 2: Variable
      dt_equ,          // 3: Equation
      dt_alias         // 4: Aliased set
      );
// Brief:
//  Enumerated type for GAMS data types
// Description:
//  This enumerated type can be used in a Delphi program directly;
//  Programs using the DLL should use the integer values instead
//  This avoids passing enums



   TgdxUELIndex    = gmsspecs.TIndex; // Array type for an index using integers
   TgdxStrIndex    = array[TgdxDimension] of shortstring; // Array type for an index using strings

   TgdxValues      = gmsspecs.tvarreca; // Array type for passing values

   TgdxSVals       = array[TgdxSpecialValue] of double; // Array type for passing special values

   TDataStoreProc  = procedure (const Indx: TgdxUELIndex; const Vals: TgdxValues); {$I decorate}; // call back function
   // for reading data slice

const

   gdxDataTypStr : array[TgdxDataType] of string[5] = (
      'Set','Par','Var','Equ', 'Alias');

   gdxDataTypStrL : array[TgdxDataType] of string[9] = (
      'Set','Parameter','Variable','Equation', 'Alias');

   DataTypSize: array[TgdxDataType] of integer = (1,1,5,5, 0);

   gdxSpecialValuesStr: array[TgdxSpecialValue] of string[5] = (
     'Undef' {sv_valund },
     'NA'    {sv_valna  },
     '+Inf'  {sv_valpin },
     '-Inf'  {sv_valmin },
     'Eps'   {sv_valeps },
     '0'     {sv_normal },
     'AcroN' {acronym   });


implementation

end.
