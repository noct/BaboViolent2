solution "BaboViolent"
    configurations {"Debug", "Release"}
    platforms {"x64"}
    startproject  "bv2"
    location  (".build/".._ACTION)
    objdir    (".build/".._ACTION.."/obj")
    targetdir (".build/".._ACTION.."/lib")
    flags {
        "NoPCH",
        "EnableSSE2",
        "ExtraWarnings",
        "Symbols"
    }

    configuration {"Debug"}
        defines {"DEBUG"}

    configuration {"Release"}
        defines {"NDEBUG"}
        flags   {"Optimize"}

    configuration {"Windows"}
        defines {
            "WIN32",
        }

    configuration {"not Windows"}
        buildoptions_cpp {
            "-std=c++11"
        }

    project "bv2"
        kind "WindowedApp"
        language "C++"
        targetdir "."
        debugdir "."

        defines {
            "SDL_MAIN_HANDLED",
            "_PRO_",
            "_MINIBOT_"
        }

        configuration {"Debug"}
            links {
                "opengl32",
                "SDL2d",
                "libcurl"
            }

        configuration {"Release"}
            links {
                "opengl32",
                "SDL2",
                "libcurl"
            }

        configuration {"vs*"}
            defines {
                "NOMINMAX",
                "_CRT_SECURE_NO_WARNINGS",
                "_WINSOCK_DEPRECATED_NO_WARNINGS",
                "_CRT_NONSTDC_NO_DEPRECATE"
            }
        configuration {}

        includedirs {
            "src/Engine/Babonet",
            "src/Engine/dko",
            "src/Engine/Zeven/dkc",
            "src/Engine/Zeven/dkf",
            "src/Engine/Zeven/dkgl",
            "src/Engine/Zeven/dki",
            "src/Engine/Zeven/dkp",
            "src/Engine/Zeven/dks",
            "src/Engine/Zeven/dksvar",
            "src/Engine/Zeven/dkt",
            "src/Engine/Zeven/dkw",
            "src/External",
            "src/Game",
            "src/Game/AStar",
            "src/Game/Master",
            "src/Menu",
            "src/Menu2",
            "src/Menu2/Dialogs",
            "src/Source",
            "src/Weather",
            "src/Zeven",
            "src/Zeven/API",
            "thirdparty/glad/include",
            "thirdparty/imgui",
            "thirdparty/imgui/examples",
            "thirdparty/sqlite",
        }

        files {
            "src/**.hpp",
            "src/**.cpp",
            "src/**.h",
            "src/**.c",
            "thirdparty/imgui/imgui_demo.cpp",
            "thirdparty/imgui/imgui_draw.cpp",
            "thirdparty/imgui/imgui_internal.h",
            "thirdparty/imgui/imgui.cpp",
            "thirdparty/imgui/imgui.h",
            "thirdparty/imgui/stb_rect_pack.h",
            "thirdparty/imgui/stb_textedit.h",
            "thirdparty/imgui/stb_truetype.h",
            "thirdparty/imgui/examples/imgui_impl_opengl3.cpp",
            "thirdparty/imgui/examples/imgui_impl_opengl3.h",
            "thirdparty/imgui/examples/imgui_impl_sdl.cpp",
            "thirdparty/imgui/examples/imgui_impl_sdl.h",
            "thirdparty/sqlite/sqlite3.c",
            "thirdparty/glad/src/glad.c",
        }

