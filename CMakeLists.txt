cmake_minimum_required(VERSION 3.10)

project(client)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost 1.66 REQUIRED)
IF(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
ELSE()
  message(FATAL "${CMAKE_SOURCE_DIR}/../include")
ENDIF(Boost_FOUND)

include_directories(SYSTEM BEFORE include)
include_directories(BEFORE .)

set(link_libs
  PRIVATE Threads::Threads
  PRIVATE OpenSSL::Crypto
  PRIVATE OpenSSL::SSL
)

macro(add_tg bts)
  add_executable(prueba main.cpp bothandler.cpp)
  target_link_libraries(prueba ${link_libs})
endmacro(add_tg bts)

add_tg(bts)
