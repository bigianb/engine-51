#include "ObjectRegistrar.h"

#include "HudObject.h"
#include "Player.h"
#include "triggerEx/TriggerExObject.h"
#include "../characters/God.h"

void ObjectRegistrar::RegisterObjects(std::vector<const object_desc*>& objectDescriptors)
{
    objectDescriptors.push_back(&HudObject::GetObjectType());
    objectDescriptors.push_back(&player::GetObjectType());
    objectDescriptors.push_back(&God::GetObjectType());
    objectDescriptors.push_back(&trigger_ex_object::GetObjectType());
    // Add more object descriptors here
    // objectDescriptors.push_back(new SomeOtherObjectDesc());
}
