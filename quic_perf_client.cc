// Small demo that reads form stdin and sents over a quic connection

#include <iostream>
#include <sstream>
#include <chrono>
#include <time.h>
#include <signal.h>

#include "net/base/ip_endpoint.h"
#include "net/tools/quic/quic_client.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_server_id.h"
#include "net/quic/quic_utils.h"

using namespace std;

static void exithandler(int _) {
  cout << "Got sigint\n";
  exit(1);
}

int main(int argc, char *argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* line = base::CommandLine::ForCurrentProcess();
  const base::CommandLine::StringVector& args = line->GetArgs();
  if (args.size() == 0) {
    cout << "No address to connect to was provided.\n";
    return 1;
  }
  std::string address = args[0];

  signal(SIGINT, exithandler);

  // Is needed for whatever reason
  base::AtExitManager exit_manager;

  unsigned char a, b, c, d;
  sscanf(address.c_str(), "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d);
  printf("Connecting to %hhu.%hhu.%hhu.%hhu\n", a, b, c, d);
  net::IPAddressNumber ip_address = (net::IPAddressNumber) std::vector<unsigned char>{a, b, c, d};
  net::IPEndPoint server_address(ip_address, 1337);
  net::QuicServerId server_id(address, 1337, /*is_http*/ false, net::PRIVACY_MODE_DISABLED);
  net::QuicVersionVector supported_versions = net::QuicSupportedVersions();

  for (int i = 0; i < 2; i++) {
    net::EpollServer epoll_server;
    net::tools::QuicClient client(server_address, server_id, supported_versions, &epoll_server);
    if (!client.Initialize()) {
      cerr << "Could not initialize client" << endl;
      return 1;
    }
    cout << "Successfully initialized client" << endl;
    if (!client.Connect()) {
      cout << "Client could not connect" << endl;
      return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    net::tools::QuicClientStream* stream = client.CreateClientStream();
    stream->WriteStringPiece(base::StringPiece("client_end"), true);

    while (!stream->read_side_closed()) {
      client.WaitForEvents();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();

    cout << "Took time: " << dur << endl;

    client.Disconnect();
  }
}

