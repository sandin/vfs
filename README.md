# Virtual File System

The Virtual File System (VFS) is a distributed file system.

# Build


## Build third-party

libraries:
* [leveldb](https://github.com/google/leveldb)
* [protobuf](https://github.com/protocolbuffers/protobuf/blob/master/src/README.md)
* [gRPC](https://www.grpc.io/docs/languages/cpp/quickstart/)

Windows:
```
$ ./build_thrid_party.sh
```

macOS:
```
$ brew install autoconf automake libtool pkg-config    
$ ./build_thrid_party_for_macos.sh
```

## Protoc

Windows:
```
$ ./protoc.bat
```

macOS:
```
$ ./protoc_for_macos.sh
```

OUTPUT:
* protoc: <project_root>/include/object.ph.h
* grpc: <project_root>/src/object.grpc.ph.h

## Build vfs

Windows/macOS:
```
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
// or: cmake -DCMAKE_BUILD_TYPE=Debug ..
$ cmake --build . 
```


# Graph

```
              Server
       PULL   ┌─────┐   PUSH
    ┌─────────┤ VFS │◄────────┐
    │         └─────┘         │
    │                         │
    ▼                         │
 ┌─────┐ SYNC ┌─────┐ SYNC ┌──┴──┐
 │ VFS │◄────►│ VFS │◄────►│ VFS │
 └─────┘      └─────┘      └─────┘
 Client       Client       Client
```

# Cli

## Start Name Server

```
$ vfs_cli startserver 5555
listen at 0.0.0.0:5555
```

## List Remote Directory

```
$ vfs_cli -s 127.0.0.1:5555 ls --path /
connected to server localhost:5555
1.txt
2.txt
3.txt
```

## List Local Directory

```
$ vfs_cli ls --path /
1.txt
2/
```

recursive:
```
$ vfs_cli ls -r --path /
/
/1.txt
/dir1/
/dir1/dir2/
/dir1/dir2/file3
```

## Create File

```
$ vfs_cli touch 6.txt --path /
```

## Create Folder

```
$ vfs_cli mkdir testdir --path /
```

## Delete File

```
$ vfs_cli rm 6.txt --path /
```
## Delete Folder

```
$ vfs_cli rm -r testdir --path /
```

## Rename File/Folder

```
$ vfs_cli mv 1.jpg 2.jpg --path /
```
