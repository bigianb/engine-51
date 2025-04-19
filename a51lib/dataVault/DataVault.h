#pragma once

#include <cstdint>

#define DATA_VAULT_NAME_LEN 31
#define MAX_DATA_BLOCK_COLLECTIONS 32

class data_vault;
class data_handle;
struct data_block;

enum data_type
{
    DATA_TYPE_NONE = 0,
    DATA_TYPE_TWEAK = 1,
    DATA_TYPE_PAIN_PROFILE = 4,
    DATA_TYPE_HEALTH_PROFILE = 5,
    DATA_TYPE_PAIN_HEALTH_PROFILE = 6,
    DATA_TYPE_MAX_TYPES,
};

class data_id
{
public:
    data_id();
    ~data_id();

    void           Setup(const char* pDataDescriptorName, data_type Type);
    void           SetName(const char* pDataDescriptorName);
    void           SetType(data_type Type);
    data_type      GetType() const;
    uint32_t       GetValue() const;
    void           SetValue(uint32_t Value);
    bool           IsValid() const;
    bool           operator==(const data_id& ID) const;
    bool           operator!=(const data_id& ID) const;
    const data_id& operator=(const data_id& ID);

private:
    uint32_t m_Value;
};

class data_handle
{
public:
    data_handle();
    ~data_handle();

    void      Setup(const char* pDataDescriptorName, data_type Type);
    void      SetName(const char* pDataDescriptorName);
    void      SetType(data_type Type);
    data_type GetType() const;

    const data_block* GetData() const;
    bool              IsValid() const;

    const data_id& GetDataID() const;
    void           SetDataID(const data_id& ID);

private:
    data_id m_DataID;

    friend data_vault;
};

//==============================================================================
// DATA_BLOCK
//==============================================================================

struct data_block
{
    data_block();
    ~data_block();
    void           Setup(const char* pDataDescriptorName, data_type Type);
    void           SetName(const char* pDataDescriptorName);
    void           SetType(data_type Type);
    data_type      GetType() const;
    const data_id& GetDataID() const;
    void           SetDataID(const data_id& ID);

private:
    data_id m_DataID;

    friend data_vault;
};

//==============================================================================
// DATA_VAULT
//==============================================================================

class data_vault
{
    //------------------------------------------------------------------------------
    //  Public Functions
    //------------------------------------------------------------------------------

public:
    data_vault();
    ~data_vault();

    void Init();
    void Kill();

    const data_block* GetData(const data_handle& Handle);

    void AddDataBlocks(const char*       pDataBlockCollectionName,
                       const data_block* pBase,
                       int               nBlocks,
                       int               SizeOfBlock);

    data_block* DelDataBlocks(const char* pDataBlockCollectionName);

    void SanityCheck();

    //------------------------------------------------------------------------------
    //  Security Functions
    //------------------------------------------------------------------------------
    // There are four tables of data blocks inside the data vault:
    // iDBC     CollectionName      C++ Structure
    //------------------------------------------------------------------------------
    // 0        "TWDATA"            tweak_data_block
    // 1        "P_PROFILE"         pain_profile
    // 2        "H_PROFILE"         health_profile
    // 3        "PH_PROFILE"        pain_health_profile
    //------------------------------------------------------------------------------

    // If default parameters are used then entire hash table or DBC are checksummed
    uint32_t ChecksumHashTable(int iFirstEntry = -1, int iLastEntry = -1);
    uint32_t ChecksumData(int iDBC, int iFirstBlock = -1, int iLastBlock = -1);

    // Copies raw data for nBlocks into the DBC starting at a particular block
    void WriteData(int iDBC, int iBlock, int nBlocks, uint8_t* pData);

    //------------------------------------------------------------------------------
    //  Private Functions
    //------------------------------------------------------------------------------
private:
    void              Clear();
    void              BuildHashTable();
    void              AddDataBlockToHashTable(const data_block* pBlock, int iDBC, int iBlock);
    const data_block* ResolveBlockPtr(int iDBC, int iBlock);

    //------------------------------------------------------------------------------
    //  Private Data
    //------------------------------------------------------------------------------
    struct data_block_collection
    {
#ifdef DATA_VAULT_KEEP_NAMES
        char m_Name[DATA_VAULT_NAME_LEN + 1];
#endif
        uint32_t m_NameHash;
        uint8_t* m_pBase;
        int      m_SizeOfBlock;
        int      m_nBlocks;
        int      m_TotalSize;
    };

    struct hash_entry
    {
        data_id m_DataID;
        int16_t m_iDBC;
        int16_t m_iBlock;
    };

    data_block_collection m_DataBlockCollection[MAX_DATA_BLOCK_COLLECTIONS];
    hash_entry*           m_pHash;
    int                   m_nHashEntries;
    int                   m_nDataEntries;
};

//==============================================================================
// GLOBALS
//==============================================================================

extern data_vault g_DataVault;
