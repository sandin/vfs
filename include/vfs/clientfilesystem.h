#ifndef VFS_CLIENTFILESYSTEM_H
#define VFS_CLIENTFILESYSTEM_H

#include "ivirtualfilesystem.h"

#include <string>
#include <list>
#include <memory>
#include "grpc++/channel.h"

#include "ab.h"
#include "object.pb.h"
#include "object.grpc.pb.h"

namespace vfs {

class ClientFileSystem : public IVirtualFileSystem
{
 public:
  ClientFileSystem(std::shared_ptr<grpc::ChannelInterface> channel);
  virtual ~ClientFileSystem() override;

  virtual bool CreateObject(ABObject* object, int64_t parent_id) override { return false; };
  virtual bool CreateObjects(ObjectList objects, int64_t parent_id) override { return false; };
  virtual bool CreateObjects(ObjectList objects) override { return false; };

  virtual bool RemoveObject(ABObject* object) override { return false; };
  virtual bool RemoveObject(const std::string& path) override { return false; };
  virtual bool DeleteDirectory(ABObject* parent, bool recursive, std::vector<std::string>* file_removed) override { return false; };
  virtual bool DeleteDirectory(const std::string& parent_path, bool recursive, std::vector<std::string>* file_removed) override { return false; };

  virtual bool UpdateObject(ABObject* object) override { return false; };
  virtual bool UpdateObjects(ObjectList objects, int64_t parent_id) override { return false; };

  virtual bool RenameObject(const std::string& old_path, const std::string& new_path) override { return false; };

  virtual ObjectList ListDirectory(int64_t parent_id) const override;
  virtual ObjectList ListDirectory(const std::string& parent_path) const override;

  virtual std::shared_ptr<ABObject> FindObject(const std::string& path) const override;
  virtual std::shared_ptr<ABObject> FindObject(const std::string& name, int64_t parent_id) const override { return nullptr; };

  virtual int64_t Version(bool forceRead = false) override { return 0; };
  virtual bool UpdateVersion(int64_t version) override { return false; };

 private:
  IVirtualFileSystem::ObjectList ListDirectory(PathRequest& request) const;
  std::unique_ptr<NameService::Stub> stub_;
};

} // namespace vfs

#endif // VFS_CLIENTFILESYSTEM_H
