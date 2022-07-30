cmake_minimum_required( VERSION 2.8.8 )
add_definitions(-fno-rtti)

add_compile_options(-std=c++11)
add_definitions(-std=c++11 -Wno-attributes -Wno-deprecated-declarations -DANDROID_STL=c++_shared)

include_directories(include)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(ROCKIT_DEP_COMMON_LIBS
        ${ROCKIT_DEP_COMMON_LIBS}
        -lpthread
    )
    message(STATUS "Build WITH linker libs for Linux")
endif()

add_subdirectory(common)
add_subdirectory(mod)
add_subdirectory(sundry)

install(PROGRAMS ${ROCKIT_DUMPSYS_FILE} DESTINATION "bin")
