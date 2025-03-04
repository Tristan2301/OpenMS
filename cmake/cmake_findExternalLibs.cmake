# --------------------------------------------------------------------------
#                   OpenMS -- Open-Source Mass Spectrometry
# --------------------------------------------------------------------------
# Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
# ETH Zurich, and Freie Universitaet Berlin 2002-2022.
#
# This software is released under a three-clause BSD license:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of any author or any participating institution
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
# For a full list of authors, refer to the file AUTHORS.
# --------------------------------------------------------------------------
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
# INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# --------------------------------------------------------------------------
# $Maintainer: Stephan Aiche, Chris Bielow $
# $Authors: Chris Bielow, Stephan Aiche $
# --------------------------------------------------------------------------

#------------------------------------------------------------------------------
# This cmake file handles finding external libs for OpenMS
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# find libs (for linking)
# On Windows:
#   * on windows we need the *.lib versions (dlls alone won't do for linking)
#   * never mix Release/Debug versions of libraries. Leads to strange segfaults,
#     stack corruption etc, due to different runtime libs ...
# compiler-wise: use the same compiler for contrib and OpenMS!
find_package(XercesC REQUIRED)

#------------------------------------------------------------------------------
# BOOST
set(OpenMS_BOOST_COMPONENTS date_time regex CACHE INTERNAL "Boost components for core lib")
find_boost(iostreams ${OpenMS_BOOST_COMPONENTS})

if(Boost_FOUND)
  message(STATUS "Found Boost version ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}" )
  set(CF_OPENMS_BOOST_VERSION_MAJOR ${Boost_MAJOR_VERSION})
	set(CF_OPENMS_BOOST_VERSION_MINOR ${Boost_MINOR_VERSION})
  set(CF_OPENMS_BOOST_VERSION_SUBMINOR ${Boost_SUBMINOR_VERSION})
	set(CF_OPENMS_BOOST_VERSION ${Boost_VERSION})
else()
  message(FATAL_ERROR "Boost or one of its components not found!")
endif()

#------------------------------------------------------------------------------
# libsvm
# creates LibSVM target
find_package(LIBSVM 2.91 REQUIRED) ## will not overwrite LIBSVM_LIBRARY if defined already
if (LIBSVM_FOUND)
	set(CF_OPENMS_LIBSVM_VERSION_MAJOR ${LIBSVM_MAJOR_VERSION})
	set(CF_OPENMS_LIBSVM_VERSION_MINOR ${LIBSVM_MINOR_VERSION})
	set(CF_OPENMS_LIBSVM_VERSION ${LIBSVM_VERSION})
endif()

#------------------------------------------------------------------------------
# COIN-OR
# Our find module creates an imported CoinOR::CoinOR target
find_package(COIN)
if (COIN_FOUND)
  set(CF_USECOINOR 1)
  set(LPTARGET "CoinOR::CoinOR")
else()
  #------------------------------------------------------------------------------
  # GLPK
  # creates GLPK::GLPK target
  find_package(GLPK)
  if (GLPK_FOUND)
    set(CF_OPENMS_GLPK_VERSION_MAJOR ${GLPK_VERSION_MAJOR})
    set(CF_OPENMS_GLPK_VERSION_MINOR ${GLPK_VERSION_MINOR})
    set(CF_OPENMS_GLPK_VERSION ${GLPK_VERSION_STRING})
    set(LPTARGET "GLPK::GLPK")
  else()
    message(FATAL_ERROR "Either COIN-OR or GLPK has to be available (COIN-OR takes precedence).")
  endif()
endif()

#------------------------------------------------------------------------------
# zlib
# creates ZLIB::ZLIB target
find_package(ZLIB REQUIRED)

#------------------------------------------------------------------------------
# bzip2
find_package(BZip2 REQUIRED)

#------------------------------------------------------------------------------
# Find eigen3
# creates Eigen3::Eigen3 package
find_package(Eigen3 3.3.4 REQUIRED)


#------------------------------------------------------------------------------
# Find Crawdad libraries if requested
# cmake args: -DCrawdad_DIR=/path/to/Crawdad/ -DWITH_CRAWDAD=TRUE
## TODO check if necessary
if (WITH_CRAWDAD)
  message(STATUS "Will compile with Crawdad support: ${Crawdad_DIR}" )
  find_package(Crawdad REQUIRED)
  # find archive (static) version and add it to the OpenMS library
  find_library(Crawdad_LIBRARY
   NAMES Crawdad.a Crawdad
    HINTS ${Crawdad_DIR})
endif()

#------------------------------------------------------------------------------
# HDF5
if (WITH_HDF5)
  # For MSVC use static linking to the HDF5 libraries
  if(MSVC)
    set(HDF5_USE_STATIC_LIBRARIES ON)
  endif()
  find_package(HDF5 MODULE REQUIRED COMPONENTS C CXX)
endif()

#------------------------------------------------------------------------------
# Done finding contrib libraries
#------------------------------------------------------------------------------

#except for the contrib libs, prefer shared libraries
if(NOT MSVC AND NOT APPLE)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.a")
endif()

#------------------------------------------------------------------------------
# QT
#------------------------------------------------------------------------------
SET(QT_MIN_VERSION "5.6.0")

