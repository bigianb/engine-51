#pragma once
#include <cstdint>
#include <string>
#include <vector>

class DFSHeader;

class DFSFile
{
public:
    DFSFile();
    ~DFSFile();
    void read(std::string filename);

    int numFiles() const;
    std::string getFilename(int entryNo) const;
    std::string getBaseFilename(int entryNo) const;
    std::string getFileExtension(int entryNo) const;
    int getFileSize(int entryNo) const;
    uint8_t* getFileData(int entryNo) const;
    int getVersion() const;

private:
    void readDataFiles();
    void freeSubfileData();
    uint32_t getSubfileOffset(int idx) const;

    std::string path;

    uint8_t* dfsData;
    DFSHeader* header;

    std::vector<uint8_t*> subFileData;
};
