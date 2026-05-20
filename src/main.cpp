#define NOMINMAX

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <wininet.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cwctype>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr COLORREF kTransparentColor = RGB(0, 0, 0);
constexpr COLORREF kOutlineColor = RGB(24, 24, 24);
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kUpdateCheckCompleteMessage = WM_APP + 2;

constexpr const wchar_t* kAppVersion = L"v1.1.1";
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

constexpr int kSettingsOverlayMargin = 24;

constexpr COLORREF kSettingsBackground = RGB(6, 8, 12);
constexpr COLORREF kSettingsPanel = RGB(29, 33, 40);
constexpr COLORREF kSettingsPanelLight = RGB(49, 55, 66);
constexpr COLORREF kSettingsText = RGB(238, 242, 247);
constexpr COLORREF kSettingsMutedText = RGB(158, 169, 182);
constexpr COLORREF kSettingsBorder = RGB(79, 88, 103);

#ifndef TBM_SETBKCOLOR
constexpr UINT TBM_SETBKCOLOR = WM_USER + 19;
#endif
#ifndef TBM_SETCHANNELCOLOR
constexpr UINT TBM_SETCHANNELCOLOR = WM_USER + 20;
#endif
#ifndef TBM_SETTHUMBCOLOR
constexpr UINT TBM_SETTHUMBCOLOR = WM_USER + 21;
#endif

enum class HotkeyId : int {
    OpenSettings = 1,
    CloseSettings,
    BackupOpenSettings,
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
    RedTrack,
    RedEdit,
    GreenTrack,
    GreenEdit,
    BlueTrack,
    BlueEdit,
    ColorSwatch,
    DotCheck,
    OutlineCheck,
    VisibleCheck,
    OpenHotkeyEdit,
    CloseHotkeyEdit,
    BackupHotkeyEdit,
    ResetButton,
    CheckUpdateButton,
    CloseButton,
    ExitButton,
    SavePresetButton,
    RemovePresetButton,
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
    std::wstring downloadUrl;
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
    HWND redTrack = nullptr;
    HWND redEdit = nullptr;
    HWND greenTrack = nullptr;
    HWND greenEdit = nullptr;
    HWND blueTrack = nullptr;
    HWND blueEdit = nullptr;
    HWND colorSwatch = nullptr;
    HWND dotCheck = nullptr;
    HWND outlineCheck = nullptr;
    HWND visibleCheck = nullptr;
    HWND openHotkeyEdit = nullptr;
    HWND closeHotkeyEdit = nullptr;
    HWND backupHotkeyEdit = nullptr;
};

struct LayoutItem {
    HWND hwnd = nullptr;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

struct HotkeyCombo {
    UINT modifiers = MOD_CONTROL | MOD_ALT;
    UINT vk = 0;
};

enum class OverlayPanel {
    None,
    Title,
    Presets,
    Shape,
    Color,
    Options,
    Actions,
};

HINSTANCE g_instance = nullptr;
HWND g_window = nullptr;
HWND g_settingsWindow = nullptr;
HWND g_previewWindow = nullptr;
SettingsControls g_controls;
CrosshairSettings g_settings;
HotkeyCombo g_openHotkey{MOD_CONTROL | MOD_ALT, 'S'};
HotkeyCombo g_closeHotkey{MOD_CONTROL | MOD_ALT, 'H'};
HWND g_hotkeyCaptureEdit = nullptr;
bool g_updatingControls = false;
bool g_updateCheckRunning = false;
std::vector<LayoutItem> g_layoutItems;
std::vector<std::wstring> g_customPresetNames;
std::vector<CrosshairSettings> g_customPresets;
POINT g_titleOffset{0, 0};
POINT g_presetOffset{0, 0};
POINT g_shapeOffset{0, 0};
POINT g_colorOffset{0, 0};
POINT g_optionOffset{0, 0};
POINT g_actionOffset{0, 0};
OverlayPanel g_dragPanel = OverlayPanel::None;
POINT g_dragStart{0, 0};
POINT g_dragStartOffset{0, 0};

HBRUSH g_settingsBackgroundBrush = CreateSolidBrush(kSettingsBackground);
HBRUSH g_settingsPanelBrush = CreateSolidBrush(kSettingsPanel);
HBRUSH g_settingsEditBrush = CreateSolidBrush(kSettingsPanelLight);

const std::array<CrosshairPreset, 0> kPresets = {};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PreviewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool RegisterHotkeys(HWND hwnd);
void UnregisterHotkeys(HWND hwnd);

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

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return {};
    }

    std::string utf8(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), length, nullptr, nullptr);
    return utf8;
}

