#/bin/sh

trim="$1/glxtrimreverse"
apitrace="$1/apitrace"
datadir="$2"
trace="$3"
frame=$4
orig_img=$5

echo "Run tests $trace"

rm ${trace}*.png
rm ${trace}*.md5
rm trim$-{trace}

${trim} "${datadir}/${trace}" --frames ${frame} -o rtrim-${trace} || exit 1
${apitrace} replay --headless "${datadir}/${trace}" --snapshot=${orig_img} --snapshot-prefix=${trace}orig || exit 1
${apitrace} replay --headless rtrim-${trace} --snapshot=frame --snapshot-prefix=r${trace} || exit 1

orig=$(printf "${trace}orig%010d.png" ${orig_img})
trim=$(ls r${trace}0*.png)

md5sum ${orig} | sed -s "s/ .*//" >${trace}.orig.md5
md5sum ${trim} | sed -s "s/ .*//" >${trace}.rtrim.md5

diff -q ${trace}.orig.md5 ${trace}.rtrim.md5 && exit 0 || exit 1