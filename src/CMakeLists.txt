# Copyright (c) 2023-2023 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

IF (MSVC)
    ADD_DEFINITIONS(-DMSVC_VMG_ENABLED /std:c++11)  #  /Z7)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg")
ENDIF (MSVC)

IF (UNIX AND NOT CYGWIN)
    ADD_DEFINITIONS(
            -fPIC -DPROFILE
    )
    if (${OHOS_LITE})
        set(CMAKE_CXX_FLAGS "-g -ggdb3 -O0 -Wall -Wl,--disable-new-dtags")
    else()
        set(CMAKE_CXX_FLAGS "-g -ggdb3 -O0 -std=c++17 -Wall -Wl,--disable-new-dtags")
    endif()

ENDIF (UNIX AND NOT CYGWIN)

ADD_DEFINITIONS(
        -D__STDC_FORMAT_MACROS
        -DHST_PLUGIN_PATH="./"
        -DHST_ANY_WITH_NO_RTTI
)
if (WIN32)
add_definitions( -DHST_PLUGIN_FILE_TAIL=".dll" )
else()
add_definitions( -DHST_PLUGIN_FILE_TAIL=".so" )
endif()

IF (CMAKE_CL_64)
    ADD_DEFINITIONS(-DWIN64)
ENDIF (CMAKE_CL_64)

######################################################
include_directories(
        ${TOP_DIR}/media_foundation/src
        ${TOP_DIR}/media_foundation/src/capi/common
        ${TOP_DIR}/media_foundation/src/buffer/avbuffer/include
        ${TOP_DIR}/media_foundation/src/buffer/avbuffer_queue/include
        ${TOP_DIR}/media_foundation/interface
        ${TOP_DIR}/media_foundation/interface/inner_api
        ${TOP_DIR}/media_foundation/interface/kits/c
)

######################################################

file(GLOB_RECURSE FOUNDATION_SRCS
        ${TOP_DIR}/media_foundation/src/buffer/*.cpp
        ${TOP_DIR}/media_foundation/src/common/*.cpp
        ${TOP_DIR}/media_foundation/src/common/cpp_ext/*.cpp
        ${TOP_DIR}/media_foundation/src/meta/*.cpp
        ${TOP_DIR}/media_foundation/src/osal/base/*.cpp
        ${TOP_DIR}/media_foundation/src/osal/filesystem/*.cpp
        ${TOP_DIR}/media_foundation/src/osal/task/pthread/*.cpp
        ${TOP_DIR}/media_foundation/src/osal/utils/*.cpp
        ${TOP_DIR}/media_foundation/src/pipeline/*.cpp
        ${TOP_DIR}/media_foundation/src/plugin/*.cpp
)

set(MEDIA_FOUNDATION_SRCS ${FOUNDATION_SRCS})

link_directories(
        ${ffmpeg_lib_path}
        ${sdl_lib_path}
        ${TOP_DIR}/3rdparty/curl/lib/windows
)

if (WIN32)
    link_libraries(
            pthread
            m
            avcodec
            avformat
            avutil
            avfilter
            swresample
            swscale
            SDL2
            curl
    )
elseif (LINUX_DEMO)
else ()
    link_libraries(
            log
            FFmpeg
            SDL2
    )
endif ()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SHARED_LIBRARY_PREFIX "libmedia_engine_")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".z.so")

add_library(media_foundation SHARED ${MEDIA_FOUNDATION_SRCS}
        ${3RDPARTY_SRCS})
