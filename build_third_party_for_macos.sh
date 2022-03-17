#!/bin/bash

PWD_DIR=$(pwd)
CUR_DIR=$(pwd)/third_party
OUT_DIR=${CUR_DIR}/prebuild/Darwin-x86_64
rm -rf ${OUT_DIR}
mkdir ${CUR_DIR}

echo "Build leveldb"
cd ${CUR_DIR}
rm -rf leveldb
git clone git@github.com:google/leveldb.git
cd leveldb
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=${OUT_DIR}/leveldb/RelWithDebInfo ..
cmake --build . --config RelWithDebInfo --target install
cd ..
rm -rf build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${OUT_DIR}/leveldb/Debug ..
cmake --build . --config Debug --target install

echo "Build gRPC"
cd ${CUR_DIR}
rm -rf grpc
git clone -b v1.43.0 https://github.com/grpc/grpc
cd grpc
git submodule update --init --recursive
mkdir release && cd release
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=${OUT_DIR}/grpc/RelWithDebInfo ..
cmake --build . --config RelWithDebInfo --target install
cd ..
rm -rf release
mkdir release && cd release
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=${OUT_DIR}/grpc/Debug ..
cmake --build . --config Debug --target install


cd ${PWD_DIR}


