#include "rbus_rpc_server.h"
#include "rbus_rpc_controller.h"
#include "rpclib/rdk_rpc.pb.h"

#include <sstream>

#include <dlfcn.h>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

using ServiceConstructor = google::protobuf::Service* (*)();

class RbusClosure : public google::protobuf::Closure {
public:
  RbusClosure(rbusHandle_t rbus, google::protobuf::Message* response, rdk::rpc::ProtocolDataUnit&& pdu) 
    : m_rbus(rbus)
    , m_response(response)
    , m_pdu(pdu) { }

  void Run() override {
    // wrap and send response
    rdk::rpc::ProtocolDataUnit pdu;    
    pdu.mutable_response()->set_message(m_response->SerializeAsString());

    std::string encoded_pdu = pdu.SerializeAsString();

    rbusMessage_t response;
    response.data = reinterpret_cast<const uint8_t *>(encoded_pdu.c_str());
    response.length = static_cast<int>(encoded_pdu.size());
    response.topic = m_reply_topic.c_str();
        
    delete this;
  };

  google::protobuf::Message* GetResponse() { return m_response.get(); }

private:
  std::unique_ptr<google::protobuf::Message> m_response;  
  rdk::rpc::ProtocolDataUnit m_pdu;
  rbusHandle_t m_rbus;
  int32_t m_correlation_key;
  std::string m_reply_topic;
};


RbusRpcServer::RbusRpcServer(rbusHandle_t rbus)
  : m_rbus(rbus)
{
}

void RbusRpcServer::RegisterService(const std::string& shared_object_name, const std::string& service_name)
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
  this->RegisterService(std::unique_ptr<google::protobuf::Service>(service)); 

  dlclose(lib);
}

void RbusRpcServer::RegisterService(std::unique_ptr<google::protobuf::Service> && service)
{
  rbusError_t err = rbusMessage_AddListener(
    m_rbus, 
    service->GetDescriptor()->full_name().c_str(),
    &RbusRpcServer::rbusMessageHandler,
    this);  
  m_services.push_back(std::move(service));
}

void RbusRpcServer::rbusMessageHandler(rbusHandle_t handle, rbusMessage_t* msg, void* userData)
{
  RbusRpcServer *rpc_server = reinterpret_cast<RbusRpcServer *>(userData);
  rpc_server->dispatchIncomingMessage(msg->data, msg->length);
}

void RbusRpcServer::dispatchIncomingMessage(const uint8_t *data, int length)
{
  rdk::rpc::ProtocolDataUnit pdu;
  pdu.ParseFromArray(data, length);

  auto itr = std::find_if(std::begin(m_services), std::end(m_services),
    [&](const std::unique_ptr<google::protobuf::Service> &service)
    {
      return service->GetDescriptor()->full_name() == pdu.request().service_name();
    });

  if (itr == std::end(m_services))
  {
    return;
  }

  google::protobuf::Service* service = itr->get();
  const google::protobuf::ServiceDescriptor *service_desc = service->GetDescriptor();
  const google::protobuf::MethodDescriptor *method_desc = service_desc->FindMethodByName(pdu.request().method_name());

  std::unique_ptr<google::protobuf::Message> req{ service->GetRequestPrototype(method_desc).New() };
  req->ParseFromArray(pdu.request().message().c_str(), pdu.request().message().size()); 

  RbusClosure *closure = new RbusClosure(m_rbus, service->GetResponsePrototype(method_desc).New(), std::move(pdu)); 

  rdk::rpc::Controller controller;    
  service->CallMethod(method_desc, &controller, req.get(), closure->GetResponse(), closure);
}