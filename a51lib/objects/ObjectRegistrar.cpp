#include "ObjectRegistrar.h"

#include "HudObject.h"
#include "Player.h"

void ObjectRegistrar::RegisterObjects(std::vector<const object_desc*>& objectDescriptors)
{
    objectDescriptors.push_back(&HudObject::GetObjectType());
    objectDescriptors.push_back(&player::GetObjectType());
    // Add more object descriptors here
    // objectDescriptors.push_back(new SomeOtherObjectDesc());
}
