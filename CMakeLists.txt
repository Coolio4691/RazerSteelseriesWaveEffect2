cmake_minimum_required(VERSION 3.0.0)
project(wave VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/src/include)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/src/main.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


find_package(PkgConfig REQUIRED)

pkg_check_modules(DBUS REQUIRED dbus-1)
include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

pkg_check_modules(RAZER REQUIRED razer)
include_directories(${RAZER_INCLUDE_DIRS})
link_directories(${RAZER_LIBRARY_DIRS})


pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
include_directories(${LIBUSB_INCLUDE_DIRS})
link_directories(${LIBUSB_LIBRARY_DIRS})


pkg_check_modules(HIDAPI REQUIRED hidapi-hidraw)
include_directories(${HIDAPI_INCLUDE_DIRS})
link_directories(${HIDAPI_LIBRARY_DIRS})


pkg_check_modules(LIBVIRT REQUIRED libvirt)
include_directories(${LIBVIRT_INCLUDE_DIRS})
link_directories(${LIBVIRT_LIBRARY_DIRS})


target_link_libraries(${PROJECT_NAME} ${DBUS_LIBRARIES} ${RAZER_LIBRARIES} ${LIBUSB_LIBRARIES} ${HIDAPI_LIBRARIES} ${LIBVIRT_LIBRARIES} m pthread)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/VMToggle.sh
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
