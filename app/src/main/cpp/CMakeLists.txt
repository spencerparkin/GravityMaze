# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("gravitymaze")

add_subdirectory(oboe)

file(GLOB_RECURSE PlanarPhysicsSources CONFIGURE_DEPENDS
        "PlanarPhysics/Engine/Source/*.h"
        "PlanarPhysics/Engine/Source/*.cpp")

file(GLOB_RECURSE ParsePartySources CONFIGURE_DEPENDS
        "ParseParty/Source/*.h"
        "ParseParty/Source/*.cpp")

file(GLOB_RECURSE AudioDataLibSources CONFIGURE_DEPENDS
        "AudioDataLib/Source/*.h"
        "AudioDataLib/Source/*.cpp")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(gravitymaze SHARED
        Main.cpp
        AndroidOut.cpp
        AudioSubSystem.cpp
        MidiManager.cpp
        GameRender.cpp
        GameLogic.cpp
        Options.cpp
        Progress.cpp
        Maze.cpp
        Color.cpp
        Shader.cpp
        ShaderProgram.cpp
        DrawHelper.cpp
        TextRenderer.cpp
        TimeKeeper.cpp
        MazeObject.cpp
        MazeObjects/MazeBall.cpp
        MazeObjects/MazeWall.cpp
        MazeObjects/MazeBlock.cpp
        MazeObjects/MazeWorm.cpp
        MazeObjects/MazeQueen.cpp
        ${PlanarPhysicsSources}
        ${ParsePartySources}
        ${AudioDataLibSources})

target_include_directories(gravitymaze PRIVATE
        "PlanarPhysics/Engine/Source"
        "ParseParty/Source"
        "AudioDataLib/Source"
        "oboe/include")

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Configure libraries CMake uses to link your target library.
target_link_libraries(gravitymaze
        # The game activity
        game-activity::game-activity

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        jnigraphics
        android
        amidi
        oboe
        log)