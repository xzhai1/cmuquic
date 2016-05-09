#!/bin/bash
set -o nounset
set -o errexit
# accepts as an argument the benchmarking result directory tree (containing ifstat files)
# and turns it into a table, which is output on stdout

# read in all the data...
tmp=$(mktemp)
cd $1
find * -name ifstat | while read file;
do echo $file | awk -F/ 'BEGIN{OFS="_"} {print $1,$2,$3}'; tail -n+3 $file | awk '{print $2}' | { sed 's/^/num=num+1\nsum=sum+/'; echo sum/num; } | bc;
done | paste -d " " - - | sort -gr > $tmp
# cat $tmp

# and use l33t hacks to turn it into an easy-to-work-with table
types=($(< $tmp awk -F_ 'BEGIN{OFS="_"} {print $1,$2}' | sort | uniq))
pipedir=$(mktemp -d)
{ echo index ${types[@]};
  paste \
      <(grep ${types[0]} $tmp | awk '{print $1}' | awk -F_ '{print $3}' | while read line; do printf "%5s\n" $line; done) \
      $(for t in ${types[@]};
	do pipe=$(mktemp -p $pipedir -u); mkfifo $pipe; grep $t $tmp | awk '{print $2}' | dd of=$pipe &>/dev/null & echo $pipe
	done); } | column -t
rm $tmp

# the above automatically determines the column headings and scales to any number of scenarios, but besides that it basically does this:
# { echo "index linux_linux linux_dpdk dpdk_linux dpdk_dpdk";
# paste \
# <(grep linux_linux $tmp | awk '{print $1}' | awk -F_ '{print $3}' | while read line; do printf "%5s\n" $line; done) \
# <(grep linux_linux $tmp | awk '{print $2}') \
# <(grep linux_dpdk $tmp | awk '{print $2}') \
# <(grep dpdk_linux $tmp | awk '{print $2}') \
# <(grep dpdk_dpdk $tmp | awk '{print $2}'); } | column -t
