#include "DataVault.h"
#include "../dataUtil/Crc.h"

#include <cstring>
#include <cassert>

#define DO_LOG_SETNAME 0

#define DATA_ID_NULL (0)
#define DATA_ID_TYPE_BITS (4)
#define DATA_ID_NAMEHASH_BITS (32 - DATA_ID_TYPE_BITS)
#define DATA_ID_NAMEHASH_MASK ((1 << (DATA_ID_NAMEHASH_BITS)) - 1)
#define DATA_ID_TYPE_MASK (((1 << (DATA_ID_TYPE_BITS)) - 1) << (DATA_ID_NAMEHASH_BITS))
#define MAX_DATA_TYPES_POSSIBLE (1 << (DATA_ID_TYPE_BITS))

data_vault g_DataVault;

// clang-format off
// List of closest prime number above 1000*N (0>=N<=256)
static uint8_t PrimeList[257] =
{0,        
    9, 3, 1, 1, 3, 7, 1, 9, 1, 7, 3, 7, 1, 9, 13, 1,
    11, 13, 1, 11, 1, 3, 3, 1, 13, 3, 11, 1, 9, 11, 13, 3,
    13, 19, 23, 7, 3, 11, 19, 9, 11, 13, 3, 17, 7, 21, 17, 17,
    3, 21, 1, 9, 3, 1, 1, 3, 37, 13, 9, 13, 1, 3, 29, 7,
    3, 29, 3, 23, 1, 1, 11, 19, 9, 17, 11, 1, 3, 7, 31, 21,
    1, 3, 3, 11, 9, 11, 11, 1, 3, 1, 9, 3, 1, 7, 3, 1,
    1, 9, 13, 3, 9, 1, 1, 3, 19, 13, 21, 7, 1, 17, 29, 19,
    11, 1, 1, 9, 17, 33, 27, 11, 1, 11, 1, 1, 3, 1, 31, 21,
    1, 3, 9, 1, 13, 33, 7, 13, 29, 7, 21, 9, 23, 7, 53, 13,
    7, 9, 11, 13, 11, 1, 7, 3, 1, 1, 3, 7, 7, 3, 13, 1,
    9, 7, 3, 11, 1, 13, 9, 13, 3, 3, 7, 1, 21, 7, 3, 17,
    7, 1, 21, 1, 1, 9, 23, 3, 21, 7, 3, 11, 11, 27, 21, 7,
    3, 3, 23, 3, 3, 13, 21, 3, 7, 1, 11, 7, 19, 9, 13, 1,
    21, 11, 7, 29, 19, 3, 51, 23, 1, 3, 1, 9, 21, 7, 7, 11,
    23, 1, 11, 13, 3, 3, 1, 3, 21, 7, 3, 17, 11, 1, 17, 7,
    13, 9, 11, 3, 23, 11, 1, 21, 17, 7, 3, 1, 3, 3, 7, 19,
};
// clang-format on

int ChoosePrimeGreaterOrEqual(int I)
{
    int i = I / 1000;
    if (i * 1000 < I) {
        i++;
    }
    assert((i >= 0) && (i <= 256));
    return (i * 1000) + PrimeList[i];
}

data_id::data_id()
{
    m_Value = DATA_ID_NULL;
}

data_id::~data_id()
{
    m_Value = DATA_ID_NULL;
}

void data_id::Setup(const char* pDataDescriptorName, data_type Type)
{
    SetName(pDataDescriptorName);
    SetType(Type);
}

static inline uint32_t ComputeHash(const char* pStr)
{
    uint32_t Hash = 5381;
    int      C;

    // Process each character to generate the hash key
    while ((C = *pStr++)) {
        if ((C >= 'a') && (C <= 'z')) {
            C += ('A' - 'a');
        }
        Hash = ((Hash << 5) + Hash) ^ C;
    }

    return Hash;
}

void data_id::SetName(const char* pDataDescriptorName)
{
    assert(pDataDescriptorName);

    uint32_t Hash = ComputeHash(pDataDescriptorName);
    m_Value = (m_Value & DATA_ID_TYPE_MASK) | (Hash & DATA_ID_NAMEHASH_MASK);
}

void data_id::SetType(data_type Type)
{
    assert((Type > DATA_TYPE_NONE) && (Type < DATA_TYPE_MAX_TYPES));
    m_Value = (m_Value & DATA_ID_NAMEHASH_MASK) |
              ((((uint32_t)Type) << DATA_ID_NAMEHASH_BITS) & DATA_ID_TYPE_MASK);
}

