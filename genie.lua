solution "BaboViolent"
    configurations {"Debug", "Release"}
    platforms {"x64"}
    startproject  "bv2"
    location  (".build/".._ACTION)
    objdir    (".build/".._ACTION.."/obj")
    targetdir (".build/".._ACTION.."/lib")
    flags {
        "NoPCH",
        "ExtraWarnings",
        "Symbols",
        "WinMain"
    }

    configuration {"Debug"}
        defines {"DEBUG"}

    configuration {"Release"}
        defines {"NDEBUG"}
        flags   {"Optimize"}

    configuration {"vs*"}
        defines {
            "NOMINMAX",
            "_CRT_SECURE_NO_WARNINGS",
            "_WINSOCK_DEPRECATED_NO_WARNINGS",
            "_CRT_NONSTDC_NO_DEPRECATE"
        }

    configuration {"Windows"}
        defines {
            "WIN32",
        }

    configuration {"not Windows"}
        buildoptions_cpp {
            "-std=c++11"
        }

    project "Zeven"
        kind "StaticLib"
        language "C++"

        includedirs {
            "src",
            "thirdparty/glad/include",
            "thirdparty/imgui",
            "thirdparty/imgui/examples",
        }

        files {
            "src/Zeven/**.hpp",
            "src/Zeven/**.cpp",
            "src/Zeven/**.h",
            "src/Zeven/**.c",
        }

    project "Babonet"
        kind "StaticLib"
        language "C++"

        includedirs {
            "src",
            "src/Babonet",
        }

        files {
            "src/Babonet/**.hpp",
            "src/Babonet/**.cpp",
            "src/Babonet/**.h",
            "src/Babonet/**.c",
        }

    project "bv2"
        kind "WindowedApp"
        language "C++"
        targetdir "."
        debugdir "Content"

        defines {
            "SDL_MAIN_HANDLED",
            "_PRO_",
            "_MINIBOT_"
        }

        configuration {"Debug"}
            links {
                "SDL2d",
            }

        configuration {"Release"}
            links {
                "SDL2"
            }

        configuration {}
            links {
                "Babonet",
                "Zeven",
                "opengl32",
                "libcurl"
            }

        includedirs {
            "src/Babonet",
            "src/bv2/External",
            "src/bv2/Game",
            "src/bv2/Game/AStar",
            "src/bv2/Game/Master",
            "src/bv2/Menu",
            "src/bv2/Menu2",
            "src/bv2/Menu2/Dialogs",
            "src/bv2/Source",
            "src/bv2/Weather",
            "src",
            "thirdparty/glad/include",
            "thirdparty/imgui",
            "thirdparty/imgui/examples",
            "thirdparty/sqlite",
        }

        files {
            "src/bv2/**.hpp",
            "src/bv2/**.cpp",
            "src/bv2/**.h",
            "src/bv2/**.c",
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

