#!/bin/sh

wgetcount=`which wget 2>/dev/null | wc -w`
if test ! $wgetcount = 1; then
  echo "Utility wget not found in your PATH."
  exit -1
fi

echo " "
echo "Running script for downloading the GAMS I/O libraries for Solaris on x86_64 machines"
echo " "

MACHINECODE=SIG
. ./FILES
FILES="${IOLIB_FILES} ${SMAGLIB_FILES}"

mkdir -p ${MACHINECODE}
cd ${MACHINECODE}
rm -f $FILES

echo "Downloading the libraries from www.gams.com..."
for file in $FILES
do
	wget http://www.gams.com/~svigerske/$file
done

echo "Unpacking the libraries..."
for file in $FILES
do
	unzip -o $file
done

echo "Deleting the zip files..."
rm -f $FILES

echo " "
echo "Done downloading the GAMS I/O libraries."
echo " "
