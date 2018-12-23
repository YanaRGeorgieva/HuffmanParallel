#!/bin/bash
#Usage: $1 exeFile $2 testFile $3 resultFile $4 numThreads $5 granularity $6 balancing $7 numTries
for ((t = 1; t <= $4; t++ ))
{
	for ((try = 1; try <= $7; try++))
	{
		echo "Currently "$t" threads, granularity is "$5" and balancing is "$6". Try number "$try"." >> $3
		./$1 -f $2 -t $t -g $5 -b $6 -q >> $3
		sleep 20
	}
	sleep 30
}


