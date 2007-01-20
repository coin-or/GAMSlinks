$setglobal PDFLINK none
$eolcom //
set g Glpk Option Groups /
        general        General Options
      /
    e / '-1', 0*100, '+n', primal, dual, barrier /
    f / def Default, lo Lower Bound, up Upper Bound, ref Reference /
    t / I Integer, R Real, S String, B Binary /
    o Options /
      writemps               create MPS file for problem
* GAMS options
      reslim                 resource limit
      iterlim                iteration limit
      optcr                  relative stopping tolerance
* immediates
      nobounds               ignores bounds on options
      readfile               read secondary option file
 /

$onembedded
    optdata(g,o,t,f) /
general.(
  writemps            .s.(def '')
* GAMS options
  reslim          .r.(def 1000)
  iterlim         .i.(def 10000)
  optcr           .r.(def 0.1)
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
 odefault(o)       / reslim     'GAMS reslim'
                     iterlim    'GAMS iterlim' 
                     optcr      'GAMS optcr' /
$onempty
 oep(o) enum options for documentation only / /;
$offempty
