#!/bin/bash
set -o nounset
tmp=$(mktemp)
cd $1
find * -name ifstat | while read file;
do echo $file | awk -F/ 'BEGIN{OFS="_"} {print $1,$2,$3}'; tail -n+3 $file | awk '{print $2}' | { sed 's/^/num=num+1\nsum=sum+/'; echo sum/num; } | bc;
done | paste -d " " - - | sort -gr > $tmp

{ echo "index linux_linux linux_dpdk dpdk_linux dpdk_dpdk";
paste \
<(grep linux_linux $tmp | awk '{print $1}' | awk -F_ '{print $3}' | while read line; do printf "%5s\n" $line; done) \
<(grep linux_linux $tmp | awk '{print $2}') \
<(grep linux_dpdk $tmp | awk '{print $2}') \
<(grep dpdk_linux $tmp | awk '{print $2}') \
<(grep dpdk_dpdk $tmp | awk '{print $2}'); } | column -t
