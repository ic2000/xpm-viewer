#include "xpm.h"
#include <algorithm>
#include <charconv>
#include <system_error>
#include <cctype>
#include "x11_colours.h"

Xpm::Xpm() : _width(0), _height(0), _colour_count(0), _chars_per_pixel(0)
{

}

const int& Xpm::width() const {
  return _width;
}

const int& Xpm::height() const {
  return _height;
}

const int& Xpm::colour_count() const {
  return _colour_count;
}

const int& Xpm::chars_per_pixel() const {
  return _chars_per_pixel;
}

const std::map<std::string, Rgba>& Xpm::colour_map() const {
  return _colour_map;
}

const std::vector<std::vector<std::string>>& Xpm::pixels() const {
  return _pixels;
}

static std::string strip_unicode(std::wstring_view s) {
  std::string result;

  for (auto ch : s)
    if (ch >= 0 && ch < 128)
      result.push_back((char)ch);

  return result;
}

static std::vector<std::string> split_string(std::string_view s,
  const char delim)
{
  auto last_delim_pos = std::string::npos;
  std::vector<std::string> result;

  for (std::string_view::size_type i = 0; i < s.size(); i++)
    if (s[i] == delim) {
      if (last_delim_pos == std::string::npos) {
        if (i)
          result.push_back((std::string)s.substr(0, i));
      }

      else
        if (i - last_delim_pos > 1)
          result.push_back((std::string)s.substr(last_delim_pos + 1,
            i - last_delim_pos - 1));

      last_delim_pos = i;
    }

    else if (i == s.size() - 1)
      result.push_back((std::string)s.substr(last_delim_pos + 1));

  return result;
}

enum class StripDirection
{
  left,
  right,
  left_and_right,
};

