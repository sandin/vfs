#ifndef VFS_FILESERVER_H
#define VFS_FILESERVER_H

#include "object.grpc.pb.h"
#include "leveldbfilesystem.h"
#include "grpc++/server_builder.h"
#include "grpc++/server.h"

namespace vfs {

class NameServiceImpl : public NameService::Service {
 public:
  explicit NameServiceImpl(LevelDbFileSystem* fs);
  virtual ~NameServiceImpl() override;

  virtual ::grpc::Status ListDirectory(::grpc::ServerContext* context, const ::vfs::PathRequest* request, ::grpc::ServerWriter<::vfs::ABObject>* writer) override;
  virtual ::grpc::Status FindObject(::grpc::ServerContext* context, const ::vfs::PathRequest* request, ::vfs::ABObject* response) override;

 private:
  LevelDbFileSystem* fs_ = nullptr;
};

class NameServer {
 public:
  NameServer(LevelDbFileSystem* fs, int port);
  virtual ~NameServer();

  void Start();
  void Stop();
 private:
 int port_;
 NameServiceImpl* service_ = nullptr;
 std::unique_ptr<grpc::Server> server_ = nullptr;
};

} // namespace vfs

#endif // VFS_FILESERVER_H