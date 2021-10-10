#!/bin/bash

prog=$1
dir=$2
use_valgrind=$3

my_out=$(mktemp)
my_err=$(mktemp)

for f in "$dir"/*.in
do
    out=${f%in}out
    err=${f%in}err

    "$prog" < "$f" 1> "$my_out" 2> "$my_err"

    if [[ "$use_valgrind" == "valgrind" ]]; then
        valgrind --error-exitcode=123 --leak-check=full --show-leak-kinds=all \
        --errors-for-leak-kinds=all "$prog" < "$f" > /dev/null 2>&1
        val_return=$?
        if [[ "$val_return" == "123" ]]; then
            val_return="ERROR"
        else
            val_return="OK"
        fi
    fi

    #> /dev/null 2>&1
    diff "$out" "$my_out" 
    status[0]=$?
    diff "$err" "$my_err"
    status[1]=$?
    test=${f#"$dir"}

    for i in 0 1
    do
        if [[ "${status[${i}]}" == "0" ]]; then
            status[$i]="OK"
        else
            status[$i]="WRONG"
        fi
    done

    if [[ "$use_valgrind" == "valgrind" ]]; then
        printf "%-40s %-20s %-20s %-20s\n" "$test: \
        " "OUT - ${status[0]}" "ERR - ${status[1]}" "VALGRIND - $val_return";
    else printf "%-40s %-20s %-20s\n" "$test: \
        " "OUT - ${status[0]}" "ERR - ${status[1]}";
    fi
done

rm "$my_out"
rm "$my_err"