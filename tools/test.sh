#!/usr/bin/zsh
# @file test.sh
# @brief
# @date 09-11-2026
#
# @author Lukas Sofka (xsofkal00)
# @author Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

for file in examples/basic/*.scala examples/extensions/*.scala; do
    output=$(./ifj26 < "$file" 2>&1)

    code=$?
    if [ "$code" -eq 0 ]; then
        echo "OK  $file"
    else
        echo "FAIL ($code) $file: $(echo "$output")"
    fi
done
