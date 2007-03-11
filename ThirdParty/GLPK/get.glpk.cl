#!/bin/sh

wgetcount=`which wget 2>/dev/null | wc -w`
if test ! $wgetcount = 1; then
  echo "Utility wget not found in your PATH."
  exit -1
fi

echo " "
echo "Running script for downloading the GNU Linear Programming Toolkit"
echo " "

echo "Downloading the library from ftp.gnu.org..."
wget ftp://ftp.gnu.org/gnu/glpk/glpk-4.15.tar.gz

echo "Unpacking the library..."
tar xzf glpk-4.15.tar.gz

echo "Deleting the tar.gz file..."
rm -f glpk-4.15.tar.gz

echo "Applying patch file..."
patch -p0 < glpk.patch

echo "Making glpk library..."
cd glpk-4.15/w32
nmake -f Makefile_VC6 glpk.lib
cd ..

echo " "
echo "Done downloading and installing glpk 4.15."

thepath=`pwd`
echo "When configuring your COIN project, add the following arguments to you configure call:"
echo "  --with-glpk-incdir=$thepath/glpk-4.15/include"
echo "  --with-glpk-lib=$thepath/glpk-4.15/w32/glpk.lib"
echo " "
