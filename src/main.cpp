#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
#include <plugin.h>

#include <sampapi/CChat.h>
#include <sampapi/CInput.h>
#include <sampapi/CNetGame.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace {

constexpr std::uintptr_t kCrosshairScaleAddr = 0x859A44;

constexpr float kBaseSize = 64.0f;

constexpr const char* kConfigFile   = "pricel.json";
constexpr const char* kPortIniFile  = "pricel.ini";
constexpr const char* kLegacyIniFile = "SAMPFUNCS\\pricelsize.ini";
constexpr const char* kIniSection   = "pricelsize"; 
constexpr const char* kIniKey       = "size"; 
constexpr const char* kLogFileName  = "pricel.log";

constexpr const char* kCommandName = "psize";

constexpr sampapi::D3DCOLOR kChatColorDefault = 0xFFFFFFFF;

constexpr const char* kColorGreen = "{66FF00}";
constexpr const char* kColorWhite = "{FFFFFF}";

constexpr const char* kMsgLoaded =
    "Crosshair size editor loaded. Author:{66FF00} mxn {FFFFFF} | Credits: {66FF00} Makaron";

constexpr const char* kMsgSizeSetFmt =
    "Crosshair size is now: {66FF00}%.1f.{FFFFFF} Setting saved";

std::string MakeAbsPath(const char* rel) {
    char cwd[MAX_PATH] = {};
    GetCurrentDirectoryA(MAX_PATH, cwd);
    return std::string(cwd) + "\\" + rel;
}

bool FileExists(const std::string& path) {
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

float IniReadFloat(const std::string& path) {
    if (!FileExists(path))
        return 0.0f;
    char buf[100] = {};
    GetPrivateProfileStringA(kIniSection, kIniKey, nullptr, buf, 100, path.c_str());
    if (!buf[0])
        return 0.0f;
    try {
        return std::stof(buf);
    } catch (...) {
        return 0.0f;
    }
}

float ConfigReadSize(const std::string& path) {
    if (!FileExists(path))
        return 0.0f;
    try {
        std::ifstream in(path);
        auto j = nlohmann::json::parse(in, /*callback=*/nullptr, /*allow_exceptions=*/false);
        if (j.is_discarded() || !j.is_object()) {
            spdlog::warn("config: {} is not parseable, using 0.0", path);
            return 0.0f;
        }
        return j.value("size", 0.0f);
    } catch (const std::exception& e) {
        spdlog::warn("config: {} is readable but invalid ({}), using 0.0", path, e.what());
        return 0.0f;
    }
}

void ConfigWriteSize(const std::string& path, float raw) {
    nlohmann::json j;
    j["size"] = raw;
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        spdlog::warn("config: {} is not writable", path);
        return;
    }
    out << j.dump(2) << '\n';
}

void ApplyCrosshairSize(float raw) {
    plugin::patch::SetFloat(kCrosshairScaleAddr, raw + kBaseSize);
}


enum class SampVer { R1, R3, R5, DL };

SampVer g_sampVer = SampVer::DL;

bool ImageContains(HMODULE mod, const char* lit, size_t len) {
    auto base = reinterpret_cast<const std::uint8_t*>(mod);
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;
    auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
    const std::size_t size = nt->OptionalHeader.SizeOfImage;
    for (std::size_t i = 0; i + len <= size; ++i)
        if (std::memcmp(base + i, lit, len) == 0)
            return true;
    return false;
}

SampVer DetectSampVersion(HMODULE samp) {
    if (ImageContains(samp, "0.3.DL", 6))
        return SampVer::DL;
    if (ImageContains(samp, "0.3.7-R5", 8))
        return SampVer::R5;
    if (ImageContains(samp, "0.3.7-R3", 8))
        return SampVer::R3;
    return SampVer::R1;
}

bool NetGameConnected() {
    switch (g_sampVer) {
    case SampVer::R1: {
        auto* net = sampapi::v037r1::RefNetGame();
        return net && net->GetState() == sampapi::v037r1::CNetGame::GAME_MODE_CONNECTED;
    }
    case SampVer::R3: {
        auto* net = sampapi::v037r3::RefNetGame();
        return net && net->GetState() == sampapi::v037r3::CNetGame::GAME_MODE_CONNECTED;
    }
    case SampVer::R5: {
        auto* net = sampapi::v037r5::RefNetGame();
        return net && net->GetState() == sampapi::v037r5::CNetGame::GAME_MODE_CONNECTED;
    }
    case SampVer::DL: {
        auto* net = sampapi::v03dl::RefNetGame();
        return net && net->GetState() == sampapi::v03dl::CNetGame::GAME_MODE_CONNECTED;
    }
    }
    return false;
}

