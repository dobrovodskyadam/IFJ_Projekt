#!/usr/bin/bash
# @file test.sh
# @brief
# @date 09-11-2026
#
# @author Lukas Sofka (xsofkal00)
# @author Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

correct_ok=0
correct_all=0
incorrect_ok=0
incorrect_all=0

for file in tests/extensive/correct_test/*.scala; do
    output=$(./ifj26 < "$file" 2>&1 > /dev/null)
    code=$?
    correct_all=$((correct_all + 1))

    if [ "$code" -eq 0 ]; then
        correct_ok=$((correct_ok + 1))
        echo "OK    $file"
    else
        echo "FAIL  $file code: $code output: $output"
    fi
done

for file in tests/extensive/incorrect_test/*.scala; do
    output=$(./ifj26 < "$file" 2>&1 > /dev/null)
    code=$?
    expected=$(basename "$file" .scala | cut -d '_' -f 2)
    incorrect_all=$((incorrect_all + 1))

    if [ "$code" -eq "$expected" ]; then
        incorrect_ok=$((incorrect_ok + 1))
        echo "OK    $file"
    else
        echo "FAIL  $file code: $code expected: $expected output: $output"
    fi
done

echo
echo "Correct programs:   OK $correct_ok/$correct_all, FAILED $((correct_all - correct_ok))/$correct_all"
echo "Incorrect programs: OK $incorrect_ok/$incorrect_all, FAILED $((incorrect_all - incorrect_ok))/$incorrect_all"
