#pragma once

#include <memory>
#include <vector>
#include <google/protobuf/service.h>
#include <rbus.h>

class RbusRpcServer {
public:
  RbusRpcServer(rbusHandle_t rbus);
  void RegisterService(std::unique_ptr<google::protobuf::Service>&& service);
  void RegisterService(const std::string& shared_object_name, const std::string& service_name);
private:
  static void rbusMessageHandler(rbusHandle_t handle, rbusMessage_t* msg, void* userData);
  void dispatchIncomingMessage(const uint8_t* data, int length);
private:
  rbusHandle_t m_rbus;
  std::vector< std::unique_ptr<google::protobuf::Service> > m_services;
};