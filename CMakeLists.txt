cmake_minimum_required(VERSION 3.5)

project(test_communication LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets SerialPort REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets SerialPort REQUIRED)

SET(QT_USE_QTMAIN TRUE)

if(ANDROID)
  add_library(test_communication SHARED
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
  )
else()
  add_executable(test_communication
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    devicecommunication.cpp
    devicecommunication.h
  )
endif()

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
target_compile_options(test_communication PUBLIC /MT)
target_link_options(test_communication PUBLIC /INCREMENTAL:NO /NODEFAULTLIB:MSVCRT)

target_link_libraries(test_communication
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
    )
