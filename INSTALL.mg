                     BUILDING AND INSTALLATION OF MG
                     ===============================
 
This file explains how to build the mg system.
 
(1) Follow the general GNU install instructions in the INSTALL file.
    NOTE: this procedure has changed from version mg-1.1 which did
    not use a GNU configure script.
    e.g.
      cd ~/mg-1.2
      ./configure --prefix=`pwd`
      make
      make check
      make install
    Report portability problems to tes@kbs.citri.edu.au .
 
(2) You will need to set the environment variable MGDATA which 
    indicates where the mg data files will be created. You will also
    need to create the directory you specify.
 
    e.g.
        setenv MGDATA ~/mgdata
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
 
