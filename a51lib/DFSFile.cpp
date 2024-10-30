#include "DFSFile.h"

#include <iostream>
#include <sstream>
#include <iomanip>



struct DFSSubfile
{
    uint32_t offset;
    uint32_t checksumIndex;
};

static_assert(sizeof(DFSSubfile) == 8, "DFSSubfile must be 8 bytes long");

struct DFSFileEntry
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
    DFSHeader(const uint8_t* data);
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

DFSHeader::DFSHeader(const uint8_t* data)
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

DFSFile::DFSFile()
{
    header = nullptr;
    dfsData = nullptr;
}

DFSFile::~DFSFile()
{
    delete header;
    delete[] dfsData;
    freeSubfileData();
}

void DFSFile::freeSubfileData()
{
    for (uint8_t* p : subFileData) {
        delete[] p;
    }
    subFileData.clear();
}

int DFSFile::numFiles() const
{
    if (header != nullptr && header->isValid()) {
        return header->numFiles;
    }
    return 0;
}

int DFSFile::getFileSize(int entryNo) const
{
    if (entryNo < 0 || entryNo >= numFiles() || header == nullptr || !header->isValid()) {
        return 0;
    }
    DFSFileEntry& file = header->files[entryNo];
    return file.length;
}

uint8_t* DFSFile::getFileData(int entryNo) const
{
    if (entryNo < 0 || entryNo >= numFiles() || header == nullptr || !header->isValid()) {
        return nullptr;
    }
    DFSFileEntry& file = header->files[entryNo];
    int subFileIdx = 0;
    while(subFileIdx < header->numSubFiles) {
        if (file.dataOffset < header->subFileTable[subFileIdx].offset){
            // assumes sub-file offsets are in descending order and entries do not span sub-files.
            break;
        }
        ++subFileIdx;
    }
    int subOffset = file.dataOffset;
    if (subFileIdx > 0){
        subOffset -= header->subFileTable[subFileIdx-1].offset;
    }
    return subFileData[subFileIdx] + subOffset;
}

std::string DFSFile::getFilename(int entryNo) const
{
    if (entryNo < 0 || entryNo >= numFiles() || header == nullptr || !header->isValid()) {
        return "";
    }

    DFSFileEntry& file = header->files[entryNo];
    std::string filename;
    // pathName seems to have the original location on the build machine.
    //filename += header->strings + file.pathNameOffset;
    filename += header->strings + file.fileNameOffset1;
    filename += header->strings + file.fileNameOffset2;
    filename += header->strings + file.extNameOffset;

    return filename;
}

std::string DFSFile::getBaseFilename(int entryNo) const
{
    if (entryNo < 0 || entryNo >= numFiles() || header == nullptr || !header->isValid()) {
        return "";
    }

    DFSFileEntry& file = header->files[entryNo];
    std::string filename;
    filename += header->strings + file.fileNameOffset1;
    filename += header->strings + file.fileNameOffset2;

    return filename;
}

std::string DFSFile::getFileExtension(int entryNo) const
{
    if (entryNo < 0 || entryNo >= numFiles() || header == nullptr || !header->isValid()) {
        return "";
    }

    DFSFileEntry& file = header->files[entryNo];
    std::string ext = header->strings + file.extNameOffset;
    return ext;
}

static uint8_t* readFile(std::string path, size_t& size)
{
    uint8_t* data = nullptr;
    FILE* file = fopen(path.c_str(), "rb");
    if (file != nullptr) {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        rewind(file);

        data = new uint8_t[size];

        fread(data, 1, size, file);
        fclose(file);
    }
    return data;
}

void DFSFile::read(std::string path)
{
    this->path = path;
    delete[] dfsData;
    dfsData = nullptr;
    delete header;
    header = nullptr;

    size_t size = 0;
    dfsData = readFile(path, size);
    if (dfsData != nullptr) {
        delete header;
        header = new DFSHeader(dfsData);
    }
    if (header != nullptr && header->isValid()) {
        readDataFiles();
    }
}

void DFSFile::readDataFiles()
{
    freeSubfileData();
    int subfileNum = 0;
    std::string pathNoExt = path.substr(0, path.length() - 3);
    for (int i = 0; i < header->numSubFiles; ++i) {
        size_t size = 0;
        std::ostringstream ss;
        ss << pathNoExt << std::setfill('0') << std::setw(3) << i;
        uint8_t* p = readFile(ss.str(), size);
        subFileData.push_back(p);
    }
}
