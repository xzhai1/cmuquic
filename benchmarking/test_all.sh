#!/bin/bash
#for client in {linux,dpdk};
#do for server in {linux,dpdk};
for client in {apache,};
do for server in {apache,};
   do for packet in {1,10,100,1000,10000};
      do ~/run_sample.sh $client $server $packet;
      done
   done
done
	 
	  
