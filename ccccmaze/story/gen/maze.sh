#!/bin/bash

namespace=$1
size=$2
data=$3

# debug this script
rm -rf ${namespace}

mkdir -p ${namespace}
for ((i=0;i<size;i++)); do
	#statements
	echo $pi $pj
	for ((j=0;j<size;j++)); do
		pi=$(printf "%02X" $i)
		pj=$(printf "%02X" $j)
		fn=${namespace}/${namespace}$pi$pj.cfg
		touch ${fn}
	    up=$(printf "%s%02X" ${pi} $((j-1)))
		down=$(printf "%s%02X" ${pi} $((j+1)))
		left=$(printf "%02X%s" $((i-1)) ${pj})
		right=$(printf "%02X%s" $((i+1)) ${pj})
		walls=$(echo "${up}${down}${left}${right}")
		walls=$(echo $walls | sed 's|FFFFFFFFFFFFFFFF|--|g;')
		echo ${walls} >> ${fn}
		for ((z=0;z<15;z++)); do
  		    line=$(mktemp -u XXXXXXXXXXXXXXXXXXXX)
  		    echo "${line}" >> ${fn}
  		done
  		echo ${fn}
  		ls ${namespace}
	done
done