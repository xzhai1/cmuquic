#ifndef NET_TOOLS_QUIC_SERVER_STREAM_
#define NET_TOOLS_QUIC_SERVER_STREAM_

#include <string>

#include "net/quic/quic_alarm.h"
#include "net/quic/quic_connection.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_data_stream.h"

namespace net {
  namespace tools {

    class QuicServerStream: public ReliableQuicStream, public QuicAlarm::Delegate {
    public:
      QuicServerStream(QuicStreamId id, QuicSession* session, QuicConnectionHelperInterface* helper);
      ~QuicServerStream();

      uint32 ProcessRawData(const char* data, uint32 data_len) override;

      void OnFinRead() override;

      void SetPacketNum(int packet_num) { packet_num_ = packet_num; }

      QuicPriority EffectivePriority() const override;

      void WriteStringPiece(base::StringPiece data, bool fin);

      void SetupPerformanceAlarm();

      QuicTime OnAlarm();

      void OnClose();

    private:
      int packet_num_;
      std::string payload_;

      uint64 bytes_received = 0;
      QuicConnectionHelperInterface* helper_;
      QuicAlarm* alarm_;
    };
  }
}

#endif
