#!/usr/bin/bash
# @file test.sh
# @brief
# @date 09-11-2026
#
# @author Lukas Sofka (xsofkal00)
# @author Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

for file in tests/extensive/correct_test/*.scala; do
    ./ifj26 < "$file" > /dev/null 2>&1
    code=$?

    if [ "$code" -eq 0 ]; then
        echo "OK    $file"
    else
        echo "FAIL  $file code: $code"
    fi
done

for file in tests/extensive/incorrect_test/*.scala; do
    ./ifj26 < "$file" > /dev/null 2>&1
    code=$?

    if [ "$code" -eq 0 ]; then
        echo "OK    $file"
    else
        echo "FAIL  $file code: $code"
    fi
done
