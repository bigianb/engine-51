
add_library(a51lib)

target_sources(a51lib PRIVATE
        CollisionVolume.cpp
        CollisionVolume.h
        DFSFile.cpp
        DFSFile.h
        Geom.cpp
        Geom.h
        InevFile.cpp
        InevFile.h
        RigidGeom.cpp
        RigidGeom.h
        VectorMath.h
)

target_compile_features(a51lib PUBLIC cxx_std_20)
