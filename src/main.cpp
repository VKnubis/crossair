#define NOMINMAX

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <wininet.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr COLORREF kTransparentColor = RGB(0, 0, 0);
constexpr COLORREF kOutlineColor = RGB(24, 24, 24);
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kUpdateCheckCompleteMessage = WM_APP + 2;

constexpr const wchar_t* kAppVersion = L"v1.0.0";
constexpr const wchar_t* kReleasesUrl = L"https://github.com/VKnubis/crossair/releases";
constexpr const wchar_t* kLatestReleaseApiUrl = L"https://api.github.com/repos/VKnubis/crossair/releases/latest";
constexpr const wchar_t* kTagsApiUrl = L"https://api.github.com/repos/VKnubis/crossair/tags";

constexpr int kMinLength = 4;
constexpr int kMaxLength = 80;
constexpr int kMinGap = 0;
constexpr int kMaxGap = 40;
constexpr int kMinThickness = 1;
constexpr int kMaxThickness = 10;
constexpr int kMinOpacity = 30;
constexpr int kMaxOpacity = 100;
constexpr int kMinOffset = -200;
constexpr int kMaxOffset = 200;

constexpr int kSettingsClientWidth = 620;
constexpr int kSettingsClientHeight = 680;

constexpr COLORREF kSettingsBackground = RGB(18, 20, 24);
constexpr COLORREF kSettingsPanel = RGB(28, 31, 37);
constexpr COLORREF kSettingsPanelLight = RGB(38, 42, 50);
constexpr COLORREF kSettingsText = RGB(238, 242, 247);
constexpr COLORREF kSettingsMutedText = RGB(158, 169, 182);

enum class HotkeyId : int {
    Settings = 1,
};

enum class MenuId : int {
    Settings = 100,
    CheckUpdates,
    Toggle,
    Longer,
    Shorter,
    CycleColor,
    ToggleDot,
    Help,
    Exit,
};

enum class ControlId : int {
    Preview = 200,
    PresetCombo,
    LengthTrack,
    LengthEdit,
    GapTrack,
    GapEdit,
    ThicknessTrack,
    ThicknessEdit,
    OpacityTrack,
    OpacityEdit,
    OffsetXTrack,
    OffsetXEdit,
    OffsetYTrack,
    OffsetYEdit,
    RedEdit,
    GreenEdit,
    BlueEdit,
    ColorSwatch,
    DotCheck,
    OutlineCheck,
    VisibleCheck,
    ResetButton,
    CheckUpdateButton,
    CloseButton,
};

struct CrosshairSettings {
    int length = 18;
    int gap = 5;
    int thickness = 2;
    int opacity = 100;
    int offsetX = 0;
    int offsetY = 0;
    int red = 0;
    int green = 255;
    int blue = 128;
    bool showDot = true;
    bool outline = true;
    bool visible = true;
};

struct CrosshairPreset {
    const wchar_t* name;
    CrosshairSettings settings;
};

enum class UpdateStatus {
    UpToDate,
    UpdateAvailable,
    NoPublishedVersion,
    NetworkError,
};

struct UpdateInfo {
    UpdateStatus status = UpdateStatus::NetworkError;
    std::wstring currentVersion = kAppVersion;
    std::wstring latestVersion;
    std::wstring details;
    std::wstring updateUrl = kReleasesUrl;
    bool manual = false;
};

struct UpdateThreadArgs {
    HWND hwnd = nullptr;
    bool manual = false;
};

struct SettingsControls {
    HWND preview = nullptr;
    HWND presetCombo = nullptr;
    HWND lengthTrack = nullptr;
    HWND lengthEdit = nullptr;
    HWND gapTrack = nullptr;
    HWND gapEdit = nullptr;
    HWND thicknessTrack = nullptr;
    HWND thicknessEdit = nullptr;
    HWND opacityTrack = nullptr;
    HWND opacityEdit = nullptr;
    HWND offsetXTrack = nullptr;
    HWND offsetXEdit = nullptr;
    HWND offsetYTrack = nullptr;
    HWND offsetYEdit = nullptr;
    HWND redEdit = nullptr;
    HWND greenEdit = nullptr;
    HWND blueEdit = nullptr;
    HWND colorSwatch = nullptr;
    HWND dotCheck = nullptr;
    HWND outlineCheck = nullptr;
    HWND visibleCheck = nullptr;
};

HINSTANCE g_instance = nullptr;
HWND g_window = nullptr;
HWND g_settingsWindow = nullptr;
HWND g_previewWindow = nullptr;
SettingsControls g_controls;
CrosshairSettings g_settings;
bool g_updatingControls = false;
bool g_updateCheckRunning = false;

HBRUSH g_settingsBackgroundBrush = CreateSolidBrush(kSettingsBackground);
HBRUSH g_settingsPanelBrush = CreateSolidBrush(kSettingsPanel);
HBRUSH g_settingsEditBrush = CreateSolidBrush(kSettingsPanelLight);

const std::array<CrosshairPreset, 9> kPresets = {
    CrosshairPreset{L"Classic Green", CrosshairSettings{18, 5, 2, 100, 0, 0, 0, 255, 128, true, true, true}},
    CrosshairPreset{L"Clean White", CrosshairSettings{16, 4, 2, 92, 0, 0, 255, 255, 255, false, true, true}},
    CrosshairPreset{L"Tiny Dot", CrosshairSettings{4, 0, 3, 100, 0, 0, 255, 255, 255, true, true, true}},
    CrosshairPreset{L"Wide Cyan", CrosshairSettings{28, 8, 2, 96, 0, 0, 45, 220, 255, true, true, true}},
    CrosshairPreset{L"Thick Red", CrosshairSettings{22, 5, 4, 100, 0, 0, 255, 58, 58, false, true, true}},
    CrosshairPreset{L"High Vis Yellow", CrosshairSettings{24, 7, 3, 100, 0, 0, 255, 230, 64, true, true, true}},
    CrosshairPreset{L"Pink Dotline", CrosshairSettings{13, 3, 2, 90, 0, 0, 255, 88, 210, true, false, true}},
    CrosshairPreset{L"Blue Offset", CrosshairSettings{18, 6, 2, 92, 0, -18, 90, 150, 255, true, true, true}},
    CrosshairPreset{L"Orange Sniper", CrosshairSettings{34, 12, 2, 88, 0, 0, 255, 150, 48, false, true, true}},
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PreviewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HFONT UiFont() {
    return reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
}

void SetUiFont(HWND hwnd) {
    SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(UiFont()), TRUE);
}

COLORREF CurrentColor() {
    return RGB(g_settings.red, g_settings.green, g_settings.blue);
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }

    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
    return wide;
}

