#/bin/bash

trim="$1/glxtrim2"
apitrace="$1/apitrace"
datadir="$2"
trace="$3"
frame=$4
orig_img=$5

echo "Run tests $trace"

rm ${trace}*.png
rm ${trace}*.md5
rm trim$-{trace}

${trim} "${datadir}/${trace}" --frames ${frame} -o trim-${trace} || exit 1
${apitrace} replay --headless "${datadir}/${trace}" --snapshot=${orig_img} --snapshot-prefix=${trace}orig || exit 1
${apitrace} replay --headless trim-${trace} --snapshot=frame --snapshot-prefix=${trace} || exit 1

orig=$(printf "${trace}orig%010d.png" ${orig_img})
trim_array_=$(ls ${trace}0*.png)
trim_array=( $trim_array_ )
trim=${trim_array[-1]}
echo $trim

if [ "x$trim" = "x" ]; then
  exit 1
fi

md5sum ${orig} | sed -s "s/ .*//" >${trace}.orig.md5
md5sum ${trim} | sed -s "s/ .*//" >${trace}.trim.md5

diff -q ${trace}.orig.md5 ${trace}.trim.md5 && exit 0 || exit 1
