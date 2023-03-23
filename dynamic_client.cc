
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
  // create a "channel". a channel is an abstraction over a transport
  // mechanism. for simplicty, we're using an in-memory transport
  // that just makes calls directly to the service. 
  auto channel = rdk::rpc::make_inproc_channel();

  // one could also instantiate a channel that uses rbus
  // rbusHandle_t rbu;
  // rbus_open(&rbus, "wifi_client")
  // auto channel = rdk::rpc::make_rbus_channel(rbus);

  rdk::rpc::register_inproc_service("wifi_service.so", "WiFiService");
  
  auto controller = new rdk::rpc::Controller();
  
  auto service = rdk::rpc::find_service("rdk.rpc.wifi.WiFiService");  
  auto method_desc = service->GetDescriptor()->FindMethodByName("CancelWPSPairing");

  std::unique_ptr<google::protobuf::Message> req{ service->GetRequestPrototype(method_desc).New() };
  google::protobuf::util::JsonStringToMessage("{}", req.get());
  
  auto future = rdk::rpc::make_future(service->GetResponsePrototype(method_desc).New());
  channel->CallMethod(method_desc, controller, req.get(), future, &future);    

  future.WaitFor(std::chrono::milliseconds(1000));
  auto& response = future.GetResponse();

  print_as_json(response);

  return 0;
}