#pragma once

#include <google/protobuf/service.h>
#include <rbus.h>

namespace rdk {
namespace rpc {

google::protobuf::RpcChannel* make_rbus_channel(rbusHandle_t rbus);
google::protobuf::RpcChannel* make_inproc_channel();
google::protobuf::Service* find_service(std::string const& service_name);

void register_inproc_service(std::string const& shared_object_name, std::string const& service_name);

} }
