/* 
 *
 *
 *
 */

/* C includes */
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

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

/* TODO legacy bullshit. Will remove soon */
uint64 FLAGS_total_transfer = 10 * 1000 * 1000;
uint64 FLAGS_chunk_size = 1000;
uint64 FLAGS_duration = 0;

string randomString(uint length) {
  string result = "";
  for (uint i = 0; i < length; i++) {
    result.push_back('a' + rand()%26);
  }
  return result;
}


/**
 * @brief usageError This is lifted out of Michael Kerrisk's TLPI book.
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
  /* TODO for now we will keep this crap */
  net::tools::QuicClientStream* stream = client.CreateClientStream();
  if (FLAGS_duration == 0) {
    for (uint64 i = 0; i < FLAGS_total_transfer; i += FLAGS_chunk_size) {
      stream->WriteStringPiece(
          base::StringPiece(randomString(FLAGS_chunk_size)), false);
      if (stream->HasBufferedData()) {
        client.WaitForEvents();
      }
    }
    
    while (stream->HasBufferedData()) {
      client.WaitForEvents();
    }
    
  } else {
    for (time_t dest = time(NULL) + FLAGS_duration; time(NULL) < dest; ) {
      stream->WriteStringPiece(
          base::StringPiece(randomString(FLAGS_chunk_size)), false);
      if (stream->HasBufferedData()) {
        client.WaitForEvents();
      }
    }
  }

  stream->CloseConnection(net::QUIC_NO_ERROR);
  client.Disconnect();
}