void ChatAddMessage(sampapi::D3DCOLOR color, const char* text) {
    switch (g_sampVer) {
    case SampVer::R1: sampapi::v037r1::RefChat()->AddMessage(color, text); break;
    case SampVer::R3: sampapi::v037r3::RefChat()->AddMessage(color, text); break;
    case SampVer::R5: sampapi::v037r5::RefChat()->AddMessage(color, text); break;
    case SampVer::DL: sampapi::v03dl::RefChat()->AddMessage(color, text); break;
    }
}

void InputAddCommand(const char* name, sampapi::CMDPROC handler) {
    switch (g_sampVer) {
    case SampVer::R1: sampapi::v037r1::RefInputBox()->AddCommand(name, handler); break;
    case SampVer::R3: sampapi::v037r3::RefInputBox()->AddCommand(name, handler); break;
    case SampVer::R5: sampapi::v037r5::RefInputBox()->AddCommand(name, handler); break;
    case SampVer::DL: sampapi::v03dl::RefInputBox()->AddCommand(name, handler); break;
    }
}


void CmdPsize(const char* params) {
    if (!params || !*params)
        return;

    float raw = 0.0f;
    try {
        raw = std::stof(params);
    } catch (...) {
        spdlog::warn("psize: invalid value '{}'", params);
        return;
    }

    const std::string cfgPath = MakeAbsPath(kConfigFile);

    ConfigWriteSize(cfgPath, raw);

    char msg[256] = {};
    std::snprintf(msg, sizeof(msg), kMsgSizeSetFmt, static_cast<double>(raw));
    ChatAddMessage(kChatColorDefault, msg);

    ApplyCrosshairSize(raw);
    spdlog::info("psize: raw={}, hud={}", raw, raw + kBaseSize);
}


#if defined(_DEBUG)
void InitLogger() {
    try {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(kLogFileName, true);
        auto logger = std::make_shared<spdlog::logger>("pricel", sink);
        logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] %v");
        spdlog::set_default_logger(logger);
    } catch (...) {
    }
}
#else
void InitLogger() {}
#endif

DWORD WINAPI InitThread(LPVOID) {
#if defined(_DEBUG)
    InitLogger();
#endif

    HMODULE samp = nullptr;
    while (!(samp = GetModuleHandleA("samp.dll")))
        Sleep(100);

    g_sampVer = DetectSampVersion(samp);

    while (!NetGameConnected())
        Sleep(100);

    const std::string cfgPath = MakeAbsPath(kConfigFile);
    const std::string portIniPath = MakeAbsPath(kPortIniFile);
    const std::string legacyPath = MakeAbsPath(kLegacyIniFile);

    if (FileExists(cfgPath)) {
        const float size = ConfigReadSize(cfgPath);
        ApplyCrosshairSize(size);
        spdlog::info("config loaded: {} ({} -> {})", cfgPath, size, size + kBaseSize);
    } else if (FileExists(portIniPath) || FileExists(legacyPath)) {
        const bool fromLegacy = FileExists(legacyPath);
        const std::string& src = fromLegacy ? legacyPath : portIniPath;
        const float size = IniReadFloat(src);
        ConfigWriteSize(cfgPath, size);
        ApplyCrosshairSize(size);
        spdlog::info("migrated: {} -> {} (size={})", src, cfgPath, size);
    } else {
        ConfigWriteSize(cfgPath, 0.0f);
        spdlog::info("first start: {} created (size=0.0)", cfgPath);
    }

    ChatAddMessage(kChatColorDefault, kMsgLoaded);

    InputAddCommand(kCommandName, CmdPsize);
    spdlog::info("initialized: samp-version={} cmd=/{}", static_cast<int>(g_sampVer), kCommandName);

    return 0;
}

struct PricelPlugin {
    PricelPlugin() {
        HANDLE h = CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
        if (h)
            CloseHandle(h);
    }
} g_pricel;

} // namespace
