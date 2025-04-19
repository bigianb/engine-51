
find_package(SDL3 REQUIRED)

file(GLOB SYSYTEM_INCLUDES system/*.h)
file(GLOB SYSYTEM_SOURCES system/*.cpp)

source_group("system" FILES ${SYSYTEM_INCLUDES})
source_group("system" FILES ${SYSYTEM_SOURCES})

add_executable(SDL_game
    gameObject.cpp
    gameObject.h
    main.cpp
    ${SYSYTEM_INCLUDES}
    ${SYSYTEM_SOURCES}
)

target_link_libraries(SDL_game
    SDL3::SDL3
    a51lib
)

add_custom_command(TARGET SDL_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tools/sdlgame/Content $<TARGET_FILE_DIR:SDL_game>/Content
)