

./third_party/prebuild/Windows-AMD64/grpc/Debug/bin/protoc.exe -I%cd%/proto --grpc_out=%cd%/src %cd%/proto/object.proto --plugin=protoc-gen-grpc=./third_party/prebuild/Windows-AMD64/grpc/Debug/bin/grpc_cpp_plugin.exe



./third_party/prebuild/Windows-AMD64/grpc/Debug/bin/protoc.exe -I%cd%/proto --cpp_out=include/vfs %cd%/protoc/object.proto