# cmuquic
15744 Project: Quic server

### Virtual Machine Set Up
We set up ``ubuntu-14.04.3-desktop-amd64`` on ``virt-manager``. Make sure you give it enough memory, 4GB at least. 3 ethernet interfaces.

Set up utilities

    sudo apt-get update
    sudo apt-get install tmux vim git openssh-server cmake build-essential

Set up ssh

    ssh-keygen -t rsa

paste public key into

    ~/.ssh/authorized_keys
  
Set up environment

    sudo apt-get install libpcap-dev 

Need to install ``go`` for ``libquic``

    wget https://storage.googleapis.com/golang/go1.6.linux-amd64.tar.gz
    sudo tar -C /usr/local/ -xzf go1.6.linux-amd64.tar.gz
    go1.6.linux-amd64.tar.gz 

Add ``/usr/local/go/bin`` to the PATH environment variable to ``/etc/profile`` for system-wide installation

    export PATH=$PATH:/usr/local/go/bin
    source /etc/profil

### build ``libquic``
Get the source

    git clone git@github.com:devsisters/libquic.git
    cd libquic
    mkdir build/
    cd build/
    cmake ..
    make -j
    
### build ``dpdk``
Try the [quick start](http://dpdk.org/doc/quick-start) first. You need to be ``sudo`` when setting up hugetable.

Compiling ``helloworld``
     make T=x86_64-native-linuxapp-gcc
     export RTE_SDK=$HOME/dpdk-2.2.0
     export RTE_TARGET=x86_64-native-linuxapp-gcc

Then setup hugepage either using what is in the quick-start or use the ``./tool/setup.sh``.
