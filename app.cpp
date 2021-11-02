#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WIN32_LEAN_AND_MEAN

#include <SDKDDKVer.h>
#include <Windows.h>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <commctrl.h>
#include <vector>
#include <commdlg.h>
#include <atlbase.h>
#include "resource.h"
#include <sstream>
#include "xpm.h"

#define IDC_STATUS_BAR 1

const std::wstring app_name = L"XPM Viewer";
const DWORD bg_colour = RGB(211, 211, 211);

void enable_console() {
  AllocConsole();
  (void)freopen("CONOUT$", "w", stdout);
  (void)freopen("CONIN$", "r", stdin);
}

std::string read_ansi_file(const std::wstring& file_path, DWORD& file_size) {
  const HANDLE file = CreateFileW(file_path.c_str(), GENERIC_READ,
    FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);

  auto invalid_file_error = []() {
    throw std::runtime_error("Could not read file");
  };

  if (file == INVALID_HANDLE_VALUE)
    invalid_file_error();

  file_size = GetFileSize(file, nullptr);

  if (file_size == INVALID_FILE_SIZE)
    invalid_file_error();

  LPSTR buff = new char[file_size];

  if (!ReadFile(file, buff, file_size, nullptr, nullptr))
    invalid_file_error();

  const std::string result = std::string(buff, file_size);

  delete[] buff;
  CloseHandle(file);

  return result;
}

void display_error(std::wstring_view what, std::wstring_view msg,
  const HWND wnd = GetActiveWindow())
{
  const std::wstring text = (std::wstring)what + L" Error: " +
    (std::wstring)msg + L".";

  MessageBoxW(wnd, text.c_str(), app_name.c_str(), MB_OK | MB_ICONERROR);
}

void win32_error(std::wstring_view func_name) {
  wchar_t buff[256] = L"";

  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buff,
    (sizeof(buff) / sizeof(wchar_t)), nullptr);

  std::wstring msg = (std::wstring)func_name + L" failed (" + buff;

  msg.pop_back();
  msg.pop_back();

  msg.back() = ')';

  display_error(L"Win32", msg);
  PostQuitMessage(EXIT_FAILURE);
}

int rect_width(const RECT rect) {
  return rect.right - rect.left;
}

int rect_height(const RECT rect) {
  return rect.bottom - rect.top;
}

void centre_wnd(const HWND wnd) {
  const int screen_width = GetSystemMetrics(SM_CXSCREEN);
  const int screen_height = GetSystemMetrics(SM_CYSCREEN);
  RECT client_rect = {};

  GetClientRect(wnd, &client_rect);
  AdjustWindowRect(&client_rect, GetWindowLongW(wnd, GWL_STYLE), FALSE);

  int client_width = rect_width(client_rect);
  int client_height = rect_height(client_rect);

  SetWindowPos(wnd, nullptr, (screen_width / 2) - (client_width / 2),
    (screen_height / 2) - (client_height / 2), 0, 0, SWP_NOSIZE);
}

HWND create_status_bar(const HWND parent, DWORD status_id, HINSTANCE instance)
{
  InitCommonControls();

  return CreateWindowExW(
    0, // no extended styles
    STATUSCLASSNAME, // name of status bar class
    nullptr, // no text when first created
    SBARS_SIZEGRIP | // includes a sizing grip
    WS_CHILD | WS_VISIBLE, // creates a visible child window
    0, 0, 0, 0, // ignores size and position
    parent, // handle to parent window
    (HMENU)status_id, // child window identifier
    instance, // handle to application instance
    nullptr // no window creation data
  );
}

HWND set_status_bar(const HWND status,
  const std::vector<std::wstring>& strings)
{
  const std::vector<std::wstring_view>::size_type& strings_size = 
    strings.size();

  int* parts = new int[strings_size];
  int right_edge = 0;
  const HDC dc = GetDC(nullptr);
  SIZE string_dimensions = {};

  for (std::vector<std::wstring_view>::size_type i = 0; i < strings_size; i++)
  {
    GetTextExtentPoint32W(dc, strings[i].c_str(), strings[i].size(),
      &string_dimensions);

    right_edge += string_dimensions.cx;
    parts[i] = right_edge;
  }

  SendMessageW(status, SB_SETPARTS, strings.size(), (LPARAM)parts);

  for (std::vector<std::wstring>::size_type i = 0; i < strings.size(); i++)
    SendMessageW(status, SB_SETTEXT, i, (LPARAM)strings[i].c_str());

  delete[] parts;

  return status;
}

