project "DrawingPad"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "pwpch.h"
    pchsource "src/pwpch.cpp"

    files {
        "src/**.h",
        "src/**.cpp",
        "vendor/glm/glm/**.hpp",
        "vendor/glm/glm/**.inl",
    }


    defines {
        "_CRT_SECURE_NO_WARNINGS",
    	"GLFW_INCLUDE_NONE"
    }

    includedirs {
        "src",
        "vendor/spdlog/include",
        "%{IncludeDir.glad}",
        "%{IncludeDir.glfw}",
        "%{IncludeDir.glm}",
    	"%{IncludeDir.VulkanSDK}"
    }

    links {
        "glfw",
        "glad",
        "%{Library.Vulkan}"
    }

    filter "system:windows"
        systemversion "latest"
        disablewarnings { "26812" }

    filter "configurations:Debug"
        defines "PW_DEBUG"
        runtime "Debug"
        symbols "on"

        links
        {
            "%{Library.ShaderC_Debug}",
            "%{Library.SPIRV_Cross_Debug}",
            "%{Library.SPIRV_Cross_GLSL_Debug}",
        }

    filter "configurations:Release"
        defines "PW_RELEASE"
        runtime "Release"
        optimize "on"

        links
        {
            "%{Library.ShaderC_Release}",
            "%{Library.SPIRV_Cross_Release}",
            "%{Library.SPIRV_Cross_GLSL_Release}",
        }

    filter "configurations:Dist"
        defines "PW_DIST"
        runtime "Release"
        optimize "on"

        links
        {
            "%{Library.ShaderC_Release}",
            "%{Library.SPIRV_Cross_Release}",
            "%{Library.SPIRV_Cross_GLSL_Release}",
        }
