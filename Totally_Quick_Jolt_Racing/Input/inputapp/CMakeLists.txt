file(GLOB inputapp_sources *.cpp *.c)

add_library(inputapp STATIC ${inputapp_sources})

target_include_directories(inputapp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (Use_SDL)
    target_link_libraries(inputapp PUBLIC SDL3::SDL3)
    target_compile_definitions(inputapp PUBLIC Use_SDL=1)
else()
    if (linux)
        target_link_libraries(inputapp PUBLIC hidapi::hidapi-hidraw)
    else ()
        target_link_libraries(inputapp PUBLIC hidapi::hidapi)
    endif()
    target_compile_definitions(inputapp PUBLIC Use_SDL=0)
endif()
