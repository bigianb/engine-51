#pragma once
#include <cstdint>
#include <string>
#include <vector>

class DFSHeader;

class DFSFile
{
public:
    DFSFile(int searchPriorityIn = 0);
    ~DFSFile();
    bool read(std::string filename, bool headerOnly = true);

    std::string getPath() const
    {
        return path;
    }

    int         numFiles() const;
    std::string getFilename(int entryNo) const;
    std::string getBaseFilename(int entryNo) const;
    std::string getFileExtension(int entryNo) const;
    int         getFileSize(int entryNo) const;
    uint8_t*    getFileData(int entryNo);
    int         getVersion() const;

    int findEntry(std::string baseFilename, std::string extension) const;

    bool hasFileData()
    {
        return subFileData.size() > 0;
    }

    friend bool operator<(const DFSFile& l, const DFSFile& r)
    {
        return l.searchPriority < r.searchPriority;
    }

private:
    void     readDataFiles();
    void     freeSubfileData();
    uint32_t getSubfileOffset(int idx) const;

    int searchPriority;

    std::string path;

    uint8_t*   dfsData;
    DFSHeader* header;

    std::vector<uint8_t*> subFileData;
};
