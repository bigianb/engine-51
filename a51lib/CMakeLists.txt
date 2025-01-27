
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
        InevFile.cpp
        InevFile.h
        LevelTemplate.cpp
        LevelTemplate.h
        Playsurface.h
        Playsurface.cpp
        PlaysurfaceDB.h
        PlaysurfaceDB.cpp
        PropertyDefs.h
        ResourceManager.cpp
        ResourceManager.h
        RigidGeom.cpp
        RigidGeom.h
        streamingOperators.cpp
        streamingOperators.h
        VectorMath.h
	${GLTF_SOURCES}
        ${ANIM_SOURCES}
        ${DATAUTIL_SOURCES}
)

target_compile_features(a51lib PUBLIC cxx_std_20)
