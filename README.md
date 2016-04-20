# cmuquic
15744 Project: Evaluating and Improving Quic Performance.

## The Plan
As a starting point, we used [maufl/quic_toy](https://github.com/maufl/quic_toy) because we know it works with ``libquic``, albeit an earlier commit of it. As far as client and server are concerned, it pulls in the utilities from ``src/net/tools/quic`` in the ``chromium`` source files and used a very watered down version of the ``quic_client_bin.cc`` and ``quic_server_bin.cc`` as testing harness. On top of that, it uses a mininet set up to test performance at different loss rates and bandwidth. This is a good starting point for us: we can build the project, and swap out the kernel networking calls with networking api in [opendp/dpdk-ans](https://github.com/opendp/dpdk-ans). Next, we can set up a topology to test different scenarios. Lastly, we can write a better harness and try to make it work with upstream chromium changes.

## Environment Set Up
We did our measurement on VM's because 

1. there are a lot of previliged setup steps and we would like not to screw up the bare metal we laid our hands on.
2. only certain NICs are compatible with DPDK and it is easy to swap out a NIC on VM when the previous one didn't work for us.

We tried to setup our testbed on [Emulab](http://www.emulab.net/index.php3) becuase it is free to us and you can easily set up a topology for testing. The problem was that DPDK couldn't build on the Xeon processors they have, which doesn't support ``SSE4_2``. We also tried to setup our testbed on AWS using its free tier micro instance. The stumbling block is that the NIC virtualization is incompatible with DPDK; while it builds fine on the EC2 instance, it doesn't support the interfaces it has. AWS's NIC virtualization is too much of a black box to us and at this point, we decided to just put up 2 VM's on some bare metal we so happened to have access to.

Make sure you give it enough memory, 4GB at least, and at least 2 compatible ethernet interfaces. We used ``e1000`` as the device model. The reason for 2 interfaces is that when one is bind to ``DPDK`` driver, it won't be available to you for ssh. [comment]: **Spencer, please add in the VM specs here. We may want to put up the VM image here**

On a virgin environment, you are going to ``sudo apt-get install`` a lot of things.

    sudo apt-get update
	
#### Utilities:

    sudo apt-get install tmux vim git openssh-server 
	
You may want to set up ssh (unless you like to use your virtual machine GUI).

    ssh-keygen -t rsa

And paste your host public key into VM's ``authorized_keys``.

    ~/.ssh/authorized_keys
	
#### Build tool chain:
	
	sudo apt-get install cmake build-essential ninja-build
	
#### DPDK dependencies:

	sudo apt-get install libpcap-dev

#### ``libquic``
You will need to install ``go``:

    wget https://storage.googleapis.com/golang/go1.6.linux-amd64.tar.gz
    sudo tar -C /usr/local/ -xzf go1.6.linux-amd64.tar.gz
	rm go1.6.linux-amd64.tar.gz

Add ``/usr/local/go/bin`` to the PATH environment variable to ``/etc/profile`` for system-wide installation:

    export PATH=$PATH:/usr/local/go/bin
    source /etc/profile


## Building and running ``dpdk``
Download the latest [release](http://dpdk.org/browse/dpdk/snapshot/dpdk-2.2.0.tar.gz) and build it:

    wget http://dpdk.org/browse/dpdk/snapshot/dpdk-2.2.0.tar.gz
    tar xf dpdk-2.2.0.tar.gz
	rm dpdk-2.2.0.tar.gz
    cd dpdk-2.2.0
  
It comes with a convenient ``setup.sh`` in the ``tools`` folder. So use that for building. Fire it up, assuming you are in the top level directory in dpdk-2.2.0:

	./tools/setup.sh
	----------------------------------------------------------
	 Step 1: Select the DPDK environment to build
	----------------------------------------------------------
	...
	[14] x86_64-native-linuxapp-gcc
	...
	[34] Exit Script

Now, after that, you will need to set up a couple of environmental variables in ``~/.bashrc``. The following is what we have; make sure you fill the correct path on your machine:

	export RTE_TARGET=x86_64-native-linuxapp-gcc
	export RTE_SDK=/root/dpdk-2.2.0
	
Then ``source ~/.bashrc``, and bring down the ethernet interface you are not using because we are going to bind it to DPDK:

	sudo ifconfig eth1 down

Fire up ``./tools/setup.sh`` again and choose the following items:

	----------------------------------------------------------
	 Step 2: Setup linuxapp environment
	----------------------------------------------------------
	[17] Insert IGB UIO module
	...
	[20] Setup hugepage mappings for non-NUMA systems
	...
	[23] Bind Ethernet device to IGB UIO module

``[17]`` inserts the driver module that lets DPDK use the NIC.
``[20]`` set up the hugepage memory mapping needed to DPDK's memory buffer and rings. Make sure you give it enough pages. We used 1024 for safety.
``[23]`` asks DPDK to binds to a particular NIC. Assuming you did bring down your ``eth1``, then you will simply put down its PICe address.

Now, as DPDK setup is concerned, this is the end of it. You may want to leave the ``./tools/setup.sh`` running so you can come back to it and easily map or unmap or bind or unbind.
 
## Running ``opendk-ans``
[``opendk-ans``](https://github.com/opendp/dpdk-ans), what used to be called ``opendk-odp``, is port of the FreeBSD network stack to DPDK. In a nutshell, it provides an almost identical set of socket APIs that is implemented in DPDK's EAL. However, to use it, you need to build it and start the network stack in user space. First clone the repo:

    git clone https://github.com/opendp/dpdk-ans.git
    
Since we started using it, the authors have made some commits(including the unfortunate name change) that have complicated our setup and broken our application. So we are sticking to the older commits:

    cd dpdk-odp
    git checkout 875a427
    cd opendp
	
Now, this is a twist, you have to make changes to ``odp_main.c``; you have to tell it what the IP address you would like to add to the interface and how you want the routing done. It is really just 2 lines of code:

	978         int ip_addr = 0x02020202;
	...
	996     route_ret = netdp_add_route(0x03030303, 1, 0x02020202, 0x00ffffff, NETDP_IP_RTF_GATEWAY);
    
You want to give it an address on your subnet. Set one last environmental variable in ``~/.bashrc``:

	export RTE_ODP=/root/dpdk-ans

and source it. Finally build it:

	make

and start the stack:

    sudo ./build/opendp -c 0x1 -n 1 -- -p 0x1 --config="(0,0,0)"
    
## Building ``libquic``
Now onto the actual protocol library. Clone the repo.
    
    git clone https://github.com/devsisters/libquic.git

We need to go back to an older commit of ``libquic``:

    git checkout 694a060

In ``libquic``'s top directory, apply the patch, ``libquic.patch``, from ``quic_toy``. We have included the patch in our repo too:

    patch < ../cmuquic/libquic.patch

For some reason, it says it doesn't know where ``src/net/quic/quic_crypto_client_stream.cc`` and ``src/net/quic/quic_crypto_client_stream.h`` are. Just repeat the names of the files and you will be fine.

    can't find file to patch at input line 5
    Perhaps you should have used the -p or --strip option?
    The text leading up to this was:
    --------------------------
    |diff --git src/net/quic/quic_crypto_client_stream.cc src/net/quic/quic_crypto_client_stream.cc
    |index 9c1089f..77bfb7d 100644
    |--- src/net/quic/quic_crypto_client_stream.cc
    |+++ src/net/quic/quic_crypto_client_stream.cc
    --------------------------
    File to patch: src/net/quic/quic_crypto_client_stream.cc
    patching file src/net/quic/quic_crypto_client_stream.cc
    can't find file to patch at input line 59
    Perhaps you should have used the -p or --strip option?
    The text leading up to this was:
    --------------------------
    |diff --git src/net/quic/quic_crypto_client_stream.h src/net/quic/quic_crypto_client_stream.h
    |index a60ce78..b5ebbae 100644
    |--- src/net/quic/quic_crypto_client_stream.h
    |+++ src/net/quic/quic_crypto_client_stream.h
    --------------------------
    File to patch: src/net/quic/quic_crypto_client_stream.h 
    patching file src/net/quic/quic_crypto_client_stream.h
 
Then build it:

    mkdir build/
    cd build/
    cmake -GNinja ..
    ninja

## Reproducing the Result
How to build our repo
