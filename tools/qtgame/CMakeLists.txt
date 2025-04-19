

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Gui ShaderTools)

set(GAME_SOURCES
        gameObject.cpp
        gameObject.h
        gameWindow.cpp
        gameWindow.h
        main.cpp
        system/QtRenderer.cpp
        system/QtRenderer.h
)

qt_add_executable(QtGame MANUAL_FINALIZATION ${GAME_SOURCES})

qt6_add_shaders(QtGame "DFSViewer_shaders"
    PREFIX
    "/"
    FILES
    "shaders/color.frag"
    "shaders/color.vert"
    "shaders/quad.frag"
    "shaders/quad.vert"
    "shaders/texture.frag"
    "shaders/texture.vert"
)

target_include_directories(QtGame PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(QtGame PRIVATE a51lib Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::GuiPrivate)

set_target_properties(QtGame PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS QtGame
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
  find_program(WINDEPLOYQT_EXE windeployqt HINTS "${QT_BINARY_DIRECTORY}")
  add_custom_command(TARGET QtGame POST_BUILD
		COMMAND "${WINDEPLOYQT_EXE}" $<IF:$<CONFIG:Debug>,--debug,--release>
									 --no-translations
									 --no-compiler-runtime
									 "$<TARGET_FILE:QtGame>"
	)
endif()

qt_finalize_executable(QtGame)


