# Install script for directory: C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/lib/SFML

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE DIRECTORY FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/lib/SFML/include" FILES_MATCHING REGEX "/[^/]*\\.hpp$" REGEX "/[^/]*\\.inl$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE DIRECTORY FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/lib" FILES_MATCHING REGEX "/[^/]*\\.pdb$")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/lib/SFML/license.md")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/." TYPE FILE FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/lib/SFML/readme.md")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./bin" TYPE DIRECTORY FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/lib/SFML/extlibs/bin/x64/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/./lib" TYPE DIRECTORY FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/lib/SFML/extlibs/libs-msvc-universal/x64/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets.cmake"
         "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/CMakeFiles/Export/3937c6824958577f216dad0a66bc6149/SFMLStaticTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML/SFMLStaticTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML" TYPE FILE FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/CMakeFiles/Export/3937c6824958577f216dad0a66bc6149/SFMLStaticTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML" TYPE FILE FILES "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/CMakeFiles/Export/3937c6824958577f216dad0a66bc6149/SFMLStaticTargets-debug.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/SFML" TYPE FILE FILES
    "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/SFMLConfig.cmake"
    "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/SFMLConfigDependencies.cmake"
    "C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/SFMLConfigVersion.cmake"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Users/Håkon/source/repos/Grafikk prosjekt/TDT4230-Assignment-1/out/build/x64-Debug/lib/SFML/src/SFML/cmake_install.cmake")

endif()

