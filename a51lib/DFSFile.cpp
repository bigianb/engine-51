#include "DFSFile.h"

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
    int sectorSize;
    unsigned int splitSize;
    int numFiles;
    int numSubFiles;
    int stringsLengthBytes;
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
    if ()
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
