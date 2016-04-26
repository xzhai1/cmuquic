#include <iostream>

#include "net/tools/quic/quic_server_stream.h"
#include "net/quic/quic_time.h"

namespace net {
  namespace tools {

    QuicServerStream::QuicServerStream(QuicStreamId id, QuicSession* session,
				       QuicConnectionHelperInterface* helper)
      : ReliableQuicStream(id, session),
        helper_(helper) {
          payload_.resize(1300);
	  // 1300 bytes is about how much data can fit in one UDP packet, after headers are included
          for (int i = 0; i < 1300; i++) {
            payload_[i] = 'x';
          }
    }

    QuicServerStream::~QuicServerStream() {
      alarm_->Cancel();
    }

    uint32 QuicServerStream::ProcessRawData(const char* data, uint32 data_len) {
      bytes_received += data_len;
      //WriteStringPiece(base::StringPiece(data), false);
      std::cout << "Got some data:" << data_len << "\n";
      return data_len;
    }

    void QuicServerStream::OnFinRead() {
      std::cout << "goint to send " << packet_num_ << " packets" << "\n";
      for (uint64 i = 0; i < packet_num_; i++) {
        WriteStringPiece(base::StringPiece(payload_), false);
      }
      WriteStringPiece(base::StringPiece("server_end"), true);
    }

    QuicPriority QuicServerStream::EffectivePriority() const {
      return (QuicPriority) 0;
    }
    
    void QuicServerStream::WriteStringPiece(base::StringPiece data, bool fin) {
      this->WriteOrBufferData(data, fin, nullptr);
    }

    void QuicServerStream::SetupPerformanceAlarm() {
      alarm_ = helper_->CreateAlarm(this);
      QuicTime onesecond = helper_->GetClock()->ApproximateNow().Add(QuicTime::Delta::FromSeconds(1));
      alarm_->Set(onesecond);
    }

    QuicTime QuicServerStream::OnAlarm() {
      QuicWallTime now = helper_->GetClock()->WallNow();
      std::cout << bytes_received << "\n";
      return helper_->GetClock()->ApproximateNow().Add(QuicTime::Delta::FromSeconds(1));
    }

    void QuicServerStream::OnClose() {
      // std::cout << bytes_received << "\n";
      ReliableQuicStream::OnClose();
      // exit(0);
    }
  }
}
