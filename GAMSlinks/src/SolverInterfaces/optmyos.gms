$setglobal PDFLINK coin
$eolcom //
set g OS Option Groups /
        general        General Options
      /
    e / '-1' /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I   Integer
        R   Real
        S   String
        B   Binary
        SD  String with defaults /
    o Options /
      readosol               read the solver options from an OSoL file
      writeosil              write the problem instance as OSiL file
      writeosrl              write the optimization result as OSrL file
      service                address of an Optimization Services Agent for a remote solve
      solver                 specification of a solver
* GAMS options
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
optdata(g,o,t,f) /
general.(
  readosol         .s.(def '')
  writeosil        .sd.(def 'osil.xml')
  writeosrl        .sd.(def 'osrl.xml')
  service          .s.(def '')
  solver           .s.(def '')
* GAMS options
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
) /
$onempty
  oe(o,e) /
  /
$offempty
 im  immediates recognized  / EolFlag , ReadFile, Message, NoBounds /
 immediate(o,im)   / NoBounds.NoBounds, ReadFile.ReadFile /
 hidden(o)         / NoBounds, ReadFile /
$onempty
 odefault(o)       / /
$offempty
$onempty
 oep(o) enum options for documentation only / /;
$offempty

*** big fudge
set optvalue(o); optvalue(o) = sum(optdata(g,o,'SD',f), yes);
optdata(g,o,'S',f) $= optdata(g,o,'SD',f); optdata(g,o,'SD',f) = no;
