#!/usr/bin/env python3

import sys
import os
import subprocess
import shutil

def print_usage() :
    print('Usage: %s /path/to/gams-sysdir /path/to/solver/library solvername sid modeltypes [dicttype [/path/to/optxyz.def]]' % sys.argv[0])
    print()
    print('   solvername - name under which to reach solver in GAMS')
    print('   sid        - 3-letter solver identifier')
    print('   modeltypes - model types for which the solver can be used, e.g., "LP MIP RMIP"')
    print('   dicttype   - dictionary type (5=have dictionary, 0=no dictionary, default: 5)')
    print('   optxyz.def - options definition file to be installed in GAMS system dir')

if len(sys.argv) < 6 :
    print_usage()
    sys.exit(1);

gamssysdir = sys.argv[1]
solverlib = os.path.abspath(sys.argv[2])
solvername = sys.argv[3].upper()
solverid = sys.argv[4]
modeltypes = sys.argv[5]
dicttype = sys.argv[6] if len(sys.argv) > 6 else "5"
optdef = sys.argv[7] if len(sys.argv) > 7 else None

iswindows = True if os.name == 'nt' else False

if not os.path.isdir(gamssysdir) :
    print("Error: %s is not a directory" % gamssysdir, file=sys.stderr)
    print_usage()
    sys.exit(1)
if not os.access(gamssysdir, os.W_OK) :
    print("Error: %s is not writable" % gamssysdir, file=sys.stderr)
    print_usage()
    sys.exit(1)

gmscmp = os.path.join(gamssysdir, 'gmscmpnt.txt' if iswindows else 'gmscmpun.txt')
if not os.path.isfile(gmscmp) :
    print("Error: File %s does not exist" % gmscmp, file=sys.stderr)
    print_usage()
    sys.exit(1)

if not os.path.isfile(solverlib) :
    print("Error: %s does not exist or is not a file" % solverlib, file=sys.stderr)
    print_usage()
    sys.exit(1)

if solverlib.endswith('.la') :
    # extract installed libname from libtool archive
    dlname = None
    libdir = None
    for line in open(solverlib, 'r') :
        if line.startswith('dlname=') :
            dlname = line.split('=')[1].strip().strip("'")
        if line.startswith('libdir=') :
            libdir = line.split('=')[1].strip().strip("'")
    if not dlname or not libdir :
        print("Error: no libdir or dlname found in", solverlib, file=sys.stderr)
        print_usage()
        sys.exit(1)
    if iswindows :
       # convert msys-path to windows path
       libdir = subprocess.check_output("cygpath -w " + libdir).decode("utf-8").strip()
       # on Windows, DLLs are in bindir and not in libdir, assume bindir is libdir/../bin
       libdir = os.path.join(libdir, '..', 'bin')
    solverlib = os.path.join(libdir, dlname)
    if not os.path.isfile(solverlib) :
        print("Error: %s does not exist or is not a file" % solverlib, file=sys.stderr)
        print_usage()
        sys.exit(1)

if len(solverid) != 3 or len(solverid.split()) > 1 :
    print("Error: Solver id %s needs to consist of exactly 3 non-whitespace characters" % solverid, file=sys.stderr)
    print_usage()
    sys.exit(1)

if len(modeltypes) < 0 :
    print("Error: At least one model type needs to be specified", file=sys.stderr)
    print_usage()
    sys.exit(1)

if not dicttype.isdigit() :
    print("Error: Dictionary type", dicttype, "should be a number", file=sys.stderr)
    print_usage()
    sys.exit(1)

if optdef and not os.path.isfile(optdef) :
    print("Error: Options definition file", optdef, "does not exist or is not a file", file=sys.stderr)
    print_usage()
    sys.exit(1)

if not os.path.exists(gmscmp + '.orig') :
    print("Creating backup", gmscmp + '.orig')
    shutil.copy(gmscmp, gmscmp + '.orig')

print("Creating backup", gmscmp + '.bak')
shutil.copy(gmscmp, gmscmp + '.bak')

infile = open(gmscmp + '.bak', 'r')
outfile = open(gmscmp, 'w')

try :
    skipLines = 0
    for line in infile :
        if line.upper().startswith(solvername + ' ') :
            # old entry for this solver
            # skip this line and remember to skip following ones
            # entry 7 of this line says how many lines additional to this and following one to skip
            sline = line.split()
            if len(sline) < 7 :
                print('Error: Expected at least 7 entries in this line:', line, file=sys.stderr)
                raise BaseException
            if not sline[6].isdigit() :
                print('Error: Expected integer at position 7 in this line:', line, file=sys.stderr)
                raise BaseException
            skipLines = int(sline[6]) + 1
            continue
    
        if skipLines > 0 :
            # skipping line of current section
            skipLines -= 1
            continue
    
        if line.startswith('DEFAULTS') :
            print('Adding section for solver', solvername, 'using lib', solverlib)
            # add config for solver before DEFAULTS section
            print("%s 111 %s 000102030405 1 0 2 %s" % (solvername, dicttype, modeltypes), file = outfile)
            print('gmsgennt.cmd' if iswindows else 'gmsgenus.run', file = outfile, end = '')
            if optdef :
                print('', optdef, file = outfile)
            else :
                print(file = outfile)
            print('gmsgennx.exe' if iswindows else 'gmsgenux.out', file = outfile)
            print('%s %s 1 0' % (solverlib, solverid), file = outfile)  # the last '0' indicates no thread-safety, which already isn't the case because of the GAMS API files
            print(file=outfile)
    
        print(line, end = '', file = outfile)

except Exception as err:
    print('Error:', err, file=sys.stderr)
    print('Restoring', gmscmp, file=sys.stderr)
    outfile.close()
    shutil.copy(gmscmp + '.bak', gmscmp)
    sys.exit(1)
