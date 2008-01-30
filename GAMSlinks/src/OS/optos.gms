$setglobal PDFLINK coin
$eolcom //
set g OS Option Groups /
        general        General Options
      /
    e / '-1', 0*100,
        'solve', getJobID, knock, kill, send, retrieve
      /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
      readosol               read the solver options from an OSoL file
      writeosil              write the problem instance as OSiL file
      writeosrl              write the optimization result as OSrL file
      service                address of an Optimization Services Agent for a remote solve
      service_method         method of an OS Agent to call 
      solver                 specification of a solver
      readospl               read knock request as OSpL file
      writeospl              write knock or kill reply into OSpL file
* GAMS options
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
optdata(g,o,t,f) /
general.(
  readosol         .s.(def '')
  writeosil        .s.(def '')
  writeosrl        .s.(def '')
  service          .s.(def '')
  service_method   .s.(def 'solve')
  solver           .s.(def '')
  readospl         .s.(def '')
  writeospl        .s.(def '')
* GAMS options
* immediates
  nobounds        .b.(def 0)
  readfile        .s.(def '')
) /
$onempty
  oe(o,e) /
  service_method.(   'solve'
                     getJobID
                     knock
                     kill
                     send
                     retrieve )
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
