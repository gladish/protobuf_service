
#include "rpclib/rbus_rpc_server.h"
#include <rbus.h>

int main()
{
  rbusHandle_t rbus;
  rbus_open(&rbus, "wifi_server");

  RbusRpcServer server(rbus);
  server.RegisterService("wifi_service.so", "WiFiService");

  while(true)
  {
    sleep(1);
  }

  return 0;
}