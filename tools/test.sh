#!/usr/bin/bash
# @file test.sh
# @brief
# @date 09-11-2026
#
# @author Lukas Sofka (xsofkal00)
# @author Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

BASIC=(examples/basic/*.scala)
EXTENSIONS=(examples/extensions/*.scala)
CORRECT_SYNTAX=(tests/extensive/correct-syntax/*.scala)
INCORRECT_SYNTAX=(tests/extensive/incorrect-syntax/*.scala)

for file in "${CORRECT_SYNTAX[@]}"; do
    output=$(./ifj26 < "$file" 2>&1)

    code=$?
    if [ "$code" -eq 0 ]; then
        echo "OK  $file"
    else
        echo "FAIL ($code) $file: $(echo "$output")"
    fi
done
