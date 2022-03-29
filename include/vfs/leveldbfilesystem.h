#ifndef VFS_LEVELDBFILESYSTEM_H
#define VFS_LEVELDBFILESYSTEM_H

#include "ivirtualfilesystem.h"

#include <string>
#include <list>
#include <memory>

#include "ab.h"
#include "object.pb.h"
#include "leveldb/db.h"

namespace vfs {

class LevelDbFileSystem : public IVirtualFileSystem
{
 public:
  LevelDbFileSystem(const std::string& dbPath);
  virtual ~LevelDbFileSystem() override;

  virtual bool CreateObject(ABObject* object, int64_t parent_id) override;
  virtual bool CreateObjects(ObjectList objects, int64_t parent_id) override;
  virtual bool CreateObjects(ObjectList objects) override;

  virtual bool RemoveObject(ABObject* object) override;
  virtual bool RemoveObject(const std::string& path) override;
  virtual bool DeleteDirectory(ABObject* parent, bool recursive, std::vector<std::string>* file_removed) override;
  virtual bool DeleteDirectory(const std::string& parent_path, bool recursive, std::vector<std::string>* file_removed) override;

  virtual bool UpdateObject(ABObject* object) override;
  virtual bool UpdateObjects(ObjectList objects, int64_t parent_id) override;

  virtual bool RenameObject(const std::string& old_path, const std::string& new_path) override;

  virtual ObjectList ListDirectory(int64_t parent_id) const override;
  virtual ObjectList ListDirectory(const std::string& parent_path) const override;

  virtual std::shared_ptr<ABObject> FindObject(const std::string& path) const override;
  virtual std::shared_ptr<ABObject> FindObject(const std::string& name, int64_t parent_id) const override;

  virtual int64_t Version(bool forceRead = false) override;
  virtual bool UpdateVersion(int64_t version) override;

  void Dump() const;
  void Backup(const std::string& filepath, bool compress = true) const;
  bool LoadBackup(const std::string& filepath);
  bool Clear() const;

 private:
  void EncodingStoreKey(int64_t entry_id, const std::string& path, std::string* key_str) const;
  void DecodingStoreKey(const std::string& key_str, int64_t* entry_id, std::string* path) const;
  bool InternalDeleteDirectory(ABObject* parent, bool recursive, std::vector<std::string>* files_removed);

  std::string dbPath_;
  leveldb::Cache* dbCache_;
  leveldb::DB* db_;
  ABObject rootNode_;
  int64_t version_;
};

} // namespace vfs

#endif // VFS_LEVELDBFILESYSTEM_H
