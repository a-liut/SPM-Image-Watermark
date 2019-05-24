#!/bin/bash

name_regex="main_(.+).cpp"
if [ -z ${1+x} ]; then
	pids=()

	for file in main_*.cpp; do
		if [[ $file =~ $name_regex ]]
	    then
			name="${BASH_REMATCH[1]}"
			echo "preparing ${name}..."

			make main="${file}" outname="${name}" &
			pids+=($!)
		fi
	done

	for pid in ${pids[*]}; do
	    wait $pid
	done
else

	file="$1"
	if [[ $file =~ $name_regex ]]
	then
		name="${BASH_REMATCH[1]}"
		echo "preparing ${name}..."

		make main="${file}" outname="${name}"
	else 
		echo "$1 doesn't matches name pattern."
	fi
fi

echo "done!"