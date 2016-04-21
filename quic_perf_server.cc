// Small demo that reads form stdin and sents over a quic connection

#include <iostream>
#include <cstdio>

#include <unistd.h>
#include <stdlib.h>

#include "net/base/ip_endpoint.h"
#include "net/tools/quic/quic_server.h"

#include "base/at_exit.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/ip_endpoint.h"
#include "net/base/privacy_mode.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_server_id.h"

using namespace std;

int main(int argc, char *argv[]) {

  int opt;
  int port_number = -1;

  while ((opt = getopt(argc, argv, ":p:")) != -1) {
    switch (opt) {
    case 'p': 
      port_number = atoi(optarg);
      break;
    case ':':
      printf("Missing argument\n");
      return 1;
    case '?':
      printf("Unrecognized option\n");
      return 1;
    default:
      printf("Usage: ./quic_perf_server -p port_number\n");
      return 1;
    }
  }
  /* can't have empty arg */
  if (port_number == -1) {
      printf("Usage: ./quic_perf_server -p port_number\n");
      return 1;
  }

  /* We have to initialize netdpsock first.
     Init runs only once for each process */
  if (netdpsock_init(NULL) != 0) {
    LOG(ERROR) << "netdpsock_init failed\n";
    return 1;
  }

  printf("port number specified at %d\n", port_number);

  // Is needed for whatever reason
  base::AtExitManager exit_manager;

  net::IPAddressNumber ip_address = (net::IPAddressNumber) std::vector<unsigned char> { 0, 0, 0, 0 };
  net::IPEndPoint listen_address(ip_address, port_number);
  net::QuicConfig config;
  net::QuicVersionVector supported_versions = net::QuicSupportedVersions();
  //net::EpollServer epoll_server;

  net::tools::QuicServer server(config, supported_versions);

  // Start listening on the specified address.
  if (!server.Listen(listen_address)) {
    cerr << "Could not listen on socket" << endl;
    return 1;
  }

  while (1) {
    server.WaitForEvents();
  }

  cout.flush();

}
