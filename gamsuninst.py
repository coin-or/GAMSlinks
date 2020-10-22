#!/usr/bin/env python3

import sys
import os
import shutil

def print_usage() :
    print('Usage: %s /path/to/gams-sysdir solvername [optxyz.def]' % sys.argv[0])
    print()
    print('   solvername - name of solver to uninstall')

if len(sys.argv) < 3 :
    print_usage()
    sys.exit(1);

gamssysdir = sys.argv[1]
solvername = sys.argv[2].upper()

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
            # entry for this solver
            print('Removing entry for', solvername)
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

        print(line, end = '', file = outfile)

except Exception as err:
    print('Error:', err, file=sys.stderr)
    print('Restoring', gmscmp, file=sys.stderr)
    outfile.close()
    shutil.copy(gmscmp + '.bak', gmscmp)
    sys.exit(1)
