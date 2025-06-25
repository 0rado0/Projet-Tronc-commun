cmake_minimum_required(VERSION 3.4.3...3.25 FATAL_ERROR)


message(STATUS "\n📦 Build mode: Use_SDL = ${Use_SDL}\n")

if (NOT Use_SDL)
    set(BUILD_SHARED_LIBS FALSE) # HIDAPI as static library on all platforms
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hidapi")
        message(FATAL_ERROR "HIDAPI directory not found!")
    endif()
    add_subdirectory(hidapi)
else()
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL")
        message(FATAL_ERROR "SDL directory not found!")
    endif()
    add_subdirectory(SDL EXCLUDE_FROM_ALL)
endif()

add_subdirectory(inputapp)
