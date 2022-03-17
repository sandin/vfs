#include <iostream>
#include <memory>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/time/time.h"
#include "grpc++/channel.h"
#include "grpc++/create_channel.h"

#include "ivirtualfilesystem.h"
#include "leveldbfilesystem.h"
#include "clientfilesystem.h"
#include "nameserver.h"
#include "ab.h"
#include "utils.h"

using namespace vfs;

ABSL_FLAG(std::string, s, "", "server");
ABSL_FLAG(bool, f, false, "force");
ABSL_FLAG(bool, r, false, "recursive");
ABSL_FLAG(bool, v, false, "verbose");
ABSL_FLAG(std::string, path, "", "path");
ABSL_FLAG(std::string, dbpath, "", "dbpath");
ABSL_FLAG(int, port, 5555, "server port");

static std::string USAGE = "USAGE: vfs <command> -s <server> --path <path>\n";


void _ls(IVirtualFileSystem* fs, std::shared_ptr<ABObject> parent, void* ctx) {
     std::string* parent_path = static_cast<std::string*>(ctx);
     if (isDirectory(parent->flags())) {
        IVirtualFileSystem::ObjectList list = fs->ListDirectory(parent->handle());
        *parent_path += parent->name() + "/";
        std::cout << *parent_path << std::endl;
        for (auto item : list) {
            _ls(fs, item, parent_path);
        }
    } else {
        std::cout << *parent_path << parent->name() << std::endl;
    }
}

int ls(IVirtualFileSystem* fs, const std::string& path, bool recursive, bool verbose) {
    std::shared_ptr<ABObject> parent = fs->FindObject(path);
    if (!parent) {
        std::cout << path << " path is not exists!" << std::endl;
        return 1;
    }

    if (recursive) {
        std::string parent_path = "";
        _ls(fs, parent, &parent_path);
    } else {
        if (isDirectory(parent->flags())) {
            IVirtualFileSystem::ObjectList list = fs->ListDirectory(parent->handle());
            for (auto item : list) {
                if (isDirectory(item->flags())) {
                    if (verbose) {
                        std::cout << item->ShortDebugString() << std::endl;
                    } else {
                        std::cout << item->name() << "/" << std::endl;
                    }
                } else {
                    if (verbose) {
                        std::cout << item->ShortDebugString() << std::endl;
                    } else {
                        std::cout << item->name() << std::endl;
                    }
                }
            }
        } else {
            if (verbose) {
                std::cout << parent->ShortDebugString() << std::endl;
            } else {
                std::cout << parent->name() << std::endl;
            }
        }
    }
    return 0;
}

int touch(IVirtualFileSystem* fs, const std::string& path, const std::string& name, bool force) {
    std::shared_ptr<ABObject> parent = fs->FindObject(path);
    if (!parent) {
        std::cout << path << " path is not exists!" << std::endl;
        return 1;
    }
    if (!isDirectory(parent->flags())) {
        std::cout << path << " path is not a directory!" << std::endl;
        return 1;
    }

    if (!force && fs->FindObject(name, parent->handle())) {
        std::cout << name << " is already exists in " << path << "!" << std::endl;
        return 1;
    }

    std::shared_ptr<ABObject> file = std::make_shared<ABObject>();
    file->set_handle(ToUnixMillis(absl::Now()));
    file->set_parenthandle(parent->handle());
    file->set_flags(AB_FLAG_FILE_ITEM);
    file->set_name(name);
    if (fs->CreateObject(file.get(), parent->handle())) {
        std::cout << "create file" << name << " in " << path << std::endl;
    } else {
        std::cout << "can not create file" << name << std::endl;
        return 1;
    }
    return 0;
}

int mkdir(IVirtualFileSystem* fs, const std::string& path, const std::string& name) {
    std::shared_ptr<ABObject> parent = fs->FindObject(path);
    if (!parent) {
        std::cout << path << " path is not exists!" << std::endl;
        return 1;
    }

    if (fs->FindObject(name, parent->handle())) { // TODO: && isDirectory(parent->flags())
        std::cout << name << " is already exists in folder " << path << "!" << std::endl;
        return 1;
    }

    std::shared_ptr<ABObject> file = std::make_shared<ABObject>();
    file->set_handle(ToUnixMillis(absl::Now()));
    file->set_parenthandle(parent->handle());
    file->set_flags(0);
    file->set_name(name);
    if (fs->CreateObject(file.get(), parent->handle())) {
        std::cout << "create folder " << name << " in " << path << std::endl;
    } else {
        std::cout << "can not create folder" << name << std::endl;
        return 1;
    }
    return 0;
}

