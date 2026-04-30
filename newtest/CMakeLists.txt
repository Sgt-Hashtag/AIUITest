cmake_minimum_required(VERSION 3.16)
project(AiShapeDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
if(COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
endif()

# --- Link Shared llama.cpp Libraries ---
set(LLAMA_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../llama.cpp/build")

# Define paths to shared libraries. In recent llama.cpp builds, libs are
# usually in bin/, but some builds place them in the root build directory.
set(LLAMA_LIB_DIR "${LLAMA_BUILD_DIR}/bin")
if(NOT EXISTS "${LLAMA_LIB_DIR}/libllama.so")
    set(LLAMA_LIB_DIR "${LLAMA_BUILD_DIR}")
endif()

set(LLAMA_LIB_SHARED "${LLAMA_LIB_DIR}/libllama.so")
set(GGML_LIB_SHARED  "${LLAMA_LIB_DIR}/libggml.so")
set(GGML_BASE_LIB_SHARED "${LLAMA_LIB_DIR}/libggml-base.so")
set(GGML_CPU_LIB_SHARED  "${LLAMA_LIB_DIR}/libggml-cpu.so")


# Include directories
set(LLAMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../llama.cpp/include")
set(GGML_INCLUDE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/../llama.cpp/ggml/include")

# Check if files exist
if(NOT EXISTS "${LLAMA_LIB_SHARED}")
    message(FATAL_ERROR "libllama.so not found. Please ensure llama.cpp is built with shared libs.")
endif()
if(NOT EXISTS "${GGML_LIB_SHARED}")
    message(FATAL_ERROR "libggml.so not found. Please ensure llama.cpp is built with shared libs.")
endif()

# Create Imported Targets for Shared Libraries
add_library(llama_shared SHARED IMPORTED)
set_target_properties(llama_shared PROPERTIES
    IMPORTED_LOCATION "${LLAMA_LIB_SHARED}"
    INTERFACE_INCLUDE_DIRECTORIES "${LLAMA_INCLUDE_DIR}"
)

add_library(ggml_shared SHARED IMPORTED)
set_target_properties(ggml_shared PROPERTIES
    IMPORTED_LOCATION "${GGML_LIB_SHARED}"
    INTERFACE_INCLUDE_DIRECTORIES "${GGML_INCLUDE_DIR}"
)

if(EXISTS "${GGML_BASE_LIB_SHARED}")
    add_library(ggml_base_shared SHARED IMPORTED)
    set_target_properties(ggml_base_shared PROPERTIES
        IMPORTED_LOCATION "${GGML_BASE_LIB_SHARED}"
        INTERFACE_INCLUDE_DIRECTORIES "${GGML_INCLUDE_DIR}"
    )
endif()

if(EXISTS "${GGML_CPU_LIB_SHARED}")
    add_library(ggml_cpu_shared SHARED IMPORTED)
    set_target_properties(ggml_cpu_shared PROPERTIES
        IMPORTED_LOCATION "${GGML_CPU_LIB_SHARED}"
        INTERFACE_INCLUDE_DIRECTORIES "${GGML_INCLUDE_DIR}"
    )
endif()

get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION)
if(NOT QT_CORE_LOCATION)
    get_target_property(QT_CORE_LOCATION Qt6::Core IMPORTED_LOCATION_RELEASE)
endif()

set(ICU_RUNTIME_LIBS)
set(ICU_RUNTIME_DIR "")

qt_add_executable(AiShapeDemo
    main.cpp
    ShapeWindow.cpp ShapeWindow.h
    AiController.cpp AiController.h
    LlamaEngine.cpp LlamaEngine.h
)

# --- Link Libraries ---
target_link_libraries(AiShapeDemo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    
    # Keep ggml's runtime pieces as direct dependencies so the executable's
    # $ORIGIN RPATH can find them without LD_LIBRARY_PATH.
    "-Wl,--no-as-needed"
    llama_shared
    ggml_shared
    $<$<TARGET_EXISTS:ggml_base_shared>:ggml_base_shared>
    $<$<TARGET_EXISTS:ggml_cpu_shared>:ggml_cpu_shared>
    ${ICU_RUNTIME_LIBS}
    "-Wl,--as-needed"
    
    # System Dependencies
    pthread
    dl
    m
)

if(ICU_RUNTIME_DIR)
    target_link_options(AiShapeDemo PRIVATE "-Wl,-rpath-link,${ICU_RUNTIME_DIR}")
endif()

set_target_properties(AiShapeDemo PROPERTIES
    BUILD_RPATH "\$ORIGIN;${LLAMA_LIB_DIR};${ICU_RUNTIME_DIR}"
    INSTALL_RPATH "\$ORIGIN"
)
# --- Copy Models & Shared Libs to Build Directory ---
# This ensures the app can find the .so files and models at runtime without setting LD_LIBRARY_PATH

# Copy Models
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)
file(GLOB MODEL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/models/*.gguf")
if(MODEL_FILES)
    file(COPY ${MODEL_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/models/)
    message(STATUS "Copied models: ${MODEL_FILES}")
else()
    message(WARNING "No .gguf files found in ${CMAKE_CURRENT_SOURCE_DIR}/models/")
endif()

# Copy all versioned llama/ggml shared libraries to the build dir so
# ./AiShapeDemo can find their SONAME files without LD_LIBRARY_PATH.
file(GLOB LLAMA_RUNTIME_LIBS CONFIGURE_DEPENDS
    "${LLAMA_LIB_DIR}/libllama*.so*"
    "${LLAMA_LIB_DIR}/libggml*.so*"
)
file(COPY ${LLAMA_RUNTIME_LIBS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "Copied shared libs to build directory for runtime linking: ${LLAMA_RUNTIME_LIBS}")

if(ICU_RUNTIME_LIBS)
    file(COPY ${ICU_RUNTIME_LIBS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    message(STATUS "Copied ICU runtime libs to build directory: ${ICU_RUNTIME_LIBS}")
endif()