std::wstring Lowercase(std::wstring text) {
    for (wchar_t& ch : text) {
        if (ch >= L'A' && ch <= L'Z') {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }

    return text;
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

std::vector<std::string> ExtractJsonStrings(const std::string& json, const std::string& key) {
    std::vector<std::string> values;
    const std::string token = "\"" + key + "\"";
    size_t searchStart = 0;

    while (true) {
        size_t position = json.find(token, searchStart);
        if (position == std::string::npos) {
            break;
        }

        position = json.find(':', position + token.size());
        if (position == std::string::npos) {
            break;
        }

        position = json.find('"', position + 1);
        if (position == std::string::npos) {
            break;
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

        values.push_back(value);
        searchStart = position + 1;
    }

    return values;
}

std::wstring PreferredDownloadUrl(const std::string& releaseJson) {
    std::wstring fallback;

    for (const std::string& value : ExtractJsonStrings(releaseJson, "browser_download_url")) {
        const std::wstring url = Utf8ToWide(value);
        const std::wstring lowerUrl = Lowercase(url);
        if (lowerUrl.find(L".exe") == std::wstring::npos) {
            continue;
        }

        const bool isX86 = lowerUrl.find(L"x86") != std::wstring::npos;
#ifdef _WIN64
        if (!isX86) {
            return url;
        }
#else
        if (isX86) {
            return url;
        }
#endif

        if (fallback.empty()) {
            fallback = url;
        }
    }

    return fallback;
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

bool DownloadFile(const std::wstring& url, const std::wstring& path, std::wstring& error) {
    error.clear();

    HINTERNET session = InternetOpenW(
        L"CppCrosshairUpdater/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr,
        nullptr,
        0);
    if (!session) {
        error = L"Could not initialize the internet session.";
        return false;
    }

    DWORD timeout = 15000;
    InternetSetOptionW(session, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(session, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET request = InternetOpenUrlW(
        session,
        url.c_str(),
        nullptr,
        0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
        0);
    if (!request) {
        InternetCloseHandle(session);
        error = L"Could not download the update file.";
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (HttpQueryInfoW(request, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusSize, nullptr) &&
        statusCode >= 400) {
        InternetCloseHandle(request);
        InternetCloseHandle(session);
        error = L"GitHub returned HTTP " + std::to_wstring(statusCode) + L" while downloading the update.";
        return false;
    }

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(request);
        InternetCloseHandle(session);
        error = L"Could not create the temporary update file.";
        return false;
    }

    char buffer[8192]{};
    DWORD bytesRead = 0;
    bool ok = true;
    while (true) {
        if (!InternetReadFile(request, buffer, sizeof(buffer), &bytesRead)) {
            ok = false;
            error = L"The update download was interrupted.";
            break;
        }

        if (bytesRead == 0) {
            break;
        }

        DWORD bytesWritten = 0;
        if (!WriteFile(file, buffer, bytesRead, &bytesWritten, nullptr) || bytesWritten != bytesRead) {
            ok = false;
            error = L"Could not write the downloaded update file.";
            break;
        }
    }

    if (!ok) {
        DeleteFileW(path.c_str());
    }

    CloseHandle(file);
    InternetCloseHandle(request);
    InternetCloseHandle(session);
    return ok;
}

UpdateInfo BuildUpdateInfo(
    const std::wstring& latestVersion,
    const std::wstring& updateUrl,
    const std::wstring& downloadUrl,
    const std::wstring& source,
    bool manual) {
    UpdateInfo info;
    info.manual = manual;
    info.latestVersion = latestVersion;
    info.updateUrl = updateUrl.empty() ? kReleasesUrl : updateUrl;
    info.downloadUrl = downloadUrl;
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
        const std::wstring downloadUrl = PreferredDownloadUrl(body);
        if (!tag.empty()) {
            return BuildUpdateInfo(tag, url, downloadUrl, L"GitHub latest release", manual);
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
            return BuildUpdateInfo(tag, kReleasesUrl, L"", L"GitHub tag fallback", manual);
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

std::wstring ExePath() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, ARRAYSIZE(path));
    return path;
}

std::wstring TempUpdatePath(const wchar_t* fileName) {
    wchar_t tempDir[MAX_PATH]{};
    const DWORD length = GetTempPathW(ARRAYSIZE(tempDir), tempDir);
    const std::wstring base = (length > 0 && length < ARRAYSIZE(tempDir)) ? tempDir : L".\\";
    return base + fileName;
}

bool WriteTextFileUtf8(const std::wstring& path, const std::string& text) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    const bool ok = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) != 0 &&
        written == static_cast<DWORD>(text.size());
    CloseHandle(file);
    return ok;
}

bool ScheduleSelfUpdate(const std::wstring& downloadedExe, std::wstring& error) {
    error.clear();

    const std::wstring currentExe = ExePath();
    const std::wstring scriptPath = TempUpdatePath(L"crosshair-update.cmd");
    const DWORD pid = GetCurrentProcessId();

    const std::wstring script =
        L"@echo off\r\n"
        L"setlocal\r\n"
        L"set \"new=" + downloadedExe + L"\"\r\n"
        L"set \"target=" + currentExe + L"\"\r\n"
        L":wait\r\n"
        L"tasklist /FI \"PID eq " + std::to_wstring(pid) + L"\" | find \"" + std::to_wstring(pid) + L"\" >nul\r\n"
        L"if not errorlevel 1 (\r\n"
        L"  timeout /t 1 /nobreak >nul\r\n"
        L"  goto wait\r\n"
        L")\r\n"
        L"copy /Y \"%new%\" \"%target%\" >nul\r\n"
        L"if errorlevel 1 (\r\n"
        L"  start \"\" \"https://github.com/VKnubis/crossair/releases\"\r\n"
        L"  exit /b 1\r\n"
        L")\r\n"
        L"start \"\" \"%target%\"\r\n"
        L"del \"%new%\" >nul 2>nul\r\n"
        L"del \"%~f0\" >nul 2>nul\r\n";

    if (!WriteTextFileUtf8(scriptPath, WideToUtf8(script))) {
        error = L"Could not create the updater script.";
        return false;
    }

    HINSTANCE result = ShellExecuteW(nullptr, L"open", scriptPath.c_str(), nullptr, nullptr, SW_HIDE);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        error = L"Could not start the updater script.";
        return false;
    }

    return true;
}

bool InstallUpdate(const UpdateInfo& info, std::wstring& error) {
    error.clear();

    if (info.downloadUrl.empty()) {
        error = L"The latest release does not include a downloadable .exe asset.";
        return false;
    }

    const std::wstring downloadedExe = TempUpdatePath(L"crosshair-update-download.exe");
    if (!DownloadFile(info.downloadUrl, downloadedExe, error)) {
        return false;
    }

    return ScheduleSelfUpdate(downloadedExe, error);
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
            L"Download, replace this exe, and restart now?";

        if (MessageBoxW(MessageOwner(), message.c_str(), L"Crosshair Update", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
            std::wstring error;
            if (InstallUpdate(info, error)) {
                MessageBoxW(
                    MessageOwner(),
                    L"The update was downloaded. Crosshair will close, replace the exe, and restart.",
                    L"Crosshair Update",
                    MB_OK | MB_ICONINFORMATION);
                DestroyWindow(g_window);
                return;
            }

            const std::wstring failedMessage =
                L"Automatic update failed.\n\n" +
                (error.empty() ? L"Please download the latest release manually." : error) +
                L"\n\nOpen the GitHub releases page?";
            if (MessageBoxW(MessageOwner(), failedMessage.c_str(), L"Crosshair Update", MB_YESNO | MB_ICONWARNING) == IDYES) {
                OpenUpdatePage(info.updateUrl);
            }
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

int ReadProfileInt(const wchar_t* section, const wchar_t* key, int defaultValue) {
    return static_cast<int>(GetPrivateProfileIntW(section, key, defaultValue, SettingsPath().c_str()));
}

std::wstring ReadProfileString(const wchar_t* section, const wchar_t* key, const wchar_t* defaultValue = L"") {
    wchar_t text[256]{};
    GetPrivateProfileStringW(section, key, defaultValue, text, ARRAYSIZE(text), SettingsPath().c_str());
    return text;
}

void WriteSettingInt(const wchar_t* key, int value) {
    const std::wstring text = std::to_wstring(value);
    WritePrivateProfileStringW(L"Crosshair", key, text.c_str(), SettingsPath().c_str());
}

void WriteProfileInt(const wchar_t* section, const wchar_t* key, int value) {
    const std::wstring text = std::to_wstring(value);
    WritePrivateProfileStringW(section, key, text.c_str(), SettingsPath().c_str());
}

void WriteProfileString(const wchar_t* section, const wchar_t* key, const std::wstring& value) {
    WritePrivateProfileStringW(section, key, value.c_str(), SettingsPath().c_str());
}

bool IsAllowedHotkeyVk(UINT vk) {
    if (vk == VK_ESCAPE || vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
        vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
        vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
        vk == VK_LWIN || vk == VK_RWIN) {
        return false;
    }

    return vk >= VK_BACK && vk <= 0xFE;
}

bool HotkeyCombosEqual(const HotkeyCombo& left, const HotkeyCombo& right) {
    return left.modifiers == right.modifiers && left.vk == right.vk;
}

HotkeyCombo ReadHotkeyCombo(const wchar_t* modifiersKey, const wchar_t* vkKey, UINT defaultModifiers, UINT defaultVk) {
    HotkeyCombo combo{defaultModifiers, defaultVk};
    combo.modifiers = static_cast<UINT>(ReadSettingInt(modifiersKey, static_cast<int>(defaultModifiers))) &
        (MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN);
    combo.vk = static_cast<UINT>(ReadSettingInt(vkKey, static_cast<int>(defaultVk)));

    if (!IsAllowedHotkeyVk(combo.vk)) {
        combo.vk = defaultVk;
    }

    return combo;
}

void WriteHotkeyCombo(const wchar_t* modifiersKey, const wchar_t* vkKey, const HotkeyCombo& combo) {
    WriteSettingInt(modifiersKey, static_cast<int>(combo.modifiers));
    WriteSettingInt(vkKey, static_cast<int>(combo.vk));
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
    g_openHotkey = ReadHotkeyCombo(L"OpenHotkeyModifiers", L"OpenHotkeyVk", MOD_CONTROL | MOD_ALT, 'S');
    g_closeHotkey = ReadHotkeyCombo(L"CloseHotkeyModifiers", L"CloseHotkeyVk", MOD_CONTROL | MOD_ALT, 'H');
}

CrosshairSettings ReadPresetSettings(const wchar_t* section) {
    CrosshairSettings settings;
    settings.length = std::clamp(ReadProfileInt(section, L"Length", settings.length), kMinLength, kMaxLength);
    settings.gap = std::clamp(ReadProfileInt(section, L"Gap", settings.gap), kMinGap, kMaxGap);
    settings.thickness = std::clamp(ReadProfileInt(section, L"Thickness", settings.thickness), kMinThickness, kMaxThickness);
    settings.opacity = std::clamp(ReadProfileInt(section, L"Opacity", settings.opacity), kMinOpacity, kMaxOpacity);
    settings.offsetX = std::clamp(ReadProfileInt(section, L"OffsetX", settings.offsetX), kMinOffset, kMaxOffset);
    settings.offsetY = std::clamp(ReadProfileInt(section, L"OffsetY", settings.offsetY), kMinOffset, kMaxOffset);
    settings.red = std::clamp(ReadProfileInt(section, L"Red", settings.red), 0, 255);
    settings.green = std::clamp(ReadProfileInt(section, L"Green", settings.green), 0, 255);
    settings.blue = std::clamp(ReadProfileInt(section, L"Blue", settings.blue), 0, 255);
    settings.showDot = ReadProfileInt(section, L"ShowDot", settings.showDot ? 1 : 0) != 0;
    settings.outline = ReadProfileInt(section, L"Outline", settings.outline ? 1 : 0) != 0;
    settings.visible = true;
    return settings;
}

void WritePresetSettings(const wchar_t* section, const CrosshairSettings& settings) {
    WriteProfileInt(section, L"Length", settings.length);
    WriteProfileInt(section, L"Gap", settings.gap);
    WriteProfileInt(section, L"Thickness", settings.thickness);
    WriteProfileInt(section, L"Opacity", settings.opacity);
    WriteProfileInt(section, L"OffsetX", settings.offsetX);
    WriteProfileInt(section, L"OffsetY", settings.offsetY);
    WriteProfileInt(section, L"Red", settings.red);
    WriteProfileInt(section, L"Green", settings.green);
    WriteProfileInt(section, L"Blue", settings.blue);
    WriteProfileInt(section, L"ShowDot", settings.showDot ? 1 : 0);
    WriteProfileInt(section, L"Outline", settings.outline ? 1 : 0);
}

void LoadCustomPresets() {
    g_customPresetNames.clear();
    g_customPresets.clear();

    const int count = std::clamp(ReadProfileInt(L"CustomPresets", L"Count", 0), 0, 50);
    for (int index = 0; index < count; ++index) {
        const std::wstring section = L"CustomPreset" + std::to_wstring(index);
        std::wstring name = ReadProfileString(section.c_str(), L"Name");
        if (name.empty()) {
            name = L"Custom " + std::to_wstring(index + 1);
        }

        g_customPresetNames.push_back(name);
        g_customPresets.push_back(ReadPresetSettings(section.c_str()));
    }
}

void SaveCustomPresets() {
    const int oldCount = std::clamp(ReadProfileInt(L"CustomPresets", L"Count", 0), 0, 50);
    WriteProfileInt(L"CustomPresets", L"Count", static_cast<int>(g_customPresets.size()));

    for (size_t index = 0; index < g_customPresets.size(); ++index) {
        const std::wstring section = L"CustomPreset" + std::to_wstring(index);
        WriteProfileString(section.c_str(), L"Name", g_customPresetNames[index]);
        WritePresetSettings(section.c_str(), g_customPresets[index]);
    }

    for (int index = static_cast<int>(g_customPresets.size()); index < oldCount; ++index) {
        const std::wstring section = L"CustomPreset" + std::to_wstring(index);
        WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, SettingsPath().c_str());
    }
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
    WriteHotkeyCombo(L"OpenHotkeyModifiers", L"OpenHotkeyVk", g_openHotkey);
    WriteHotkeyCombo(L"CloseHotkeyModifiers", L"CloseHotkeyVk", g_closeHotkey);
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
    HPEN border = CreatePen(PS_SOLID, 1, kSettingsBorder);
    HGDIOBJ oldBrush = SelectObject(hdc, background);
    HGDIOBJ oldPen = SelectObject(hdc, border);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 18, 18);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(background);
    DeleteObject(border);

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

RECT OverlayRect(HWND hwnd, int x, int y, int width, int height) {
    RECT client{};
    GetClientRect(hwnd, &client);

    const int clientWidth = client.right - client.left;
    const int clientHeight = client.bottom - client.top;
    const int leftGroupX = std::max(44, clientWidth / 18);
    const int rightGroupX = std::max(leftGroupX + 380, clientWidth - 540);
    const int bottomX = std::max(kSettingsOverlayMargin, (clientWidth - 620) / 2);

    int left = x;
    int top = y;

    POINT offset{0, 0};

    if (y >= 780) {
        left = bottomX + (x - 24);
        top = std::max(kSettingsOverlayMargin, clientHeight - 72);
        offset = g_actionOffset;
    } else if (y >= 740) {
        left = bottomX + (x - 24);
        top = std::max(kSettingsOverlayMargin, clientHeight - 120);
        offset = g_optionOffset;
    } else if (y >= 498) {
        left = rightGroupX + (x - 24);
        top = 470 + (y - 520);
        offset = g_colorOffset;
    } else if (y >= 258) {
        left = rightGroupX + (x - 24);
        top = 170 + (y - 282);
        offset = g_shapeOffset;
    } else if (y >= 214) {
        left = leftGroupX + (x - 24);
        top = 178 + (y - 232);
        offset = g_presetOffset;
    } else {
        left = leftGroupX + x;
        top = kSettingsOverlayMargin + y;
        offset = g_titleOffset;
    }

    left += offset.x;
    top += offset.y;
    return {left, top, left + width, top + height};
}

void RegisterLayoutItem(HWND hwnd, int x, int y, int width, int height) {
    g_layoutItems.push_back(LayoutItem{hwnd, x, y, width, height});
}

void MoveControl(HWND control, int x, int y, int width, int height) {
    if (!control) {
        return;
    }

    const RECT rect = OverlayRect(g_settingsWindow, x, y, width, height);
    SetWindowPos(control, nullptr, rect.left, rect.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void LayoutSettingsControls() {
    if (!g_settingsWindow) {
        return;
    }

    for (const LayoutItem& item : g_layoutItems) {
        MoveControl(item.hwnd, item.x, item.y, item.width, item.height);
    }
}

void RedrawOverlay(HWND hwnd) {
    if (!hwnd) {
        return;
    }

    InvalidateRect(hwnd, nullptr, TRUE);
    UpdateWindow(hwnd);
}

void UpdateSettingsControls();
void RebuildPresetCombo();

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

    for (size_t index = 0; index < g_customPresets.size(); ++index) {
        if (SettingsMatchPreset(g_settings, g_customPresets[index])) {
            return static_cast<int>(kPresets.size() + index);
        }
    }

    return -1;
}

void ApplyPreset(size_t index) {
    CrosshairSettings preset;
    if (index < kPresets.size()) {
        preset = kPresets[index].settings;
    } else {
        const size_t customIndex = index - kPresets.size();
        if (customIndex >= g_customPresets.size()) {
            return;
        }
        preset = g_customPresets[customIndex];
    }

    const bool wasVisible = g_settings.visible;
    g_settings = preset;
    g_settings.visible = wasVisible;
    NotifySettingsChanged(true);
}

void SaveCurrentPreset() {
    const std::wstring name = L"Custom " + std::to_wstring(g_customPresets.size() + 1);
    g_customPresetNames.push_back(name);
    g_customPresets.push_back(g_settings);
    SaveCustomPresets();
    RebuildPresetCombo();
}

void RemoveSelectedCustomPreset() {
    if (!g_controls.presetCombo) {
        return;
    }

    const int selection = static_cast<int>(SendMessageW(g_controls.presetCombo, CB_GETCURSEL, 0, 0));
    const int customIndex = selection - static_cast<int>(kPresets.size());
    if (customIndex < 0 || customIndex >= static_cast<int>(g_customPresets.size())) {
        MessageBoxW(MessageOwner(), L"Only saved custom presets can be removed.", L"Crosshair Presets", MB_OK | MB_ICONINFORMATION);
        return;
    }

    g_customPresetNames.erase(g_customPresetNames.begin() + customIndex);
    g_customPresets.erase(g_customPresets.begin() + customIndex);
    SaveCustomPresets();
    RebuildPresetCombo();
}

void CycleColor(HWND hwnd) {
    const size_t presetCount = kPresets.size() + g_customPresets.size();
    if (presetCount == 0) {
        return;
    }

    const int currentPreset = MatchingPresetIndex();
    const size_t nextPreset = currentPreset < 0
        ? 0
        : (static_cast<size_t>(currentPreset) + 1) % presetCount;

    ApplyPreset(nextPreset);
    RedrawOverlay(hwnd);
}

HWND CreateLabel(HWND parent, const wchar_t* text, int x, int y, int width, int height) {
    const RECT rect = OverlayRect(parent, x, y, width, height);
    HWND control = CreateWindowExW(
        WS_EX_TRANSPARENT,
        L"STATIC",
        text,
        WS_CHILD | WS_VISIBLE,
        rect.left,
        rect.top,
        width,
        height,
        parent,
        nullptr,
        g_instance,
        nullptr);
    SetUiFont(control);
    RegisterLayoutItem(control, x, y, width, height);
    return control;
}

HWND CreateEditBox(HWND parent, ControlId id, int x, int y, int width = 64) {
    const RECT rect = OverlayRect(parent, x, y, width, 24);
    HWND control = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_RIGHT,
        rect.left,
        rect.top,
        width,
        24,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SetUiFont(control);
    RegisterLayoutItem(control, x, y, width, 24);
    return control;
}

HWND CreateTrackbar(HWND parent, ControlId id, int x, int y, int minValue, int maxValue) {
    constexpr int width = 316;
    const RECT rect = OverlayRect(parent, x, y, width, 28);
    HWND control = CreateWindowExW(
        0,
        TRACKBAR_CLASSW,
        L"",
        WS_CHILD | WS_VISIBLE | TBS_NOTICKS,
        rect.left,
        rect.top,
        width,
        28,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SendMessageW(control, TBM_SETRANGEMIN, FALSE, minValue);
    SendMessageW(control, TBM_SETRANGEMAX, TRUE, maxValue);
    SendMessageW(control, TBM_SETBKCOLOR, 0, kSettingsPanel);
    SendMessageW(control, TBM_SETCHANNELCOLOR, 0, RGB(210, 216, 224));
    SendMessageW(control, TBM_SETTHUMBCOLOR, 0, RGB(238, 242, 247));
    RegisterLayoutItem(control, x, y, width, 28);
    return control;
}

HWND CreateTrackbar(HWND parent, ControlId id, int x, int y, int width, int minValue, int maxValue) {
    const RECT rect = OverlayRect(parent, x, y, width, 28);
    HWND control = CreateWindowExW(
        0,
        TRACKBAR_CLASSW,
        L"",
        WS_CHILD | WS_VISIBLE | TBS_NOTICKS,
        rect.left,
        rect.top,
        width,
        28,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SendMessageW(control, TBM_SETRANGEMIN, FALSE, minValue);
    SendMessageW(control, TBM_SETRANGEMAX, TRUE, maxValue);
    SendMessageW(control, TBM_SETBKCOLOR, 0, kSettingsPanel);
    SendMessageW(control, TBM_SETCHANNELCOLOR, 0, RGB(210, 216, 224));
    SendMessageW(control, TBM_SETTHUMBCOLOR, 0, RGB(238, 242, 247));
    RegisterLayoutItem(control, x, y, width, 28);
    return control;
}

HWND CreateCheckbox(HWND parent, ControlId id, const wchar_t* text, int x, int y, int width) {
    const RECT rect = OverlayRect(parent, x, y, width, 24);
    HWND control = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        rect.left,
        rect.top,
        width,
        24,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SetUiFont(control);
    RegisterLayoutItem(control, x, y, width, 24);
    return control;
}

HWND CreateButton(HWND parent, ControlId id, const wchar_t* text, int x, int y, int width) {
    const RECT rect = OverlayRect(parent, x, y, width, 30);
    HWND control = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        rect.left,
        rect.top,
        width,
        30,
        parent,
        reinterpret_cast<HMENU>(static_cast<int>(id)),
        g_instance,
        nullptr);
    SetUiFont(control);
    RegisterLayoutItem(control, x, y, width, 30);
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

std::wstring HotkeyName(UINT vk) {
    if (vk >= 'A' && vk <= 'Z') {
        return std::wstring(1, static_cast<wchar_t>(vk));
    }

    if (vk >= '0' && vk <= '9') {
        return std::wstring(1, static_cast<wchar_t>(vk));
    }

    if (vk >= VK_F1 && vk <= VK_F12) {
        return L"F" + std::to_wstring(vk - VK_F1 + 1);
    }

    switch (vk) {
    case VK_BACK:
        return L"Backspace";
    case VK_TAB:
        return L"Tab";
    case VK_RETURN:
        return L"Enter";
    case VK_SPACE:
        return L"Space";
    case VK_PRIOR:
        return L"PageUp";
    case VK_NEXT:
        return L"PageDown";
    case VK_END:
        return L"End";
    case VK_HOME:
        return L"Home";
    case VK_LEFT:
        return L"Left";
    case VK_UP:
        return L"Up";
    case VK_RIGHT:
        return L"Right";
    case VK_DOWN:
        return L"Down";
    case VK_INSERT:
        return L"Insert";
    case VK_DELETE:
        return L"Delete";
    default:
        break;
    }

    const UINT scanCode = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    wchar_t name[64]{};
    if (scanCode != 0 && GetKeyNameTextW(static_cast<LONG>(scanCode << 16), name, ARRAYSIZE(name)) > 0) {
        return name;
    }

    return L"VK " + std::to_wstring(vk);
}

std::wstring HotkeyComboName(const HotkeyCombo& combo) {
    std::wstring text;
    if (combo.modifiers & MOD_CONTROL) {
        text += L"Ctrl+";
    }
    if (combo.modifiers & MOD_ALT) {
        text += L"Alt+";
    }
    if (combo.modifiers & MOD_SHIFT) {
        text += L"Shift+";
    }
    if (combo.modifiers & MOD_WIN) {
        text += L"Win+";
    }

    text += HotkeyName(combo.vk);
    return text;
}

HotkeyCombo CurrentPressedHotkeyCombo(UINT vk) {
    HotkeyCombo combo{0, vk};
    if (GetKeyState(VK_CONTROL) & 0x8000) {
        combo.modifiers |= MOD_CONTROL;
    }
    if (GetKeyState(VK_MENU) & 0x8000) {
        combo.modifiers |= MOD_ALT;
    }
    if (GetKeyState(VK_SHIFT) & 0x8000) {
        combo.modifiers |= MOD_SHIFT;
    }
    if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) {
        combo.modifiers |= MOD_WIN;
    }

    return combo;
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
    SetTrackValue(g_controls.redTrack, g_settings.red);
    SetTrackValue(g_controls.greenTrack, g_settings.green);
    SetTrackValue(g_controls.blueTrack, g_settings.blue);

    SetEditInt(g_controls.lengthEdit, g_settings.length);
    SetEditInt(g_controls.gapEdit, g_settings.gap);
    SetEditInt(g_controls.thicknessEdit, g_settings.thickness);
    SetEditInt(g_controls.opacityEdit, g_settings.opacity);
    SetEditInt(g_controls.offsetXEdit, g_settings.offsetX);
    SetEditInt(g_controls.offsetYEdit, g_settings.offsetY);
    SetEditInt(g_controls.redEdit, g_settings.red);
    SetEditInt(g_controls.greenEdit, g_settings.green);
    SetEditInt(g_controls.blueEdit, g_settings.blue);
    if (g_hotkeyCaptureEdit != g_controls.openHotkeyEdit) {
        SetWindowTextW(g_controls.openHotkeyEdit, HotkeyComboName(g_openHotkey).c_str());
    }
    if (g_hotkeyCaptureEdit != g_controls.closeHotkeyEdit) {
        SetWindowTextW(g_controls.closeHotkeyEdit, HotkeyComboName(g_closeHotkey).c_str());
    }
    SetWindowTextW(g_controls.backupHotkeyEdit, L"Ctrl+Alt+Shift+F12");

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

void RebuildPresetCombo() {
    if (!g_controls.presetCombo) {
        return;
    }

    SendMessageW(g_controls.presetCombo, CB_RESETCONTENT, 0, 0);
    for (const CrosshairPreset& preset : kPresets) {
        SendMessageW(g_controls.presetCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(preset.name));
    }

    for (const std::wstring& name : g_customPresetNames) {
        SendMessageW(g_controls.presetCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }

    const int matchingPreset = MatchingPresetIndex();
    SendMessageW(
        g_controls.presetCombo,
        CB_SETCURSEL,
        matchingPreset >= 0 ? static_cast<WPARAM>(matchingPreset) : static_cast<WPARAM>(-1),
        0);
}

void CreateSettingsControls(HWND hwnd) {
    g_controls = {};
    g_layoutItems.clear();

    CreateLabel(hwnd, L"Preset", 24, 236, 84, 22);
    const RECT presetRect = OverlayRect(hwnd, 112, 232, 260, 220);
    g_controls.presetCombo = CreateWindowExW(
        0,
        L"COMBOBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
        presetRect.left,
        presetRect.top,
        260,
        220,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<int>(ControlId::PresetCombo)),
        g_instance,
        nullptr);
    SetUiFont(g_controls.presetCombo);
    RegisterLayoutItem(g_controls.presetCombo, 112, 232, 260, 220);
    SendMessageW(g_controls.presetCombo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), 26);
    SendMessageW(g_controls.presetCombo, CB_SETITEMHEIGHT, 0, 26);
    RebuildPresetCombo();

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

    CreateLabel(hwnd, L"Red", labelX, 524, 90, 22);
    g_controls.redTrack = CreateTrackbar(hwnd, ControlId::RedTrack, trackX, 520, 244, 0, 255);
    g_controls.redEdit = CreateEditBox(hwnd, ControlId::RedEdit, editX, 521);

    CreateLabel(hwnd, L"Green", labelX, 562, 90, 22);
    g_controls.greenTrack = CreateTrackbar(hwnd, ControlId::GreenTrack, trackX, 558, 244, 0, 255);
    g_controls.greenEdit = CreateEditBox(hwnd, ControlId::GreenEdit, editX, 559);

    CreateLabel(hwnd, L"Blue", labelX, 600, 90, 22);
    g_controls.blueTrack = CreateTrackbar(hwnd, ControlId::BlueTrack, trackX, 596, 244, 0, 255);
    g_controls.blueEdit = CreateEditBox(hwnd, ControlId::BlueEdit, editX, 597);

    CreateLabel(hwnd, L"Color", labelX, 636, 90, 22);
    const RECT swatchRect = OverlayRect(hwnd, 122, 632, 244, 24);
    g_controls.colorSwatch = CreateWindowExW(
        0,
        L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        swatchRect.left,
        swatchRect.top,
        244,
        24,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<int>(ControlId::ColorSwatch)),
        g_instance,
        nullptr);
    RegisterLayoutItem(g_controls.colorSwatch, 122, 632, 244, 24);
    CreateLabel(hwnd, L"Open", 386, 633, 52, 22);
    g_controls.openHotkeyEdit = CreateEditBox(hwnd, ControlId::OpenHotkeyEdit, 442, 632, 124);
    SendMessageW(g_controls.openHotkeyEdit, EM_SETREADONLY, TRUE, 0);
    CreateLabel(hwnd, L"Hide", 386, 670, 52, 22);
    g_controls.closeHotkeyEdit = CreateEditBox(hwnd, ControlId::CloseHotkeyEdit, 442, 669, 124);
    SendMessageW(g_controls.closeHotkeyEdit, EM_SETREADONLY, TRUE, 0);
    CreateLabel(hwnd, L"Backup", 386, 707, 52, 22);
    g_controls.backupHotkeyEdit = CreateEditBox(hwnd, ControlId::BackupHotkeyEdit, 442, 706, 124);
    SendMessageW(g_controls.backupHotkeyEdit, EM_SETREADONLY, TRUE, 0);

    g_controls.dotCheck = CreateCheckbox(hwnd, ControlId::DotCheck, L"Center dot", 24, 740, 120);
    g_controls.outlineCheck = CreateCheckbox(hwnd, ControlId::OutlineCheck, L"Dark outline", 182, 740, 130);
    g_controls.visibleCheck = CreateCheckbox(hwnd, ControlId::VisibleCheck, L"Show crosshair", 350, 740, 150);

    CreateButton(hwnd, ControlId::SavePresetButton, L"Save preset", 24, 780, 96);
    CreateButton(hwnd, ControlId::RemovePresetButton, L"Remove", 130, 780, 74);
    CreateButton(hwnd, ControlId::CheckUpdateButton, L"Updates", 290, 780, 82);
    CreateButton(hwnd, ControlId::ResetButton, L"Reset", 380, 780, 64);
    CreateButton(hwnd, ControlId::CloseButton, L"Hide", 452, 780, 64);
    CreateButton(hwnd, ControlId::ExitButton, L"Exit", 524, 780, 64);

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
    } else if (track == g_controls.redTrack) {
        g_settings.red = GetTrackValue(track);
    } else if (track == g_controls.greenTrack) {
        g_settings.green = GetTrackValue(track);
    } else if (track == g_controls.blueTrack) {
        g_settings.blue = GetTrackValue(track);
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

void BeginHotkeyCapture(HWND hwnd, HWND edit) {
    if (g_hotkeyCaptureEdit && g_hotkeyCaptureEdit != edit) {
        UpdateSettingsControls();
    }

    g_hotkeyCaptureEdit = edit;
    SetWindowTextW(edit, L"Press key...");
    SetFocus(hwnd);
}

void ApplyCapturedHotkey(HWND hwnd, HWND edit, const HotkeyCombo& newHotkey) {
    constexpr UINT backupModifiers = MOD_CONTROL | MOD_ALT | MOD_SHIFT;
    const HotkeyCombo backupHotkey{backupModifiers, VK_F12};

    if (!IsAllowedHotkeyVk(newHotkey.vk)) {
        MessageBoxW(hwnd, L"That key cannot be used for this shortcut.", L"Crosshair Shortcuts", MB_OK | MB_ICONINFORMATION);
        g_hotkeyCaptureEdit = nullptr;
        UpdateSettingsControls();
        return;
    }

    const bool editingOpen = edit == g_controls.openHotkeyEdit;
    if (HotkeyCombosEqual(newHotkey, backupHotkey)) {
        MessageBoxW(hwnd, L"That combo is reserved for emergency open.", L"Crosshair Shortcuts", MB_OK | MB_ICONINFORMATION);
        g_hotkeyCaptureEdit = nullptr;
        UpdateSettingsControls();
        return;
    }

    const HotkeyCombo oldOpen = g_openHotkey;
    const HotkeyCombo oldClose = g_closeHotkey;
    if (editingOpen) {
        g_openHotkey = newHotkey;
    } else {
        g_closeHotkey = newHotkey;
    }

    UnregisterHotkeys(g_window);
    if (!RegisterHotkeys(g_window)) {
        UnregisterHotkeys(g_window);
        g_openHotkey = oldOpen;
        g_closeHotkey = oldClose;
        RegisterHotkeys(g_window);
        MessageBoxW(hwnd, L"Windows would not register that shortcut. Try another key.", L"Crosshair Shortcuts", MB_OK | MB_ICONWARNING);
        g_hotkeyCaptureEdit = nullptr;
        UpdateSettingsControls();
        return;
    }

    g_hotkeyCaptureEdit = nullptr;
    SaveSettings();
    UpdateSettingsControls();
}

void HandleSettingsCommand(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    const int id = LOWORD(wParam);
    const int notification = HIWORD(wParam);

    switch (static_cast<ControlId>(id)) {
    case ControlId::PresetCombo:
        if (notification == CBN_SELCHANGE) {
            const int selection = static_cast<int>(SendMessageW(g_controls.presetCombo, CB_GETCURSEL, 0, 0));
            if (selection >= 0 &&
                selection < static_cast<int>(kPresets.size() + g_customPresets.size())) {
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

    case ControlId::OpenHotkeyEdit:
    case ControlId::CloseHotkeyEdit:
        if (notification == EN_SETFOCUS) {
            BeginHotkeyCapture(hwnd, reinterpret_cast<HWND>(lParam));
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

    case ControlId::SavePresetButton:
        if (notification == BN_CLICKED) {
            SaveCurrentPreset();
        }
        break;

    case ControlId::RemovePresetButton:
        if (notification == BN_CLICKED) {
            RemoveSelectedCustomPreset();
        }
        break;

    case ControlId::CloseButton:
        if (notification == BN_CLICKED) {
            DestroyWindow(hwnd);
        }
        break;

    case ControlId::ExitButton:
        if (notification == BN_CLICKED) {
            DestroyWindow(g_window);
        }
        break;

    default:
        break;
    }
}

void DrawPresetComboItem(const DRAWITEMSTRUCT& item) {
    if (item.itemID == static_cast<UINT>(-1) ||
        item.itemID >= kPresets.size() + g_customPresets.size()) {
        return;
    }

    const bool selected = (item.itemState & ODS_SELECTED) != 0;
    const COLORREF backgroundColor = selected ? RGB(0, 118, 90) : kSettingsPanelLight;
    const COLORREF textColor = kSettingsText;
    const bool custom = item.itemID >= kPresets.size();
    const size_t customIndex = custom ? item.itemID - kPresets.size() : 0;
    const CrosshairSettings& preset = custom ? g_customPresets[customIndex] : kPresets[item.itemID].settings;
    const wchar_t* name = custom ? g_customPresetNames[customIndex].c_str() : kPresets[item.itemID].name;

    HBRUSH background = CreateSolidBrush(backgroundColor);
    FillRect(item.hDC, &item.rcItem, background);
    DeleteObject(background);

    RECT swatch{
        item.rcItem.left + 8,
        item.rcItem.top + 5,
        item.rcItem.left + 28,
        item.rcItem.bottom - 5,
    };
    HBRUSH swatchBrush = CreateSolidBrush(RGB(preset.red, preset.green, preset.blue));
    FillRect(item.hDC, &swatch, swatchBrush);
    DeleteObject(swatchBrush);
    FrameRect(item.hDC, &swatch, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    RECT textRect = item.rcItem;
    textRect.left += 38;
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, textColor);
    SelectObject(item.hDC, UiFont());
    DrawTextW(item.hDC, name, -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    if (item.itemState & ODS_FOCUS) {
        DrawFocusRect(item.hDC, &item.rcItem);
    }
}

void DrawColorSwatch(const DRAWITEMSTRUCT& item) {
    RECT rect = item.rcItem;
    HBRUSH frame = CreateSolidBrush(kSettingsBorder);
    HPEN pen = CreatePen(PS_SOLID, 1, kSettingsBorder);
    HGDIOBJ oldBrush = SelectObject(item.hDC, frame);
    HGDIOBJ oldPen = SelectObject(item.hDC, pen);
    RoundRect(item.hDC, rect.left, rect.top, rect.right, rect.bottom, 12, 12);
    SelectObject(item.hDC, oldBrush);
    SelectObject(item.hDC, oldPen);
    DeleteObject(frame);
    DeleteObject(pen);

    InflateRect(&rect, -3, -3);
    HBRUSH color = CreateSolidBrush(CurrentColor());
    oldBrush = SelectObject(item.hDC, color);
    oldPen = SelectObject(item.hDC, reinterpret_cast<HPEN>(GetStockObject(NULL_PEN)));
    RoundRect(item.hDC, rect.left, rect.top, rect.right, rect.bottom, 9, 9);
    SelectObject(item.hDC, oldBrush);
    SelectObject(item.hDC, oldPen);
    DeleteObject(color);
}

void FillRoundedRect(HDC hdc, const RECT& rect, COLORREF fillColor, COLORREF borderColor, int radius) {
    HBRUSH brush = CreateSolidBrush(fillColor);
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawOverlayButton(const DRAWITEMSTRUCT& item) {
    RECT rect = item.rcItem;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    const bool focused = (item.itemState & ODS_FOCUS) != 0;
    const COLORREF fill = pressed ? RGB(210, 216, 224) : kSettingsPanelLight;
    const COLORREF text = pressed ? RGB(20, 24, 30) : kSettingsText;

    FillRoundedRect(item.hDC, rect, fill, focused ? RGB(148, 170, 205) : kSettingsBorder, 12);

    wchar_t label[64]{};
    GetWindowTextW(item.hwndItem, label, ARRAYSIZE(label));

    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, text);
    SelectObject(item.hDC, UiFont());
    DrawTextW(item.hDC, label, -1, &rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
}

bool PointInRect(const RECT& rect, POINT point) {
    return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
}

RECT InflatedRect(RECT rect, int dx, int dy) {
    InflateRect(&rect, dx, dy);
    return rect;
}

RECT TitlePanelRect() {
    return {
        kSettingsOverlayMargin + g_titleOffset.x,
        kSettingsOverlayMargin + g_titleOffset.y,
        340 + g_titleOffset.x,
        86 + g_titleOffset.y,
    };
}

OverlayPanel HitTestOverlayPanel(HWND hwnd, POINT point) {
    if (PointInRect(TitlePanelRect(), point)) {
        return OverlayPanel::Title;
    }

    if (PointInRect(InflatedRect(OverlayRect(hwnd, 12, 214, 350, 248), 12, 12), point)) {
        return OverlayPanel::Presets;
    }

    if (PointInRect(InflatedRect(OverlayRect(hwnd, 12, 264, 530, 232), 12, 12), point)) {
        return OverlayPanel::Shape;
    }

    if (PointInRect(InflatedRect(OverlayRect(hwnd, 12, 506, 566, 234), 12, 12), point)) {
        return OverlayPanel::Color;
    }

    if (PointInRect(InflatedRect(OverlayRect(hwnd, 12, 740, 530, 34), 12, 10), point)) {
        return OverlayPanel::Options;
    }

    if (PointInRect(InflatedRect(OverlayRect(hwnd, 12, 780, 588, 46), 12, 10), point)) {
        return OverlayPanel::Actions;
    }

    return OverlayPanel::None;
}

POINT* OffsetForPanel(OverlayPanel panel) {
    switch (panel) {
    case OverlayPanel::Title:
        return &g_titleOffset;
    case OverlayPanel::Presets:
        return &g_presetOffset;
    case OverlayPanel::Shape:
        return &g_shapeOffset;
    case OverlayPanel::Color:
        return &g_colorOffset;
    case OverlayPanel::Options:
        return &g_optionOffset;
    case OverlayPanel::Actions:
        return &g_actionOffset;
    default:
        return nullptr;
    }
}

void DrawSettingsChrome(HWND hwnd, HDC hdc) {
    RECT client{};
    GetClientRect(hwnd, &client);
    HBRUSH backdrop = CreateSolidBrush(kSettingsBackground);
    FillRect(hdc, &client, backdrop);
    DeleteObject(backdrop);

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, UiFont());

    RECT titlePanel = TitlePanelRect();
    FillRoundedRect(hdc, titlePanel, kSettingsPanel, kSettingsBorder, 18);
    RECT accent{titlePanel.left, titlePanel.top, titlePanel.left + 6, titlePanel.bottom};
    HBRUSH accentBrush = CreateSolidBrush(CurrentColor());
    FillRect(hdc, &accent, accentBrush);
    DeleteObject(accentBrush);

    const std::wstring titleText = std::wstring(L"Crosshair Studio ") + kAppVersion;
    RECT title{titlePanel.left + 20, titlePanel.top + 10, titlePanel.right - 14, titlePanel.top + 34};
    SetTextColor(hdc, kSettingsText);
    DrawTextW(hdc, titleText.c_str(), -1, &title, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    RECT subtitle{titlePanel.left + 20, titlePanel.top + 34, titlePanel.right - 14, titlePanel.bottom - 8};
    SetTextColor(hdc, kSettingsMutedText);
    DrawTextW(hdc, L"Transparent HUD editor - Esc hides", -1, &subtitle, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    const int clientWidth = client.right - client.left;
    const int clientHeight = client.bottom - client.top;
    RECT preview{
        std::max(380, (clientWidth - 440) / 2),
        std::max(110, (clientHeight - 220) / 2),
        std::max(380, (clientWidth - 440) / 2) + 440,
        std::max(110, (clientHeight - 220) / 2) + 220,
    };
    FillRoundedRect(hdc, preview, RGB(24, 28, 34), kSettingsBorder, 22);
    RECT previewInner = preview;
    InflateRect(&previewInner, -10, -10);
    DrawPreviewPanel(hdc, previewInner);

    RECT presetPanel = OverlayRect(hwnd, 12, 214, 350, 248);
    InflateRect(&presetPanel, 12, 12);
    FillRoundedRect(hdc, presetPanel, kSettingsPanel, kSettingsBorder, 18);

    RECT shapePanel = OverlayRect(hwnd, 12, 264, 530, 232);
    InflateRect(&shapePanel, 12, 12);
    FillRoundedRect(hdc, shapePanel, kSettingsPanel, kSettingsBorder, 18);

    RECT colorPanel = OverlayRect(hwnd, 12, 506, 566, 234);
    InflateRect(&colorPanel, 12, 12);
    FillRoundedRect(hdc, colorPanel, kSettingsPanel, kSettingsBorder, 18);

    RECT optionPanel = OverlayRect(hwnd, 12, 740, 530, 34);
    InflateRect(&optionPanel, 12, 10);
    FillRoundedRect(hdc, optionPanel, kSettingsPanel, kSettingsBorder, 18);

    RECT actionPanel = OverlayRect(hwnd, 12, 780, 588, 46);
    InflateRect(&actionPanel, 12, 10);
    FillRoundedRect(hdc, actionPanel, kSettingsPanel, kSettingsBorder, 18);

    RECT shapeLabel = OverlayRect(hwnd, 24, 258, 220, 20);
    SetTextColor(hdc, kSettingsMutedText);
    DrawTextW(hdc, L"Shape", -1, &shapeLabel, DT_LEFT | DT_SINGLELINE | DT_VCENTER);

    RECT colorLabel = OverlayRect(hwnd, 24, 498, 220, 20);
    DrawTextW(hdc, L"Color", -1, &colorLabel, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
}

bool RegisterHotkeys(HWND hwnd) {
    const UINT backupModifiers = MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT;
    const bool openOk = RegisterHotKey(
        hwnd,
        static_cast<int>(HotkeyId::OpenSettings),
        g_openHotkey.modifiers | MOD_NOREPEAT,
        g_openHotkey.vk) != 0;
    const bool sameToggle = HotkeyCombosEqual(g_openHotkey, g_closeHotkey);
    const bool closeOk = sameToggle
        ? true
        : RegisterHotKey(
            hwnd,
            static_cast<int>(HotkeyId::CloseSettings),
            g_closeHotkey.modifiers | MOD_NOREPEAT,
            g_closeHotkey.vk) != 0;
    RegisterHotKey(hwnd, static_cast<int>(HotkeyId::BackupOpenSettings), backupModifiers, VK_F12);
    return openOk && closeOk;
}

void UnregisterHotkeys(HWND hwnd) {
    UnregisterHotKey(hwnd, static_cast<int>(HotkeyId::OpenSettings));
    UnregisterHotKey(hwnd, static_cast<int>(HotkeyId::CloseSettings));
    UnregisterHotKey(hwnd, static_cast<int>(HotkeyId::BackupOpenSettings));
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
    const std::wstring openShortcut = HotkeyComboName(g_openHotkey);
    const std::wstring closeShortcut = HotkeyComboName(g_closeHotkey);
    const std::wstring text =
        L"Version: " + std::wstring(kAppVersion) + L"\n\n"
        L"Hotkeys:\n"
        + openShortcut + L": Open settings\n"
        + closeShortcut + L": Hide settings\n\n"
        L"Ctrl+Alt+Shift+F12: Emergency open settings\n\n"
        L"Double-click or right-click the tray icon if you forget the custom shortcut.\n"
        L"The update checker looks at VKnubis/crossair on GitHub.";

    MessageBoxW(hwnd, text.c_str(), L"C++ Crosshair", MB_OK | MB_ICONINFORMATION);
}

bool IsTrackbarControl(HWND hwnd) {
    return hwnd == g_controls.lengthTrack ||
        hwnd == g_controls.gapTrack ||
        hwnd == g_controls.thicknessTrack ||
        hwnd == g_controls.opacityTrack ||
        hwnd == g_controls.offsetXTrack ||
        hwnd == g_controls.offsetYTrack ||
        hwnd == g_controls.redTrack ||
        hwnd == g_controls.greenTrack ||
        hwnd == g_controls.blueTrack;
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

void ShowSettingsWindow() {
    RECT screen = VirtualScreenRect();

    if (g_settingsWindow) {
        ShowWindow(g_settingsWindow, SW_SHOW);
        SetWindowPos(
            g_settingsWindow,
            HWND_TOPMOST,
            screen.left,
            screen.top,
            screen.right - screen.left,
            screen.bottom - screen.top,
            SWP_SHOWWINDOW);
        SetForegroundWindow(g_settingsWindow);
        LayoutSettingsControls();
        UpdateSettingsControls();
        InvalidateRect(g_settingsWindow, nullptr, TRUE);
        return;
    }

    if (!RegisterWindowClasses()) {
        MessageBoxW(g_window, L"Could not create the settings window.", L"C++ Crosshair", MB_OK | MB_ICONERROR);
        return;
    }

    constexpr DWORD settingsStyle = WS_POPUP | WS_CLIPCHILDREN;
    constexpr DWORD settingsExStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW;

    g_settingsWindow = CreateWindowExW(
        settingsExStyle,
        L"CppCrosshairSettingsWindow",
        L"Crosshair Overlay",
        settingsStyle,
        screen.left,
        screen.top,
        screen.right - screen.left,
        screen.bottom - screen.top,
        nullptr,
        nullptr,
        g_instance,
        nullptr);

    if (!g_settingsWindow) {
        MessageBoxW(g_window, L"Could not create the settings window.", L"C++ Crosshair", MB_OK | MB_ICONERROR);
        return;
    }

    SetLayeredWindowAttributes(g_settingsWindow, 0, 224, LWA_ALPHA);
    UpdateSettingsControls();
    ShowWindow(g_settingsWindow, SW_SHOW);
    SetWindowPos(
        g_settingsWindow,
        HWND_TOPMOST,
        screen.left,
        screen.top,
        screen.right - screen.left,
        screen.bottom - screen.top,
        SWP_SHOWWINDOW);
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
    if (static_cast<HotkeyId>(wParam) == HotkeyId::OpenSettings ||
        static_cast<HotkeyId>(wParam) == HotkeyId::BackupOpenSettings) {
        if (static_cast<HotkeyId>(wParam) == HotkeyId::OpenSettings &&
            HotkeyCombosEqual(g_openHotkey, g_closeHotkey) &&
            g_settingsWindow) {
            DestroyWindow(g_settingsWindow);
        } else {
            ShowSettingsWindow();
        }
    } else if (static_cast<HotkeyId>(wParam) == HotkeyId::CloseSettings && g_settingsWindow) {
        DestroyWindow(g_settingsWindow);
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
        SetTextColor(hdc, kSettingsText);
        if (message == WM_CTLCOLORSTATIC && IsTrackbarControl(reinterpret_cast<HWND>(lParam))) {
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, kSettingsPanel);
            return reinterpret_cast<LRESULT>(g_settingsPanelBrush);
        }

        if (message == WM_CTLCOLORBTN) {
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, kSettingsPanel);
            return reinterpret_cast<LRESULT>(g_settingsPanelBrush);
        }

        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
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

    case WM_SIZE:
        LayoutSettingsControls();
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            POINT point{};
            GetCursorPos(&point);
            ScreenToClient(hwnd, &point);
            if (HitTestOverlayPanel(hwnd, point) != OverlayPanel::None) {
                SetCursor(LoadCursorW(nullptr, IDC_SIZEALL));
                return TRUE;
            }
        }
        break;

    case WM_LBUTTONDOWN: {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        g_dragPanel = HitTestOverlayPanel(hwnd, point);
        POINT* offset = OffsetForPanel(g_dragPanel);
        if (offset) {
            g_dragStart = point;
            g_dragStartOffset = *offset;
            SetCapture(hwnd);
            return 0;
        }
        break;
    }

    case WM_MOUSEMOVE:
        if (g_dragPanel != OverlayPanel::None && (wParam & MK_LBUTTON)) {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            POINT* offset = OffsetForPanel(g_dragPanel);
            if (offset) {
                offset->x = g_dragStartOffset.x + (point.x - g_dragStart.x);
                offset->y = g_dragStartOffset.y + (point.y - g_dragStart.y);
                LayoutSettingsControls();
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }
        }
        break;

    case WM_LBUTTONUP:
        if (g_dragPanel != OverlayPanel::None) {
            g_dragPanel = OverlayPanel::None;
            ReleaseCapture();
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (g_hotkeyCaptureEdit) {
            if (wParam == VK_ESCAPE) {
                g_hotkeyCaptureEdit = nullptr;
                DestroyWindow(hwnd);
                return 0;
            }

            ApplyCapturedHotkey(hwnd, g_hotkeyCaptureEdit, CurrentPressedHotkeyCombo(static_cast<UINT>(wParam)));
            return 0;
        }

        if (wParam == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

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
        if (wParam == static_cast<WPARAM>(ControlId::CheckUpdateButton) ||
            wParam == static_cast<WPARAM>(ControlId::ResetButton) ||
            wParam == static_cast<WPARAM>(ControlId::CloseButton) ||
            wParam == static_cast<WPARAM>(ControlId::ExitButton) ||
            wParam == static_cast<WPARAM>(ControlId::SavePresetButton) ||
            wParam == static_cast<WPARAM>(ControlId::RemovePresetButton)) {
            DrawOverlayButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
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
    LoadCustomPresets();

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
