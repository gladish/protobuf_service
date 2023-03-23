#include "rpclib/rbus_rpc_channel.h"
#include "rpclib/rdk_rpc.pb.h"

#include <google/protobuf/message.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <sstream>

#include <dlfcn.h>

using ServiceConstructor = google::protobuf::Service* (*)();

static std::map< std::string, google::protobuf::Service* > s_services;

google::protobuf::Service* rdk::rpc::find_service(std::string const& service_name)
{
  auto it = s_services.find(service_name);
  if (it != s_services.end()) {
    return it->second;
  }
  return nullptr;
}

class RbusChannel : public google::protobuf::RpcChannel {
public:
  RbusChannel(rbusHandle_t rbus);
  
  void CallMethod(
    google::protobuf::MethodDescriptor const* method,
    google::protobuf::RpcController* controller,
    google::protobuf::Message const* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done) override;
private:
  rbusHandle_t m_rbus;
  std::atomic<int32_t> m_correlation_id;
};

class InProcChannel : public google::protobuf::RpcChannel {
public:
  void CallMethod(
    google::protobuf::MethodDescriptor const* method,
    google::protobuf::RpcController* controller,
    google::protobuf::Message const* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done) override;
  static void RegisterService(google::protobuf::Service* service);
};

void rdk::rpc::register_inproc_service(std::string const& shared_object_name, std::string const& service_name)
{
  // if shared_object_name doesn't begin with "lib" then prepend it
  std::string name = shared_object_name;
  if (name.find("lib") != 0) {
    name = "lib" + shared_object_name;
  }

  void *lib = dlopen(name.c_str(), RTLD_NOW);
  if (!lib) {
    std::stringstream message;
    message << "Failed to load shared object: " << name;
    message << " error: " << dlerror();
    throw std::runtime_error(message.str());
  }

  std::string constructor_name = service_name + "_New";
  void *constructor = dlsym(lib, constructor_name.c_str());
  if (!constructor) {
    std::stringstream message;
    message << "Failed to find constructor: " << constructor_name;
    message << " in " << name.c_str();
    throw std::runtime_error(message.str());
  }
  
  auto service_constructor = reinterpret_cast<ServiceConstructor>(constructor);
  auto service = service_constructor();
  InProcChannel::RegisterService(service);  
}

google::protobuf::RpcChannel* rdk::rpc::make_rbus_channel(rbusHandle_t rbus)
{
  return new RbusChannel(rbus);
}

google::protobuf::RpcChannel* rdk::rpc::make_inproc_channel()
{
  static InProcChannel channel;
  return &channel;
}

RbusChannel::RbusChannel(rbusHandle_t rbus)
  : m_rbus(rbus)
  , m_correlation_id(1000)
{
}

void RbusChannel::CallMethod(
    google::protobuf::MethodDescriptor const* method,
    google::protobuf::RpcController* controller,
    google::protobuf::Message const* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done)
{
  // create pdu
  rdk::rpc::ProtocolDataUnit pdu;
  pdu.mutable_request()->set_service_name(method->service()->full_name());
  pdu.mutable_request()->set_method_name(method->name());
  pdu.mutable_request()->set_message(request->SerializeAsString());
  pdu.mutable_request()->set_correlation_key(m_correlation_id++);

  // send as rbus message
  rbusMessage_t msg;
  std::string encoded_request = pdu.SerializeAsString();
  msg.data = reinterpret_cast<const uint8_t *>(encoded_request.c_str());
  msg.length = static_cast<int>(encoded_request.size());
  msg.topic = method->service()->full_name().c_str();

  rbusError_t err = rbusMessage_Send(m_rbus, &msg, RBUS_MESSAGE_NONE);  
  if (err) {
    std::stringstream buff;
    buff << "Failed to send rbus message: ";
    buff << rbusError_ToString(err);
    //controller->SetFailed(buff.str());
    return;
  }

  // put outstanding request in queue, wait for response or timeout
}



void InProcChannel::RegisterService(google::protobuf::Service* service)
{
  s_services[service->GetDescriptor()->full_name()] = service;
}

void InProcChannel::CallMethod(
    const google::protobuf::MethodDescriptor *method,
    google::protobuf::RpcController *controller,
    const google::protobuf::Message *request,
    google::protobuf::Message *response,
    google::protobuf::Closure *done)
{
  auto service = rdk::rpc::find_service(method->service()->full_name());
  if (!service) {
    std::stringstream buff;
    buff << "Service not found: ";
    buff << method->service()->full_name();
    controller->SetFailed(buff.str());
    return;
  }
  else
    service->CallMethod(method, controller, request, response, done);
}