int rm(IVirtualFileSystem* fs, const std::string& path, const std::string& name, bool force, bool recursive) {
    std::shared_ptr<ABObject> parent = fs->FindObject(path);
    if (!parent) {
        std::cout << path << " path is not exists!" << std::endl;
        return 1;
    }

    IVirtualFileSystem::ObjectList list;
    if (name == ".") {
        list = fs->ListDirectory(parent->handle());
    } else {
        std::shared_ptr<ABObject> object = fs->FindObject(name, parent->handle());
          if (!object) { 
            std::cout << name << " is not exists in " << path << "!" << std::endl;
            return 1;
        }
        list.emplace_back(object);
    }

    for (auto object : list) {
        if (isDirectory(object->flags())) {
            std::vector<std::string> file_removed;
            if (fs->DeleteDirectory(object.get(), recursive, &file_removed)) {
                for (std::string file : file_removed) {
                    std::cout << "Delete " << file << std::endl;
                }
            } else {
                std::cout << "Failed to delete " << object->name() << std::endl;
            }
        } else {
            if (fs->RemoveObject(object.get())) {
                std::cout << "Delete " << object->name() << std::endl;
            } else {
                std::cout << "Failed to delete " << object->name() << std::endl;
            }
        }
    }
   
    return 0;
}

int mv(IVirtualFileSystem* fs, const std::string& path, const std::string& src_name, const std::string& dst_name, bool force, bool recursive) {
    std::shared_ptr<ABObject> src_obj = fs->FindObject(src_name);
    if (!src_obj) {
        std::cout << src_name << " source is not exists!" << std::endl;
        return 1;
    }

   if (!force && fs->FindObject(dst_name)) {
        std::cout << "dst " << dst_name << " is already exists!" << std::endl;
        return 1;
    }
    
    if (fs->RenameObject(src_name, dst_name)) {
        std::cout << "rename " << src_name << " -> " << dst_name << std::endl;
    } else {
        std::cout << "Can not rename " << src_name << " -> " << dst_name << std::endl;
    }
    return 0;
}

int backup(LevelDbFileSystem* fs, const std::string& output_filename) {
    fs->Backup(output_filename, true);
    return 0;
}

int loadbackup(LevelDbFileSystem* fs, const std::string& input_filename) {
    fs->LoadBackup(input_filename);
    return 0;
}

int main(int argc, char* argv[]) {
    std::vector<char*> args = absl::ParseCommandLine(argc, argv);
    std::string server = absl::GetFlag(FLAGS_s);
    std::string path = absl::GetFlag(FLAGS_path);
    std::string db_path = absl::GetFlag(FLAGS_dbpath);
    bool force = absl::GetFlag(FLAGS_f);
    bool recursive = absl::GetFlag(FLAGS_r);
    bool verbose = absl::GetFlag(FLAGS_v);
    int port = absl::GetFlag(FLAGS_port);

    if (args.size() < 2) {
        std::cout << USAGE;
        return 1;
    }
    if (db_path.empty()) {
        db_path = "./tmp";
    }
    if (path.empty()) {
        path = "/";
    }

    IVirtualFileSystem* fs = nullptr;
    std::shared_ptr<grpc::Channel> channel;
    if (!server.empty()) {
        channel = grpc::CreateChannel(server, grpc::InsecureChannelCredentials());
        if (!channel->WaitForConnected(gpr_time_add(
              gpr_now(GPR_CLOCK_REALTIME),
              gpr_time_from_seconds(10, GPR_TIMESPAN)))) {
            std::cout << "Timeout: can not connect to server " << server << std::endl;
            return 1;
        }

        std::cout << "connected to server " << server << std::endl;
        fs = new ClientFileSystem(channel);
    } else {
        fs = new LevelDbFileSystem(db_path);
        if (verbose) {
            //static_cast<LevelDbFileSystem*>(fs)->Dump();
        }
    }


    std::string command = args[1];
    //std::cout << "execute command: " << command << std::endl;
    if (command == "ls" || command == "list") {
        return ls(fs, path, recursive, verbose);
    } else if (command == "touch") {
        if (args.size() < 3) {
            std::cout << "missing `name` arg" << std::endl;
            return 1;
        }
        std::string name = args[2];
        return touch(fs, path, name, force);
    } else if (command == "mkdir") {
        if (args.size() < 3) {
            std::cout << "missing `name` arg" << std::endl;
            return 1;
        }
        std::string name = args[2];
        return mkdir(fs, path, name);
    } else if (command == "rm") {
        if (args.size() < 3) {
            std::cout << "missing `name` arg" << std::endl;
            return 1;
        }
        std::string name = args[2];
        return rm(fs, path, name, force, recursive);
    } else if (command == "mv") {
        if (args.size() < 3) {
            std::cout << "missing `src_name` arg" << std::endl;
            return 1;
        }
        if (args.size() < 4) {
            std::cout << "missing `dst_name` arg" << std::endl;
            return 1;
        }
        std::string src_name = args[2];
        std::string dst_name = args[3];
        return mv(fs, path, src_name, dst_name, force, recursive);
    } else if (command == "backup") {
        if (args.size() < 3) {
            std::cout << "missing `output_filename` arg" << std::endl;
            return 1;
        }
        std::string output_filename = args[2];
        return backup(static_cast<LevelDbFileSystem*>(fs), output_filename);
    } else if (command == "loadbackup") {
        if (args.size() < 3) {
            std::cout << "missing `input_filename` arg" << std::endl;
            return 1;
        }
        std::string input_filename = args[2];
        return loadbackup(static_cast<LevelDbFileSystem*>(fs), input_filename);
    } else if (command == "startserver") {
        NameServer server(static_cast<LevelDbFileSystem*>(fs), port);
        server.Start();
    } else {
        std::cout << "unknown command: " << command << std::endl;
    }
}
