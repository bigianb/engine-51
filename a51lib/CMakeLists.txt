
add_library(a51lib)

target_sources(a51lib PRIVATE
        DFSFile.cpp
        DFSFile.h
        Geom.h
        InevFile.cpp
        InevFile.h
        RigidGeom.cpp
        RigidGeom.h
)

target_compile_features(a51lib PUBLIC cxx_std_20)
