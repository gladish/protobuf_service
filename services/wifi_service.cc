#include "services/wifi.pb.h"


class MyWiFiService : public rdk::rpc::wifi::WiFiService {
public:
  void CancelWPSPairing(
    google::protobuf::RpcController* controller,
    const rdk::rpc::wifi::CancelWPSPairingRequest* request,
    rdk::rpc::wifi::CancelWPSPairingResponse* response,
    google::protobuf::Closure* done) override
  {    
    response->set_success(false);
    response->set_error("WPS just won't stop");    
    done->Run(); 
  } 
};

extern "C" {
  google::protobuf::Service* WiFiService_New()
  {
    return new MyWiFiService();
  }
}
