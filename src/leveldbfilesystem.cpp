#include "leveldbfilesystem.h"

#include <iostream>
#include <fstream>
#include <stdio.h>

#include "leveldb/cache.h"
#include "leveldb/write_batch.h"
#include "absl/strings/str_split.h"
#include "absl/strings/numbers.h"
#include "absl/time/time.h"
#include "absl/time/clock.h"
#include "zlib.h"
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#include "utils.h"

using namespace vfs;

const int64_t kRootHandle = 1;
const int64_t kCacheSizeInMb = 100;
const uint8_t kBackupFileVersion = 0x2;

LevelDbFileSystem::LevelDbFileSystem(const std::string& dbPath)
    : dbPath_(dbPath), version_(0) {
  leveldb::Options options;
  options.create_if_missing = true;
  dbCache_ = leveldb::NewLRUCache(kCacheSizeInMb * 1024L * 1024L);
  options.block_cache = dbCache_;
  leveldb::Status status = leveldb::DB::Open(options, dbPath_, &this->db_);
  if (!status.ok()) {
    printf("Can not open db %s", dbPath_.c_str());
  }

  rootNode_.set_handle(kRootHandle);
  rootNode_.set_name("");
  rootNode_.set_dbpath("/");
}

LevelDbFileSystem::~LevelDbFileSystem() { delete this->db_; }

void LevelDbFileSystem::EncodingStoreKey(int64_t entry_id,
                                         const std::string& path,
                                         std::string* key_str) const {
  key_str->resize(8);
  EncodeBigEndian(&(*key_str)[0], static_cast<uint64_t>(entry_id));
  key_str->append(path);
}

void LevelDbFileSystem::DecodingStoreKey(const std::string& key_str,
                                         int64_t* entry_id,
                                         std::string* path) const {
  assert(key_str.size() >= 8UL);
  if (entry_id) {
    *entry_id = static_cast<int64_t>(DecodeBigEndian(key_str.c_str()));
  }
  if (path) {
    path->assign(key_str, 8, std::string::npos);
  }
}

void LevelDbFileSystem::Dump() const {
  std::cout << "--------------VFS DUMP----------------" << std::endl;
  int i = 0;
  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ABObject obj;
    obj.ParseFromArray(it->value().data(), it->value().size());
    std::cout << it->key().ToString() << ": `" << obj.ShortDebugString() << "`"
              << std::endl;
    i++;
  }
  delete it;
  std::cout << "----------------(" << i << ")-----------------" << std::endl;
}

void LevelDbFileSystem::Backup(const std::string& filepath, bool compress) const {
  std::ofstream output(filepath, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);

  if (!compress) {
    output << "VFS";
    output << (uint8_t)kBackupFileVersion;
    output << (uint8_t)compress;
    output << (int64_t)version_;
  }
  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  uint32_t key_size, value_size;
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    key_size = it->key().size();
    output.write((char*)&key_size, sizeof(uint32_t));
    output.write((char*)it->key().data(), it->key().size());

    value_size = it->value().size();
    output.write((char*)&value_size, sizeof(uint32_t));
    output.write((char*)it->value().data(), it->value().size());
  }
  output.close();
  delete it;

  if (!compress) {
    return;
  }

  std::string dest = filepath + ".zip";
  compress_backup_file(filepath.c_str(), kBackupFileVersion, version_, dest.c_str());
  remove(filepath.c_str());
  rename(dest.c_str(), filepath.c_str());
}

void LevelDbFileSystem::LoadBackup(const std::string& filepath) const {
  std::string dest = filepath + ".tmp";
  int64_t db_version = 0;
  decompress_backup_file(filepath.c_str(), dest.c_str(), &db_version);

  std::ifstream input(dest.c_str(), std::ofstream::in | std::ofstream::binary);
  uint32_t key_size;
  uint32_t value_size;
  std::string key;
  std::string value;
  leveldb::WriteBatch batch;

  std::string version_key(8, 0);
  version_key.append("version");
  std::string version_str = std::to_string(db_version); 
  batch.Put(version_key, version_str);

  while (!input.eof()) {
    // read key
    input.read((char*)&key_size, sizeof(uint32_t));
    key.resize(key_size, '\0');
    input.read(&key[0], key_size);

    // read value
    input.read((char*)&value_size, sizeof(uint32_t));
    value.resize(value_size, '\0');
    input.read(&value[0], value_size);

    batch.Put(key, value);
  }
  leveldb::Status s = db_->Write(leveldb::WriteOptions(), &batch);
  if (!s.ok()) {
    std::cout << "Error: can not LoadBackup file: " << filepath << std::endl;
  }


  input.close();
  remove(dest.c_str());
}

