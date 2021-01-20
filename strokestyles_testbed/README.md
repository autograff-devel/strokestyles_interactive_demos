# Strokestyles testbed 
Interactive stroke-based stylization and layering app. 

The project currently compiles on OSX only, and requires [cmake](https://cmake.org) to build. 
The compiled executable is not a bundle and needs to be run from the terminal.

## Dependencies and installation
Follow the instructions in the [README.md](https://github.com/colormotor/strokestyles_system/blob/master/README.md) file to install this and the other projects in the strokestyles system. 

Specifically, this project depends on source code from [cutograff](https://github.com/colormotor/strokestyles_system/tree/master/cautograff) which is statically linked and will be compiled with the executable.


### Compiling and running
To compile, from the repository directory do:
```
mkdir build; cd build
cmake ..
make -j4
```
With `-j` followed by the number of cores on your machine.

To execute the app, navigate to the `bin` directory and:
```
./strokestyles_testbed
```

## Demos
Tha application has a top menu (in the window):

![](images/menu.jpg)
 
 which allows to select between different demos. 
 
The demo source files are headers in the `./demos/` directory. 
Demo paramters are automatically saved as XML files in the `./bin/data/configurations/` directory.

The app currently contains the following demos:

### "Interactive Graffiti"
Demonstrates stroking and layering. 
The demo has a floating toolbar that can be used to edit strokes
![](images/tools.jpg)

Select the pencil tool and click to add stroke points. When a new stroke is created, its paramters will appear in a "Stroke" window.

![](images/strokewin.jpg)

Use the arrow tool to drag point, or to adjust smoothness by dragging the handles

![](images/smooth_demo.gif)

### "Stroke Font Stylization" and "Graffiti Font Stylization"
Demonstrate font stylization. These demos require font/svg segmentation data to operate. Pre-segmented glyph data can be downloaded for pre-segmented fonts at the this [address](https://www.dropbox.com/s/cnd0wu3gk2vmghd/glyph_data.zip?dl=0).

The zip file contains a directory `glyph_data`. The simplest way to use the pre-segmented glyphs is to copy the contents of the unzipped directory into the `apps/strokestyles_testbed/bin/data/glyph_data` directory.  Otherwise you can manually set the glyph directory from the application by selecting the **Options** menu (at the top of the app window) and then **Set glyph database** item.

Custom stroke segmentations can be generated using 
 the [strokestyles](https://github.com/colormotor/strokestyles_system/tree/master/python/projects/strokestyles) Pyhon scripts.


# Project dependency details
While it is suggested to follow the instructions in [README.md](https://github.com/colormotor/strokestyles_system/blob/master/README.md), this project only depends on statically linked files from [cutograff](https://github.com/colormotor/strokestyles_system/tree/master/cautograff) and  on the following non-default libraries, which can be installed through the packager manager of choice:

- [boost](https://www.boost.org) For graphs and some additional utilities.
- [OpenCV 4](https://opencv.org) For imaging
- [armadillo](http://arma.sourceforge.net) (compiled with LAPACK and BLAS) For linear algebra

With [Anaconda](https://anaconda.org) these can be installed with:
```
conda install -c conda-forge armadillo
conda install -c conda-forge opencv
conda install -c conda-forge boost 
```
And cmake with
```
conda install -c conda-forge cmake 
```

Equivalent procedures exist for Homebrew as well. 

### Internal dependencies
Other dependencies that are included in the repository include [Dear IMGUI](https://github.com/ocornut/imgui) (for the UI, using the [Adobe fork](https://github.com/adobe/imgui), with a small tweak), [gfx-ui](https://github.com/colormotor/gfx_ui) (for geometry interaction), [GLFW3](https://github.com/glfw/glfw) (for OpenGL contexts and windowing), and others.
