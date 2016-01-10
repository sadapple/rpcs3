#pragma once

namespace vfs
{
	// Convert PS3/PSV path to fs-compatible path
	std::string get(const std::string& vpath);
}
