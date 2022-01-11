#!/bin/bash
x=1000
while [ $x -le 2000 ]
do
	echo $x > "fil$x"
	x=$(( $x+1 ))
done
