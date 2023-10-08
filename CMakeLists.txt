cmake_minimum_required(VERSION 3.5)
project(inpcom)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_EXTRA_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")

set(CMAKE_CXX_STANDARD 14)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)

add_library(${PROJECT_NAME} SHARED
    ${SRC_LIST}
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/test TEST_SRC_LIST)
add_executable(test ${TEST_SRC_LIST})
target_link_libraries(test PUBLIC ${PROJECT_NAME} pthread)

install(TARGETS ${PROJECT_NAME}
    DESTINATION lib)

install(DIRECTORY include/
    DESTINATION include)

