include "dependencies.lua"

workspace "GK-Bezier"
	architecture "x86_64"
	startproject "Sandbox"

	configurations {
		"Debug",
		"Release",
		"Dist"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "kEn/kEn/vendor/glfw.lua"
	include "kEn/kEn/vendor/glad.lua"
	include "kEn/kEn/vendor/imgui.lua"
	include "kEn/kEn/vendor/assimp.lua"
group ""

include "kEn/kEn"
include "Bezier"

