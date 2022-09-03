#include "dppch.h"
#include "Log.h"

#include "spdlog/sinks/stdout_sinks.h"

namespace DrawingPad {

	std::shared_ptr<spdlog::logger> Log::s_Logger;

	void Log::Init() {
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_Logger = spdlog::stdout_color_mt("DRAWINGPAD");
		s_Logger->set_level(spdlog::level::trace);
	}
}
