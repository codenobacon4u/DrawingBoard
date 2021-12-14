include "dependencies.lua"

workspace "DrawingPad"
    architecture "x64"
    startproject "UISandbox"

    configurations {
        "Debug",
        "Release",
        "Dist"
    }

    flags {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    include "DrawingPad/vendor/glfw"
    include "DrawingPad/vendor/glad"
    include "DrawingPad/vendor/imgui"
group ""

include "DrawingPad"
include "Sandbox"
include "UISandbox"