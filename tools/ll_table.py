# @file ll_table.py
# @brief Helper script for creating the LL table
# @date 01-11-2026
#
# @author Lukas Sofka (xsofkal00)
# @author Email: xsofkal00@stud.fit.vutbr.cz
# Faculty of Information Technology, Brno University of Technology

import csv
from pathlib import Path

# TODO: Define all possible tokens that can appear in expression on first place
EXPRESSION_FIRST_SET = {
    # "TOKEN_ID",
    "TOKEN_LPAREN",
    "TOKEN_INT",
    "TOKEN_FLOAT",
    "TOKEN_STRING",
    "TOKEN_NOT",
    "TOKEN_MINUS",
    "TOKEN_TRUE",
    "TOKEN_FALSE",
    "TOKEN_LIST",
}

grammar_path = Path(__file__).parent.parent / "grammar.txt"
csv_path = Path(__file__).parent.parent / "grammar.csv"
grammar_stripped_path = Path(__file__).parent.parent / "grammar_striped.txt"


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
    # Create dictionary of nonterminals and their first set
    first_set = {left: set() for left, right in rules}
    first_set["<expression>"] = set(EXPRESSION_FIRST_SET)

    cycle = True
    while cycle:
        cycle = False

        for left, right in rules:
            # Cycle over X1...Xn in A -> X1X2...Xn
            for symbol in right:
                if is_terminal(symbol):
                    new = {symbol}
                else:
                    # If it's nonterminal add First(symbol) to the First(left)
                    new = first_set[symbol]

                # New tokens to add
                if new - first_set[left]:
                    # Adds new elements into the set
                    first_set[left].update(new)
                    cycle = True

                if symbol not in empty:
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
                # Skip if terminal or expression
                if is_terminal(symbol) or symbol == "<expression>":
                    continue

                # All follow values
                new = set()
                can_empty = True
                for j in right[i + 1 :]:
                    if is_terminal(j):
                        new.add(j)
                    else:
                        new.update(first[j])

                    if j not in empty:
                        can_empty = False
                        break

                if can_empty:
                    new.update(follow[left])

                # If something new was added (at least once), we continue the cycle
                if new - follow[symbol]:
                    follow[symbol].update(new)
                    cycle = True

    return follow


def find_predict(rules, empty, first, follow):
    predict = []

    for left, right in rules:
        new = set()
        can_empty = True
        for i in right:
            # We need to manually add terminals since we don't have them in first_set
            if is_terminal(i):
                new.add(i)
            else:
                new.update(first[i])

            # Checking for emptiness of each nonterminal
            if i not in empty:
                can_empty = False
                break

        if can_empty:
            new.update(follow[left])

        predict.append(new)

    return predict


def build_table(rules, predict):
    table = {}
    for i, (left, right) in enumerate(rules):
        for token in predict[i]:
            if (left, token) in table:
                print(
                    f"CONFLICT ({left},{token}): rules {table[(left, token)]} and {i + 1}"
                )
            table[(left, token)] = i + 1

    return table


def save_to_csv(table):
    rows = list(dict.fromkeys(key[0] for key in table))
    columns = list(dict.fromkeys(key[1] for key in table))

    with open(csv_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)

        # Write header of the csv file
        writer.writerow([""] + columns)

        for i in rows:
            line = [i]
            for j in columns:
                # Find rule number for given coordinates
                result = table.get((i, j))
                if result == None:
                    result = ""

                line.append(result)
            writer.writerow(line)


def save_rules(rules):
    with open(grammar_stripped_path, "w", encoding="utf-8") as f:
        for i, (left, right) in enumerate(rules):
            if right:
                right_str = " ".join(right)
            else:
                right_str = "ε"

            f.write(f"{i + 1}: {left} -> {right_str}\n")


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
    follow_set = find_follow(rules, empty=empty_set, first=first_set)
    predict_list = find_predict(
        rules, empty=empty_set, first=first_set, follow=follow_set
    )
    table = build_table(rules, predict_list)

    save_to_csv(table)
    save_rules(rules)
