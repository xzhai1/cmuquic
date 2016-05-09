#!/bin/bash
set -o errexit
set -o nounset

client=$1
server=$2
packet=$3

# same directory structure in each place
sampledir=samples_adjacent/$client/$server/$packet
# path on the servers
clientpath=/mnt/data/$sampledir/client
serverpath=/mnt/data/$sampledir/server
hostpath=$HOME/quic/$sampledir/host

on_client="ssh -n -i /home/sbaugh/.ssh/id_rsa.quic root@client-quic.club.cc.cmu.edu --"
on_server="ssh -n -i /home/sbaugh/.ssh/id_rsa.quic root@server-quic.club.cc.cmu.edu --"

case $server in
    linux)
	server_address=128.237.157.132
	host_interface=tap0
	;;
    dpdk)
	server_address=128.237.157.134
	host_interface=tap1
	;;
    apache)
	server_address=128.237.157.132
	host_interface=tap0
	;;
    *)
	echo "Invalid server"
	exit 1
	;;
esac

function cleanup() {
    echo "Performing clean up"
    $on_server kill -s SIGINT $($on_server tmux list-windows -t quic -F '\#{pane_pid}' 2>/dev/null) || true
    $on_server tmux kill-session -t quic || true
    $on_client kill -s SIGINT $($on_client tmux list-windows -t quic -F '\#{pane_pid}' 2>/dev/null) || true
    $on_client tmux kill-session -t quic || true
    echo "Clean up done"
}

function do_it() {
    echo "Starting server"
    $on_server tmux new-session -s quic -d
    $on_server /root/run_server.sh $server $serverpath $packet
    sleep 5

    # Hardcode to just start three clients
    echo "Starting clients"
    $on_client tmux new-session -s quic -d
    echo "Starting client 1"
    $on_client /root/run_client.sh $client $clientpath $server_address
    sleep 1
    echo "Starting client 2"
    $on_client /root/run_client.sh $client $clientpath $server_address
    sleep 1
    echo "Starting client 3"
    $on_client /root/run_client.sh $client $clientpath $server_address
    sleep 1

    # ifstatcount=10
    # settletime=15
    ifstatcount=300
    settletime=30
    echo "Sleeping to let things settle..."
    sleep $settletime
    echo "Beginning recording"
    mkdir -p $hostpath
    ifstat -b -w -n -t -i $host_interface 1 $ifstatcount | tee $hostpath/ifstat
}

# on exiting the program, we'll perform cleanup again
trap 'cleanup' EXIT

echo "Run parameters: client: $client, server: $server, packets: $packet"
# kill any leftovers from a previous run
cleanup
# do it
do_it
