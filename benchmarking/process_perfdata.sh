#!/bin/bash
tmp=$(mktemp)
set -o nounset
find $1 -name perf.data | while read file;
do
    echo $file | awk -F/ 'BEGIN{ORS="/";OFS="/";} {print $(NF-4),$(NF-3),$(NF-2)}';
    perf report -f -i $file 2>/dev/null | head -n 1000 | grep -v '^#' | head -n 5
done

