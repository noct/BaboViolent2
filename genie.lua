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
        "Symbols"
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

    project "ZevenCore"
        kind "StaticLib"
        language "C++"

        includedirs {
            "src",
        }

        files {
            "src/Zeven/*.h",
            "src/Zeven/Core/**.hpp",
            "src/Zeven/Core/**.cpp",
            "src/Zeven/Core/**.h",
            "src/Zeven/Core/**.c",
        }

        excludes {
            "src/Zeven/Gfx.h"
        }

    project "ZevenGfx"
        kind "StaticLib"
        language "C++"

        includedirs {
            "src",
            "thirdparty/glad/include",
            "thirdparty/imgui",
            "thirdparty/imgui/examples",
        }

        files {
            "src/Zeven/Gfx.h",
            "src/Zeven/Gfx/**.hpp",
            "src/Zeven/Gfx/**.cpp",
            "src/Zeven/Gfx/**.h",
            "src/Zeven/Gfx/**.c",
        }

    project "BaboNet"
        kind "StaticLib"
        language "C++"

        includedirs {
            "src",
            "src/BaboNet",
        }

        files {
            "src/BaboNet/**.hpp",
            "src/BaboNet/**.cpp",
            "src/BaboNet/**.h",
            "src/BaboNet/**.c",
        }

    project "BaboCommon"
        kind "StaticLib"
        language "C++"

        includedirs {
            "src",
            "src/Common",
            "src/Common/External",
            "src/BaboNet",
            "thirdparty/sqlite",
        }

        files {
            "src/Common/**.hpp",
            "src/Common/**.cpp",
            "src/Common/**.h",
            "src/Common/**.c",
            "thirdparty/sqlite/sqlite3.c",
        }

    project "bv2"
        kind "WindowedApp"
        language "C++"
        targetdir "."
        debugdir "."

        defines {
            "SDL_MAIN_HANDLED",
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
                "BaboNet",
                "BaboCommon",
                "ZevenCore",
                "ZevenGfx",
                "opengl32",
                "libcurl"
            }

        includedirs {
            "src/BaboNet",
            "src/Common",
            "src/Common/External",
            "src/Client",
            "src/Client/Menu",
            "src/Client/Menu2",
            "src/Client/Menu2/Dialogs",
            "src/Client/Weather",
            "src",
            "thirdparty/glad/include",
            "thirdparty/imgui",
            "thirdparty/imgui/examples",
            "thirdparty/sqlite",
        }

        files {
            "src/Client/**.hpp",
            "src/Client/**.cpp",
            "src/Client/**.h",
            "src/Client/**.c",
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
            "thirdparty/glad/src/glad.c",
        }


    project "bv2server"
        kind "ConsoleApp"
        language "C++"
        targetdir "."
        debugdir "Content"

        defines {
            "SDL_MAIN_HANDLED",
            "DEDICATED_SERVER"
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
                "BaboNet",
                "BaboCommon",
                "ZevenCore",
                "libcurl"
            }

        includedirs {
            "src/BaboNet",
            "src/Common",
            "src/Common/External",
            "src",
            "thirdparty/sqlite",
        }

        files {
            "src/Server/**.hpp",
            "src/Server/**.cpp",
            "src/Server/**.h",
            "src/Server/**.c",
        }

