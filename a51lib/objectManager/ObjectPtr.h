#pragma once

#include "ObjectManager.h"

template <class object_type>
struct object_ptr
{
    object_ptr(Object* pObject)
    {
        m_pObject = nullptr;

        if (pObject != nullptr) {
            if (pObject->IsKindOf(object_type::GetRTTI())) {
                m_pObject = (object_type*)pObject;
            }
        }
    }

    object_ptr(const guid& rGuid, ObjectManager* om)
    {
        m_pObject = nullptr;

        if (rGuid != NULL_GUID) {
            Object* pObject = om->GetObjectByGuid(rGuid);

            if (pObject != nullptr) {
                if (pObject->IsKindOf(object_type::GetRTTI())) {
                    m_pObject = (object_type*)pObject;
                }
            }
        }
    }

    inline bool IsValid() { return (m_pObject != nullptr); }

    inline object_type& operator*() { return *m_pObject; }
    inline object_type* operator->() { return m_pObject; }
    inline bool         operator==(const Object* pObject) { return (m_pObject == pObject); }
    inline              operator bool() { return (m_pObject != nullptr); }

    object_type* m_pObject;
};
