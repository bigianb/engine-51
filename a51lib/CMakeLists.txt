
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

set(SYSTEM_SOURCES
        system/Renderer.h
        )

set(LEVELS_SOURCES
        levels/LevelLoader.cpp
        levels/LevelLoader.h
        )

set(IO_SOURCES
        io/FileSystem.cpp
        io/FileSystem.h
        )

set(UI_SOURCES
        ui/dialogs/esrbDialog.cpp
        ui/dialogs/esrbDialog.h
        ui/UIControl.cpp
        ui/UIControl.h
        ui/UIDialog.cpp
        ui/UIDialog.h
        ui/UIManager.cpp
        ui/UIManager.h
        ui/UIText.cpp
        ui/UIText.h
        ui/UIWindow.cpp
        ui/UIWindow.h
        )

set(STATE_SOURCES
        state/StateMachine.cpp
        state/StateMachine.h
        )

set(RESOURCE_MGR_SOURCES
        resourceManager/ResourceLoaders.cpp
        resourceManager/ResourceLoaders.h
        resourceManager/ResourceManager.cpp
        resourceManager/ResourceManager.h
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
        ${RESOURCE_MGR_SOURCES}
        ${UI_SOURCES}
        ${STATE_SOURCES}
)

target_compile_features(a51lib PUBLIC cxx_std_20)
