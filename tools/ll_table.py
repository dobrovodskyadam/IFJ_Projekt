# @file ll_table.py
# @brief Helper script for creating the LL table
# @date 01-11-2026
#
# @author Lukas Sofka (xsofkal00)
# @author Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

from pathlib import Path

grammar_path = Path(__file__).parent.parent / "grammar.txt"

def follow():
    pass

def first():
    pass

# Creates array of all rules
rules = []
with open(grammar_path, encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith("#"):
            rules.append(line.split("->"))

