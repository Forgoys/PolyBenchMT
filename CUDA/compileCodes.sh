#!/bin/bash

for currDir in *
do
    echo $currDir
    if [ -d $currDir ]
	then
		cd $currDir
		pwd
		make clean
		make
		cd ..
    fi
done