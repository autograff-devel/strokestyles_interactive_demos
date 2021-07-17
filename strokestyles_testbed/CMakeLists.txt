cmake_minimum_required(VERSION 3.0.0)
project(strokestyles_testbed VERSION 0.1.0)

set( CMAKE_VERBOSE_MAKEFILE on )
set(CMAKE_CXX_STANDARD 14)

if(1) #APPLE)
  set(CMAKE_C_COMPILER "/usr/bin/clang")
  set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable some warning flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-absolute-value") # fabs
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format") # Clipper
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch") # Clipper
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-macro-redefined") # AG_DISABLE_COLORMOTOR (fixme)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations") # AG_DISABLE_COLORMOTOR (fixme)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dynamic-class-memaccess") # Chunk swig binding. Non-critical now but fix
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-extra-qualification") # Imgui
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes") # stdcall Gfx tessellation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dangling-else")  # stb_truetype


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/)

set(CAUTOGRAFF_DIR ${PROJECT_SOURCE_DIR}/../../cautograff)

# Add base path where your principal library/include are located here
# In order of priority
# This should not require editing unless
set(BASE_DIRS
    $ENV{HOME}/miniconda3
    $ENV{HOME}/opt/miniconda3
    $ENV{HOME}/anaconda3
    $ENV{HOME}/opt/anaconda3
    /usr/local
)

foreach(DIR ${BASE_DIRS})
    message("Testing possible base path:" ${DIR})
    if(EXISTS ${DIR})
        set(BASE_SEARCH_DIR ${DIR})
        break()
    endif()
endforeach()

# The following can be used to force a base path
# set(BASE_SEARCH_DIR $ENV{HOME}/miniconda3) 

message("Base path is " ${BASE_SEARCH_DIR})

find_package(OpenGL REQUIRED)
find_package(CGAL REQUIRED)

add_definitions(-g)
add_definitions(-DGL_SILENCE_DEPRECATION)
#add_definitions(-std=gnu++14)
set(PROTOTYPE_DIR ${PROJECT_SOURCE_DIR}/src/strokestyles_prototypes)
if(EXISTS ${PROTOTYPE_DIR}) #"${PROJECT_SOURCE_DIR}/src/strokestyles_prototypes")
   message("src/strokestyles_prototypes folder exists: adding definitions")
   add_definitions(-DINCLUDE_PROTOTYPES)
   # add_subdirectory(src/strokestyles_prototypes)
   include_directories(
        ${PROTOTYPE_DIR}
   )
else()
    message("src/strokestyles_prototypes folder does not exist")
endif()

include_directories(include
	  ${PROJECT_SOURCE_DIR}/src/
      ${PROJECT_SOURCE_DIR}/src/colormotor
      ${PROJECT_SOURCE_DIR}/src/colormotor/deps
      ${PROJECT_SOURCE_DIR}/src/colormotor/app
	  ${PROJECT_SOURCE_DIR}/src/colormotor/addons/app
      /usr/local/include
)

# #${PROJECT_SOURCE_DIR}/../../../lib/
# link_directories(
# 	${PROJECT_SOURCE_DIR}/../colormotor/addons/pycolormotor/modules
# )
if(APPLE)
    set(PLATFORM_SRC
      "${PROJECT_SOURCE_DIR}/src/colormotor/platform/cm_utils_osx.mm"
      )
elseif(UNIX)
   set(PLATFORM_SRC
      "${PROJECT_SOURCE_DIR}/src/colormotor/platform/cm_utils_linux.cpp"
      )
endif()

set(ARMADILLO_LIBRARIES
        ${ARMADILLO_LIBRARIES}
        lapack
        blas)

link_directories(
  ${BASE_SEARCH_DIR}/lib
  ${PROJECT_SOURCE_DIR}/src/colormotor/app/glfw3/lib/osx
  /usr/local/lib
)

# add_definitions("-F${PROJECT_SOURCE_DIR}/src/deps/syphon_wrapper")

include_directories(
    ${BASE_SEARCH_DIR}/include
    ${BASE_SEARCH_DIR}/include/opencv4

    # autograff
    ${CAUTOGRAFF_DIR}/src/
    ${CAUTOGRAFF_DIR}/src/geom
    ${CAUTOGRAFF_DIR}/src/graffiti
    ${CAUTOGRAFF_DIR}/src/deps/ftgrays
    ${CAUTOGRAFF_DIR}/src/deps/tinyspline
    ${CAUTOGRAFF_DIR}/src/deps/arma_ext
)

