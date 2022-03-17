#!/bin/bash

CUR_DIR=$(pwd)
./third_party/prebuild/Darwin-x86_64/grpc/Debug/bin/protoc -I${CUR_DIR}/proto --grpc_out=src ${CUR_DIR}/proto/object.proto --plugin=protoc-gen-grpc=./third_party/prebuild/Darwin-x86_64/grpc/Debug/bin/grpc_cpp_plugin
./third_party/prebuild/Darwin-x86_64/grpc/Debug/bin/protoc -I${CUR_DIR}/proto --cpp_out=include/vfs ${CUR_DIR}/proto/object.proto