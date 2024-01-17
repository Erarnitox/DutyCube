# Add zlib.
find_package(ZLIB REQUIRED)

if(DROP_ENGINE_BUILD_CLIENT)
  # Add OpenGL.
  find_package(OpenGL REQUIRED)

  # Platform specific libraries.
  if(UNIX AND NOT APPLE)
    set(PLATFORM_LIBRARIES rt)
  endif()
endif()

# Add third party libraries (e.g. built from source).
add_subdirectory(third_party)

# Additional compiler flags.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Silence warnings from Clang.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-exception-spec")
endif()

if(DROP_ENGINE_BUILD_CLIENT)
  # Client executable.
  set(CLIENT_SOURCES
      shared/crypto.cpp
      shared/geom.cpp
      shared/glemu.cpp
      shared/stream.cpp
      shared/tools.cpp
      shared/zip.cpp
      engine/aa.cpp
      engine/bih.cpp
      engine/blend.cpp
      engine/client.cpp
      engine/command.cpp
      engine/console.cpp
      engine/dynlight.cpp
      engine/grass.cpp
      engine/light.cpp
      engine/main.cpp
      engine/material.cpp
      engine/menus.cpp
      engine/movie.cpp
      engine/normal.cpp
      engine/octa.cpp
      engine/octaedit.cpp
      engine/octarender.cpp
      engine/physics.cpp
      engine/pvs.cpp
      engine/rendergl.cpp
      engine/renderlights.cpp
      engine/rendermodel.cpp
      engine/renderparticles.cpp
      engine/rendersky.cpp
      engine/rendertext.cpp
      engine/renderva.cpp
      engine/server.cpp
      engine/serverbrowser.cpp
      engine/shader.cpp
      engine/sound.cpp
      engine/stain.cpp
      engine/texture.cpp
      engine/ui.cpp
      engine/water.cpp
      engine/world.cpp
      engine/worldio.cpp
      game/ai.cpp
      game/client.cpp
      game/entities.cpp
      game/game.cpp
      game/render.cpp
      game/scoreboard.cpp
      game/server.cpp
      game/waypoint.cpp
      game/weapon.cpp
    )
  set(CLIENT_INCLUDES
      ${CMAKE_CURRENT_SOURCE_DIR}
      )
  set(CLIENT_LIBRARIES
      enet
      sdl2
      ${OPENGL_LIBRARIES}
      ${ZLIB_LIBRARIES}
      ${PLATFORM_LIBRARIES}
      )
  add_executable(dropengine ${CLIENT_SOURCES})
  target_include_directories(dropengine PRIVATE ${CLIENT_INCLUDES})
  target_link_libraries(dropengine ${CLIENT_LIBRARIES})

  # clang_format(dropengine ${CMAKE_CURRENT_LIST_DIR})
  set_property(TARGET dropengine PROPERTY CXX_STANDARD ${CPP_STANDARD})
  if (MSVC)
    # for windows msvc or clang-cl
    # target_compile_options(dropengine PRIVATE "/W4")
  else()
    # target_compile_options(dropengine PRIVATE -Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
  endif()

  # DropEngine client launcher.
  if(UNIX)
    configure_file("scripts/runclient.sh.in" "${CMAKE_CURRENT_BINARY_DIR}/runclient.sh" @ONLY)
  endif()
endif()

if(DROP_ENGINE_BUILD_SERVER)
  # Server executable.
  set(SERVER_SOURCES
      shared/crypto.cpp
      shared/stream.cpp
      shared/tools.cpp
      engine/command.cpp
      engine/server.cpp
      engine/worldio.cpp
      game/entities.cpp
      game/server.cpp
      )
  set(SERVER_INCLUDES
      ${CMAKE_CURRENT_SOURCE_DIR}
      )
  set(SERVER_LIBRARIES
      enet
      ${ZLIB_LIBRARIES}
      )
  add_executable(drop_server ${SERVER_SOURCES})
  target_compile_definitions(drop_server PRIVATE -DSTANDALONE)
  target_include_directories(drop_server PRIVATE ${SERVER_INCLUDES})
  target_link_libraries(drop_server ${SERVER_LIBRARIES})
  
  # clang_format(drop_server ${CMAKE_CURRENT_LIST_DIR})
  set_property(TARGET drop_server PROPERTY CXX_STANDARD ${CPP_STANDARD})
  if (MSVC)
    # for windows msvc or clang-cl
    # target_compile_options(drop_server PRIVATE "/W4")
  else()
    # target_compile_options(drop_server PRIVATE -Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
  endif()

  # Master executable.
  set(MASTER_SOURCES
      shared/crypto.cpp
      shared/stream.cpp
      shared/tools.cpp
      engine/command.cpp
      engine/master.cpp
      )
  set(MASTER_INCLUDES
      ${CMAKE_CURRENT_SOURCE_DIR}
      )
  set(MASTER_LIBRARIES
      enet
      ${ZLIB_LIBRARIES}
      )
  add_executable(drop_master ${MASTER_SOURCES})
  target_compile_definitions(drop_master PRIVATE -DSTANDALONE)
  target_include_directories(drop_master PRIVATE ${MASTER_INCLUDES})
  target_link_libraries(drop_master ${MASTER_LIBRARIES})
  
  # clang_format(ress_master ${CMAKE_CURRENT_LIST_DIR})
  set_property(TARGET drop_master PROPERTY CXX_STANDARD ${CPP_STANDARD})
  if (MSVC)
    # for windows msvc or clang-cl
    # target_compile_options(drop_master PRIVATE "/W4")
  else()
    # target_compile_options(drop_master PRIVATE -Werror -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
  endif()
endif()
