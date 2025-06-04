workspace "Engine"
	architecture "x86_64"
	startproject "App"

	configurations
	{
		"Debug",
		"Release",
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "third_party"
	include "Engine/third_party/msdf-atlas-gen"
	include "Engine/third_party/glad"
	include "Engine/third_party/GLFW"
group ""

project "Engine"
	location "Engine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "Engine/src/pch.cpp"
	characterset "unicode"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/src/core",
		"%{prj.name}/third_party",
		"%{prj.name}/third_party/glad/include",
		"%{prj.name}/third_party/glfw/include",

		"%{prj.name}/third_party/msdf-atlas-gen/msdfgen",
		"%{prj.name}/third_party/msdf-atlas-gen/msdf-atlas-gen",
	}

	filter "system:windows"
		systemversion "latest"
		characterset "MBCS"

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		runtime "Release"
		optimize "on"

project "App"
	location "App"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	characterset "unicode"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/resources/shaders/**.hlsl",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

	includedirs
	{
		"Engine/src",
		"Engine/src/core",
		"Engine/third_party",
		"%{prj.name}/src",
		"%{prj.name}/third_party"
	}

	links
	{
		"Engine",
		"msdf-atlas-gen",
		"glad",
		"GLFW",
	}

	filter "system:windows"
		systemversion "latest"
		characterset "MBCS"

	filter "configurations:Debug"
		defines "ENGINE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "ENGINE_RELEASE"
		runtime "Release"
		optimize "on"
