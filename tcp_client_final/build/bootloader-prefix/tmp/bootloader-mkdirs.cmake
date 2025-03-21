# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/karlieli/esp/esp-idf/components/bootloader/subproject"
  "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader"
  "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader-prefix"
  "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader-prefix/tmp"
  "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader-prefix/src"
  "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/karlieli/Documents/VPU-feather/VPU_ESP32/tcp_client_final/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
