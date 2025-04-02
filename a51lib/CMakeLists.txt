
add_library(a51lib)

set(GLTF_SOURCES
        gltf/json.hpp
	gltf/tiny_gltf.cpp
	gltf/tiny_gltf.h
)

set(ANIM_SOURCES
        animation/animData.cpp
        animation/animData.h
        animation/animDecompress.cpp
        )

set(DATAUTIL_SOURCES
        dataUtil/Bitstream.cpp
        dataUtil/Bitstream.h
        dataUtil/Crc.cpp
        dataUtil/Crc.cpp
        dataUtil/TextIn.cpp
        dataUtil/TextIn.h
        dataUtil/Tokenizer.cpp
        dataUtil/Tokenizer.h
        )

file(GLOB SYSTEM_SOURCES
        system/*.h
        )

set(LEVELS_SOURCES
        levels/LevelLoader.cpp
        levels/LevelLoader.h
        )

set(IO_SOURCES
        io/FileSystem.cpp
        io/FileSystem.h
        )

file(GLOB DIALOGS_INCLUDES ui/dialogs/*.h)
file(GLOB DIALOGS_SOURCES ui/dialogs/*.cpp)
file(GLOB UI_INCLUDES ui/*.h)
file(GLOB UI_SOURCES ui/*.cpp)

set(UI_FILES
        ${DIALOGS_INCLUDES}
        ${DIALOGS_SOURCES}
        ${UI_INCLUDES}
        ${UI_SOURCES}
        )

file(GLOB STATE_INCLUDES state/*.h)
file(GLOB STATE_SOURCES state/*.cpp)

set(STRING_SOURCES
        strings/StringTable.cpp
        strings/StringTable.h
        )

set(RESOURCE_MGR_SOURCES
        resourceManager/ResourceLoaders.cpp
        resourceManager/ResourceLoaders.h
        resourceManager/ResourceManager.cpp
        resourceManager/ResourceManager.h
)

set(OBJECT_MGR_SOURCES
        objectManager/ObjectManager.cpp
        objectManager/ObjectManager.h
)

target_sources(a51lib PRIVATE
        BinLevel.cpp
        BinLevel.h
        Bitmap.cpp
        Bitmap.h
        CollisionVolume.cpp
        CollisionVolume.h
        DFSFile.cpp
        DFSFile.h
        Geom.cpp
        Geom.h
        Guid.h
        InevFile.cpp
        InevFile.h
        LevelTemplate.cpp
        LevelTemplate.h
        Playsurface.h
        Playsurface.cpp
        PlaysurfaceDB.h
        PlaysurfaceDB.cpp
        Property.cpp
        Property.h
        PropertyDefs.h
        RigidGeom.cpp
        RigidGeom.h
        streamingOperators.cpp
        streamingOperators.h
        VectorMath.h
	${GLTF_SOURCES}
        ${ANIM_SOURCES}
        ${DATAUTIL_SOURCES}
        ${SYSTEM_SOURCES}
        ${LEVELS_SOURCES}
        ${IO_SOURCES}
        ${OBJECT_MGR_SOURCES}
        ${RESOURCE_MGR_SOURCES}
        ${UI_FILES}
        ${STATE_INCLUDES}
        ${STATE_SOURCES}
        ${STRING_SOURCES}
)

target_compile_features(a51lib PUBLIC cxx_std_20)
