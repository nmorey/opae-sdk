#!/usr/bin/cmake -P
## Copyright(c) 2021, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

macro(ofs_add_driver yml_file driver)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${driver}.h
        COMMAND ${PYTHON_EXECUTABLE} ${OPAE_LIBS_ROOT}/scripts/ofs/ofs_parse.py ${CMAKE_CURRENT_LIST_DIR}/${yml_file} headers c ${CMAKE_CURRENT_BINARY_DIR} --driver ${driver}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
            ${CMAKE_CURRENT_LIST_DIR}/${yml_file}
            ${OPAE_LIBS_ROOT}/scripts/ofs/ofs_parse.py
            ${OPAE_LIBS_ROOT}/scripts/ofs/umd.py
    )
    add_library(${driver} SHARED
        ${CMAKE_CURRENT_BINARY_DIR}/${driver}.h
        ${ARGN}
    )
target_include_directories(${driver} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/ofs/libofs>
    $<BUILD_INTERFACE:${OPAE_INCLUDE_PATH}>
)

target_link_libraries(${driver} PUBLIC
    opae-c
    ofs
)
endmacro(ofs_add_driver yml_file)


