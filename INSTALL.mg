                     BUILDING AND INSTALLATION OF MG
                     ===============================
 
This file explains how to build the mg system.  It has been updated for
mg version 1.2.1.
 
(1) Follow the general GNU install instructions in the INSTALL file.
    NOTE: this procedure has changed from version mg-1.1 which did
    not use a GNU configure script.
      cd ~/mg-1.2.1
      ./configure --prefix=`pwd`
      make
      make check
      make install

    If you wish to build using a different compiler than the one
    selected by the configure script, set the CC environment variable
    and re-run configure.  For example (using the bash shell):
      export CC=egcc
      rm -f config.cache
      ./configure

    If you have the GNU readline libraries installed and wish to use
    them for command-line history and editing functions, then add 
    "--with-gnu-readline" to the configure command above.  On a non
    GNU-based system you may have to specify the path to the library
    and header files in the environment of the configure script.  For
    instance, on a Solaris 2.5.1 system that keeps GNU stuff under
    /opt/gnu, using the bash shell, I type:

      LDFLAGS="$LDFLAGS -L/opt/gnu/lib" \
        CPPFLAGS="$CPPFLAGS -I/opt/gnu/include" \
	./configure --prefix=`pwd` --with-gnu-readline

    all on one logical line.  These build flags will be written into
    the makefiles that configure creates, so there is no need to 
    respecify them when you run make.

    If you wish to override the use of GCC's "long long" integer types,
    configure with
      ./configure --enable-override-longlong
    GCC's "long long" types are used in a few places to avoid overflowing
    counters when building large collections.  (This condition is detected
    and reported during the build process if it will be a problem on your 
    collection.)

(2) You will need to set the environment variable MGDATA which 
    indicates where the mg data files will be created. You will also
    need to create the directory you specify.
    e.g. (under csh/tcsh)
        setenv MGDATA ~/mgdata
	mkdir ~/mgdata
    e.g. (under bash)
        export MGDATA=~/mgdata
	mkdir ~/mgdata

(4) You may also need to set the environment variable MGSAMPLE which
    is used by mg_get and indicates where the sample data included
    with this package is located.
    If this is not specified it defaults to ``./SampleData''

(4) Included with mg is an X11 interface to mgquery called xmg. 
    Xmg is a ``wish'' script which uses the Tcl/Tk packages available
    by anonymous ftp from allspice.berkeley.edu [128.32.150.27].
 
(5) Another environment variable that you may wish to set is
    MGIMAGEVIEWER, this variable sets the image viewer to be used
    to display images. The image viewer must take the image from stdin.
    If this is not specified it defaults to ``xv -''.
 
