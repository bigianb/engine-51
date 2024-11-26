

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets ShaderTools)

set(DFSVIEW_SOURCES
        gltfExporter.cpp
        gltfExporter.h
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        modelWidget.cpp
        modelWidget.h
)

qt_add_executable(DFSViewer MANUAL_FINALIZATION ${DFSVIEW_SOURCES})

qt6_add_shaders(DFSViewer "DFSViewer_shaders"
    PREFIX
    "/"
    FILES
    "shaders/texture.frag"
    "shaders/texture.vert"
)

target_include_directories(DFSViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(DFSViewer PRIVATE a51lib Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::GuiPrivate)

set_target_properties(DFSViewer PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS DFSViewer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
  find_program(WINDEPLOYQT_EXE windeployqt HINTS "${QT_BINARY_DIRECTORY}")
  add_custom_command(TARGET DFSViewer POST_BUILD
		COMMAND "${WINDEPLOYQT_EXE}" $<IF:$<CONFIG:Debug>,--debug,--release>
									 --no-translations
									 --no-compiler-runtime
									 "$<TARGET_FILE:DFSViewer>"
	)
endif()

qt_finalize_executable(DFSViewer)


