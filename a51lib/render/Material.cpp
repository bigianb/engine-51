#include "Material.h"

material::material(ResourceManager* rm) : m_DiffuseMap(rm),
    m_EnvironmentMap(rm),
    m_DetailMap(rm),
    m_Type(0),
    m_DetailScale(1.0f),
    m_FixedAlpha(1.0f),
    m_Flags(0),
    m_RefCount(0)
{
}

material::~material()
{
}


bool material::operator== ( const material& RHS ) const
{
    if ( m_DiffuseMap.getIndex()     != RHS.m_DiffuseMap.getIndex() )       return false;
    if ( m_EnvironmentMap.getIndex() != RHS.m_EnvironmentMap.getIndex() )   return false;
    if ( m_DetailMap.getIndex()      != RHS.m_DetailMap.getIndex() )        return false;
    if ( m_Type                      != RHS.m_Type )                        return false;
    if ( m_DetailScale               != RHS.m_DetailScale )                 return false;
    if ( m_FixedAlpha                != RHS.m_FixedAlpha )                  return false;
    if ( m_Flags                     != RHS.m_Flags )                       return false;

    if ( memcmp( &m_UVAnim, &RHS.m_UVAnim, sizeof(m_UVAnim) ) )
        return false;

    return true;
}
