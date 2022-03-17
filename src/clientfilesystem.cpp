#include "clientfilesystem.h"

#include <iostream>

#include "utils.h"

using namespace vfs;
using namespace grpc;

ClientFileSystem::ClientFileSystem(std::shared_ptr<grpc::ChannelInterface> channel)
    : stub_(NameService::NewStub(channel)) {
  
}

ClientFileSystem::~ClientFileSystem() { }


IVirtualFileSystem::ObjectList ClientFileSystem::ListDirectory(int64_t parent_id) const {
    PathRequest request;
    request.set_handle(parent_id);
    return ListDirectory(request);
}

IVirtualFileSystem::ObjectList ClientFileSystem::ListDirectory(const std::string& parent_path) const {
    PathRequest request;
    request.set_name(parent_path);
    return ListDirectory(request);
}

IVirtualFileSystem::ObjectList ClientFileSystem::ListDirectory(PathRequest& request) const {
    ObjectList list; 
    ClientContext context;
    std::unique_ptr<ClientReader<ABObject>> reader(stub_->ListDirectory(&context, request));
    std::shared_ptr<ABObject> obj = std::make_shared<ABObject>();
    while (reader->Read(obj.get())) {
        list.emplace_back(obj);
        obj = std::make_shared<ABObject>();
    }
    Status status = reader->Finish();
    // TODO: check status
    return list;
}

std::shared_ptr<ABObject> ClientFileSystem::FindObject(const std::string& path) const {
    ClientContext context;
    PathRequest request;
    request.set_name(path);

    std::shared_ptr<ABObject> obj = std::make_shared<ABObject>();
    Status status = stub_->FindObject(&context, request, obj.get());
    // TODO: check status
    return obj;
}