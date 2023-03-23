#include "services/wifi.pb.h"
#include "rpclib/rbus_rpc_controller.h"
#include "rpclib/rbus_rpc_channel.h"

#include <google/protobuf/util/json_util.h>
#include <iostream>

void print_as_json(google::protobuf::Message const& message)
{
  std::string json;
  google::protobuf::util::JsonPrintOptions options;
  options.add_whitespace = true;  
  options.always_print_primitive_fields = true;
  google::protobuf::util::MessageToJsonString(message, &json, options);
  std::cout << json << std::endl;
}

int main()
{ 
  // do this once 
  auto channel = rdk::rpc::make_inproc_channel();  
  auto controller = new rdk::rpc::Controller();
  auto service = new rdk::rpc::wifi::WiFiService::Stub(channel);

  // make request
  rdk::rpc::wifi::CancelWPSPairingRequest req;  

  auto future = rdk::rpc::make_future(new rdk::rpc::wifi::CancelWPSPairingResponse{});  

  service->CancelWPSPairing(controller, &req, future, &future);

  future.WaitFor(std::chrono::milliseconds(1000));
  const rdk::rpc::wifi::CancelWPSPairingResponse&  response = future.GetResponse();
  
  print_as_json(response);
  return 0;
}