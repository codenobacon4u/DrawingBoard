#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace DrawingPad
{
	class Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }
	private:
		static std::shared_ptr<spdlog::logger> s_Logger;
	};
}

#define DP_TRACE(...) ::DrawingPad::Log::GetLogger()->trace(__VA_ARGS__)
#define DP_INFO(...)  ::DrawingPad::Log::GetLogger()->info(__VA_ARGS__)
#define DP_WARN(...)  ::DrawingPad::Log::GetLogger()->warn(__VA_ARGS__)
#define DP_ERROR(...) ::DrawingPad::Log::GetLogger()->error(__VA_ARGS__)
#define DP_CRITICAL(...) ::DrawingPad::Log::GetLogger()->critical(__VA_ARGS__)
