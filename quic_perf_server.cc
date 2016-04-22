// Small demo that reads form stdin and sents over a quic connection

#include <iostream>

#include <stdlib.h> /* printf */
#include <unistd.h> /* getopt */

#include "net/base/ip_endpoint.h"
#include "net/tools/quic/quic_server.h"

#include "base/at_exit.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/ip_endpoint.h"
#include "net/base/privacy_mode.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_server_id.h"
#include <signal.h>

using namespace std;

static void exithandler(int _) {
  cout << "Got sigint\n";
  exit(1);
}

int main(int argc, char *argv[]) {
  signal(SIGINT, exithandler);

  int opt;
  int packet_num = -1;

  /* : at start of optstring to indicate missing argument */
  while ((opt = getopt(argc, argv, ":n:")) != -1) {
    switch (opt) {
    case 'n': 
      packet_num = atoi(optarg);
      break;
    case ':':
      printf("Missing argument\n");
      return 1;
    case '?':
      printf("Unrecognized option\n");
      return 1;
    default:
      printf("Usage: ./quic_perf_server -c chunk_size\n");
      return 1;
    }
  }

  /* can't have empty arg */
  if (packet_num == -1) {
      printf("Usage: ./quic_perf_server -n [number of packet]\n");
      return 1;
  }

  /* Is needed for whatever reason */
  base::AtExitManager exit_manager;

  net::IPAddressNumber ip_address = (net::IPAddressNumber) std::vector<unsigned char> { 0, 0, 0, 0 };
  net::IPEndPoint listen_address(ip_address, 1337);
  net::QuicConfig config;
  net::QuicVersionVector supported_versions = net::QuicSupportedVersions();
  net::EpollServer epoll_server;

  net::tools::QuicServer server(config, supported_versions);
  server.SetPacketNum(packet_num);

  // Start listening on the specified address.
  if (!server.Listen(listen_address)) {
    cerr << "Could not listen on socket" << endl;
    return 1;
  }

  // server listens and responds until killed
  // (or until it calls exit somewhere inside WaitForEvents)
  while (1) {
    server.WaitForEvents();
  }
}
