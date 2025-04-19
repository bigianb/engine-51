#pragma once

class corpse;
class pain;

#include "../VectorMath.h"
#include "../Guid.h"

// Corpse pain class for setting up, applying, and sending across the net
class corpse_pain
{

    // Defines
public:
    enum type
    {
        TYPE_SUICIDE,    // Pain was caused by suicide
        TYPE_MELEE,      // Pain was caused by melee pain
        TYPE_SPLASH,     // Pain was caused by splash damage
        TYPE_RIGID_BODY, // Pain was caused by rigid body pain
        TYPE_BLAST,      // Pain was caused by blast
        TYPE_COUNT
    };

    // Functions
public:
    // Constructor
    corpse_pain();

    // Clears defaults to a suicide
    void Clear(void);

    // Read/Write to bitstream
    //void        Read            ( const bitstream& BS );
    //void        Write           (       bitstream& BS );

    // Sets up values from pain hitting a corpse
    void Setup(const pain& Pain, corpse& Corpse);

    // Applies pain to corpse
    void Apply(corpse& Corpse);

    // Query functions
    guid           GetOriginGuid() const;
    const Vector3& GetDirection() const;
    bool           IsDirectHit() const;

    // Data
private:
    uint8_t m_Type;           // Type of pain
    int8_t  m_iRigidBody;     // Index of rigid body that was hit (or -1 if none)
    uint8_t m_bOnDeath : 1;   // TRUE if came from killing an actor
    uint8_t m_bDirectHit : 1; // TRUE if direct hit
    int16_t m_OriginNetSlot;  // Origin net slot
    Vector3 m_Position;       // Position of pain force (relative to victim)
    Vector3 m_Direction;      // Direction of pain force
    float   m_Force;          // Force amount
    float   m_ForceFarDist;   // Fade off
};