std::string ExtractJsonString(const std::string& json, const std::string& key) {
    const std::string token = "\"" + key + "\"";
    size_t position = json.find(token);
    if (position == std::string::npos) {
        return {};
    }

    position = json.find(':', position + token.size());
    if (position == std::string::npos) {
        return {};
    }

    position = json.find('"', position + 1);
    if (position == std::string::npos) {
        return {};
    }

    std::string value;
    bool escaped = false;
    for (++position; position < json.size(); ++position) {
        const char ch = json[position];
        if (escaped) {
            value.push_back(ch);
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '"') {
            break;
        } else {
            value.push_back(ch);
        }
    }

    return value;
}

std::vector<int> VersionParts(const std::wstring& version) {
    std::vector<int> parts;
    int value = 0;
    bool readingNumber = false;

    for (wchar_t ch : version) {
        if (ch >= L'0' && ch <= L'9') {
            value = (value * 10) + (ch - L'0');
            readingNumber = true;
        } else if (readingNumber) {
            parts.push_back(value);
            value = 0;
            readingNumber = false;
        }
    }

    if (readingNumber) {
        parts.push_back(value);
    }

    return parts;
}

bool IsRemoteVersionNewer(const std::wstring& currentVersion, const std::wstring& latestVersion) {
    const std::vector<int> current = VersionParts(currentVersion);
    const std::vector<int> latest = VersionParts(latestVersion);

    if (!current.empty() && !latest.empty()) {
        const size_t maxCount = std::max(current.size(), latest.size());
        for (size_t index = 0; index < maxCount; ++index) {
            const int currentPart = index < current.size() ? current[index] : 0;
            const int latestPart = index < latest.size() ? latest[index] : 0;
            if (latestPart != currentPart) {
                return latestPart > currentPart;
            }
        }

        return false;
    }

    return !latestVersion.empty() && latestVersion != currentVersion;
}

