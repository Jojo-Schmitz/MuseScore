#!/bin/bash

S=$1
B=$2

echo "---- Copy all mtest scheme files ----"

cp $S/ms.scm $B
cp $S/decl.scm $B

for F in $(find $S | grep '\.scm$')
do
  echo copy scheme file $F to $B/libmscore
  cp $F $B/libmscore
done

echo "---- Generate mtest scheme stubs c code ----"

cat $S/decl.scm | $B/../s7/s7 $S/../s7/lib.scm $S/ms.scm $S/gen.scm
