# @file ll_table.py
# @brief Helper script for creating the LL table
# @date 01-11-2026
#
# @author Lukas Sofka (xsofkal00)
# @author Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

from pathlib import Path

# TODO: Define all possible tokens that can appear in expression on first place 
EXPRESSION_FIRST_SET = {"TOKEN_ID", "TOKEN_INT"}

grammar_path = Path(__file__).parent.parent / "grammar.txt"


def is_terminal(string: str):
    return string.isupper()


def find_empty(rules):
    empty_set = set()

    cycle = True
    while cycle:
        cycle = False

        for left, right in rules:
            if left in empty_set:
                continue

            can_be_emtpy = True
            for symbol in right:
                if symbol not in empty_set:
                    can_be_emtpy = False
                    break

            if can_be_emtpy:
                empty_set.add(left)
                cycle = True

    return empty_set


def find_first(rules, empty):
    first_set = {left: set() for left, right in rules}
    first_set["<expression>"] = set(EXPRESSION_FIRST_SET)

    cycle = True
    while cycle:
        cycle = False

        for left, right in rules:
            for symbol in right:
                if is_terminal(symbol):
                    new = {symbol}
                else:
                    new = first_set[symbol]

                for token in new:
                    if token not in first_set[left]:
                        first_set[left].add(token)
                        cycle = True

                if symbol not in empty_set:
                    break
    return first_set


# NOT COMPLETED
def find_follow(rules, empty, first):
    follow = {left: set() for left, right in rules}

    cycle = True
    while cycle:
        cycle = False

        for left, right in rules:
            for i, symbol in enumerate(right):
                if is_terminal(symbol) or symbol == "<expression>":
                    continue
                print(i, symbol)
        break


# Creates array of all rules
rules = []
with open(grammar_path, encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith("#"):
            a, b = line.split("->")
            right = tuple(b.split())
            if right == ("ε",):
                right = ()
            rules.append((a.strip(), right))

    empty_set = find_empty(rules)
    first_set = find_first(rules, empty=empty_set)
    # [print(i, first_set[i]) for i in first_set]
    follow_set = find_follow(rules, empty=empty_set, first=first_set)
