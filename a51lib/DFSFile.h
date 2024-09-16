#pragma once

#include <string>

class DFSHeader;

class DFSFile
{
public:
    DFSFile();
    ~DFSFile();
    void read(std::string filename);

private:
    unsigned char* buffer;
    long size;
    DFSHeader* header;
};
