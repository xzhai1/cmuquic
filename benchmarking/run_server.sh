#!/bin/bash
version=$1
outpath=$2
packet=$3
if test -z "$version" || test -z "$outpath" || test -z "$packet";
then echo "Usage: $0 version outpath packet"; exit 1
fi
mkdir -p $outpath
rm -f $outpath/perf.data
tmux set-option -t quic set-remain-on-exit on
case $version in
    linux)
	tmux new-window -c $outpath "perf record /mnt/data/linuxquic/quic_perf_server -n $packet >/dev/null"
	;;
    dpdk)
	if ! pgrep opendp &>/dev/null;
	then
	    tmux new-window -c $outpath "/mnt/data/odp_start.sh"
	    sleep 5
	fi
	tmux new-window -c $outpath "perf record /mnt/data/cmuquic/quic_perf_server -p 1337 -n $packet >/dev/null"
	;;
    apache)
	tmux new-window -c $outpath "ln -f /var/www/html/$packet /var/www/html/bench"
	;;
    *)
	echo Invalid version
	exit 1
	;;
esac

		
