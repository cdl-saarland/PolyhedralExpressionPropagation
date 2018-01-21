#!/bin/sh -e

LLFILE=`echo $1 | sed -e 's/\.c/.ll/g'`
LLFILE_TMP=${LLFILE}.tmp

# The number of lines to cut of the LLVM-IR file clang produces.
CUT_N_LINES=0

clang -c -S -ffast-math -emit-llvm -O0 $1 -o ${LLFILE}
sed -i".tmp" 's/optnone //' ${LLFILE}

opt -correlated-propagation -mem2reg -instcombine -loop-simplify -indvars \
-instnamer ${LLFILE} -S -o ${LLFILE_TMP}

# Insert a header into the new testcase containing a sample RUN line a FIXME and
# an XFAIL. Then insert the formated C code and finally the LLVM-IR without
# attributes, the module ID or the target triple.
echo '; RUN: opt %loadPolly -analyze < %s | FileCheck %s' > ${LLFILE}
echo ';' >> ${LLFILE}
echo '; FIXME: Edit the run line and add checks!' >> ${LLFILE}
echo ';' >> ${LLFILE}
echo '; XFAIL: *' >> ${LLFILE}
echo ';' >> ${LLFILE}
clang-format $1 | sed -e 's/^[^$]/;    &/' -e 's/^$/;/' >> ${LLFILE}
echo ';' >> ${LLFILE}

cat ${LLFILE_TMP}  >> ${LLFILE}
sed -i".tmp" '/; Function Attrs:/d' ${LLFILE}
sed -i".tmp" '/; ModuleID =/d' ${LLFILE}
sed -i".tmp" '/target triple/d' ${LLFILE}

head --lines=-${CUT_N_LINES} ${LLFILE} > ${LLFILE_TMP}

mv ${LLFILE_TMP} ${LLFILE}
