cmake_minimum_required(VERSION 3.18)
# CMake version requirements:
#  - 3.11+ is required for FetchContent module
#  - 3.12+ is required for Python3 module
#  - 3.18+ is required for Python3 module support of "Development.Module" (required for manylinux cibuildwheel)

set(PYMP3_PYTHON_MODULE_NAME "mp3" CACHE STRING "Name of the C extension module file")

if(WIN32)
    set(PYMP3_PYTHON_MODULE_EXT ".pyd" CACHE STRING "Extension of the C extension module file")
else()
    set(PYMP3_PYTHON_MODULE_EXT ".so" CACHE STRING "Extension of the C extension module file")
endif()

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: default is Release")
endif()

project(pymp3 
    LANGUAGES C
)

set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "Building ${PROJECT_NAME} python extension version ${PROJECT_VERSION}...")

find_package (Python3 
    COMPONENTS Interpreter Development.Module
    REQUIRED
)
message(STATUS "Found Python ${Python3_VERSION} at ${Python3_EXECUTABLE} (include_dirs=${Python3_INCLUDE_DIRS})")

# ---------------------------------------------------------------
# Define library (python extension) "pymp3.[so|pyd]"
# ---------------------------------------------------------------
add_library(${PROJECT_NAME} SHARED
    src/mp3_encoder.c
    src/mp3_decoder.c
    src/py_module.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  POSITION_INDEPENDENT_CODE ON                 # enable "-fPIC" (required for a library)
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${PYMP3_PYTHON_MODULE_NAME}"
    SUFFIX "${PYMP3_PYTHON_MODULE_EXT}"
    LINKER_LANGUAGE C
)


# Tell the compiler/linker where to find Python.h and library files
set_target_properties(${PROJECT_NAME} PROPERTIES 
    INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
    LINK_DIRECTORIES "${Python3_LIBRARY_DIRS}"
)

target_compile_options(${PROJECT_NAME}
    PRIVATE
        -DVERSION="${PROJECT_VERSION}"
)


list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# -----------------------------------------------------
# "libmad" dependency
# -----------------------------------------------------
option(PYMP3_USE_SYSTEM_LIBMAD "Link with libmad, which is installed in system, rather than downloading it from github" OFF)

if(PYMP3_USE_SYSTEM_LIBMAD)
    find_package (mad)
    target_link_libraries(${PROJECT_NAME} PRIVATE mad)
else()

    set(PYMP3_LIBMAD_REPO_URL "https://github.com/miarec/libmad" CACHE STRING "Git repository URL, from where to fetch the libmad library")
    set(PYMP3_LIBMAD_REPO_TAG "master" CACHE STRING "Git repository tag/branch to fetch the libmad project")

    include(FetchContent)

    FetchContent_Declare(
        mad
        GIT_REPOSITORY "${PYMP3_LIBMAD_REPO_URL}"
        GIT_TAG "${PYMP3_LIBMAD_REPO_TAG}"
    )
    FetchContent_MakeAvailable(mad)

    message(STATUS "mad_SOURCE_DIR=${mad_SOURCE_DIR}, mad_BINARY_DIR=${mad_BINARY_DIR}")

    target_link_libraries(${PROJECT_NAME} PRIVATE mad)

endif()


# -----------------------------------------------------
# "libmp3lame" dependency
# -----------------------------------------------------
option(PYMP3_USE_SYSTEM_LAME "Link with libmp3lame (lame), which is installed in system, rather than downloading it from github" OFF)

if(PYMP3_USE_SYSTEM_LAME)
    find_package (mp3lame)
    target_link_libraries(${PROJECT_NAME} PRIVATE mp3lame)
else()

    set(PYMP3_LAME_REPO_URL "https://github.com/miarec/lame" CACHE STRING "Git repository URL, from where to fetch the lame library")
    set(PYMP3_LAME_REPO_TAG "master" CACHE STRING "Git repository tag/branch to fetch the lame project")

    include(FetchContent)

    FetchContent_Declare(
        mp3lame
        GIT_REPOSITORY "${PYMP3_LAME_REPO_URL}"
        GIT_TAG "${PYMP3_LAME_REPO_TAG}"
    )
    FetchContent_MakeAvailable(mp3lame)

    message(STATUS "mp3lame_SOURCE_DIR=${mp3lame_SOURCE_DIR}, mp3lame_BINARY_DIR=${mp3lame_BINARY_DIR}")

    target_link_libraries(${PROJECT_NAME} PRIVATE mp3lame)
endif()
