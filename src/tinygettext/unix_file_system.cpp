// tinygettext - A gettext replacement that works directly on .po files
// Copyright (c) 2009 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "tinygettext/unix_file_system.hpp"

#if __cplusplus >= 201703L // C++17
#include <filesystem>
#include <fstream>
#include <stdlib.h>
#else
#include <sys/types.h>
#include <fstream>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#endif

namespace tinygettext {

UnixFileSystem::UnixFileSystem()
{
}

std::vector<std::string>
UnixFileSystem::open_directory(const std::string& pathname)
{
#if __cplusplus >= 201703L // C++17
	std::vector<std::string> files;
	for(auto const& p : std::filesystem::directory_iterator(pathname))
	{
		files.push_back(p.path().filename().string());
	}
	return files;
#else
	DIR *dir = opendir(pathname.c_str());
	if (!dir)
	{
		// FIXME: error handling
		return std::vector<std::string>();
	}
	else
	{
		std::vector<std::string> files;

		struct dirent *dp;
		while ((dp = readdir(dir)) != nullptr)
		{
			files.push_back(dp->d_name);
		}
		closedir(dir);

		return files;
	}
#endif
}

#if __cplusplus >= 201703L // C++17
	std::unique_ptr<std::istream>
	UnixFileSystem::open_file(const std::string& filename)
	{
		return std::unique_ptr<std::istream>(new std::ifstream(filename));
	}
#elif __cplusplus >= 201103L // C++11
	std::unique_ptr<std::istream>
	UnixFileSystem::open_file(const std::string& filename)
	{
		return std::unique_ptr<std::istream>(new std::ifstream(filename.c_str()));
	}
#else
	std::auto_ptr<std::istream>
	UnixFileSystem::open_file(const std::string& filename)
	{
		return std::auto_ptr<std::istream>(new std::ifstream(filename.c_str()));
	}
#endif

} // namespace tinygettext

/* EOF */