static bool is_whitespace(const char ch) {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

static std::string strip_whitespace(std::string_view s,
  const StripDirection& direction = StripDirection::left_and_right)
{
  auto find_start_pos = [&s]() {
    auto start_pos = std::string::npos;

    for (std::string_view::size_type i = 0; i < s.size(); i++)
      if (!is_whitespace(s[i])) {
        start_pos = i;

        break;
      }

    return start_pos;
  };

  auto find_end_pos = [&s](const std::string_view::size_type start_pos = 0) {
    auto end_pos = s.size() - 1;

    for (std::string_view::size_type i = s.size() - 1; i >= start_pos; i--)
      if (!is_whitespace(s[i])) {
        end_pos = i;

        break;
      }

    return end_pos;
  };

  if (direction == StripDirection::right)
    return (std::string)s.substr(0, find_end_pos() + 1);

  const auto start_pos = find_start_pos();

  if (start_pos == std::string::npos)
    return "";

  if (direction == StripDirection::left)
    return (std::string)s.substr(start_pos);

  return (std::string)s.substr(start_pos,
    find_end_pos(start_pos) - start_pos + 1);
}

std::string Xpm::Xpm2ToXpm3(std::wstring_view file_name,
  std::string file_contents)
{
  std::string array_name = strip_unicode(file_name);
  const auto dot_pos = file_name.find('.');

  if (dot_pos != std::string::npos)
    array_name = array_name.substr(0, dot_pos);

  std::replace(array_name.begin(), array_name.end(), ' ', '_');

  std::string result = "/* XPM */\nstatic char* " + array_name +
    "_xpm[] = {\n";

  std::replace(file_contents.begin(), file_contents.end(), '\r', '\n');

  const std::vector<std::string> lines = split_string(file_contents, '\n');

  if (!lines.empty()) {
    const std::string line_0_left_stripped = strip_whitespace(lines[0],
      StripDirection::left);

    bool has_header = false;

    if (!line_0_left_stripped.empty())
      has_header = line_0_left_stripped[0] == '!';

    for (std::vector<std::string>::size_type i = has_header; i < lines.size();
      i++)
    {
      const std::string& line = lines[i];
      result += "  \"" + line + '"';

      if (line != lines.back())
        result += ',';

      result += '\n';
    }
  }

  return result + "};";
}

std::string Xpm::Xpm3ToXpm2(std::string_view file_contents) {
  const auto start_pos = file_contents.find('{');

  if (start_pos == std::string::npos)
    return "";

  const auto end_pos = file_contents.find('}');

  if (end_pos == std::string::npos)
    return "";

  std::string elements_area = (std::string)file_contents.substr(start_pos + 1,
    end_pos - start_pos - 1);

  bool in_quote = false;
  std::vector<std::string> raw_elements = { "" };

  for (std::string::size_type i = 0, raw_element_index = 0;
    i < file_contents.size(); i++)
  {
    if (file_contents[i] == '"') {
      in_quote = !in_quote;

      continue;
    }

    if (in_quote)
      raw_elements[raw_element_index].push_back(file_contents[i]);

    else if (file_contents[i] == ',') {
      raw_elements.push_back("");
      raw_element_index += 1;
    }
  }

  std::string result = "! XPM2";

  for (auto& raw_element : raw_elements) {
    if (!raw_element.empty()) {
      if (raw_element.front() == '"')
        raw_element.erase(0, 1);

      if (!raw_element.empty() && raw_element.back() == '"')
        raw_element.pop_back();

      result += '\n' + raw_element;
    }
  }
  
   return result;
}

static bool string_to_int(std::string_view s, int& result, const int base = 10)
{
  auto [ptr, ec] { std::from_chars(s.data(), s.data() + s.size(), result, base)
    };

  if (ec == std::errc())
    if (ptr == s.data() + s.size())
      return true;

  return false;
}

static void string_to_lower(std::string& s) {
  std::transform(s.begin(), s.end(), s.begin(),
    [](unsigned char c) { return std::tolower(c); });
}

void Xpm::ParseXpm2(std::string file_contents) {
  std::replace(file_contents.begin(), file_contents.end(), '\r', '\n');

  std::vector<std::string> lines = split_string(file_contents, '\n');

  auto invalid_file_error = [](unsigned int line) {
    // printf("Runtime error thrown on line: %d\n", line);
    throw std::runtime_error("the specified XPM file is invalid");
  };

  if (lines.empty())
    invalid_file_error(__LINE__);

  const auto sections_start = strip_whitespace(lines[0],
    StripDirection::left)[0] == '!';

  if (lines.size() < 2)
    invalid_file_error(__LINE__);

  auto parse_values = [this, &invalid_file_error](std::string& values_line) {
    std::replace(values_line.begin(), values_line.end(), '\t', ' ');

    const std::vector<std::string> raw_values = split_string(values_line, ' ');

    if (raw_values.size() < 4)
      invalid_file_error(__LINE__);

    for (auto i = 0; i < 4; i++) {
      int result;

      if (!string_to_int(raw_values[i], result))
        invalid_file_error(__LINE__);

      if (result < 1)
        invalid_file_error(__LINE__);

      switch (i) {
      case 0:
        _width = result;

        break;

      case 1:
        _height = result;

        break;

      case 2:
        _colour_count = result;

        break;

      case 3:
        _chars_per_pixel = result;

        break;
      }
    }
  };

  parse_values(lines[sections_start]);

  if (_width % _chars_per_pixel != 0)
    invalid_file_error(__LINE__);

  if (lines.size() - 1 - sections_start != (unsigned int)_colour_count + _height)
    invalid_file_error(__LINE__);

  const auto colours_end = sections_start + 1 + _colour_count;

  auto parse_colours = [&sections_start, &colours_end, &lines, this,
    &invalid_file_error]()
  {
    for (auto i = sections_start + 1; i < colours_end; i++) {
      std::string_view line = lines[i];

      if ((int)line.size() < _chars_per_pixel + 1)
        invalid_file_error(__LINE__);

      const std::vector<std::string> colour_type_and_value = split_string(
        line.substr(_chars_per_pixel + 1), ' ');

      if (colour_type_and_value.size() < 2)
        invalid_file_error(__LINE__);

      std::string_view chars = line.substr(0, _chars_per_pixel);

      if (colour_type_and_value[0].size() != 1)
        invalid_file_error(__LINE__);

      const char& colour_type = colour_type_and_value[0][0];

      auto unsupported_colour_type_error = [](std::string_view colour_type) {
        throw std::runtime_error((std::string)colour_type +
          " colours are not yet supported");
      };

      if (colour_type == 's')
        unsupported_colour_type_error("symbolic");

      if (colour_type == 'm')
        unsupported_colour_type_error("monochrome");

      if (colour_type == 'g')
        unsupported_colour_type_error("greyscale");

      if (colour_type != 'c')
        invalid_file_error(__LINE__);

      std::string colour = colour_type_and_value[1];
      Rgba rgba = {};

      if (colour[0] == '#' && colour.size() == 7) {
        if (!string_to_int(colour.substr(1, 2), rgba.r, 16))
          invalid_file_error(__LINE__);

        if (!string_to_int(colour.substr(3, 2), rgba.g, 16))
          invalid_file_error(__LINE__);

        if (!string_to_int(colour.substr(5, 2), rgba.b, 16))
          invalid_file_error(__LINE__);

        rgba.a = 255;
      }

      else {
        for (std::vector<std::string>::size_type j = 2;
          j < colour_type_and_value.size(); j++)
        {
          colour += colour_type_and_value[j];
        }

        string_to_lower(colour);

        if (colour != "none")
          if (x11_colour_map.count(colour)) {
            Rgb rgb = x11_colour_map.find(colour)->second;
            rgba = { rgb.r, rgb.g, rgb.b, 255 };
          }

          else
            invalid_file_error(__LINE__);
      }

      _colour_map.insert(std::pair<std::string, Rgba>(chars, rgba));
    }
  };

  parse_colours();

  auto parse_pixels = [&colours_end, &lines, this, &invalid_file_error]() {
    for (std::vector<std::string>::size_type r = colours_end; r < lines.size();
      r++)
    {
      std::string_view row = lines[r];

      if (row.size() / _chars_per_pixel != (unsigned int)_width)
        invalid_file_error(__LINE__);

      _pixels.push_back({});

      for (std::string_view::size_type c = 0; c < row.size();
        c += _chars_per_pixel)
      {
        _pixels[r - colours_end].push_back((std::string)row.substr(c,
          _chars_per_pixel));
      }
    }
  };

  parse_pixels();
}

void Xpm::ParseXpm3(std::string file_contents) {
  ParseXpm2(Xpm3ToXpm2(file_contents));
}