data_type data_id::GetType() const
{
    return (data_type)(m_Value >> DATA_ID_NAMEHASH_BITS);
}

uint32_t data_id::GetValue() const
{
    return (uint32_t)m_Value;
}

void data_id::SetValue(uint32_t Value)
{
    m_Value = Value;
}

bool data_id::IsValid() const
{
    return (m_Value != DATA_ID_NULL);
}

const data_id& data_id::operator=(const data_id& ID)
{
    m_Value = ID.m_Value;

    return (*this);
}

bool data_id::operator==(const data_id& ID) const
{
    return (m_Value == ID.m_Value);
}

bool data_id::operator!=(const data_id& ID) const
{
    return (m_Value != ID.m_Value);
}

data_handle::data_handle()
{
}

data_handle::~data_handle()
{
}

void data_handle::Setup(const char* pDataDescriptorName, data_type Type)
{
    m_DataID.Setup(pDataDescriptorName, Type);
}

void data_handle::SetName(const char* pDataDescriptorName)
{
    m_DataID.SetName(pDataDescriptorName);
}

void data_handle::SetType(data_type Type)
{
    m_DataID.SetType(Type);
}

data_type data_handle::GetType() const
{
    return m_DataID.GetType();
}

const data_block* data_handle::GetData() const
{
    const data_block* pData = g_DataVault.GetData(*this);
    return pData;
}

bool data_handle::IsValid() const
{
    return m_DataID.IsValid();
}

const data_id& data_handle::GetDataID() const
{
    return m_DataID;
}

void data_handle::SetDataID(const data_id& ID)
{
    m_DataID = ID;
}

data_block::data_block()
{
}

data_block::~data_block()
{
}

void data_block::Setup(const char* pDataDescriptorName, data_type Type)
{
    m_DataID.Setup(pDataDescriptorName, Type);
}

void data_block::SetName(const char* pDataDescriptorName)
{
    m_DataID.SetName(pDataDescriptorName);
}

void data_block::SetType(data_type Type)
{
    m_DataID.SetType(Type);
}

data_type data_block::GetType() const
{
    return m_DataID.GetType();
}

const data_id& data_block::GetDataID() const
{
    return m_DataID;
}

void data_block::SetDataID(const data_id& ID)
{
    m_DataID = ID;
}

data_vault::data_vault()
{
    m_pHash = nullptr;
    Clear();
}

data_vault::~data_vault()
{
    Clear();
}

void data_vault::Kill()
{
    Clear();
}

void data_vault::Init()
{
    Clear();
    m_nHashEntries = 0;
    m_pHash = nullptr;
}

void data_vault::Clear()
{
    assert(DATA_TYPE_MAX_TYPES <= MAX_DATA_TYPES_POSSIBLE);

    // Clear Hash
    free(m_pHash);
    m_pHash = nullptr;
    m_nHashEntries = 0;
    m_nDataEntries = 0;

    // Clear data blocks
    for (int i = 0; i < MAX_DATA_BLOCK_COLLECTIONS; i++) {
        data_block_collection& DBC = m_DataBlockCollection[i];
        DBC.m_NameHash = 0;
        DBC.m_pBase = nullptr;
        DBC.m_SizeOfBlock = 0;
        DBC.m_nBlocks = 0;
        DBC.m_TotalSize = 0;
    }
}

const data_block* data_vault::GetData(const data_handle& Handle)
{
    // Confirm handle data is decent
    assert(Handle.m_DataID.GetValue() != DATA_ID_NULL);
    assert((Handle.GetType() > DATA_TYPE_NONE) && (Handle.GetType() < DATA_TYPE_MAX_TYPES));

    // No entries?
    if (m_nHashEntries == 0) {
        return nullptr;
    }

    // Loop through hash table looking for a match
    int iHash = Handle.m_DataID.GetValue() % m_nHashEntries;
    while (m_pHash[iHash].m_DataID.GetValue() != DATA_ID_NULL) {
        // Found match so return ptr
        if (m_pHash[iHash].m_DataID.GetValue() == Handle.m_DataID.GetValue()) {
            return ResolveBlockPtr(m_pHash[iHash].m_iDBC, m_pHash[iHash].m_iBlock);
        }

        // Move to next hash slot
        iHash = iHash + 1;
        if (iHash == m_nHashEntries) {
            iHash = 0;
        }
    }

    // We hit an empty slot before finding the entry.  It must not be in the table.
    return nullptr;
}

