
add_executable(async main.cpp)

target_link_libraries(async
  PUBLIC
    dbimpl)

target_include_directories(async
  PUBLIC
    ${CMAKE_SOURCE_DIR}/dbimpl
    ${CMAKE_SOURCE_DIR}/fmt/include)