add_executable(strokestyles_testbed
    ${PROJECT_SOURCE_DIR}/src/main.cpp

    # Demo additional
    ${PROJECT_SOURCE_DIR}/src/common.cpp
    ${PROJECT_SOURCE_DIR}/src/demos/font_stylization_base.cpp

    # Colormotor
    ${PROJECT_SOURCE_DIR}/src/colormotor/cm_gfx.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/cm_math.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/cm_params.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/cm_utils.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/cm_vector_export.cpp

    # App
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/cm_imgui.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/cm_imgui_app.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/gfx_ui.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/imgui.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/src/colormotor/app/spectrum.cpp

    ${PROJECT_SOURCE_DIR}/src/colormotor/deps/tinyxml2/tinyxml2.cpp

    # Platform
    ${PLATFORM_SRC}

    # Autograff
    # - geom
    ${CAUTOGRAFF_DIR}/src/geom/ag_geom.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_lqt.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_dynamic_spline.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_turtle.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_clothoid.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_planar_map.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_sweepline_intersections.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_alpha_shape.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_alpha_shape.cpp
    ${CAUTOGRAFF_DIR}/src/geom/ag_lqt_helpers.cpp


    # - graffiti
    ${CAUTOGRAFF_DIR}/src/graffiti/ag_graff_font.cpp
    ${CAUTOGRAFF_DIR}/src/graffiti/ag_graff_primitives.cpp
    ${CAUTOGRAFF_DIR}/src/graffiti/ag_layering.cpp
    ${CAUTOGRAFF_DIR}/src/graffiti/ag_layering_data.cpp
    ${CAUTOGRAFF_DIR}/src/graffiti/ag_schematize.cpp
    ${CAUTOGRAFF_DIR}/src/graffiti/ag_skeletal_strokes.cpp

    # - root
    ${CAUTOGRAFF_DIR}/src/ag_color.cpp
    ${CAUTOGRAFF_DIR}/src/ag_colormotor.cpp
    ${CAUTOGRAFF_DIR}/src/ag_imaging.cpp
    ${CAUTOGRAFF_DIR}/src/ag_json.cpp
    ${CAUTOGRAFF_DIR}/src/ag_numeric.cpp
    ${CAUTOGRAFF_DIR}/src/ag_random.cpp
    ${CAUTOGRAFF_DIR}/src/ag_svg.cpp
    ${CAUTOGRAFF_DIR}/src/ag_utils.cpp
    ${CAUTOGRAFF_DIR}/src/autograff.cpp

    # - deps
    ${CAUTOGRAFF_DIR}/src/deps/tinyspline/tinyspline.c
    ${CAUTOGRAFF_DIR}/src/deps/tinyspline/tinysplinecpp.cpp
    ${CAUTOGRAFF_DIR}/src/deps/polypartition/polypartition.cpp
    ${CAUTOGRAFF_DIR}/src/deps/clipper2/clipper.cpp
    ${CAUTOGRAFF_DIR}/src/deps/clipper2/clipper_offset.cpp
    ${CAUTOGRAFF_DIR}/src/deps/clipper2/clipper_triangulation.cpp

    ${CAUTOGRAFF_DIR}/src/deps/ase/ase.cpp

    ${CAUTOGRAFF_DIR}/src/deps/tinyspline/tinysplinecpp.cpp

    ${CAUTOGRAFF_DIR}/src/deps/colours/algo.cpp
    ${CAUTOGRAFF_DIR}/src/deps/colours/ciecam02.c
    ${CAUTOGRAFF_DIR}/src/deps/colours/representation.cpp

    ${CAUTOGRAFF_DIR}/src/deps/tinyutf8.cpp
    ${CAUTOGRAFF_DIR}/src/deps/FastNoise.cpp

    ${CAUTOGRAFF_DIR}/src/deps/qhull/geom.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/geom2.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/global.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/io.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/libqhull.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/mem.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/merge.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/poly.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/poly2.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/qset.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/random.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/rboxlib.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/stat.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/user.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/usermem.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/userprintf.c
    ${CAUTOGRAFF_DIR}/src/deps/qhull/userprintf_rbox.c

    ${CAUTOGRAFF_DIR}/src/deps/fitCurves.cpp

    #${PROJECT_SOURCE_DIR}/src/deps/syphon_wrapper/syphon_wrapper.mm
    #${PROJECT_SOURCE_DIR}/src/deps/syphon_wrapper/SyphonNameboundClient.m
)

target_link_libraries(strokestyles_testbed
    opencv_core
    opencv_imgproc
    opencv_highgui
    opencv_imgcodecs
    opencv_videoio
    armadillo
    #LAPACK
    blas
    boost_system
    boost_filesystem
    boost_thread
    mpfr
    gmp
    )

if(EXISTS ${PROTOTYPE_DIR}) #"${PROJECT_SOURCE_DIR}/src/strokestyles_prototypes")
include(${PROJECT_SOURCE_DIR}/src/strokestyles_prototypes/dependencies.cmake)
endif()

message(${CMAKE_SYSTEM_NAME})

if(APPLE)
  # find_library(SYPHON Syphon)
  message("Platform: Apple")

    include_directories(
      ${PROJECT_SOURCE_DIR}/src/colormotor/app/glfw3/include
      #${PROJECT_SOURCE_DIR}/src/deps/syphon_wrapper
      #${PROJECT_SOURCE_DIR}/src/deps/syphon_wrapper/Syphon.framework/Headers
    )

    target_link_libraries(strokestyles_testbed
        "-framework CoreFoundation"
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework AppKit"
        "-framework CoreVideo"
        "-framework QTKit"
        "-framework IOKit"
        "-framework AGL"
        "-framework GLUT"
        "-framework Accelerate"
        #"-framework Syphon"
        #${PROJECT_SOURCE_DIR}/src/deps/syphon_wrapper/Syphon.framework
        glfw3
        )
elseif(UNIX)
    message("Platform: Linux")

    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${BASE_SEARCH_DIR}/lib/pkgconfig")

    # Use the package PkgConfig to detect GTK+ headers/library files
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(GTK2 REQUIRED gtk+-2.0)
    find_package(GLEW REQUIRED)

    # Setup CMake to use GTK+, tell the compiler where to look for headers
    # and to the linker where to look for libraries
    INCLUDE_DIRECTORIES(${GTK2_INCLUDE_DIRS})
    LINK_DIRECTORIES(${GTK2_LIBRARY_DIRS})

    # Add other flags to the compiler
    ADD_DEFINITIONS(${GTK2_CFLAGS_OTHER})

        target_link_libraries(strokestyles_testbed
            glfw
            pthread
            ${OPENGL_LIBRARIES}
            ${GTK2_LIBRARIES}
            ${GLEW_LIBRARIES}
        )
endif()