bool LevelDbFileSystem::Clear() const {
  leveldb::WriteBatch batch;
  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    batch.Delete(it->key());
  }
  delete it;
  return db_->Write(leveldb::WriteOptions(), &batch).ok();
}

bool LevelDbFileSystem::CreateObject(ABObject* object, int64_t parent_id) {
  if (parent_id == -1) {
    parent_id = kRootHandle;
  }
  object->set_parenthandle(parent_id);

  std::string key;
  EncodingStoreKey(parent_id, object->name(), &key);
  std::string value;
  object->SerializeToString(&value);
  return db_->Put(leveldb::WriteOptions(), key, value).ok();
}

bool LevelDbFileSystem::CreateObjects(ObjectList objects, int64_t parent_id) {
  if (parent_id == -1) {
    parent_id = kRootHandle;
  }

  leveldb::WriteBatch batch;
  std::string key;
  std::string value;
  for (auto obj : objects) {
    obj->set_parenthandle(parent_id);
    obj->SerializeToString(&value);
    EncodingStoreKey(parent_id, obj->name(), &key);
    // printf("key: %s\n", key.c_str());
    batch.Put(key, value);
  }
  leveldb::Status s = db_->Write(leveldb::WriteOptions(), &batch);
  return s.ok();
}

bool LevelDbFileSystem::CreateObjects(ObjectList objects) {
  leveldb::WriteBatch batch;
  std::string key;
  std::string value;
  for (auto obj : objects) {
    obj->SerializeToString(&value);
    EncodingStoreKey(obj->parenthandle(), obj->name(), &key);
    // printf("key: %s\n", key.c_str());
    batch.Put(key, value);
  }
  leveldb::Status s = db_->Write(leveldb::WriteOptions(), &batch);
  return s.ok();
}

bool LevelDbFileSystem::RemoveObject(ABObject* object) {
  std::string key;
  EncodingStoreKey(object->parenthandle(), object->name(), &key);
  return db_->Delete(leveldb::WriteOptions(), key).ok();
}

bool LevelDbFileSystem::RemoveObject(const std::string& path) {
  std::shared_ptr<ABObject> obj = FindObject(path);
  if (!obj) {
    return false;
  }
  return RemoveObject(obj.get());
}

bool LevelDbFileSystem::DeleteDirectory(ABObject* parent, bool recursive, std::vector<std::string>* files_removed) {
  return InternalDeleteDirectory(parent, recursive, files_removed);
}

bool LevelDbFileSystem::DeleteDirectory(const std::string& parent_path, bool recursive, std::vector<std::string>* files_removed) {
  std::shared_ptr<ABObject> parent = FindObject(parent_path);
  if (!parent) {
    return false;
  }
  return InternalDeleteDirectory(parent.get(), recursive, files_removed);
}

bool LevelDbFileSystem::InternalDeleteDirectory(ABObject* parent, bool recursive, std::vector<std::string>* files_removed) {
  if (!parent) {
    return false;
  }
  if (!isDirectory(parent->flags())) {
    return false;
  }

  int64_t parent_id = parent->handle();
  leveldb::WriteBatch batch;
  ObjectList children = ListDirectory(parent_id);
  std::string key;
  for (auto child : children) {
    if (recursive && isDirectory(child->flags())) {
      InternalDeleteDirectory(child.get(), recursive, files_removed);
    } else {
      EncodingStoreKey(parent_id, child->name(), &key);
      batch.Delete(key);
      (*files_removed).emplace_back(child->name());
    }
  }

  EncodingStoreKey(parent->parenthandle(), parent->name(), &key);
  batch.Delete(key);
  (*files_removed).emplace_back(parent->name());

  leveldb::Status s = db_->Write(leveldb::WriteOptions(), &batch);
  return s.ok();
}

bool LevelDbFileSystem::UpdateObject(ABObject* object) {
  std::string key;
  EncodingStoreKey(object->parenthandle(), object->name(), &key);

  std::string value;
  object->SerializeToString(&value);
  return db_->Put(leveldb::WriteOptions(), key, value).ok();
}

bool LevelDbFileSystem::UpdateObjects(ObjectList objects, int64_t parent_id) {
  return CreateObjects(objects, parent_id);
}