const data_block* data_vault::ResolveBlockPtr(int iDBC, int iBlock)
{
    assert((iDBC >= 0) && (iDBC < MAX_DATA_BLOCK_COLLECTIONS));
    data_block_collection& DBC = m_DataBlockCollection[iDBC];

    assert(DBC.m_NameHash != 0);
    assert((iBlock >= 0) && (iBlock < DBC.m_nBlocks));

    // Calculate ptr to block
    const data_block* pBlock = (data_block*)(DBC.m_pBase + (DBC.m_SizeOfBlock * iBlock));

    // Do whatever validation we can
    assert((pBlock->GetType() >= 0) && (pBlock->GetType() < DATA_TYPE_MAX_TYPES));

    return pBlock;
}

void data_vault::SanityCheck()
{
    int i;

    // Loop through all entries in the hash table and be sure the blocks match IDs.
    for (i = 0; i < m_nHashEntries; i++) {
        if (m_pHash[i].m_DataID.GetValue() != DATA_ID_NULL) {
            assert((m_pHash[i].m_DataID.GetType() > DATA_TYPE_NONE) && (m_pHash[i].m_DataID.GetType() < DATA_TYPE_MAX_TYPES));
            //assert( m_pHash[i].m_pBlock->m_DataID.GetValue() == m_pHash[i].m_DataID.GetValue() );
        }
    }
}

void data_vault::BuildHashTable()
{
    int i, j;

    // Free the old table
    free(m_pHash);
    m_pHash = nullptr;

    // Decide size of new table
    m_nHashEntries = 0;
    for (i = 0; i < MAX_DATA_BLOCK_COLLECTIONS; i++) {
        if (m_DataBlockCollection[i].m_NameHash != 0) {
            m_nHashEntries += m_DataBlockCollection[i].m_nBlocks;
        }
    }
    m_nHashEntries = ChoosePrimeGreaterOrEqual(m_nHashEntries * 3);

    // Allocate new table
    m_pHash = (hash_entry*)malloc(sizeof(hash_entry) * m_nHashEntries);
    assert(m_pHash);

    // Clear new table
    for (i = 0; i < m_nHashEntries; i++) {
        m_pHash[i].m_DataID.SetValue(DATA_ID_NULL);
        m_pHash[i].m_iDBC = -1;
        m_pHash[i].m_iBlock = -1;
    }

    // Add entries into the table
    for (i = 0; i < MAX_DATA_BLOCK_COLLECTIONS; i++) {
        if (m_DataBlockCollection[i].m_NameHash != 0) {
            data_block_collection& DBC = m_DataBlockCollection[i];
            for (j = 0; j < DBC.m_nBlocks; j++) {
                data_block* pBlock = (data_block*)(DBC.m_pBase + (DBC.m_SizeOfBlock * j));
                assert((pBlock->m_DataID.GetType() > DATA_TYPE_NONE) && (pBlock->m_DataID.GetType() < DATA_TYPE_MAX_TYPES));

                int iHash = pBlock->m_DataID.GetValue() % m_nHashEntries;
                while (m_pHash[iHash].m_DataID.GetValue() != DATA_ID_NULL) {
                    iHash = iHash + 1;
                    if (iHash == m_nHashEntries) {
                        iHash = 0;
                    }
                }

                m_pHash[iHash].m_DataID = pBlock->m_DataID;
                m_pHash[iHash].m_iDBC = (int16_t)i;
                m_pHash[iHash].m_iBlock = (int16_t)j;
            }
        }
    }
}

void data_vault::AddDataBlocks(const char*       pDataBlockCollectionName,
                               const data_block* pBase,
                               int               nBlocks,
                               int               SizeOfBlock)
{
    int i;

    // Look for empty slot
    for (i = 0; i < MAX_DATA_BLOCK_COLLECTIONS; i++) {
        if (m_DataBlockCollection[i].m_NameHash == 0) {
            break;
        }
    }
    assert(i != MAX_DATA_BLOCK_COLLECTIONS);
    if (i == MAX_DATA_BLOCK_COLLECTIONS) {
        return;
    }

    //x_DebugMsg("AddingDataBlocks %s to entry %d\n",pDataBlockCollectionName,i);

    // Fill out the new Entry
    data_block_collection& DBC = m_DataBlockCollection[i];
    DBC.m_NameHash = ComputeHash(pDataBlockCollectionName);
    DBC.m_pBase = (uint8_t*)pBase;
    DBC.m_SizeOfBlock = SizeOfBlock;
    DBC.m_nBlocks = nBlocks;
    DBC.m_TotalSize = DBC.m_SizeOfBlock * DBC.m_nBlocks;

    BuildHashTable();
}

