#include "FileSystem.h"
#include "../DFSFile.h"

#include <iostream>
#include <algorithm>

FileSystem::FileSystem()
{
}

FileSystem::~FileSystem()
{
    for (DFSFile* p : dfsFiles) {
        delete p;
    }
    dfsFiles.clear();
}

bool FileSystem::mount(std::string pathName, int searchPriority)
{
    // Read the DFS dictionary but not the sub-file daata.
    DFSFile* dfsFile = new DFSFile(searchPriority);
    bool     success = dfsFile->read(pathName + ".DFS", true);

    if (!success) {
        std::cout << "Failed to read " << pathName << std::endl;;
        delete dfsFile;
        dfsFile = nullptr;
    } else {
        std::cout << "Read DFS, path is " << dfsFile->getPath() << std::endl;
        dfsFiles.push_back(dfsFile);
        // Keep sorted in search priority. If equal, first mount wins.
        std::stable_sort(dfsFiles.begin(), dfsFiles.end());
    }

    return success;
}

uint8_t* FileSystem::readFile(std::string filename, int& len_out)
{
    uint8_t* data = nullptr;
    len_out = 0;

    size_t lastDotpos = filename.rfind('.', filename.length());
    if (lastDotpos == std::string::npos) {
        // no extension.
        return data;
    }

    std::string basename = filename.substr(0, lastDotpos);
    std::string extension = filename.substr(lastDotpos, filename.length() - lastDotpos);

    for (DFSFile* dfs : dfsFiles) {
        int entry = dfs->findEntry(basename, extension);
        if (entry >= 0) {

            data = dfs->getFileData(entry);
            len_out = dfs->getFileSize(entry);
            break;
        }
    }

    return data;
}

DFSFile* FileSystem::getMount(std::string name)
{
    std::string dfsName = name + ".DFS";
    for (DFSFile* file : dfsFiles) {
        if (file->getPath() == dfsName) {
            return file;
        }
    }
    return nullptr;
}
