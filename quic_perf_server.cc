#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h> /* printf */
#include <unistd.h> /* getopt */

#include <iostream>
#include <cstdio>

#include "net/base/ip_endpoint.h"
#include "net/tools/quic/quic_server.h"

#include "base/at_exit.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/ip_endpoint.h"
#include "net/base/privacy_mode.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_server_id.h"

#define printable(ch) (isprint((unsigned char) ch) ? ch : '#')

using namespace std;

static void 
exithandler(int _) 
{
  cout << "Got sigint\n";
  exit(1);
}

/**
 * @brief usageError
 *
 * This is lifted out of Michael Kerrisk's TLPI book.
 *
 * @param progName  argv[0]
 * @param msg       error message
 * @param opt       returned option value
 */
static void
usageError(char *progName, char *msg, int opt)
{
    if (msg != NULL && opt != 0)
        fprintf(stderr, "%s (-%c)\n", msg, printable(opt));
    fprintf(stderr, "Usage: %s -p port_number -n num_of_packets\n", progName);
    exit(EXIT_FAILURE);
}

int 
main(int argc, char *argv[]) 
{

  int opt;
  int port_number = -1;
  int packet_num = -1;

  while ((opt = getopt(argc, argv, ":p:n:")) != -1) {
    switch (opt) {
    case 'p': 
      port_number = atoi(optarg);
      break;
    case 'n': 
      packet_num = atoi(optarg);
      break;
    case ':':
      usageError(argv[0], "Missing argument", optopt);
      return 1;
    case '?':
      usageError(argv[0], "Unrecognized option", optopt);
      return 1;
    default:
      usageError(argv[0], NULL, optopt);
      return 1;
    }
  }

  /* can't have empty arg */
  if (port_number == -1 || packet_num == -1) {
    usageError(argv[0], NULL, optopt);
    return 1;
  }

  printf("port number specified at %d\n", port_number);
  printf("number of packets specified at %d\n", packet_num);

  /* We have to initialize netdpsock first.
     Init runs only once for each process */
  if (netdpsock_init(NULL) != 0) {
    LOG(ERROR) << "netdpsock_init failed\n";
    return 1;
  }

  /* Is needed for whatever reason */
  base::AtExitManager exit_manager;

  net::IPAddressNumber ip_address = 
    (net::IPAddressNumber) std::vector<unsigned char> { 0, 0, 0, 0 };
  net::IPEndPoint listen_address(ip_address, port_number);
  net::QuicConfig config;
  net::QuicVersionVector supported_versions = net::QuicSupportedVersions();
  net::tools::QuicServer server(config, supported_versions);
  server.SetPacketNum(packet_num);

  /* Start listening on the specified address. */
  if (!server.Listen(listen_address)) {
    cerr << "Could not listen on socket" << endl;
    return 1;
  }

  /* Install signal handler to gracefully exit */
  signal(SIGINT, exithandler);

  /* server listens and responds until killed
   * (or until it calls exit somewhere inside WaitForEvents) */
  while (1) {
    server.WaitForEvents();
  }
}
