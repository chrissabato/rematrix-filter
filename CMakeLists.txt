cmake_minimum_required(VERSION 3.16)

project(rematrix-filter VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(libobs REQUIRED)

add_library(rematrix-filter MODULE rematrix-filter.cpp)

target_link_libraries(rematrix-filter PRIVATE OBS::libobs)

# Installation paths vary by platform to match OBS's expected layout
if(UNIX AND NOT APPLE)
  include(GNUInstallDirs)
  install(TARGETS rematrix-filter
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins"
  )
  install(DIRECTORY data/
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/rematrix-filter"
  )
elseif(WIN32)
  install(TARGETS rematrix-filter
    RUNTIME DESTINATION "obs-plugins/64bit"
    LIBRARY DESTINATION "obs-plugins/64bit"
  )
  install(DIRECTORY data/
    DESTINATION "data/obs-plugins/rematrix-filter"
  )
elseif(APPLE)
  install(TARGETS rematrix-filter
    LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins/rematrix-filter/bin"
  )
  install(DIRECTORY data/
    DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins/rematrix-filter/data"
  )
endif()
