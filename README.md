# GLOW
### GL Object-oriented Windowing toolkit
A Cross-Platform Modern OpenGL Toolkit that Abstracts Native Windowing and System Calls

#### Dependencies

* Epoxy
* FreeType

#### Install (Windows)

* Download and install Microsoft Visual Studio
    * Recommended [Windows Visual Studio Express 2013 for Windows Desktop](https://www.microsoft.com/en-us/download/details.aspx?id=44914)
* Download and install [Git](https://git-scm.com/downloads) (Windows)
* Download and install [CMake](https://cmake.org/download/) (Windows win64-x64 Installer)
* Clone and build Epoxy
    * Right-click on the Desktop, select 'Git GUI Here'
        * Select 'Clone Existing Repository' from the Git GUI
        * Type 'https://github.com/anholt/libepoxy.git' for Source Location
        * Click 'Browse' for the Target Directory and select the location to download Epoxy into
            * Add '/libepoxy' to the end of the path for the Target Directory
        * Click 'Clone'
        * Close Git GUI
    * Open the 'libepoxy' folder just created in Windows Explorer
        * Right-click, select 'New' -> 'Folder', name it 'build'
        * Right-click, select 'New' -> 'Folder', name it 'install'
    * Open 'CMake (cmake-gui)'
        * Click 'Browse Source...' and select the 'libepoxy' folder
        * Click 'Browse Build...' and select the 'libepoxy/build' folder
        * Click 'Configure', click 'Finish' in popup window
        * Edit CMAKE_INSTALL_PREFIX to be the full path for the 'libepoxy/install' folder
        * Uncheck 'EPOXY_SUPPORT_EGL'
        * Click 'Configure'
        * Click 'Generate'
        * Close CMake
    * Open the 'libepoxy/build' folder in Windows Explorer and double-click 'Epoxy.sln' 
    * Project will open in Microsoft Visual Studio
        * In the Solution Explorer, right-click 'ALL_BUILD' and select 'Build'
        * In the Solution Explorer, right-click 'INSTALL' and select 'Build'
        * Close Microsoft Visual Studio
* Download and install [FreeType](http://gnuwin32.sourceforge.net/packages/freetype.htm) (Complete package, except sources)

#### Install (Mac OS X)

* Download and install [Homebrew](http://brew.sh/) (Type command in 'Terminal' app)
* Open Terminal, then execute the following:
    * brew install libepoxy
    * brew install freetype
	