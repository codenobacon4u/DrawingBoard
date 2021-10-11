workspace "DrawingPad"
architecture "x64"
startproject "Sandbox"

configurations {
    "Debug",
    "Release",
    "Dist"
}

flags {
    "MultiProcessorCompile"
}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

--Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["glfw"] = "DrawingPad/vendor/glfw/include"
IncludeDir["GLAD"] = "DrawingPad/vendor/glad/include"
IncludeDir["ImGui"] = "DrawingPad/vendor/imgui"
IncludeDir["glm"] = "DrawingPad/vendor/glm"
IncludeDir["spdlog"] = "DrawingPad/vendor/spdlog/include"
IncludeDir["stb_image"] = "DrawingPad/vendor/stb_image"
IncludeDir["Vulkan"] = "C:/VulkanSDK/1.2.176.1/Include"
IncludeDir["SPIRV"] = "DrawingPad/vendor/spirv"

group "Dependencies"
include "DrawingPad/vendor/glfw"
include "DrawingPad/vendor/glad"
include "DrawingPad/vendor/imgui"
group ""

project "DrawingPad"
location "DrawingPad"
kind "StaticLib"
language "C++"
cppdialect "C++17"
staticruntime "on"

targetdir("bin/" .. outputdir .. "/%{prj.name}")
objdir("bin-int/" .. outputdir .. "/%{prj.name}")

pchheader "pwpch.h"
pchsource "DrawingPad/src/pwpch.cpp"

files {
    "%{prj.name}/src/**.h",
    "%{prj.name}/src/**.cpp",
    "%{prj.name}/vendor/glm/glm/**.hpp",
    "%{prj.name}/vendor/glm/glm/**.inl",
    "%{prj.name}/vendor/spirv/**.h",
    "%{prj.name}/vendor/spirv/**.hpp",
    "%{prj.name}/vendor/spirv/**.cpp",
}


defines {
    "_CRT_SECURE_NO_WARNINGS",
	"GLFW_INCLUDE_NONE",
	"SPIRV_CROSS_C_API_HLSL",
	"SPIRV_CROSS_C_API_GLSL"
}

includedirs {
    "%{prj.name}/src",
    "%{IncludeDir.spdlog}",
    "%{IncludeDir.glfw}",
    "%{IncludeDir.GLAD}",
    "%{IncludeDir.ImGui}",
    "%{IncludeDir.glm}",
    "%{IncludeDir.stb_image}",
	"%{IncludeDir.Vulkan}",
	"%{IncludeDir.SPIRV}",
}

libdirs {
	"C:/VulkanSDK/1.2.176.1/Lib"
}

links {
    "glfw",
    "GLAD",
    "ImGui",
	"vulkan-1.lib",
    "shaderc_shared.lib",
}

filter "system:windows"
systemversion "latest"
toolset "v142"
disablewarnings { "26812" }

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

project "Sandbox"
location "Sandbox"
kind "ConsoleApp"
language "C++"
cppdialect "C++17"
staticruntime "on"

targetdir("bin/" .. outputdir .. "/%{prj.name}")
objdir("bin-int/" .. outputdir .. "/%{prj.name}")

files {
    "%{prj.name}/src/**.h",
    "%{prj.name}/src/**.cpp"
}

includedirs {
    "DrawingPad/src",
    "DrawingPad/vendor",
    "%{IncludeDir.glm}",
    "%{IncludeDir.glfw}",
    "%{IncludeDir.GLAD}",
	"%{IncludeDir.Vulkan}",
	"%{IncludeDir.SPIRV}",
	"%{IncludeDir.stb_image}"
}

libdirs {
	"C:/VulkanSDK/1.2.176.1/Lib"
}

links {
    "DrawingPad",
	"vulkan-1.lib",
    "shaderc_shared.lib",
}


filter "system:linux"
pic "on"

links {
	"glfw",
	"GLAD",
    "SPIRV",
    "Xrandr",
    "Xi",
    "GL",
	"dl",
	"pthread",
    "X11"
}

filter "system:windows"
systemversion "latest"
toolset "v142"

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

project "UISandbox"
location "UISandbox"
kind "ConsoleApp"
language "C++"
cppdialect "C++17"
staticruntime "on"

targetdir("bin/" .. outputdir .. "/%{prj.name}")
objdir("bin-int/" .. outputdir .. "/%{prj.name}")

files {
    "%{prj.name}/src/**.h",
    "%{prj.name}/src/**.cpp"
}

includedirs {
    "DrawingPad/src",
    "DrawingPad/vendor",
    "%{IncludeDir.glm}",
    "%{IncludeDir.glfw}",
    "%{IncludeDir.GLAD}",
	"%{IncludeDir.Vulkan}",
	"%{IncludeDir.SPIRV}",
	"%{IncludeDir.stb_image}"
}

libdirs {
	"C:/VulkanSDK/1.2.176.1/Lib"
}

links {
    "DrawingPad",
	"vulkan-1.lib",
    "shaderc_shared.lib",
}


filter "system:linux"
pic "on"

links {
	"glfw",
	"GLAD",
    "SPIRV",
    "Xrandr",
    "Xi",
    "GL",
	"dl",
	"pthread",
    "X11"
}

filter "system:windows"
systemversion "latest"
toolset "v142"

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