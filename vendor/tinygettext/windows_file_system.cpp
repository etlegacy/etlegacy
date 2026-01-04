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

#include "tinygettext/windows_file_system.hpp"

#include <Windows.h>
#include <fstream>
#include <string>

namespace tinygettext {

WindowsFileSystem::WindowsFileSystem()
{
}

std::vector<std::string>
WindowsFileSystem::open_directory(const std::string& pathname)
{
	std::vector<std::string> files;
	WIN32_FIND_DATA          findData;
	HANDLE                   dir = FindFirstFile((pathname + "\\*").c_str(), &findData);

	if (dir == INVALID_HANDLE_VALUE)
	{
		return files;
	}

	do
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL & ~FILE_ATTRIBUTE_DIRECTORY)
		{
			files.push_back(findData.cFileName);
		}
	}
	while (FindNextFile(dir, &findData) != 0);

	FindClose(dir);
	return files;
}

#if __cplusplus >= 201103L // C++11
std::unique_ptr<std::istream>
WindowsFileSystem::open_file(const std::string& filename)
{
	return std::unique_ptr<std::istream>(new std::ifstream(filename.c_str()));
}
#else
std::auto_ptr<std::istream>
WindowsFileSystem::open_file(const std::string& filename)
{
	return std::auto_ptr<std::istream>(new std::ifstream(filename.c_str()));
}
#endif
} // namespace tinygettext

/* EOF */
