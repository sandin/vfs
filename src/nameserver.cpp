#include "nameserver.h"

#include <string>
#include <iostream>

using namespace vfs;
using namespace grpc;

NameServiceImpl::NameServiceImpl(LevelDbFileSystem* fs) : fs_(fs) {
}

NameServiceImpl::~NameServiceImpl() {
}

Status NameServiceImpl::ListDirectory(ServerContext* context, const PathRequest* request, ServerWriter< ::vfs::ABObject>* writer) {
 std::cout << "ListDirectory " << request->name() << request->handle() << std::endl;
 IVirtualFileSystem::ObjectList list;
 if (request->has_name()) {
  list = fs_->ListDirectory(request->name());
 } else {
  list = fs_->ListDirectory(request->handle());
 }
 for (auto object : list) {
   writer->Write(*object.get());
 }
  return Status::OK;
}

Status NameServiceImpl::FindObject(ServerContext* context, const PathRequest* request, ABObject* response) {
 std::cout << "FindObject " << request->name() << request->handle() << std::endl;
 std::shared_ptr<ABObject> obj = fs_->FindObject(request->name());
 if (obj) {
   response->CopyFrom(*obj.get()); // TODO: need copy?
 }
 return Status::OK;
}

NameServer::NameServer(LevelDbFileSystem* fs, int port) : port_(port), service_(new NameServiceImpl(fs)) {
}

NameServer::~NameServer() {
  delete service_;
}

void NameServer::Start() {
  std::string server_address = "0.0.0.0:" + std::to_string(port_);
  std::cout << "listen at " << server_address << std::endl;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, InsecureServerCredentials());
  builder.RegisterService(service_);
  server_ = builder.BuildAndStart();
  server_->Wait();
}

void NameServer::Stop() {
  // TODO:
}