std::wstring open_file_dialog(const HWND wnd, LPCWSTR filter) {
  wchar_t buff[MAX_PATH] = L"";
  OPENFILENAME ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = wnd;
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = buff;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = L"txt";

  if (GetOpenFileNameW(&ofn))
    return buff;

  return L"";
}

WNDCLASSEX create_wc(const HINSTANCE instance, const WNDPROC wnd_proc,
  LPCWSTR class_name = L"Window Class")
{
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = instance;
  wc.lpszClassName = class_name;

  return wc;
}

void set_wc_icon(WNDCLASSEX& wc, const DWORD icon_id) {
  wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(icon_id));

  wc.hIconSm = (HICON)LoadImageW(wc.hInstance, MAKEINTRESOURCEW(icon_id),
    IMAGE_ICON, 16, 16, 0);
}

void set_wc_background_colour(WNDCLASSEX& wc, COLORREF colour) {
  wc.hbrBackground = CreateSolidBrush(colour);
}

void set_wc_menu(WNDCLASSEX& wc, const DWORD menu_id) {
  wc.lpszMenuName = MAKEINTRESOURCEW(menu_id);
}

HWND create_wnd(const WNDCLASSEX& wc, const DWORD wnd_style, const int width,
  const int height)
{
  if (!RegisterClassExW(&wc))
    win32_error(L"RegisterClassEx");

  HWND wnd = CreateWindowExW(
    0, // optional window styles
    wc.lpszClassName, // window class
    app_name.c_str(), // window text
    wnd_style, // window style
    CW_USEDEFAULT, CW_USEDEFAULT, width, height, // size and position
    nullptr, // parent window    
    nullptr, // menu
    wc.hInstance, // instance handle
    nullptr // additional application data
  );

  if (!wnd)
    win32_error(L"CreateWindowEx");

  return wnd;
}

void event_loop(const HWND wnd, const int cmd_show) {
  ShowWindow(wnd, cmd_show);
  UpdateWindow(wnd);

  MSG msg = {};

  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
}

HBITMAP xpm_to_bitmap(const Xpm& xpm) {
  HDC screen_dc = GetDC(nullptr);
  HDC memory_dc = CreateCompatibleDC(screen_dc);
  HBITMAP bmp = CreateCompatibleBitmap(screen_dc, xpm.width(), xpm.height());
  HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(memory_dc, bmp));

  for (std::vector<std::vector<std::string>>::size_type r = 0;
    r < xpm.pixels().size(); r++)
  {
    for (std::vector<std::string>::size_type c = 0; c < xpm.pixels()[r].size();
      c++)
    {
      Rgba rgba = xpm.colour_map().at(xpm.pixels()[r][c]);
      COLORREF color_ref = 0;

      if (rgba.a == 0)
        color_ref = bg_colour;

      else
        color_ref = RGB(rgba.r, rgba.g, rgba.b);

      SetPixel(memory_dc, c, r, color_ref);
    }
  }

  SelectObject(memory_dc, old_bmp);
  DeleteDC(memory_dc);
  ReleaseDC(nullptr, screen_dc);

  return bmp;
}

void blit_bmp(HDC dest_dc, HBITMAP bmp, int x, int y, int width, int height,
  int scale = 1)
{
  HDC screen_dc = GetDC(nullptr);
  HDC memory_dc = CreateCompatibleDC(screen_dc);
  HBITMAP old_bmp = static_cast<HBITMAP>(SelectObject(memory_dc, bmp));

  StretchBlt(dest_dc, x, y, width * scale, height * scale, memory_dc, 0, 0,
    width, height, SRCCOPY);

  SelectObject(memory_dc, old_bmp);
  DeleteDC(memory_dc);
  ReleaseDC(nullptr, screen_dc);
}

std::wstring save_file_dialog(const HWND wnd, LPCWSTR filter)
{
  wchar_t buff[MAX_PATH] = L"";
  OPENFILENAME ofn = {};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = wnd;
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = buff;
  ofn.nMaxFile = MAX_PATH;

  ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
    OFN_OVERWRITEPROMPT;

  ofn.lpstrDefExt = L"txt";

  if (GetSaveFileNameW(&ofn))
    return buff;

  return L"";
}

