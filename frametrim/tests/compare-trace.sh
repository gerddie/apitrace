#/bin/sh

trim="$1/glxtrim"
apitrace="$1/apitrace"
datadir="$2"
trace="$3"
frame=$4
orig_img=$5
trim_img=$6

echo "Run tests $trace" 

${trim} "${datadir}/${trace}" --frames ${frame} -o trim-${trace}
${apitrace} replay "${datadir}/${trace}" --snapshot=${orig_img} --snapshot-prefix=orig 
${apitrace} replay trim-${trace} --snapshot=${trim_img} --snapshot-prefix=trim 

orig=$(printf "orig%010d.png" ${orig_img})
trim=$(printf "trim%010d.png" ${trim_img})

md5sum ${orig} | sed -s "s/ .*//" >${trace}.orig.md5
md5sum ${trim} | sed -s "s/ .*//" >${trace}.trim.md5

diff -q ${trace}.orig.md5 ${trace}.trim.md5 && exit 0 || exit 1

