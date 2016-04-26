/* C includes */
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <time.h>
#include <signal.h>

/* This is the quic client & server implementation from google that we have
 * modified with netdp socket */
#include "net/tools/quic/quic_client.h"

/* These are helper includes */
#include "base/at_exit.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
/* These are the helper includes related to networking in general */
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
/* These are the quic protocol related header files */
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_server_id.h"
#include "net/quic/quic_utils.h"

#define printable(ch) (isprint((unsigned char) ch) ? ch : '#')

using namespace std;


/**
 * @brief exithandler
 * 
 * Short handler to catch Ctl-C and exit the program
 *
 * @param _
 */
static void exithandler(int _) {
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
    fprintf(stderr, "Usage: %s [-p port_number]\n", progName);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

  int   opt;
  char  *ip_str;
  int   port_number = -1; 

  while ((opt = getopt(argc, argv, ":p:")) != -1) {
    switch (opt) {
    case 'p': 
      port_number = atoi(optarg);
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

  /* We can't have empty options */
  if (port_number == -1) {
      usageError(argv[0], NULL, optopt);
      return 1;
  }

  /* And server ip address needs to be supplied */
  if (optind == argc) {
      fprintf(stderr, "You need to supply an ip address!\n");
      usageError(argv[0], NULL, optopt);
      return 1;
  }

  /* The first nonoption argument has to be ip address */
  /* TODO validate ip address */
  ip_str = argv[optind];

  /* We have to initialize netdpsock first. 
     Init runs only once for each process */
  if (netdpsock_init(NULL) != 0) {
    LOG(ERROR) << "netdpsock_init failed\n";
    return 1;
  }

  LOG(INFO) << "Connecting to " << ip_str << ":" << port_number;
  signal(SIGINT, exithandler);

  /* This is required. Otherwise, this error will be thrown:
   * [0408/210628:FATAL:at_exit.cc(53)] Check failed: false. 
   * Tried to RegisterCallback without an AtExitManager */
  base::AtExitManager exit_manager;

  unsigned char a, b, c, d;
  sscanf(ip_str, "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d);

  net::IPAddressNumber ip_address =
    (net::IPAddressNumber) std::vector<unsigned char>{a, b, c, d};
  net::IPEndPoint server_address(ip_address, port_number);
  net::QuicServerId server_id(ip_str, port_number, 
                              /*is_http*/ false, net::PRIVACY_MODE_DISABLED);
  net::QuicVersionVector supported_versions = net::QuicSupportedVersions();

  //if (FLAGS_duration == 0) {
  while (1) {
    net::EpollServer epoll_server;
    net::tools::QuicClient client(server_address, server_id, 
                                  supported_versions, &epoll_server);
    if (!client.Initialize()) {
      LOG(ERROR) << "Could not initialize client";
      return 1;
    }
    if (!client.Connect()) {
      LOG(ERROR) << "Client could not connect";
      return 1;
    }

    /* A stream is a "bi-directional flow of bytes across a logical channel
     * within a QUIC connection". In other words, we can have multiple streams 
     * in the same QUIC pipe. */
    net::tools::QuicClientStream* stream = client.CreateClientStream();
    auto start = std::chrono::high_resolution_clock::now();

    /* Half close */
    stream->WriteStringPiece(base::StringPiece("client_end"), true);
    /* And wait for data to come back */
    while (!stream->read_side_closed()) {
      client.WaitForEvents();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();

    client.Disconnect();
  }
}
