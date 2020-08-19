#/bin/sh

trim="$1/glxtrim"
apitrace="$1/apitrace"
datadir="$2"
trace="$3"
frame=$4
orig_img=$5


echo "Run tests $trace" 

rm ${trace}0*.png

${trim} "${datadir}/${trace}" --frames ${frame} -o trim-${trace}
${apitrace} replay --headless "${datadir}/${trace}" --snapshot=${orig_img} --snapshot-prefix=orig || exit 1
${apitrace} replay --headless trim-${trace} --snapshot=frame --snapshot-prefix=${trace} || exit 1

orig=$(printf "orig%010d.png" ${orig_img})
trim=$(ls ${trace}0*.png)

md5sum ${orig} | sed -s "s/ .*//" >${trace}.orig.md5
md5sum ${trim} | sed -s "s/ .*//" >${trace}.trim.md5

diff -q ${trace}.orig.md5 ${trace}.trim.md5 && exit 0 || exit 1