void data_vault::AddDataBlockToHashTable(const data_block* pBlock, int iDBC, int iBlock)
{
    assert(pBlock);
    assert((pBlock->GetType() > DATA_TYPE_NONE) && (pBlock->GetType() < DATA_TYPE_MAX_TYPES));
    assert(pBlock->m_DataID.GetValue() != DATA_ID_NULL);

    // Add to table
    int iHash = pBlock->m_DataID.GetValue() % m_nHashEntries;
    while (m_pHash[iHash].m_DataID.GetValue() != DATA_ID_NULL) {
        iHash = iHash + 1;
        if (iHash == m_nHashEntries) {
            iHash = 0;
        }
    }

    m_pHash[iHash].m_DataID = pBlock->m_DataID;
    m_pHash[iHash].m_iDBC = (int16_t)iDBC;
    m_pHash[iHash].m_iBlock = (int16_t)iBlock;
}

data_block* data_vault::DelDataBlocks(const char* pDataBlockCollectionName)
{
    assert(pDataBlockCollectionName);

    int i;

    uint32_t NameHash = ComputeHash(pDataBlockCollectionName);
    for (i = 0; i < MAX_DATA_BLOCK_COLLECTIONS; i++) {
        if (m_DataBlockCollection[i].m_NameHash == NameHash) {
            break;
        }
    }
    assert(i != MAX_DATA_BLOCK_COLLECTIONS);
    if (i == MAX_DATA_BLOCK_COLLECTIONS) {
        return nullptr;
    }

    // Fill out the new Entry
    int                    iDBC = i;
    data_block_collection& DBC = m_DataBlockCollection[iDBC];
    data_block*            pBase = (data_block*)DBC.m_pBase;

    // Clear the DBC entry.  We are going to leave the extra data there
    // on purpose in case we need it for debugging.
    DBC.m_NameHash = 0;

    // Now rebuild the hash table from the DBC entries
    BuildHashTable();

    return pBase;
}

uint32_t data_vault::ChecksumHashTable(int iFirstEntry, int iLastEntry)
{
    // If indices are -1, checksum the entire table
    if ((iFirstEntry == -1) && (iLastEntry == -1)) {
        return calcCRC(m_pHash, sizeof(hash_entry) * m_nHashEntries);
    } else {
        uint8_t* pFirstByte = (uint8_t*)&m_pHash[iFirstEntry];
        uint8_t* pLastByte = (uint8_t*)&m_pHash[iLastEntry + 1];
        return calcCRC(pFirstByte, (pLastByte - pFirstByte));
    }
}

//==============================================================================

uint32_t data_vault::ChecksumData(int iDBC, int iFirstBlock, int iLastBlock)
{
    // Handle case of no data blocks
    assert(m_DataBlockCollection[iDBC].m_pBase);
    if (!m_DataBlockCollection[iDBC].m_pBase) {
        return 0;
    }

    // Get reference to data block collection
    data_block_collection& DBC = m_DataBlockCollection[iDBC];

    // If indices are -1, checksum the entire collection
    if ((iFirstBlock == -1) && (iLastBlock == -1)) {
        uint8_t* pFirstByte = DBC.m_pBase;
        uint8_t* pLastByte = DBC.m_pBase + ((int)DBC.m_SizeOfBlock * (int)DBC.m_nBlocks);
        return calcCRC(pFirstByte, (pLastByte - pFirstByte));
    } else {
        // checksum subrange of blocks
        uint8_t* pFirstByte = DBC.m_pBase + iFirstBlock * DBC.m_SizeOfBlock;
        uint8_t* pLastByte = DBC.m_pBase + (iLastBlock + 1) * DBC.m_SizeOfBlock;
        return calcCRC(pFirstByte, (pLastByte - pFirstByte));
    }
}

void data_vault::WriteData(int iDBC, int iBlock, int nBlocks, uint8_t* pData)
{
    // Handle case of no data blocks
    assert(m_DataBlockCollection[iDBC].m_pBase);
    assert((iBlock + nBlocks) <= m_DataBlockCollection[iDBC].m_nBlocks);

    // Get reference to data block collection
    data_block_collection& DBC = m_DataBlockCollection[iDBC];

    // Get ptr to start of blocks to be overwritten
    uint8_t* pFirstByte = DBC.m_pBase + iBlock * DBC.m_SizeOfBlock;

    // Copy data into blocks
    memcpy(pFirstByte, pData, nBlocks * DBC.m_SizeOfBlock);
}
