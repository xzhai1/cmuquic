# cmuquic
15744 Project: Evaluating and Improving Quic Performance.

## The Plan
As a starting point, we are using [maufl/quic_toy](https://github.com/maufl/quic_toy) because we know it works with ``libquic``, albeit an earlier commit of it. As far as client and server are concerned, it pulls in the utilities from ``src/net/tools/quic`` in the ``chromium`` source files and used a very watered down version of the ``quic_client_bin.cc`` and ``quic_server_bin.cc`` as testing harness. On top of that, it uses a mininet set up to test performance at different loss rates and bandwidth. This is a good starting point for us: we can build the project, and swap out the kernel networking calls with networking api in [opendp/dpdk-odp](https://github.com/opendp/dpdk-odp). Next, we can set up a topology to test different scenarios. Lastly, we can write a better harness and try to make it work with upstream chromium changes.

## Environment Set Up
### Virtual Machine
We are going to do this VM because my laptop has no ethernet interface and there is a limited list of NICs that ``DPDK`` supports. We set up ``ubuntu-14.04.3-desktop-amd64`` using ``virt-manager``. Make sure you give it enough memory, 4GB at least, and at least 2 compatible ethernet interfaces. We used ``e1000`` as the device model. The reason for 2 interfaces is that when one is bind to ``DPDK`` driver, it won't be available to you for ssh.

On a virgin environment, you are going to ``sudo apt-get install`` a lot of things.

    sudo apt-get update
    sudo apt-get install tmux vim git openssh-server cmake build-essential libpcap-dev

You will need to install ``go`` for ``libquic``:

    wget https://storage.googleapis.com/golang/go1.6.linux-amd64.tar.gz
    sudo tar -C /usr/local/ -xzf go1.6.linux-amd64.tar.gz

Add ``/usr/local/go/bin`` to the PATH environment variable to ``/etc/profile`` for system-wide installation:

    export PATH=$PATH:/usr/local/go/bin
    source /etc/profil

Lastly, you will want to set up ssh (unless you like to use your virtual machine GUI).

    ssh-keygen -t rsa

And paste your host public key into VM's ``authorized_keys``.

    ~/.ssh/authorized_keys
    
### build ``dpdk``
Download the latest [release](http://dpdk.org/browse/dpdk/snapshot/dpdk-2.2.0.tar.gz) and unzip it somewhere in your home directory:

    wget http://dpdk.org/browse/dpdk/snapshot/dpdk-2.2.0.tar.gz
    tar xf dpdk-2.2.0.tar.gz
    cd dpdk-2.2.0
    
Try the [quick start](http://dpdk.org/doc/quick-start) first. 

## Build the Baseline
### Get ``quic_toy``
Clone the code to somewhere in your directory.

    git clone https://github.com/maufl/quic_toy.git

We can't do anything yet because we haven't build the ``libquic`` library. And to build it, we need a patch from ``quic_toy``. Now we can go build ``libquic`` and we will be back to build ``quic_toy``.

### Build ``libquic``
Somewhere in your home directory (You can put it anywhere you want. However, you will need to change the Makefile so that it can find the header files and ``.a`` library), clone the repo.
    
    git clone https://github.com/devsisters/libquic.git

We need to go back to an older commit of ``libquic``. In its top level directory:

    git checkout 694a060

Apply the patch from ``quic_toy``:

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

Your top ``libquic`` directory should look like this:

    .
    ├── AUTHORS
    ├── boringssl
    ├── build
    ├── CMakeLists.txt
    ├── custom
    ├── DEPS
    ├── getdep.py
    ├── LICENSE
    ├── Makefile
    ├── manage.py
    ├── patch
    ├── protobuf
    ├── README.md
    └── src

and your ``build`` directory should look like this:

    build
    ├── boringssl
    ├── build.ninja
    ├── CMakeCache.txt
    ├── CMakeFiles
    ├── cmake_install.cmake
    ├── libquic.a
    ├── protobuf
    └── rules.ninja

where you can see the ``libquic.a`` library.

### Back to ``quic_toy``
Come back to cmuquic and build it:

    make

## Start Developing
### DPDK
When developing and debugging, you will need to start ``DPDK``. We like to use ``./tools/setup.sh`` in the ``dpdk`` directory to start it up. You will need to bring down your ``eth0``:

    ifdown eth0

Then start up the setup script:

    ./tools/setup.sh
    
Below is the sequence of things you will want to do:

    [17] Insert IGB UIO module
    [23] Bind Ethernet device to IGB UIO module
    [20] Setup hugepage mappings for non-NUMA systems
    
A word on the last one, we used 1024 * 64. Too small will crash it.

### opendk-odp
[``opendk-odp``](https://github.com/opendp/dpdk-odp) is port of the FreeBSD network stack to DPDK. In a nutshell, it provides an almost identical set of socket APIs that is implemented in DPDK's EAL. However, to use it, you need to build it and start the network stack in user space. First clone the repo:

    git clone https://github.com/opendp/dpdk-odp.git
    
and build the stack:

    cd opendp
    make

Once built, you can then start the stack:

    sudo ./build/opendp -c 0x1 -n 1 -- -p 0x1 --config="(0,0,0)"
    
Please go read its wiki; plenty of good info there on how to use the stack and setup.
