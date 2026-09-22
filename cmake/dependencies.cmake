include(FetchContent)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
)
set(SPDLOG_HEADER_ONLY ON)
add_compile_options(/MT$<$<CONFIG:Debug>:d>)


# sampapi (multiver: 0.3.7 R1 / R3-1 / R5-1 / 0.3.DL-1 gleichzeitig gelinkt)
FetchContent_Declare(
    sampapi
    GIT_REPOSITORY https://github.com/BlastHackNet/SAMP-API.git
    GIT_TAG        "multiver"
)

# nlohmann_json (Konfig: pricel.json)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
)

FetchContent_MakeAvailable(spdlog sampapi nlohmann_json)
