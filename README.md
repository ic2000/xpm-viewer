# XPM Viewer

A Windows XPM2 and XPM3 image viewer written in C++ using WinAPI and GDI with the ability to export between both formats. The viewer is designed to be used with icon pixmaps but can be used with regular images. The parser also supports the [X11 colour names](https://en.wikipedia.org/wiki/X11_color_names) where "None" yields transparency. Currently, the colour types other than `c` are unspported as well as the `<Optional Extensions>` section.

The parser is decoupled from the Windows application and does not include any platform-specific API code so it may be used for other projects.

![Screenshot](https://i.imgur.com/PLTP0Yb.png)

## Known Bugs
- The XPM parser throws an invalid file execption when the colour count is of an extreme number (e.g. in the tens of thousands). 

## Todo
-    Implement support for the other colour types and the `<Optional Extensions>` section.
-    Implement a horizontal and vertical scroll when the image exceeds the window's dimensions.
