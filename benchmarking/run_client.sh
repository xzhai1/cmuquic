#!/bin/bash
version=$1
outpath=$2
address=$3
if test -z "$version" || test -z "$outpath" || test -z "$address";
then echo "Usage: $0 version outpath address"; exit 1
fi
mkdir -p $outpath
tmux set-option -t quic set-remain-on-exit on
case $version in
    linux)
	tmux new-window -c $outpath "/mnt/data/linuxquic/quic_perf_client $address"
	;;
    dpdk)
	if ! pgrep opendp &>/dev/null;
	then
	    tmux new-window -c $outpath "/mnt/data/odp_start.sh"
	    sleep 5
	fi
	tmux new-window -c $outpath "/mnt/data/cmuquic/quic_perf_client -p 1337 $address"
	;;
    apache)
	tmux new-window -c $outpath "/root/http_client.sh"
	;;
    *)
	echo Invalid version
	exit 1
	;;
esac

		
