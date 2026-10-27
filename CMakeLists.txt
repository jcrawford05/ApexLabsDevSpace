cmake_minimum_required(VERSION 3.20)
project(ApexLabsUI VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# nlohmann_json 3.11.3 asks for CMake 3.1, which CMake 4 refuses without this.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "")

# --- Dependencies (pinned) ---------------------------------------------------
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.8
  GIT_SHALLOW TRUE)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
  GIT_SHALLOW TRUE)

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(SDL3 imgui nlohmann_json)

# Dear ImGui has no CMake project of its own, so build it here.
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# --- App ---------------------------------------------------------------------
add_executable(apex_labs_ui
  src/main.cpp
  src/app.cpp
  src/data.cpp
  src/theme.cpp
  src/widgets.cpp
  src/textures.cpp
  src/track_map.cpp
  src/screens/start_screen.cpp
  src/screens/setup_screen.cpp
  src/screens/results_screen.cpp
  src/screens/settings_screen.cpp
  src/screens/compare_screen.cpp
  src/screens/import_export_screen.cpp)
target_include_directories(apex_labs_ui PRIVATE src third_party)
target_link_libraries(apex_labs_ui PRIVATE imgui nlohmann_json::nlohmann_json)
if(MSVC)
  target_compile_options(apex_labs_ui PRIVATE /W4 /utf-8)
else()
  target_compile_options(apex_labs_ui PRIVATE -Wall -Wextra)
endif()
if(WIN32)
  set_target_properties(apex_labs_ui PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
endif()

if(APPLE)
  # A normal macOS app: build/Apex Labs.app, double-click to open.
  set_target_properties(apex_labs_ui PROPERTIES
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME "Apex Labs"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/Info.plist.in
    MACOSX_BUNDLE_BUNDLE_NAME "Apex Labs"
    MACOSX_BUNDLE_GUI_IDENTIFIER "edu.apexlabs.ui"
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
  # SDL_GetBasePath() points at Contents/Resources inside a bundle.
  set(APEX_DATA_DIR $<TARGET_BUNDLE_CONTENT_DIR:apex_labs_ui>/Resources)
else()
  set(APEX_DATA_DIR $<TARGET_FILE_DIR:apex_labs_ui>)
endif()

# Fonts, images and sample data go where the app looks for them.
add_custom_command(TARGET apex_labs_ui POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${APEX_DATA_DIR}/assets
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${APEX_DATA_DIR}/data)
