#pragma once

#include <string>
#include <vector>

class DFSFile;

class FileSystem
{
    public:
        FileSystem();
        ~FileSystem();
        bool mount(std::string pathname, int searchPriority);

        DFSFile* getMount(std::string name);

        uint8_t* readFile(std::string filename, int& len_out);

    private:
        std::vector<DFSFile*> dfsFiles;
};
