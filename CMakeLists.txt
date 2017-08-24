cmake_minimum_required(VERSION 3.0)
project(LinkPie)

set(CMAKE_CXX_FLAGS "-std=c++11 -Werror -Wno-multichar -DLINK_PLATFORM_LINUX=1")

set(FILES
	${FILES}
	main.cpp
    )

include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/link/include and ${CMAKE_SOURCE_DIR}/link/modules/asio-standalone/asio/include
)

add_executable(LinkPie ${FILES})
target_link_libraries(LinkPie rt pthread)