bool LevelDbFileSystem::RenameObject(const std::string& old_path, const std::string& new_path) {
  std::shared_ptr<ABObject> old_obj = FindObject(old_path);
  if (!old_obj) {
    return false;
  }
  //std::cout << "old_obj" << old_obj->name();

  std::vector<std::string> new_paths = absl::StrSplit(new_path, "/");
  if (new_paths.size() == 0) {
    return false;
  }

  int64_t parent_id = kRootHandle;
  std::shared_ptr<ABObject> obj;
  for (int i = 0; i < new_paths.size() - 1; ++i) {
    obj = FindObject(new_paths[i], parent_id);
    if (!obj) {
      return false;
    }
    parent_id = obj->handle();
    if (!isDirectory(obj->flags())) {
      return false;
    }
    if (obj->handle() == old_obj->handle()) {
      return false;
    }
  }

  leveldb::WriteBatch batch;

  std::string old_key;
  EncodingStoreKey(old_obj->parenthandle(), old_obj->name(), &old_key);
  batch.Delete(old_key);

  std::string new_key;
  std::string new_name = new_paths[new_paths.size() - 1];
  old_obj->set_name(new_name);
  old_obj->set_parenthandle(parent_id);
  EncodingStoreKey(parent_id, old_obj->name(), &new_key);

  std::string new_value;
  old_obj->SerializeToString(&new_value);
  batch.Put(new_key, new_value);

  return db_->Write(leveldb::WriteOptions(), &batch).ok();
}

IVirtualFileSystem::ObjectList LevelDbFileSystem::ListDirectory(int64_t parent_id) const {
   if (parent_id == -1) {
    parent_id = kRootHandle;
  }

  ObjectList list;

  std::string key_start, key_end;
  EncodingStoreKey(parent_id, "", &key_start);
  EncodingStoreKey(parent_id + 1, "", &key_end);
  leveldb::Iterator* it = db_->NewIterator(leveldb::ReadOptions());
  for (it->Seek(key_start); it->Valid(); it->Next()) {
    leveldb::Slice key = it->key();
    if (key.compare(key_end) >= 0) {
      break;
    }

    std::shared_ptr<ABObject> obj = std::make_shared<ABObject>();
    obj->ParseFromArray(it->value().data(),
                        static_cast<int>(it->value().size()));
    list.emplace_back(obj);
  }
  delete it;
  return list;
}

IVirtualFileSystem::ObjectList LevelDbFileSystem::ListDirectory(const std::string& parent_path) const {
  if (parent_path == "/" || parent_path == "") {
    return ListDirectory(kRootHandle);
  }

  std::shared_ptr<ABObject> parent = FindObject(parent_path);
  if (!parent) {
    return {}; // TODO: throw error, can not found parent node by path
  }
  return ListDirectory(parent->handle());
}

std::shared_ptr<ABObject> LevelDbFileSystem::FindObject(const std::string& path) const {
  std::shared_ptr<ABObject> obj = nullptr;
  if (path == "/" || path == "") {
    obj = std::make_shared<ABObject>();
    obj->CopyFrom(rootNode_);
    return obj;
  }

  std::vector<std::string> paths = absl::StrSplit(path, "/");
  if (paths.size() == 0) {
    return nullptr;
  }

  int64_t parent_id = kRootHandle;
  int64_t entry_id = kRootHandle;
  for (size_t i = 0; i < paths.size(); ++i) {
    obj = FindObject(paths[i], entry_id);
    if (!obj) {
      return nullptr;
    }
    parent_id = entry_id;
    entry_id = obj->handle();
  }
  return obj;
}

std::shared_ptr<ABObject> LevelDbFileSystem::FindObject(const std::string& name, int64_t parent_id) const {
  std::string key;
  EncodingStoreKey(parent_id, name, &key);
  std::string value;
  leveldb::Status s = db_->Get(leveldb::ReadOptions(), key, &value);
  if (s.ok()) {
    std::shared_ptr<ABObject> obj = std::make_shared<ABObject>();
    if (obj->ParseFromString(value)) {
      return obj; 
    }
  }
  return nullptr;
}

int64_t LevelDbFileSystem::Version(bool forceRead) {
  if (forceRead || version_ == 0) {
    std::string version_key(8, 0);
    version_key.append("version");
    std::string version_str;
    leveldb::Status s = db_->Get(leveldb::ReadOptions(), version_key, &version_str);
    if (s.ok()) {
      absl::SimpleAtoi(version_str, &version_);
    } else {
      version_ = ToUnixMillis(absl::Now());
      UpdateVersion(version_);
    }
  }
  return version_; 
}

bool LevelDbFileSystem::UpdateVersion(int64_t version) {
  std::string version_key(8, 0);
  version_key.append("version");
  if (version == 0) {
    version = ToUnixMillis(absl::Now());
  }
  std::string version_str = std::to_string(version); 
  if (db_->Put(leveldb::WriteOptions(), version_key, version_str).ok()) {
    version_ = version;
    return true;
  }
  return false;
}
