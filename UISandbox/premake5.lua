project "UISandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs {
        "%{wks.location}/DrawingPad/src",
        "%{wks.location}/DrawingPad/vendor",
        "%{wks.location}/DrawingPad/vendor/spdlog/include",
        "%{IncludeDir.glad}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.imgui}",
    	"%{IncludeDir.stb_image}",
        "%{wks.location}/Sandbox/vendor/tiny_obj_loader"
    }

    links {
        "DrawingPad",
        "glfw",
        "imgui"
    }

    filter "system:linux"
        pic "on"

        links {
            "Xrandr",
            "Xi",
            "GL",
        	"dl",
        	"pthread",
            "X11"
        }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines "PW_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "PW_RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "PW_DIST"
        runtime "Release"
        optimize "on"