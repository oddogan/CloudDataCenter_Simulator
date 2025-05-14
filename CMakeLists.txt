cmake_minimum_required(VERSION 3.16)

project(CDCSimulator LANGUAGES CXX)

# Enable the Qt MOC & UIC features (Meta-Object Compiler & User Interface Compiler)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} "")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# Collect Core source files
file(GLOB_RECURSE CORE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/include/*.h")
file(GLOB_RECURSE CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/src/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE ${CORE_HEADERS} ${CORE_SOURCES})

# Collect UI source files
file(GLOB_RECURSE UI_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/include/*.h")
file(GLOB_RECURSE UI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/src/*.cpp")
file(GLOB_RECURSE UI_FORMS "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/src/*.ui")
target_sources(${PROJECT_NAME} PRIVATE ${UI_HEADERS} ${UI_SOURCES} ${UI_FORMS})

# Find Qt
set(CMAKE_PREFIX_PATH "/Users/oddogan/Qt/6.8.1/macos")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6Charts_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Charts)
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS)

# Link CPLEX
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/cplex")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/cplex/libcplex.a")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/cplex/libilocplex.a")
target_link_libraries(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib/cplex/libconcert.a")

# Link libtorch
find_package(Torch REQUIRED PATHS "${CMAKE_CURRENT_SOURCE_DIR}/lib/libtorch/share/cmake/Torch")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
target_include_directories(${PROJECT_NAME} PRIVATE ${TORCH_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${TORCH_LIBRARIES})

# Link Eigen
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
target_link_libraries (${PROJECT_NAME} PRIVATE Eigen3::Eigen)