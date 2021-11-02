#pragma once

#include <map>
#include <string>
#include "rgb.h"
#include <vector>

class Xpm {
  int _width;
  int _height;
  int _colour_count;
  int _chars_per_pixel;
  std::map<std::string, Rgba> _colour_map;
  std::vector<std::vector<std::string>> _pixels;

public:
  static std::string Xpm2ToXpm3(std::wstring_view file_name,
    std::string file_contents);

  static std::string Xpm3ToXpm2(std::string_view file_contents);

  Xpm();
  const int& width() const;
  const int& height() const;
  const int& colour_count() const;
  const int& chars_per_pixel() const;
  const std::map<std::string, Rgba>& colour_map() const;
  const std::vector<std::vector<std::string>>& pixels() const;
  void ParseXpm2(std::string file_contents);
  void ParseXpm3(std::string file_contents);
};