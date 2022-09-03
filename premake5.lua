include "./vendor/premake/premake_customization/solution_items.lua"
include "dependencies.lua"

workspace "DrawingPad"
    architecture "x64"
    startproject "UISandbox"

    configurations {
        "Debug",
        "Release",
        "Dist"
    }

    solution_items {
        ".editorconfig"
    }

    flags {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    include "vendor/premake"
    include "DrawingPad/vendor/glfw"
    include "DrawingPad/vendor/glad"
    include "DrawingPad/vendor/imgui"
group ""

include "DrawingPad"
include "Sandbox"
include "UISandbox"
include "Editor"