std::string HttpGet(const wchar_t* url, DWORD& statusCode, std::wstring& error) {
    statusCode = 0;
    error.clear();

    HINTERNET session = InternetOpenW(
        L"CppCrosshairUpdater/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr,
        nullptr,
        0);
    if (!session) {
        error = L"Could not initialize the internet session.";
        return {};
    }

    DWORD timeout = 7000;
    InternetSetOptionW(session, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(session, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    const wchar_t* headers =
        L"Accept: application/vnd.github+json\r\n"
        L"X-GitHub-Api-Version: 2022-11-28\r\n";
    HINTERNET request = InternetOpenUrlW(
        session,
        url,
        headers,
        static_cast<DWORD>(-1),
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
        0);
    if (!request) {
        InternetCloseHandle(session);
        error = L"Could not reach GitHub.";
        return {};
    }

    DWORD statusSize = sizeof(statusCode);
    HttpQueryInfoW(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, nullptr);

    std::string body;
    char buffer[4096]{};
    DWORD bytesRead = 0;
    while (InternetReadFile(request, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        body.append(buffer, buffer + bytesRead);
        bytesRead = 0;
    }

    InternetCloseHandle(request);
    InternetCloseHandle(session);
    return body;
}

UpdateInfo BuildUpdateInfo(
    const std::wstring& latestVersion,
    const std::wstring& updateUrl,
    const std::wstring& source,
    bool manual) {
    UpdateInfo info;
    info.manual = manual;
    info.latestVersion = latestVersion;
    info.updateUrl = updateUrl.empty() ? kReleasesUrl : updateUrl;
    info.details = source;
    info.status = IsRemoteVersionNewer(kAppVersion, latestVersion)
        ? UpdateStatus::UpdateAvailable
        : UpdateStatus::UpToDate;
    return info;
}

UpdateInfo CheckForUpdates(bool manual) {
    DWORD statusCode = 0;
    std::wstring error;
    std::string body = HttpGet(kLatestReleaseApiUrl, statusCode, error);

    if (statusCode == 200) {
        const std::wstring tag = Utf8ToWide(ExtractJsonString(body, "tag_name"));
        const std::wstring url = Utf8ToWide(ExtractJsonString(body, "html_url"));
        if (!tag.empty()) {
            return BuildUpdateInfo(tag, url, L"GitHub latest release", manual);
        }
    } else if (statusCode != 404 && statusCode != 0) {
        UpdateInfo info;
        info.manual = manual;
        info.status = UpdateStatus::NetworkError;
        info.details = L"GitHub returned HTTP " + std::to_wstring(statusCode) + L".";
        return info;
    } else if (statusCode == 0 && !error.empty()) {
        UpdateInfo info;
        info.manual = manual;
        info.status = UpdateStatus::NetworkError;
        info.details = error;
        return info;
    }

    body = HttpGet(kTagsApiUrl, statusCode, error);
    if (statusCode == 200) {
        const std::wstring tag = Utf8ToWide(ExtractJsonString(body, "name"));
        if (!tag.empty()) {
            return BuildUpdateInfo(tag, kReleasesUrl, L"GitHub tag fallback", manual);
        }

        UpdateInfo info;
        info.manual = manual;
        info.status = UpdateStatus::NoPublishedVersion;
        info.details = L"No GitHub release or tag was found.";
        return info;
    }

    UpdateInfo info;
    info.manual = manual;
    info.status = statusCode == 404 ? UpdateStatus::NoPublishedVersion : UpdateStatus::NetworkError;
    info.details = statusCode == 0 && !error.empty()
        ? error
        : L"GitHub returned HTTP " + std::to_wstring(statusCode) + L".";
    return info;
}

HWND MessageOwner() {
    return g_settingsWindow ? g_settingsWindow : g_window;
}

void OpenUpdatePage(const std::wstring& url) {
    ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void ShowUpdateResult(const UpdateInfo& info) {
    if (!info.manual && info.status != UpdateStatus::UpdateAvailable) {
        return;
    }

    if (info.status == UpdateStatus::UpdateAvailable) {
        const std::wstring message =
            L"A newer Crosshair version is available.\n\n"
            L"Current: " + info.currentVersion + L"\n"
            L"Latest: " + info.latestVersion + L"\n"
            L"Source: " + info.details + L"\n\n"
            L"Open the GitHub releases page now?";

        if (MessageBoxW(MessageOwner(), message.c_str(), L"Crosshair Update", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
            OpenUpdatePage(info.updateUrl);
        }
        return;
    }

    if (info.status == UpdateStatus::UpToDate) {
        const std::wstring message =
            L"You are already up to date.\n\n"
            L"Current: " + info.currentVersion + L"\n"
            L"Latest: " + info.latestVersion + L"\n"
            L"Source: " + info.details;
        MessageBoxW(MessageOwner(), message.c_str(), L"Crosshair Update", MB_OK | MB_ICONINFORMATION);
        return;
    }

    if (info.status == UpdateStatus::NoPublishedVersion) {
        const std::wstring message =
            L"No published version was found yet in VKnubis/crossair.\n\n"
            L"Publish a GitHub release or version tag like v1.0.1, then this checker can recommend it.";
        MessageBoxW(MessageOwner(), message.c_str(), L"Crosshair Update", MB_OK | MB_ICONINFORMATION);
        return;
    }

    const std::wstring message =
        L"Could not check for updates right now.\n\n" +
        (info.details.empty() ? L"Please try again later." : info.details);
    MessageBoxW(MessageOwner(), message.c_str(), L"Crosshair Update", MB_OK | MB_ICONWARNING);
}

DWORD WINAPI UpdateCheckThreadProc(LPVOID parameter) {
    std::unique_ptr<UpdateThreadArgs> args(static_cast<UpdateThreadArgs*>(parameter));
    UpdateInfo* result = new UpdateInfo(CheckForUpdates(args->manual));

    if (!PostMessageW(args->hwnd, kUpdateCheckCompleteMessage, reinterpret_cast<WPARAM>(result), 0)) {
        delete result;
    }

    return 0;
}

void StartUpdateCheck(HWND hwnd, bool manual) {
    if (g_updateCheckRunning) {
        if (manual) {
            MessageBoxW(MessageOwner(), L"An update check is already running.", L"Crosshair Update", MB_OK | MB_ICONINFORMATION);
        }
        return;
    }

    g_updateCheckRunning = true;
    auto* args = new UpdateThreadArgs{hwnd, manual};
    HANDLE thread = CreateThread(nullptr, 0, UpdateCheckThreadProc, args, 0, nullptr);
    if (!thread) {
        delete args;
        g_updateCheckRunning = false;
        if (manual) {
            MessageBoxW(MessageOwner(), L"Could not start the update checker.", L"Crosshair Update", MB_OK | MB_ICONWARNING);
        }
        return;
    }

    CloseHandle(thread);
}

std::wstring ParentDirectory(std::wstring path) {
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) {
        return L".";
    }

    return path.substr(0, slash);
}

std::wstring ExeDirectory() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    return ParentDirectory(path);
}

std::wstring BuildSettingsPath() {
    wchar_t appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, ARRAYSIZE(appData));

    if (length > 0 && length < ARRAYSIZE(appData)) {
        std::wstring directory = std::wstring(appData) + L"\\CppCrosshair";
        CreateDirectoryW(directory.c_str(), nullptr);
        return directory + L"\\settings.ini";
    }

    return ExeDirectory() + L"\\settings.ini";
}

const std::wstring& SettingsPath() {
    static const std::wstring path = BuildSettingsPath();
    return path;
}

int ReadSettingInt(const wchar_t* key, int defaultValue) {
    return static_cast<int>(GetPrivateProfileIntW(L"Crosshair", key, defaultValue, SettingsPath().c_str()));
}

void WriteSettingInt(const wchar_t* key, int value) {
    const std::wstring text = std::to_wstring(value);
    WritePrivateProfileStringW(L"Crosshair", key, text.c_str(), SettingsPath().c_str());
}

void LoadSettings() {
    const CrosshairSettings defaults;

    g_settings.length = std::clamp(ReadSettingInt(L"Length", defaults.length), kMinLength, kMaxLength);
    g_settings.gap = std::clamp(ReadSettingInt(L"Gap", defaults.gap), kMinGap, kMaxGap);
    g_settings.thickness = std::clamp(ReadSettingInt(L"Thickness", defaults.thickness), kMinThickness, kMaxThickness);
    g_settings.opacity = std::clamp(ReadSettingInt(L"Opacity", defaults.opacity), kMinOpacity, kMaxOpacity);
    g_settings.offsetX = std::clamp(ReadSettingInt(L"OffsetX", defaults.offsetX), kMinOffset, kMaxOffset);
    g_settings.offsetY = std::clamp(ReadSettingInt(L"OffsetY", defaults.offsetY), kMinOffset, kMaxOffset);
    g_settings.red = std::clamp(ReadSettingInt(L"Red", defaults.red), 0, 255);
    g_settings.green = std::clamp(ReadSettingInt(L"Green", defaults.green), 0, 255);
    g_settings.blue = std::clamp(ReadSettingInt(L"Blue", defaults.blue), 0, 255);
    g_settings.showDot = ReadSettingInt(L"ShowDot", defaults.showDot ? 1 : 0) != 0;
    g_settings.outline = ReadSettingInt(L"Outline", defaults.outline ? 1 : 0) != 0;
    g_settings.visible = true;
}

void SaveSettings() {
    WriteSettingInt(L"Length", g_settings.length);
    WriteSettingInt(L"Gap", g_settings.gap);
    WriteSettingInt(L"Thickness", g_settings.thickness);
    WriteSettingInt(L"Opacity", g_settings.opacity);
    WriteSettingInt(L"OffsetX", g_settings.offsetX);
    WriteSettingInt(L"OffsetY", g_settings.offsetY);
    WriteSettingInt(L"Red", g_settings.red);
    WriteSettingInt(L"Green", g_settings.green);
    WriteSettingInt(L"Blue", g_settings.blue);
    WriteSettingInt(L"ShowDot", g_settings.showDot ? 1 : 0);
    WriteSettingInt(L"Outline", g_settings.outline ? 1 : 0);
}

RECT VirtualScreenRect() {
    RECT rect{};
    rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rect.right = rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
    return rect;
}

POINT PrimaryMonitorCenter() {
    POINT origin{0, 0};
    HMONITOR monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{};
    info.cbSize = sizeof(info);

    if (GetMonitorInfoW(monitor, &info)) {
        return {
            info.rcMonitor.left + ((info.rcMonitor.right - info.rcMonitor.left) / 2),
            info.rcMonitor.top + ((info.rcMonitor.bottom - info.rcMonitor.top) / 2),
        };
    }

    RECT screen = VirtualScreenRect();
    return {
        screen.left + ((screen.right - screen.left) / 2),
        screen.top + ((screen.bottom - screen.top) / 2),
    };
}

void ApplyLayeredAttributes(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    const BYTE alpha = static_cast<BYTE>((std::clamp(g_settings.opacity, kMinOpacity, kMaxOpacity) * 255) / 100);
    SetLayeredWindowAttributes(hwnd, kTransparentColor, alpha, LWA_COLORKEY | LWA_ALPHA);
}

void RepositionOverlay(HWND hwnd) {
    RECT rect = VirtualScreenRect();
    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOACTIVATE | (g_settings.visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

void DrawLine(HDC hdc, int x1, int y1, int x2, int y2) {
    MoveToEx(hdc, x1, y1, nullptr);
    LineTo(hdc, x2, y2);
}

void DrawCrosshairShape(
    HDC hdc,
    int cx,
    int cy,
    int length,
    int gap,
    int thickness,
    COLORREF color,
    bool showDot) {
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    DrawLine(hdc, cx - gap - length, cy, cx - gap, cy);
    DrawLine(hdc, cx + gap, cy, cx + gap + length, cy);
    DrawLine(hdc, cx, cy - gap - length, cx, cy - gap);
    DrawLine(hdc, cx, cy + gap, cx, cy + gap + length);

    if (showDot) {
        const int radius = std::max(1, thickness / 2);
        HBRUSH brush = CreateSolidBrush(color);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        Ellipse(hdc, cx - radius, cy - radius, cx + radius + 1, cy + radius + 1);
        SelectObject(hdc, oldBrush);
        DeleteObject(brush);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawConfiguredCrosshair(HDC hdc, int cx, int cy, int length, int gap, int thickness) {
    if (g_settings.outline) {
        DrawCrosshairShape(hdc, cx, cy, length, gap, thickness + 2, kOutlineColor, g_settings.showDot);
    }

    DrawCrosshairShape(hdc, cx, cy, length, gap, thickness, CurrentColor(), g_settings.showDot);
}

void DrawCrosshair(HDC hdc, const RECT& clientRect) {
    HBRUSH background = CreateSolidBrush(kTransparentColor);
    FillRect(hdc, &clientRect, background);
    DeleteObject(background);

    RECT virtualRect = VirtualScreenRect();
    POINT center = PrimaryMonitorCenter();
    const int cx = center.x - virtualRect.left + g_settings.offsetX;
    const int cy = center.y - virtualRect.top + g_settings.offsetY;

    DrawConfiguredCrosshair(hdc, cx, cy, g_settings.length, g_settings.gap, g_settings.thickness);
}

void DrawPreviewPanel(HDC hdc, const RECT& rect) {
    HBRUSH background = CreateSolidBrush(RGB(31, 34, 39));
    FillRect(hdc, &rect, background);
    DeleteObject(background);

    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    const int maxRadius = std::max(1, g_settings.length + g_settings.gap + g_settings.thickness + 4);
    const double scale = std::min((width - 36) / static_cast<double>(maxRadius * 2),
                                  (height - 24) / static_cast<double>(maxRadius * 2));

    const int previewLength = std::max(3, static_cast<int>(g_settings.length * scale));
    const int previewGap = std::max(0, static_cast<int>(g_settings.gap * scale));
    const int previewThickness = std::max(1, static_cast<int>(g_settings.thickness * scale));
    const int cx = rect.left + (width / 2);
    const int cy = rect.top + (height / 2) + 6;

    DrawConfiguredCrosshair(hdc, cx, cy, previewLength, previewGap, previewThickness);

    RECT labelRect{rect.left + 12, rect.top + 10, rect.right - 12, rect.top + 30};
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(218, 224, 232));
    SelectObject(hdc, UiFont());
    DrawTextW(hdc, L"Live preview", -1, &labelRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
}

void DrawPreview(HWND hwnd, HDC hdc) {
    RECT rect{};
    GetClientRect(hwnd, &rect);
    DrawPreviewPanel(hdc, rect);
}

void RedrawOverlay(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);
}

void UpdateSettingsControls();

void NotifySettingsChanged(bool persist) {
    ApplyLayeredAttributes(g_window);
    RedrawOverlay(g_window);

    if (g_previewWindow) {
        InvalidateRect(g_previewWindow, nullptr, TRUE);
    }

    if (g_settingsWindow) {
        InvalidateRect(g_settingsWindow, nullptr, FALSE);
    }

    UpdateSettingsControls();

    if (persist) {
        SaveSettings();
    }
}

void SetOverlayVisible(HWND hwnd, bool visible) {
    g_settings.visible = visible;
    ShowWindow(hwnd, g_settings.visible ? SW_SHOWNA : SW_HIDE);
    UpdateSettingsControls();
}

void ToggleVisibility(HWND hwnd) {
    SetOverlayVisible(hwnd, !g_settings.visible);
}

void AdjustLength(HWND hwnd, int delta) {
    g_settings.length = std::clamp(g_settings.length + delta, kMinLength, kMaxLength);
    NotifySettingsChanged(true);
    RedrawOverlay(hwnd);
}

void ToggleDot(HWND hwnd) {
    g_settings.showDot = !g_settings.showDot;
    NotifySettingsChanged(true);
    RedrawOverlay(hwnd);
}

bool SettingsMatchPreset(const CrosshairSettings& left, const CrosshairSettings& right) {
    return left.length == right.length &&
           left.gap == right.gap &&
           left.thickness == right.thickness &&
           left.opacity == right.opacity &&
           left.offsetX == right.offsetX &&
           left.offsetY == right.offsetY &&
           left.red == right.red &&
           left.green == right.green &&
           left.blue == right.blue &&
           left.showDot == right.showDot &&
           left.outline == right.outline;
}

int MatchingPresetIndex() {
    for (size_t index = 0; index < kPresets.size(); ++index) {
        if (SettingsMatchPreset(g_settings, kPresets[index].settings)) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

void ApplyPreset(size_t index) {
    if (index >= kPresets.size()) {
        return;
    }

    const bool wasVisible = g_settings.visible;
    g_settings = kPresets[index].settings;
    g_settings.visible = wasVisible;
    NotifySettingsChanged(true);
}

void CycleColor(HWND hwnd) {
    const int currentPreset = MatchingPresetIndex();
    const size_t nextPreset = currentPreset < 0
        ? 0
        : (static_cast<size_t>(currentPreset) + 1) % kPresets.size();

    ApplyPreset(nextPreset);
    RedrawOverlay(hwnd);
}

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int width, int height) {
    HWND control = CreateWindowExW(
        0,
        L"STATIC",
        text,
        WS_CHILD | WS_VISIBLE,
        x,
        y,
        width,
        height,
        parent,
        nullptr,
        g_instance,
        nullptr);
    SetUiFont(control);
    return control;
}

HWND CreateEditBox(HWND parent, ControlId id, int x, int y, int width = 64) {
    HWND control = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_RIGHT,
        x,
        y,
        width,
        24,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SetUiFont(control);
    return control;
}

HWND CreateTrackbar(HWND parent, ControlId id, int x, int y, int minValue, int maxValue) {
    HWND control = CreateWindowExW(
        0,
        TRACKBAR_CLASSW,
        L"",
        WS_CHILD | WS_VISIBLE | TBS_NOTICKS,
        x,
        y,
        316,
        28,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SendMessageW(control, TBM_SETRANGEMIN, FALSE, minValue);
    SendMessageW(control, TBM_SETRANGEMAX, TRUE, maxValue);
    return control;
}

HWND CreateCheckbox(HWND parent, ControlId id, const wchar_t* text, int x, int y) {
    HWND control = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        x,
        y,
        180,
        24,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SetUiFont(control);
    return control;
}

HWND CreateButton(HWND parent, ControlId id, const wchar_t* text, int x, int y, int width) {
    HWND control = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x,
        y,
        width,
        30,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SetUiFont(control);
    return control;
}

void SetTrackValue(HWND track, int value) {
    SendMessageW(track, TBM_SETPOS, TRUE, value);
}

int GetTrackValue(HWND track) {
    return static_cast<int>(SendMessageW(track, TBM_GETPOS, 0, 0));
}

void SetEditInt(HWND edit, int value) {
    SetWindowTextW(edit, std::to_wstring(value).c_str());
}

bool TryReadEditInt(HWND edit, int& value) {
    wchar_t text[32]{};
    GetWindowTextW(edit, text, ARRAYSIZE(text));

    wchar_t* end = nullptr;
    const long parsed = wcstol(text, &end, 10);
    if (end == text || *end != L'\0') {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

void UpdateSettingsControls() {
    if (!g_settingsWindow) {
        return;
    }

    g_updatingControls = true;

    SetTrackValue(g_controls.lengthTrack, g_settings.length);
    SetTrackValue(g_controls.gapTrack, g_settings.gap);
    SetTrackValue(g_controls.thicknessTrack, g_settings.thickness);
    SetTrackValue(g_controls.opacityTrack, g_settings.opacity);
    SetTrackValue(g_controls.offsetXTrack, g_settings.offsetX);
    SetTrackValue(g_controls.offsetYTrack, g_settings.offsetY);

    SetEditInt(g_controls.lengthEdit, g_settings.length);
    SetEditInt(g_controls.gapEdit, g_settings.gap);
    SetEditInt(g_controls.thicknessEdit, g_settings.thickness);
    SetEditInt(g_controls.opacityEdit, g_settings.opacity);
    SetEditInt(g_controls.offsetXEdit, g_settings.offsetX);
    SetEditInt(g_controls.offsetYEdit, g_settings.offsetY);
    SetEditInt(g_controls.redEdit, g_settings.red);
    SetEditInt(g_controls.greenEdit, g_settings.green);
    SetEditInt(g_controls.blueEdit, g_settings.blue);

    const int matchingPreset = MatchingPresetIndex();
    SendMessageW(g_controls.presetCombo, CB_SETCURSEL, matchingPreset >= 0 ? static_cast<WPARAM>(matchingPreset) : static_cast<WPARAM>(-1), 0);
    SendMessageW(g_controls.dotCheck, BM_SETCHECK, g_settings.showDot ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.outlineCheck, BM_SETCHECK, g_settings.outline ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(g_controls.visibleCheck, BM_SETCHECK, g_settings.visible ? BST_CHECKED : BST_UNCHECKED, 0);

    g_updatingControls = false;

    if (g_previewWindow) {
        InvalidateRect(g_previewWindow, nullptr, TRUE);
    }

    if (g_controls.colorSwatch) {
        InvalidateRect(g_controls.colorSwatch, nullptr, TRUE);
    }
}

void CreateSettingsControls(HWND hwnd) {
    g_controls = {};

    CreateLabel(hwnd, L"Preset", 24, 236, 84, 22);
    g_controls.presetCombo = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
        112,
        232,
        260,
        220,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<int>(ControlId::PresetCombo)),
        g_instance,
        nullptr);
    SetUiFont(g_controls.presetCombo);
    SendMessageW(g_controls.presetCombo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 26);
    SendMessageW(g_controls.presetCombo, CB_SETITEMHEIGHT, 0, 26);

    for (const CrosshairPreset& preset : kPresets) {
        SendMessageW(g_controls.presetCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(preset.name));
    }

    constexpr int labelX = 24;
    constexpr int trackX = 122;
    constexpr int editX = 462;
    constexpr int rowHeight = 38;
    int y = 282;

    CreateLabel(hwnd, L"Length", labelX, y + 4, 90, 22);
    g_controls.lengthTrack = CreateTrackbar(hwnd, ControlId::LengthTrack, trackX, y, kMinLength, kMaxLength);
    g_controls.lengthEdit = CreateEditBox(hwnd, ControlId::LengthEdit, editX, y + 1);
    y += rowHeight;

    CreateLabel(hwnd, L"Gap", labelX, y + 4, 90, 22);
    g_controls.gapTrack = CreateTrackbar(hwnd, ControlId::GapTrack, trackX, y, kMinGap, kMaxGap);
    g_controls.gapEdit = CreateEditBox(hwnd, ControlId::GapEdit, editX, y + 1);
    y += rowHeight;

    CreateLabel(hwnd, L"Thickness", labelX, y + 4, 90, 22);
    g_controls.thicknessTrack = CreateTrackbar(hwnd, ControlId::ThicknessTrack, trackX, y, kMinThickness, kMaxThickness);
    g_controls.thicknessEdit = CreateEditBox(hwnd, ControlId::ThicknessEdit, editX, y + 1);
    y += rowHeight;

    CreateLabel(hwnd, L"Opacity", labelX, y + 4, 90, 22);
    g_controls.opacityTrack = CreateTrackbar(hwnd, ControlId::OpacityTrack, trackX, y, kMinOpacity, kMaxOpacity);
    g_controls.opacityEdit = CreateEditBox(hwnd, ControlId::OpacityEdit, editX, y + 1);
    y += rowHeight;

    CreateLabel(hwnd, L"Offset X", labelX, y + 4, 90, 22);
    g_controls.offsetXTrack = CreateTrackbar(hwnd, ControlId::OffsetXTrack, trackX, y, kMinOffset, kMaxOffset);
    g_controls.offsetXEdit = CreateEditBox(hwnd, ControlId::OffsetXEdit, editX, y + 1);
    y += rowHeight;

    CreateLabel(hwnd, L"Offset Y", labelX, y + 4, 90, 22);
    g_controls.offsetYTrack = CreateTrackbar(hwnd, ControlId::OffsetYTrack, trackX, y, kMinOffset, kMaxOffset);
    g_controls.offsetYEdit = CreateEditBox(hwnd, ControlId::OffsetYEdit, editX, y + 1);

    CreateLabel(hwnd, L"RGB", labelX, 524, 90, 22);
    CreateLabel(hwnd, L"R", 122, 524, 16, 22);
    g_controls.redEdit = CreateEditBox(hwnd, ControlId::RedEdit, 142, 520, 52);
    CreateLabel(hwnd, L"G", 208, 524, 16, 22);
    g_controls.greenEdit = CreateEditBox(hwnd, ControlId::GreenEdit, 228, 520, 52);
    CreateLabel(hwnd, L"B", 294, 524, 16, 22);
    g_controls.blueEdit = CreateEditBox(hwnd, ControlId::BlueEdit, 314, 520, 52);
    g_controls.colorSwatch = CreateWindowExW(
        0,
        L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        386,
        520,
        140,
        24,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<int>(ControlId::ColorSwatch)),
        g_instance,
        nullptr);

    g_controls.dotCheck = CreateCheckbox(hwnd, ControlId::DotCheck, L"Center dot", 24, 572);
    g_controls.outlineCheck = CreateCheckbox(hwnd, ControlId::OutlineCheck, L"Dark outline", 160, 572);
    g_controls.visibleCheck = CreateCheckbox(hwnd, ControlId::VisibleCheck, L"Show crosshair", 300, 572);

    CreateLabel(hwnd, L"Typed values save live. Use Ctrl+Alt+S to reopen this editor.", 24, 620, 260, 22);
    CreateButton(hwnd, ControlId::CheckUpdateButton, L"Updates", 296, 612, 96);
    CreateButton(hwnd, ControlId::ResetButton, L"Reset", 408, 612, 80);
    CreateButton(hwnd, ControlId::CloseButton, L"Close", 516, 612, 80);

    UpdateSettingsControls();
}

void ApplyTrackbarValue(HWND track) {
    if (track == g_controls.lengthTrack) {
        g_settings.length = GetTrackValue(track);
    } else if (track == g_controls.gapTrack) {
        g_settings.gap = GetTrackValue(track);
    } else if (track == g_controls.thicknessTrack) {
        g_settings.thickness = GetTrackValue(track);
    } else if (track == g_controls.opacityTrack) {
        g_settings.opacity = GetTrackValue(track);
    } else if (track == g_controls.offsetXTrack) {
        g_settings.offsetX = GetTrackValue(track);
    } else if (track == g_controls.offsetYTrack) {
        g_settings.offsetY = GetTrackValue(track);
    } else {
        return;
    }

    NotifySettingsChanged(true);
}

void ApplyEditValue(HWND edit) {
    if (g_updatingControls) {
        return;
    }

    int value = 0;
    if (!TryReadEditInt(edit, value)) {
        return;
    }

    if (edit == g_controls.lengthEdit) {
        g_settings.length = std::clamp(value, kMinLength, kMaxLength);
    } else if (edit == g_controls.gapEdit) {
        g_settings.gap = std::clamp(value, kMinGap, kMaxGap);
    } else if (edit == g_controls.thicknessEdit) {
        g_settings.thickness = std::clamp(value, kMinThickness, kMaxThickness);
    } else if (edit == g_controls.opacityEdit) {
        g_settings.opacity = std::clamp(value, kMinOpacity, kMaxOpacity);
    } else if (edit == g_controls.offsetXEdit) {
        g_settings.offsetX = std::clamp(value, kMinOffset, kMaxOffset);
    } else if (edit == g_controls.offsetYEdit) {
        g_settings.offsetY = std::clamp(value, kMinOffset, kMaxOffset);
    } else if (edit == g_controls.redEdit) {
        g_settings.red = std::clamp(value, 0, 255);
    } else if (edit == g_controls.greenEdit) {
        g_settings.green = std::clamp(value, 0, 255);
    } else if (edit == g_controls.blueEdit) {
        g_settings.blue = std::clamp(value, 0, 255);
    } else {
        return;
    }

    NotifySettingsChanged(true);
}

void ResetSettings() {
    const bool wasVisible = g_settings.visible;
    g_settings = CrosshairSettings{};
    g_settings.visible = wasVisible;
    NotifySettingsChanged(true);
}

void HandleSettingsCommand(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    const int id = LOWORD(wParam);
    const int notification = HIWORD(wParam);

    switch (static_cast<ControlId>(id)) {
    case ControlId::PresetCombo:
        if (notification == CBN_SELCHANGE) {
            const int selection = static_cast<int>(SendMessageW(g_controls.presetCombo, CB_GETCURSEL, 0, 0));
            if (selection >= 0 && selection < static_cast<int>(kPresets.size())) {
                ApplyPreset(static_cast<size_t>(selection));
            }
        }
        break;

    case ControlId::LengthEdit:
    case ControlId::GapEdit:
    case ControlId::ThicknessEdit:
    case ControlId::OpacityEdit:
    case ControlId::OffsetXEdit:
    case ControlId::OffsetYEdit:
    case ControlId::RedEdit:
    case ControlId::GreenEdit:
    case ControlId::BlueEdit:
        if (notification == EN_CHANGE) {
            ApplyEditValue(reinterpret_cast<HWND>(lParam));
        } else if (notification == EN_KILLFOCUS) {
            UpdateSettingsControls();
        }
        break;

    case ControlId::DotCheck:
        if (notification == BN_CLICKED) {
            g_settings.showDot = SendMessageW(g_controls.dotCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            NotifySettingsChanged(true);
        }
        break;

    case ControlId::OutlineCheck:
        if (notification == BN_CLICKED) {
            g_settings.outline = SendMessageW(g_controls.outlineCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
            NotifySettingsChanged(true);
        }
        break;

    case ControlId::VisibleCheck:
        if (notification == BN_CLICKED) {
            SetOverlayVisible(g_window, SendMessageW(g_controls.visibleCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
        }
        break;

    case ControlId::ResetButton:
        if (notification == BN_CLICKED) {
            ResetSettings();
        }
        break;

    case ControlId::CheckUpdateButton:
        if (notification == BN_CLICKED) {
            StartUpdateCheck(g_window, true);
        }
        break;

    case ControlId::CloseButton:
        if (notification == BN_CLICKED) {
            DestroyWindow(hwnd);
        }
        break;

    default:
        break;
    }
}

void DrawPresetComboItem(const DRAWITEMSTRUCT& item) {
    if (item.itemID == static_cast<UINT>(-1) || item.itemID >= kPresets.size()) {
        return;
    }

    const bool selected = (item.itemState & ODS_SELECTED) != 0;
    const COLORREF backgroundColor = selected ? RGB(0, 118, 90) : kSettingsPanelLight;
    const COLORREF textColor = kSettingsText;

    HBRUSH background = CreateSolidBrush(backgroundColor);
    FillRect(item.hDC, &item.rcItem, background);
    DeleteObject(background);

    RECT swatch{
        item.rcItem.left + 8,
        item.rcItem.top + 5,
        item.rcItem.left + 28,
        item.rcItem.bottom - 5,
    };
    const CrosshairSettings& preset = kPresets[item.itemID].settings;
    HBRUSH swatchBrush = CreateSolidBrush(RGB(preset.red, preset.green, preset.blue));
    FillRect(item.hDC, &swatch, swatchBrush);
    DeleteObject(swatchBrush);
    FrameRect(item.hDC, &swatch, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    RECT textRect = item.rcItem;
    textRect.left += 38;
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, textColor);
    SelectObject(item.hDC, UiFont());
    DrawTextW(item.hDC, kPresets[item.itemID].name, -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    if (item.itemState & ODS_FOCUS) {
        DrawFocusRect(item.hDC, &item.rcItem);
    }
}

void DrawColorSwatch(const DRAWITEMSTRUCT& item) {
    RECT rect = item.rcItem;
    HBRUSH frame = CreateSolidBrush(kSettingsPanelLight);
    FillRect(item.hDC, &rect, frame);
    DeleteObject(frame);

    InflateRect(&rect, -3, -3);
    HBRUSH color = CreateSolidBrush(CurrentColor());
    FillRect(item.hDC, &rect, color);
    DeleteObject(color);

    FrameRect(item.hDC, &rect, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
}

void DrawSettingsChrome(HWND hwnd, HDC hdc) {
    RECT client{};
    GetClientRect(hwnd, &client);
    FillRect(hdc, &client, g_settingsBackgroundBrush);

    RECT header{0, 0, client.right, 58};
    FillRect(hdc, &header, g_settingsPanelBrush);

    RECT accent{0, 0, 6, 58};
    HBRUSH accentBrush = CreateSolidBrush(CurrentColor());
    FillRect(hdc, &accent, accentBrush);
    DeleteObject(accentBrush);

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, UiFont());

    const std::wstring titleText = std::wstring(L"Crosshair Studio ") + kAppVersion;
    RECT title{24, 12, client.right - 24, 34};
    SetTextColor(hdc, kSettingsText);
    DrawTextW(hdc, titleText.c_str(), -1, &title, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    RECT subtitle{24, 34, client.right - 24, 54};
    SetTextColor(hdc, kSettingsMutedText);
    DrawTextW(hdc, L"Presets, exact values, full RGB", -1, &subtitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    RECT preview{24, 72, client.right - 24, 214};
    DrawPreviewPanel(hdc, preview);

    RECT shapePanel{16, 224, client.right - 16, 558};
    FillRect(hdc, &shapePanel, g_settingsPanelBrush);

    RECT optionsPanel{16, 562, client.right - 16, 648};
    FillRect(hdc, &optionsPanel, g_settingsPanelBrush);

    RECT shapeLabel{24, 258, 220, 278};
    SetTextColor(hdc, kSettingsMutedText);
    DrawTextW(hdc, L"Shape", -1, &shapeLabel, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    RECT colorLabel{24, 498, 220, 518};
    DrawTextW(hdc, L"Color", -1, &colorLabel, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
}

bool RegisterHotkeys(HWND hwnd) {
    const UINT modifiers = MOD_CONTROL | MOD_ALT | MOD_NOREPEAT;
    return RegisterHotKey(hwnd, static_cast<int>(HotkeyId::Settings), modifiers, 'S') != 0;
}

void UnregisterHotkeys(HWND hwnd) {
    UnregisterHotKey(hwnd, static_cast<int>(HotkeyId::Settings));
}

void AddTrayIcon(HWND hwnd) {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hwnd;
    data.uID = kTrayIconId;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = kTrayMessage;
    data.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(data.szTip, L"C++ Crosshair", ARRAYSIZE(data.szTip));

    Shell_NotifyIconW(NIM_ADD, &data);
    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);
}

void RemoveTrayIcon(HWND hwnd) {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hwnd;
    data.uID = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &data);
}

void ShowHelp(HWND hwnd) {
    const std::wstring text =
        L"Version: " + std::wstring(kAppVersion) + L"\n\n"
        L"Hotkeys:\n"
        L"Ctrl+Alt+S: Open settings\n\n"
        L"Use the settings window or tray menu for everything else.\n"
        L"The update checker looks at VKnubis/crossair on GitHub.";

    MessageBoxW(hwnd, text.c_str(), L"C++ Crosshair", MB_OK | MB_ICONINFORMATION);
}

bool RegisterWindowClasses() {
    static bool registered = false;
    if (registered) {
        return true;
    }

    WNDCLASSEXW settingsClass{};
    settingsClass.cbSize = sizeof(settingsClass);
    settingsClass.lpfnWndProc = SettingsProc;
    settingsClass.hInstance = g_instance;
    settingsClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    settingsClass.hbrBackground = g_settingsBackgroundBrush;
    settingsClass.lpszClassName = L"CppCrosshairSettingsWindow";

    WNDCLASSEXW previewClass{};
    previewClass.cbSize = sizeof(previewClass);
    previewClass.lpfnWndProc = PreviewProc;
    previewClass.hInstance = g_instance;
    previewClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    previewClass.hbrBackground = g_settingsPanelBrush;
    previewClass.lpszClassName = L"CppCrosshairPreviewWindow";

    if (!RegisterClassExW(&settingsClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    if (!RegisterClassExW(&previewClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    registered = true;
    return true;
}

void CenterWindowOnScreen(HWND hwnd) {
    RECT rect{};
    GetWindowRect(hwnd, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

    const int x = workArea.left + ((workArea.right - workArea.left - width) / 2);
    const int y = workArea.top + ((workArea.bottom - workArea.top - height) / 2);
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void ShowSettingsWindow() {
    if (g_settingsWindow) {
        ShowWindow(g_settingsWindow, SW_SHOW);
        SetWindowPos(g_settingsWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetForegroundWindow(g_settingsWindow);
        UpdateSettingsControls();
        return;
    }

    if (!RegisterWindowClasses()) {
        MessageBoxW(g_window, L"Could not create the settings window.", L"C++ Crosshair", MB_OK | MB_ICONERROR);
        return;
    }

    constexpr DWORD settingsStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT rect{0, 0, kSettingsClientWidth, kSettingsClientHeight};
    AdjustWindowRectEx(&rect, settingsStyle, FALSE, WS_EX_APPWINDOW | WS_EX_TOPMOST);

    g_settingsWindow = CreateWindowExW(
        WS_EX_APPWINDOW | WS_EX_TOPMOST,
        L"CppCrosshairSettingsWindow",
        L"Crosshair Settings",
        settingsStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        g_instance,
        nullptr);

    if (!g_settingsWindow) {
        MessageBoxW(g_window, L"Could not create the settings window.", L"C++ Crosshair", MB_OK | MB_ICONERROR);
        return;
    }

    CenterWindowOnScreen(g_settingsWindow);
    UpdateSettingsControls();
    ShowWindow(g_settingsWindow, SW_SHOW);
    SetWindowPos(g_settingsWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(g_settingsWindow);
    UpdateWindow(g_settingsWindow);
}

void ShowTrayMenu(HWND hwnd) {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }

    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuId::Settings), L"Customize...");
    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuId::CheckUpdates), L"Check for updates");
    AppendMenuW(
        menu,
        MF_STRING | (g_settings.visible ? MF_CHECKED : MF_UNCHECKED),
        static_cast<UINT_PTR>(MenuId::Toggle),
        L"Show crosshair");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuId::Longer), L"Bigger");
    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuId::Shorter), L"Smaller");
    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuId::CycleColor), L"Next preset");
    AppendMenuW(
        menu,
        MF_STRING | (g_settings.showDot ? MF_CHECKED : MF_UNCHECKED),
        static_cast<UINT_PTR>(MenuId::ToggleDot),
        L"Center dot");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuId::Help), L"Help");
    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(MenuId::Exit), L"Exit");
    SetMenuDefaultItem(menu, static_cast<UINT>(MenuId::Settings), FALSE);

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hwnd, nullptr);
    DestroyMenu(menu);
    PostMessageW(hwnd, WM_NULL, 0, 0);
}

void HandleCommand(HWND hwnd, WPARAM wParam) {
    switch (static_cast<MenuId>(LOWORD(wParam))) {
    case MenuId::Settings:
        ShowSettingsWindow();
        break;
    case MenuId::CheckUpdates:
        StartUpdateCheck(hwnd, true);
        break;
    case MenuId::Toggle:
        ToggleVisibility(hwnd);
        break;
    case MenuId::Longer:
        AdjustLength(hwnd, 2);
        break;
    case MenuId::Shorter:
        AdjustLength(hwnd, -2);
        break;
    case MenuId::CycleColor:
        CycleColor(hwnd);
        break;
    case MenuId::ToggleDot:
        ToggleDot(hwnd);
        break;
    case MenuId::Help:
        ShowHelp(hwnd);
        break;
    case MenuId::Exit:
        DestroyWindow(hwnd);
        break;
    }
}

void HandleHotkey(HWND, WPARAM wParam) {
    if (static_cast<HotkeyId>(wParam) == HotkeyId::Settings) {
        ShowSettingsWindow();
    }
}

LRESULT CALLBACK PreviewProc(HWND hwnd, UINT message, WPARAM, LPARAM) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC hdc = BeginPaint(hwnd, &paint);
        DrawPreview(hwnd, hdc);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;
    }

    return DefWindowProcW(hwnd, message, 0, 0);
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateSettingsControls(hwnd);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC hdc = BeginPaint(hwnd, &paint);
        DrawSettingsChrome(hwnd, hdc);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, kSettingsText);
        return reinterpret_cast<LRESULT>(g_settingsPanelBrush);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, kSettingsPanelLight);
        SetTextColor(hdc, kSettingsText);
        return reinterpret_cast<LRESULT>(g_settingsEditBrush);
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkColor(hdc, kSettingsPanelLight);
        SetTextColor(hdc, kSettingsText);
        return reinterpret_cast<LRESULT>(g_settingsEditBrush);
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_HSCROLL:
        ApplyTrackbarValue(reinterpret_cast<HWND>(lParam));
        return 0;

    case WM_COMMAND:
        HandleSettingsCommand(hwnd, wParam, lParam);
        return 0;

    case WM_DRAWITEM:
        if (wParam == static_cast<WPARAM>(ControlId::PresetCombo)) {
            DrawPresetComboItem(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        if (wParam == static_cast<WPARAM>(ControlId::ColorSwatch)) {
            DrawColorSwatch(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;

    case WM_MEASUREITEM:
        if (wParam == static_cast<WPARAM>(ControlId::PresetCombo)) {
            auto* item = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            item->itemHeight = 26;
            return TRUE;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (hwnd == g_settingsWindow) {
            g_settingsWindow = nullptr;
            g_previewWindow = nullptr;
            g_controls = {};
        }
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        ApplyLayeredAttributes(hwnd);
        AddTrayIcon(hwnd);
        RegisterHotkeys(hwnd);
        RepositionOverlay(hwnd);
        StartUpdateCheck(hwnd, false);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC hdc = BeginPaint(hwnd, &paint);
        RECT client{};
        GetClientRect(hwnd, &client);
        DrawCrosshair(hdc, client);
        EndPaint(hwnd, &paint);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_HOTKEY:
        HandleHotkey(hwnd, wParam);
        return 0;

    case WM_COMMAND:
        HandleCommand(hwnd, wParam);
        return 0;

    case kUpdateCheckCompleteMessage: {
        std::unique_ptr<UpdateInfo> info(reinterpret_cast<UpdateInfo*>(wParam));
        g_updateCheckRunning = false;
        if (info) {
            ShowUpdateResult(*info);
        }
        return 0;
    }

    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
        RepositionOverlay(hwnd);
        RedrawOverlay(hwnd);
        return 0;

    case kTrayMessage:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            ShowSettingsWindow();
        } else if (LOWORD(lParam) == WM_LBUTTONUP) {
            ToggleVisibility(hwnd);
        } else if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
            ShowTrayMenu(hwnd);
        }
        return 0;

    case WM_DESTROY:
        if (g_settingsWindow) {
            DestroyWindow(g_settingsWindow);
        }
        UnregisterHotkeys(hwnd);
        RemoveTrayIcon(hwnd);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    SetProcessDPIAware();

    g_instance = instance;
    LoadSettings();

    INITCOMMONCONTROLSEX commonControls{};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&commonControls);

    const wchar_t* className = L"CppCrosshairOverlayWindow";

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = className;

    if (!RegisterClassExW(&windowClass)) {
        MessageBoxW(nullptr, L"Could not register window class.", L"C++ Crosshair", MB_OK | MB_ICONERROR);
        return 1;
    }

    RECT rect = VirtualScreenRect();
    g_window = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        className,
        L"C++ Crosshair",
        WS_POPUP,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!g_window) {
        MessageBoxW(nullptr, L"Could not create overlay window.", L"C++ Crosshair", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(g_window, SW_SHOWNA);
    UpdateWindow(g_window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
