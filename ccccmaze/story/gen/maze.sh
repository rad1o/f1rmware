#!/bin/bash

namespace=$1
size=$2
data=$3

# debug this script
rm -rf ${namespace}

mkdir -p ${namespace}
for ((i=0;i<size;i++)); do
	# x
	pi=$(printf "%02X" $i)
	for ((j=0;j<size;j++)); do
		# y
		pj=$(printf "%02X" $j)
		fn=${namespace}/${namespace}$pi$pj.cfg

		touch ${fn}
		nexti=$(printf "%02X" $((i+1)))
		nextj=$(printf "%02X" $((j+1)))

		previ=$(printf "%02X" $((i-1)))
		prevj=$(printf "%02X" $((j-1)))
		
	    up=$(printf "%s%s" ${pi} ${prevj})
		down=$(printf "%s" ${pi} ${nextj})
		left=$(printf "%s" ${previ} ${pj})
		right=$(printf "%s" ${nexti} ${pj})
#echo $i $j
#echo $up $down $left $right
	    num_auto_lines=14

		if [ $j == $((size - 1)) ]; then
			down=----
		fi
		if [ $j == 0 ]; then
			up=----
		fi
		if [ $i == $((size - 1))  ]; then
			right=----
		fi
		if [ $i == 0 ]; then
			left=----
		fi

		walls=$(echo "${up}${down}${left}${right}")
		echo ${walls} >> ${fn}
		for ((z=0;z<num_auto_lines;z++)); do
			line=$(mktemp -u XXXXXXXXXXXXXXXXX)
  		    echo "${line}" >> ${fn}
  		done
  		echo ${fn}
	done
done