# find qt
set(OpenMS_QT_COMPONENTS Core Network CACHE INTERNAL "QT components for core lib")
find_package(Qt5 ${QT_MIN_VERSION} COMPONENTS ${OpenMS_QT_COMPONENTS} REQUIRED)

IF (NOT Qt5Core_FOUND)
  message(STATUS "Qt5Core not found!")
  message(FATAL_ERROR "To find a custom Qt installation use: cmake <..more options..> -DCMAKE_PREFIX_PATH='<path_to_parent_folder_of_lib_folder_withAllQt5Libs>' <src-dir>")
ELSE()
  message(STATUS "Found Qt ${Qt5Core_VERSION}")
ENDIF()

# see https://github.com/ethereum/solidity/issues/4124
if("${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}" VERSION_LESS "1.59")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_VARIANT_USE_RELAXED_GET_BY_DEFAULT")
endif()

#------------------------------------------------------------------------------
# PTHREAD
#------------------------------------------------------------------------------
# Prefer the -pthread compiler flag to be consistent with SQLiteCpp and avoid
# rebuilds
# TODO Do we even need this, when OpenMP is not active?
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)


if (WITH_GUI)
  # --------------------------------------------------------------------------
  # Find additional Qt libs
  #---------------------------------------------------------------------------
  set (TEMP_OpenMS_GUI_QT_COMPONENTS Gui Widgets Svg)

  # On macOS the platform plugin of QT requires PrintSupport. We link
  # so it's packaged via the bundling/dependency tools/scripts
  if (APPLE)
    set (TEMP_OpenMS_GUI_QT_COMPONENTS ${TEMP_OpenMS_GUI_QT_COMPONENTS} PrintSupport)
  endif()

  set(OpenMS_GUI_QT_COMPONENTS ${TEMP_OpenMS_GUI_QT_COMPONENTS} CACHE INTERNAL "QT components for GUI lib")

  if(NOT NO_WEBENGINE_WIDGETS)
    set(OpenMS_GUI_QT_COMPONENTS_OPT WebEngineWidgets)
  endif()

  find_package(Qt5 REQUIRED COMPONENTS ${OpenMS_GUI_QT_COMPONENTS})

  IF (NOT Qt5Widgets_FOUND OR NOT Qt5Gui_FOUND OR NOT Qt5Svg_FOUND)
    message(STATUS "Qt5Widgets not found!")
    message(FATAL_ERROR "To find a custom Qt installation use: cmake <..more options..> -DCMAKE_PREFIX_PATH='<path_to_parent_folder_of_lib_folder_withAllQt5Libs>' <src-dir>")
  ENDIF()

  ## QuickWidgets is a runtime-only dependency that we need to copy and install when WebEngine is found.
  # https://gitlab.kitware.com/cmake/cmake/-/issues/16462
  # https://bugreports.qt.io/browse/QTBUG-110118
  find_package(Qt5 QUIET COMPONENTS ${OpenMS_GUI_QT_COMPONENTS_OPT} QuickWidgets)

  # TODO only works if WebEngineWidgets is the only optional component
  set(OpenMS_GUI_QT_FOUND_COMPONENTS_OPT)
  if(Qt5WebEngineWidgets_FOUND)
    list(APPEND OpenMS_GUI_QT_FOUND_COMPONENTS_OPT "WebEngineWidgets")
    # we assume that it is available for now. They should have dependencies when installing Qt.
    install(IMPORTED_RUNTIME_ARTIFACTS "Qt5::QuickWidgets"
            DESTINATION "${INSTALL_LIB_DIR}"
            RUNTIME_DEPENDENCY_SET OPENMS_GUI_DEPS
            COMPONENT Dependencies)
  else()
    message(WARNING "Qt5WebEngineWidgets not found or disabled, disabling JS Views in TOPPView!")
  endif()

  # The following can be checked since Qt 5.12 https://github.com/qtwebkit/qtwebkit/issues/846
  # evaluates to False if it does not exist
  # TODO check what needs to be done for other values
  if (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL GLESv2)
      install(IMPORTED_RUNTIME_ARTIFACTS "Qt5::Gui_EGL"
            DESTINATION "${INSTALL_LIB_DIR}"
            RUNTIME_DEPENDENCY_SET OPENMS_GUI_DEPS
            COMPONENT Dependencies)
      install(IMPORTED_RUNTIME_ARTIFACTS "Qt5::Gui_GLESv2"
            DESTINATION "${INSTALL_LIB_DIR}"
            RUNTIME_DEPENDENCY_SET OPENMS_GUI_DEPS
            COMPONENT Dependencies)
  endif()

  set(OpenMS_GUI_DEP_LIBRARIES "OpenMS")

  foreach(COMP IN LISTS OpenMS_GUI_QT_COMPONENTS)
    list(APPEND OpenMS_GUI_DEP_LIBRARIES "Qt5::${COMP}")
  endforeach()

  foreach(COMP IN LISTS OpenMS_GUI_QT_FOUND_COMPONENTS_OPT)
    list(APPEND OpenMS_GUI_DEP_LIBRARIES "Qt5::${COMP}")
  endforeach()

endif()
#------------------------------------------------------------------------------
