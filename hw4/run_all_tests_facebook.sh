#!/bin/bash

count_failes=0
count_pass=0

if [ ! -f "sim_main" ]; then
    echo -e ">\tNo sim_main exist, running 'make'"
    make
fi

echo Running tests...
echo The test succeed if there are no diffs printed.
echo

for filename in tests/*.in; do
    test_num=`echo $filename | cut -d'.' -f1`
    ./sim_main ${filename} > ${test_num}.YoursOut
done

for filename in tests/*.out; do
    test_num=`echo $filename | cut -d'.' -f1`
    diff_result=$(diff ${test_num}.out ${test_num}.YoursOut)
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
        ((count_failes=count_failes+1))
        
    else
        ((count_pass=count_pass+1))
    fi
done

echo
echo Ran all tests.

echo Pass=$count_pass
echo Fail=$count_failes

if [ "$1" == "clean" ]; then 
    echo -e ">\tmake clean"
    make clean
fi
if [ "$1" != "" ]; then
    yes | rm tests/*YoursOut*
    echo -e ">\trm tests/*YoursOut*"
fi