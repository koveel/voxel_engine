project "GLFW"
	kind "StaticLib"
	language "C"
	cdialect "C17"
    staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.c",
	}

	includedirs
	{
		"include"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"_GLFW_WIN32"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"