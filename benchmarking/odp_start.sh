#!/bin/bash
set -o nounset
set -o errexit

RTE_TARGET=x86_64-native-linuxapp-gcc
RTE_SDK=/mnt/data/dpdk-2.2.0
RTE_ODP=/mnt/data/dpdk-ans

if pgrep opendp;
then
    echo "ODP is already running! See its pid? ^ Refusing to start"
    exit 1
fi

get_pci_address()
{
    raw_data="$(lshw -class network -businfo | grep $1)"
    echo $raw_data | cut -d ' ' -f 1 | cut -d '@' -f 2
}

load_igb_uio_module()
{
	if [ ! -f $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko ];then
		echo "## ERROR: Target does not have the DPDK UIO Kernel Module."
		echo "       To fix, please try to rebuild target."
		return
	fi

	if /sbin/lsmod | grep -q igb_uio; then
	    echo "Unloading existing DPDK UIO module"
	    sudo /sbin/rmmod igb_uio
	fi
	
	if ! /sbin/lsmod | grep -q uio; then
		modinfo uio > /dev/null
		if [ $? -eq 0 ]; then
			echo "Loading uio module"
			sudo /sbin/modprobe uio
		fi
	fi

	# UIO may be compiled into kernel, so it may not be an error if it can't
	# be loaded.

	echo "Loading DPDK UIO module"
	if ! sudo /sbin/insmod $RTE_SDK/$RTE_TARGET/kmod/igb_uio.ko; then
	    echo "## ERROR: Could not load kmod/igb_uio.ko."
	    quit
	fi
}

# Loads new igb_uio.ko (and uio module if needed).
load_igb_uio_module

# Set up hugepages
pages=1024
nr_hugepages_path=/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
if [ $(cat $nr_hugepages_path) -eq $pages ];
then echo "Hugepages already set up";
else 
    echo $pages > $nr_hugepages_path
    sudo mkdir -p /mnt/huge
    if ! grep -q '/mnt/huge' /proc/mounts ; then
	sudo mount -t hugetlbfs nodev /mnt/huge
    fi
fi

# Set up NIC
nic=eth1
pci_path=0000:00:03.0
if ip link | grep -q $nic; then
    ip link set $nic down
fi
${RTE_SDK}/tools/dpdk_nic_bind.py -b igb_uio $pci_path
${RTE_SDK}/tools/dpdk_nic_bind.py --status

# Run ODP in foreground - make sure to run script with &
${RTE_ODP}/opendp/build/opendp  -c 0x1 -n 1 -- -p 0x1 --config="(0,0,0)"
