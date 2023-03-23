
project (pbuff_examples)
cmake_minimum_required(VERSION 3.5)
find_package(Protobuf  REQUIRED)
find_package(rbus CONFIG REQUIRED)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

add_library(rbus_rpc SHARED     
    "${CMAKE_CURRENT_SOURCE_DIR}/rpclib/rdk_rpc.proto"
    rpclib/rbus_rpc_controller.cc 
    rpclib/rbus_rpc_channel.cc
    rpclib/rbus_rpc_server.cc)
target_include_directories(rbus_rpc PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")
target_include_directories(rbus_rpc PUBLIC ${RBUS_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(rbus_rpc PUBLIC protobuf::libprotobuf ${RBUS_LIBRARIES})

protobuf_generate(
    TARGET rbus_rpc        
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    IMPORT_DIRS "/usr/include"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

add_library(wifi_service SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/services/wifi.proto"
    services/wifi_service.cc)
target_include_directories(wifi_service PUBLIC "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>")
target_include_directories(wifi_service PUBLIC ${RBUS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(wifi_service PUBLIC protobuf::libprotobuf)
add_dependencies(wifi_service rbus_rpc)

protobuf_generate(
    TARGET wifi_service        
    IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    IMPORT_DIRS "/usr/include"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")

add_executable(dynamic_client dynamic_client.cc)
target_include_directories(dynamic_client PUBLIC ${RBUS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(dynamic_client protobuf::libprotobuf rbus_rpc ${RBUS_LIBRARIES})
target_link_directories(dynamic_client PUBLIC ${PROTO_BINARY_DIR})

add_executable(static_client static_client.cc)
target_include_directories(static_client PUBLIC ${RBUS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(static_client protobuf::libprotobuf rbus_rpc wifi_service ${RBUS_LIBRARIES})
target_link_directories(static_client PUBLIC ${PROTO_BINARY_DIR})

add_executable(server server.cc)
target_include_directories(server PUBLIC ${RBUS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(server protobuf::libprotobuf rbus_rpc ${RBUS_LIBRARIES})