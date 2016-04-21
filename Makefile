CC=g++
CFLAGS=--std=c++11 -fpermissive -c -g -pg
INC=-I . \
    -I ../libquic/src \
    -I ../libquic/src/third_party/protobuf/src \
    -I ../dpdk-ans/librte_netdp/include \
    -I ../dpdk-ans/librte_netdpsock/include

# should use ./dpdk-2.2.0/$(RTE_TARGET)/lib but somehow gdb doesn't pick up
# the environmental variales
LDFLAGS=-L ../libquic/build -l quic \
        -L ../libquic/build/boringssl/ssl -l ssl \
        -L ../libquic/build/boringssl/crypto -l crypto \
        -L ../libquic/build/protobuf -l protobuf \
        -L ../dpdk-ans/librte_netdp -l rte_netdp \
        -L ../dpdk-ans/librte_netdpsock -l rte_netdpsock \
        -L ../dpdk-2.2.0/x86_64-native-linuxapp-gcc/lib -l rte_mbuf \
        -L ../dpdk-2.2.0/x86_64-native-linuxapp-gcc/lib -l rte_eal \
        -L ../dpdk-2.2.0/x86_64-native-linuxapp-gcc/lib -l rte_mempool \
        -L ../dpdk-2.2.0/x86_64-native-linuxapp-gcc/lib -l rte_ring \
        -l pthread \
        -l dl \
        -pg

SRCFILES=$(wildcard net/tools/quic/*.cc) $(wildcard net/tools/epoll_server/*.cc)

OBJFILES=$(SRCFILES:.cc=.o)

all: quic_perf_client quic_perf_server tcp_perf_server tcp_perf_client

quic_perf_client: $(OBJFILES) quic_perf_client.o
	$(CC) $(OBJFILES) $@.o -o $@ $(LDFLAGS)

quic_perf_server: $(OBJFILES) quic_perf_server.o
	$(CC) $(OBJFILES) $@.o -o $@ $(LDFLAGS)

tcp_perf_server: $(OBJFILES) tcp_perf_server.o
	$(CC) $(OBJFILES) $@.o -o $@ $(LDFLAGS)

tcp_perf_client: $(OBJFILES) tcp_perf_client.o
	$(CC) $(OBJFILES) $@.o -o $@ $(LDFLAGS)

.cc.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

print-%:
	@echo $* = $($*)

clean:
	rm $(OBJFILES) quic_perf_client.o quic_perf_server.o
