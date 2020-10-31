#!/usr/bin/env python3

import sys
import os
import subprocess

def print_usage() :
    print('Usage: %s /path/to/gamsconfig.yaml /path/to/install' % sys.argv[0])
    print()

if len(sys.argv) < 3 :
    print_usage()
    sys.exit(1);

gamsconfig = sys.argv[1]
installdir = os.path.abspath(sys.argv[2])

if not os.path.exists(gamsconfig) :
    print("Error: %s does not exist" % gamsconfig, file=sys.stderr)
    print_usage()
    sys.exit(1)
if not os.path.isdir(installdir) :
    print("Error: %s is not a directory" % installdir, file=sys.stderr)
    print_usage()
    sys.exit(1)
if not os.access(installdir, os.W_OK) :
    print("Error: %s is not writable" % installdir, file=sys.stderr)
    print_usage()
    sys.exit(1)

print('Writing', os.path.join(installdir, 'gamsconfig.yaml'))
out = open(os.path.join(installdir, 'gamsconfig.yaml'), 'w')

libdirs = set()
for line in open(gamsconfig, 'r') :
   if not line.startswith('#') and line.find('libName') >= 0 :
      solverlib = line.split()[-1]
      if solverlib.endswith('.la') :
         # extract installed libname from libtool archive
         dlname = None
         libdir = None
         for laline in open(solverlib, 'r') :
             if laline.startswith('dlname=') :
                 dlname = laline.split('=')[1].strip().strip("'")
             if laline.startswith('libdir=') :
                 libdir = laline.split('=')[1].strip().strip("'")
         if not dlname or not libdir :
             print("Error: no libdir or dlname found in", solverlib, file=sys.stderr)
             print_usage()
             sys.exit(1)
         if os.name == 'nt' :
            # convert msys-path to windows path
            libdir = subprocess.check_output("cygpath -w " + libdir).decode("utf-8").strip()
            # on Windows, DLLs are in bindir and not in libdir, assume bindir is libdir/../bin
            libdir = os.path.join(libdir, '..', 'bin')
            # no rpath on Windows, so let GAMS know to add libdir to PATH when using solver
            libdirs.add(libdir)
         solverlib_installed = os.path.join(libdir, dlname)
         if not os.path.isfile(solverlib_installed) :
             print("Error: %s does not exist or is not a file" % solverlib_installed, file=sys.stderr)
             print_usage()
             sys.exit(1)
         #print(solverlib, '->', solverlib_installed)
         line = line.replace(solverlib, solverlib_installed)
   if os.name == 'nt' and not line.startswith('#') and line.find('defName') >= 0 :
      deffile = line.split()[-1]
      # convert msys-path to windows path
      deffile_win = subprocess.check_output("cygpath -w " + deffile).decode("utf-8").strip()
      if not os.path.isfile(deffile_win) :
          print("Error: %s does not exist or is not a file" % deffile_win, file=sys.stderr)
          print_usage()
          sys.exit(1)
      line = line.replace(deffile, deffile_win)
   print(line, file=out, end='')

if len(libdirs) > 0 :
   print(file=out)
   print('environmentVariables:', file=out)
for libdir in libdirs :
   print('- PATH:', file=out)
   print('    value:', libdir, file=out)
   print('    pathVariable: True', file=out)
