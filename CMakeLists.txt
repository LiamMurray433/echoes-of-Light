cmake_minimum_required(VERSION 3.11)
project(echoes-of-light)
# Require modern C++
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#### Setup Directories ####
#Main output directory
SET(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
# Ouput all DLLs from all libs into main build folder
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY})

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#### Add External Dependencies ####
add_subdirectory("lib/SFML")
set(SFML_INCS "lib/SFML/include")
link_directories("${CMAKE_BINARY_DIR}/lib/SFML/lib")
  
#### Platformer Game ####
set(EOL_SOURCES
    main.cpp
    src/Game.cpp
    src/MainMenu.cpp
    src/components/Component.cpp
    src/components/TransformComponent.cpp
    src/components/RenderComponent.cpp
    src/components/AnimationComponent.cpp
    src/components/LightComponent.cpp
    src/components/LightEmitterComponent.cpp
    src/components/PlayerComponent.cpp
    src/components/EnemyComponent.cpp
    src/components/MeleeAttackComponent.cpp
    src/components/LightSourceComponent.cpp
    src/components/MirrorComponent.cpp
    src/components/PuzzleComponent.cpp
    src/components/UpgradeComponent.cpp
    src/systems/InputSystem.cpp
    src/systems/AnimationSystem.cpp
    src/systems/RenderSystem.cpp
    src/systems/CombatSystem.cpp
    src/systems/LightSystem.cpp
  
    
)

add_executable(echoes-of-light ${EOL_SOURCES} )
target_include_directories(echoes-of-light PRIVATE ${SFML_INCS} include)
target_link_libraries(echoes-of-light sfml-graphics)

set_target_properties(echoes-of-light PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)


file(COPY "resources" DESTINATION "bin")
