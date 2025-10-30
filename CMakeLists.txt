cmake_minimum_required(VERSION 3.12)
project(SeleniumCpp)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)

find_path(WEBDRIVERXX_INCLUDE_DIR webdriverxx/webdriverxx.h)
find_library(WEBDRIVERXX_LIBRARY webdriverxx)

add_executable(selenium_app 
    main.cpp 
    auth.cpp 
    redirect.cpp
    monitor.cpp
)

target_include_directories(selenium_app PRIVATE 
    ${WEBDRIVERXX_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(selenium_app 
    ${WEBDRIVERXX_LIBRARY} 
    pthread
    curl
    jsoncpp
)