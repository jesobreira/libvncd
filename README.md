# About
A VNC server providing a custom framebuffer. Forked from [ivysaur.me](https://code.ivysaur.me/libvncd/) (credits are given to the original author). This fork contains a few bug fixes, reverse connection support and some interesting examples.

You can inherit from the VncdConnection class as per the sample, and respond to the simple framebuffer and input callbacks. This design allows using the VNC/RFB protocol as a wrapper for arbitrary graphical tasks.

# Features

* Supports raw, zlib, ZRLE, and TightPNG image encoding
* Supports optional VNC authentication
* Single-threaded asynchronous design supporting multiple simultaneous clients
* Tested working with TightVNC Viewer 2.7 and RealVNC Viewer 4.1
* Tested compilation with Visual Studio 2013, but cross-platform/compiler ports should be trivial
* Designed to be connected to a custom framebuffer implementation

# License

Unlike most other VNC implementations (e.g. libvncserver, QEMU-kvm, and RealVNC), no code in this project is based on the original GPL'd source code release of AT&T VNC. This project is a clean implementation from RFC6143, made freely available under non-copyleft software license (along with all its bundled dependencies):

* libvncd, by the libvncd author. (ISC license)
* ASIO, by Christopher M. Kohlhoff. (Boost Software License)
* D3DES, by Richard Outerbridge. (Public Domain)
* miniz, by Rich Geldreich. (Public Domain)
* X11 (for keysymdef.h), by The Open Group. (MIT/X11 License)

SEE ALSO

* https://tools.ietf.org/rfc/rfc6143.txt
* http://vncdotool.readthedocs.org/en/latest/rfbproto.html
