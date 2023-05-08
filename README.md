# XPM Viewer

> **Note:**
> This is my first C++ project with the aim of learning the language and WinAPI, the code is in need of a refactor with modern C++ practices.

A Windows XPM2 and XPM3 image viewer written in C++ using WinAPI and GDI with the ability to export between both formats. The viewer is designed to be used with icon pixmaps but can be used with regular images. The parser also supports the [X11 colour names](https://en.wikipedia.org/wiki/X11_color_names) where "None" yields transparency. Currently, the colour types other than `c` are unsupported as well as the `<Optional Extensions>` section.

The parser is decoupled from the Windows application and does not include any platform-specific API code so it may be used for other projects.

![Screenshot](https://i.imgur.com/PLTP0Yb.png)

## Known Bugs
- The XPM parser throws an invalid file exception when the colour count is of an extreme number (e.g. in the tens of thousands). 

## Todo
-    Implement support for the other colour types and the `<Optional Extensions>` section.
-    Implement a horizontal and vertical scroll when the image exceeds the window's dimensions.
