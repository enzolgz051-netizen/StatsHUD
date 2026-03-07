cmake_minimum_required(VERSION 3.15)
project(StatsHUD)

set(CMAKE_CXX_STANDARD 17)

# ── Path to BakkesMod SDK ─────────────────────────────────────────────────────
# Adjust this path to where you installed the BakkesModSDK
set(BAKKESMOD_SDK_PATH "C:/Users/$ENV{USERNAME}/AppData/Roaming/bakkesmod/bakkesmod/sdk" CACHE PATH "Path to BakkesMod SDK")

# ── Source files ──────────────────────────────────────────────────────────────
set(SOURCES
    StatsHUD.cpp
)

set(HEADERS
    StatsHUD.h
    pch.h
)

# ── DLL target ────────────────────────────────────────────────────────────────
add_library(StatsHUD SHARED ${SOURCES} ${HEADERS})

target_include_directories(StatsHUD PRIVATE
    ${BAKKESMOD_SDK_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StatsHUD PRIVATE
    ${BAKKESMOD_SDK_PATH}/lib/pluginsdk.lib
)

target_precompile_headers(StatsHUD PRIVATE pch.h)

# ── Output straight to BakkesMod plugins folder ───────────────────────────────
set_target_properties(StatsHUD PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "C:/Users/$ENV{USERNAME}/AppData/Roaming/bakkesmod/bakkesmod/plugins"
    SUFFIX ".dll"
)

# ── Compiler flags ────────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(StatsHUD PRIVATE /W3 /MP)
    target_compile_definitions(StatsHUD PRIVATE
        _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        NOMINMAX
    )
endif()
