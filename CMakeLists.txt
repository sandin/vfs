cmake_minimum_required(VERSION 3.5)

project(vfs LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing(true)

include_directories(src)
include_directories(include/vfs)

# prebuild
message("Build ${CMAKE_BUILD_TYPE} version on ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(PREBUILD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/prebuild/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(VFS_BUILD_TYPE "Debug")
else()
    set(VFS_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()
message("GRPC_PATH:  ${PREBUILD_PATH}/grpc/${VFS_BUILD_TYPE}/lib/cmake/")
message("LEVELDB_PATH:  ${PREBUILD_PATH}/leveldb/${VFS_BUILD_TYPE}/lib/cmake/")
list(APPEND CMAKE_PREFIX_PATH ${PREBUILD_PATH}/grpc/${VFS_BUILD_TYPE}/cmake/
                              ${PREBUILD_PATH}/grpc/${VFS_BUILD_TYPE}/lib/
                              ${PREBUILD_PATH}/grpc/${VFS_BUILD_TYPE}/lib/cmake
                              ${PREBUILD_PATH}/leveldb/${VFS_BUILD_TYPE}/lib/
                              ${PREBUILD_PATH}/leveldb/${VFS_BUILD_TYPE}/lib/cmake
)

find_package(leveldb CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
get_target_property(leveldb_LOCATION_RELWITHDEBINFO leveldb::leveldb LOCATION_RELWITHDEBINFO)
message("leveldb FOUND: ${leveldb_FOUND}, VERSION: ${leveldb_VERSION}, LOCATION_RELWITHDEBINFO: ${leveldb_LOCATION_RELWITHDEBINFO}")
message("Protobuf FOUND: ${Protobuf_FOUND}, VERSION: ${Protobuf_VERSION}")
message("absl FOUND: ${absl_FOUND}, VERSION: ${absl_VERSION}")
message("gRPC FOUND: ${gRPC_FOUND}, VERSION: ${gRPC_VERSION}")

# gtest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(PROTO_SOURCES
    include/vfs/object.pb.h
    include/vfs/object.pb.cc
    src/object.grpc.pb.h
    src/object.grpc.pb.cc
)
set(PROJECT_HEADERS
    include/vfs/ab.h
    include/vfs/utils.h
    include/vfs/ivirtualfilesystem.h
    include/vfs/leveldbfilesystem.h
    include/vfs/clientfilesystem.h
    include/vfs/nameserver.h
    include/vfs/rollingchecksum.h
)
set(PROJECT_SOURCES
    src/utils.cpp
    src/leveldbfilesystem.cpp
    src/clientfilesystem.cpp
    src/nameserver.cpp
    src/rollingchecksum.cpp

    ${PROJECT_HEADERS}
    ${PROTO_SOURCES}
)

# Library: vfs
add_library(${PROJECT_NAME} ${PROJECT_SOURCES})
#add_test(NAME ABLiteTest COMMAND ABLiteTest)
target_link_libraries(${PROJECT_NAME} leveldb::leveldb gRPC::grpc++ absl::strings absl::time)

# Test
add_executable(${PROJECT_NAME}_test
    test/virtualfilesystemtest.cpp
    test/nameservertest.cpp
    test/rollingchecksumtest.cpp
)
target_link_libraries(${PROJECT_NAME}_test ${PROJECT_NAME} gtest gtest_main)
add_test(NAME ${PROJECT_NAME}_test COMMAND ${PROJECT_NAME}_test)

# cli tool
add_executable(${PROJECT_NAME}_cli
    test/vfscli.cpp
)
target_link_libraries(${PROJECT_NAME}_cli ${PROJECT_NAME} absl::flags absl::flags_parse absl::time)
