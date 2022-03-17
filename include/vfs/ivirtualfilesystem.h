#ifndef VFS_IVIRTUALFILESYSTEM_H
#define VFS_IVIRTUALFILESYSTEM_H

#include <string>
#include <list>
#include <memory>
#include "object.pb.h"

namespace vfs {


class IVirtualFileSystem
{
 public:
  using ObjectList = std::list<std::shared_ptr<ABObject>>;

  virtual ~IVirtualFileSystem() {}

  virtual bool CreateObject(ABObject* object, int64_t parent_id) = 0;
  virtual bool CreateObjects(ObjectList objects, int64_t parent_id) = 0;
  virtual bool CreateObjects(ObjectList objects) = 0;

  virtual bool RemoveObject(ABObject* object) = 0;
  virtual bool RemoveObject(const std::string& path) = 0;
  virtual bool DeleteDirectory(ABObject* parent, bool recursive, std::vector<std::string>* file_removed) = 0;
  virtual bool DeleteDirectory(const std::string& parent_path, bool recursive, std::vector<std::string>* file_removed) = 0;

  virtual bool UpdateObject(ABObject* object) = 0;
  virtual bool UpdateObjects(ObjectList objects, int64_t parent_id) = 0;

  virtual bool RenameObject(const std::string& old_path, const std::string& new_path) = 0;

  virtual ObjectList ListDirectory(int64_t parent_id) const = 0;
  virtual ObjectList ListDirectory(const std::string& parent_path) const = 0;

  virtual std::shared_ptr<ABObject> FindObject(const std::string& path) const = 0;
  virtual std::shared_ptr<ABObject> FindObject(const std::string& name, int64_t parent_id) const = 0;

  virtual int64_t Version(bool forceRead = false) = 0;
  virtual bool UpdateVersion(int64_t version) = 0;
};

} // namespace vfs

#endif // VFS_IVIRTUALFILESYSTEM_H