void write_ansi_file(const std::wstring& file_path,
  const std::string& file_contents)
{
  HANDLE file = CreateFile(file_path.c_str(), GENERIC_WRITE, 0, nullptr,
    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  auto invalid_file_error = []() {
    throw std::runtime_error("Could not write to file");
  };

  if (file == INVALID_HANDLE_VALUE)
    invalid_file_error();

  DWORD file_size = GetFileSize(file, nullptr);

  if (file_size == INVALID_FILE_SIZE) {
    CloseHandle(file);
    invalid_file_error();
  }

  if (!WriteFile(file, file_contents.c_str(), file_contents.length(), nullptr,
    nullptr))
  {
    CloseHandle(file);
    invalid_file_error();
  }

  CloseHandle(file);
}

LRESULT CALLBACK wnd_proc(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  static std::string file_contents = "";
  static Xpm xpm;
  static HBITMAP xpm_hbm = nullptr;
  static BITMAP xpm_bm = {};
  static int scale = 1;

  HINSTANCE instance = GetModuleHandleW(nullptr);

  auto draw_xpm_hbm = [wnd](const HBITMAP xpm_hbm, const BITMAP& xpm_bm) {
    InvalidateRect(wnd, nullptr, true);

    PAINTSTRUCT ps;
    RECT r;

    GetClientRect(wnd, &r);

    if (r.bottom == 0)
      return;

    HDC hdc = BeginPaint(wnd, &ps);
    RECT client_rect = {};

    GetClientRect(wnd, &client_rect);

    int blit_bpm_x = (rect_width(client_rect) / 2) -
      ((xpm_bm.bmWidth * scale) / 2);

    int blit_bpm_y = (rect_height(client_rect) / 2) -
      ((xpm_bm.bmHeight * scale) / 2);

    blit_bmp(hdc, xpm_hbm, blit_bpm_x, blit_bpm_y, xpm_bm.bmWidth,
      xpm_bm.bmHeight, scale);

    EndPaint(wnd, &ps);
  };

  auto update_status_bar_zoom = [&wnd]() {
    SendMessageW(GetDlgItem(wnd, IDC_STATUS_BAR), SB_SETTEXT, 3,
      (LPARAM)(std::to_wstring(scale * 100) + L'%').c_str());
  };

  auto zoom_in = [&draw_xpm_hbm, &update_status_bar_zoom, &wnd]() {
    const int max = 25;

    if (scale < max) {
      scale += 1;

      draw_xpm_hbm(xpm_hbm, xpm_bm);
      update_status_bar_zoom();
      
      EnableMenuItem(GetMenu(wnd), ID_ZOOM_OUT, MF_ENABLED);

      if (scale == max)
        EnableMenuItem(GetMenu(wnd), ID_ZOOM_IN, MF_DISABLED);
    }
  };

  auto zoom_out = [&draw_xpm_hbm, &update_status_bar_zoom, &wnd]() {
    if (scale > 1) {
      scale -= 1;

      draw_xpm_hbm(xpm_hbm, xpm_bm);
      update_status_bar_zoom();

      EnableMenuItem(GetMenu(wnd), ID_ZOOM_IN, MF_ENABLED);

      if (scale == 1)
        EnableMenuItem(GetMenu(wnd), ID_ZOOM_OUT, MF_DISABLED);
    }
  };

  auto export_as = [&wnd](unsigned int msg) {
    bool xpm2 = msg == ID_EXPORTAS_XPM2;
    LPCWSTR xpm2_filter = L"XPM2 Files (*.xpm2)\0*.xpm2\0";
    LPCWSTR xpm3_filter = L"XPM3 Files (*.xpm3)\0*.xpm3\0";

    const std::wstring file_path = save_file_dialog(wnd, xpm2 ? xpm2_filter :
      xpm3_filter);

    if (!file_path.empty()) {
      try {
        if (xpm2)
          write_ansi_file(file_path, xpm.Xpm3ToXpm2(file_contents));

        else {
          std::wstring file_name = file_path.substr(
            file_path.find_last_of(L"/\\") + 1);

          write_ansi_file(file_path, xpm.Xpm2ToXpm3(file_name, file_contents));
        }
      }

      catch (const std::exception& ex) {
        display_error(L"File", ATL::CA2W(ex.what()).m_psz);
      }
    }
  };

  switch (msg)
  {
  case WM_CREATE:
    centre_wnd(wnd);
    create_status_bar(wnd, IDC_STATUS_BAR, instance);

    break;

  case WM_COMMAND:
    switch (LOWORD(w_param))
    {
    case ID_FILE_OPEN: {
        const std::wstring file_path = open_file_dialog(wnd,
          L"XPM Files (*.xpm2;*.xpm3)\0*.xpm2;*.xpm3\0");

        if (file_path.empty())
          break;

        DWORD file_size = 0;

        try {
          file_contents = read_ansi_file(file_path, file_size);
        }

        catch (const std::exception& ex) {
          display_error(L"File", ATL::CA2W(ex.what()).m_psz);

          break;
        }

        xpm = Xpm();
        
        bool xpm2 = false;

        try {
          if (file_path.back() == L'2') {
            xpm2 = true;

            xpm.ParseXpm2(file_contents);
          }

          else // 3
            xpm.ParseXpm3(file_contents);
        }

        catch (const std::exception& ex) {
          display_error(xpm2 ? L"XPM2" : L"XPM3", ATL::CA2W(ex.what()).m_psz);

          break;
        }

        const std::wstring file_name = file_path.substr(
          file_path.find_last_of(L"/\\") + 1);

        std::wstringstream file_size_ss;
        file_size_ss << file_size;

        std::vector<std::wstring> status_bar_strings = {
          file_name,
          file_size_ss.str() + L" bytes",

          std::to_wstring(xpm.width()) + L" x " +
            std::to_wstring(xpm.height()),

          L"100% "
        };

        set_status_bar(GetDlgItem(wnd, IDC_STATUS_BAR), status_bar_strings);

        xpm_hbm = xpm_to_bitmap(xpm);

        GetObject(xpm_hbm, sizeof(BITMAP), &xpm_bm);

        scale = 1;

        draw_xpm_hbm(xpm_hbm, xpm_bm);

        EnableMenuItem(GetMenu(wnd), ID_ZOOM_IN, MF_ENABLED);
        EnableMenuItem(GetMenu(wnd), ID_ZOOM_OUT, MF_DISABLED);

        if (xpm2) {
          EnableMenuItem(GetMenu(wnd), ID_EXPORTAS_XPM3, MF_ENABLED);
          EnableMenuItem(GetMenu(wnd), ID_EXPORTAS_XPM2, MF_DISABLED);
        }

        else {
          EnableMenuItem(GetMenu(wnd), ID_EXPORTAS_XPM2, MF_ENABLED);
          EnableMenuItem(GetMenu(wnd), ID_EXPORTAS_XPM3, MF_DISABLED);
        }
      }

      break;

    case ID_EXPORTAS_XPM2:
      export_as(msg);

      break;

    case ID_EXPORTAS_XPM3:
      export_as(msg);

      break;

    case ID_ZOOM_IN:
      zoom_in();

      break;

    case ID_ZOOM_OUT:
      zoom_out();

      break;
    }

    break;

  case WM_KEYDOWN:
    if (w_param == VK_ADD || w_param == VK_UP)
      zoom_in();

    else if (w_param == VK_SUBTRACT || w_param == VK_DOWN)
      zoom_out();

    break;

  case WM_SIZE:
    SendMessageW(GetDlgItem(wnd, IDC_STATUS_BAR), WM_SIZE, 0, 0);
    draw_xpm_hbm(xpm_hbm, xpm_bm);

    break;

  case WM_CLOSE:
    DestroyWindow(wnd);

    break;

  case WM_DESTROY:
    PostQuitMessage(0);

    break;

  default:
    return DefWindowProcW(wnd, msg, w_param, l_param);
  }

  return false;
}

int APIENTRY wWinMain(_In_ HINSTANCE instance,
  _In_opt_ HINSTANCE prev_instance, _In_ LPWSTR cmd_line, _In_ int cmd_show)
{
  // enable_console();

  WNDCLASSEX wc = create_wc(instance, wnd_proc);

  set_wc_background_colour(wc, bg_colour);
  set_wc_menu(wc, IDR_MENU);

  const HWND wnd = create_wnd(wc, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 800,
    600);

  event_loop(wnd, cmd_show);

  return 0;
}