#include "DFSFile.h"

#include <iostream>

DFSFile::DFSFile()
{
    buffer = nullptr;
    size = 0;
}

DFSFile::~DFSFile()
{
    if (buffer) {
        free(buffer);
        buffer = nullptr;
    }
}

class DFSSubfile
{
    uint32_t offset;
    uint32_t checksumIndex;
};

static_assert(sizeof(DFSSubfile) == 8, "DFSSubfile must be 24 bytes long");

class DFSFileEntry
{
    uint32_t fileNameOffset1;
    uint32_t fileNameOffset2;
    uint32_t pathNameOffset;
    uint32_t extNameOffset;
    uint32_t dataOffset;
    uint32_t length;
};

static_assert(sizeof(DFSFileEntry) == 24, "DFSFileEntry must be 24 bytes long");

class DFSHeader
{
public:
    DFSHeader(const unsigned char* data);
    bool isValid()
    {
        return magic == 'XDFS' && version == 3;
    }

    unsigned int magic;
    unsigned int version;
    unsigned int checksum;
    unsigned int sectorSize;
    unsigned int splitSize;
    unsigned int numFiles;
    unsigned int numSubFiles;
    unsigned int stringsLengthBytes;
    DFSSubfile* subFileTable;
    DFSFileEntry* files;
    uint16_t* checksums;
    char* strings;
};

DFSHeader::DFSHeader(const unsigned char* data)
{
    const uint32_t* u32Data = (const uint32_t*)data;
    magic = u32Data[0];
    version = u32Data[1];
    checksum = u32Data[2];
    if (isValid()) {
        sectorSize = u32Data[3];
        splitSize = u32Data[4];
        numFiles = u32Data[5];
        numSubFiles = u32Data[6];
        stringsLengthBytes = u32Data[7];
        subFileTable = (DFSSubfile*)(data + u32Data[8]);
        files = (DFSFileEntry*)(data + u32Data[9]);
        checksums = (uint16_t*)(data + u32Data[10]);
        strings = (char*)(data + u32Data[11]);
    } else {
        std::cerr << "DFS File is not valid" << std::endl;
    }
}

void DFSFile::read(std::string path)
{
    if (buffer) {
        free(buffer);
        buffer = nullptr;
    }

    FILE* file = fopen(path.c_str(), "r");
    if (file != nullptr) {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        rewind(file);

        buffer = (unsigned char*)malloc(size);

        fread(buffer, 1, size, file);
        fclose(file);

        delete header;
        header = new DFSHeader(buffer);
    }
}
