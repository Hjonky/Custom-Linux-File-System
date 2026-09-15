#include "myfs.h"
#include <string.h>
#include <iostream>
#include <math.h>
#include <sstream>

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_):blkdevsim(blkdevsim_) 
{
	struct myfs_header header;
	blkdevsim->read(0, sizeof(header), (char *)&header);

	if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
	    (header.version != CURR_VERSION)) 
		{
		std::cout << "Did not find myfs instance on blkdev" << std::endl;
		std::cout << "Creating..." << std::endl;
		format();
		std::cout << "Finished!" << std::endl;
	}
}

void MyFs::parse_path(std::string path, std::string &parent, std::string &name) 
{
	size_t pos = 0;
	pos = path.rfind("/");
	if (pos != std::string::npos) 
	{
		parent = path.substr(0, pos);
		name = path.substr(pos + 1);
	}
	else 
	{
		name = path;
	}
}

void MyFs::format() 
{

	// put the header in place
	struct myfs_header header;
	strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
	header.version = CURR_VERSION;
	blkdevsim->write(0, sizeof(header), (const char*)&header);

	struct myfs_entry table[MAX_ENTRIES] = {};
	blkdevsim->write(sizeof(myfs_header), sizeof(table), (const char*)&table);

}

void MyFs::create_file(std::string path_str, bool directory) 
{
	std::string parent = "/";
	std::string name;

	parse_path(path_str, parent, name);

	struct myfs_entry entry;
	int i = 0;
	for (i = 0; i < MAX_ENTRIES; i++) 
	{
		blkdevsim->read(sizeof(myfs_header) + i * sizeof(myfs_entry), sizeof(entry), (char*)&entry);
		if (entry.name[0] == 0) 
		{
			break;
		}
	}

	if (i == MAX_ENTRIES) throw std::runtime_error("Run Out Of Memory");

	strncpy(entry.parent, parent.c_str(), sizeof(entry.parent));
	strncpy(entry.name, name.c_str(), sizeof(entry.name));
	entry.is_dir = directory;
	
	entry.addr = CONTENT_START + i * MAX_FILE_SIZE;

	blkdevsim->write(sizeof(myfs_header) + i * sizeof(myfs_entry), sizeof(entry), (char*)&entry);
}

std::string MyFs::get_content(std::string path_str) 
{
	std::string parent = "/";
	std::string name;

	parse_path(path_str, parent, name);

	struct myfs_entry entry;
	int i = 0;

	for (i = 0; i < MAX_ENTRIES; i++) 
	{
		blkdevsim->read(sizeof(myfs_header) + i * sizeof(myfs_entry), sizeof(entry), (char*)&entry);
		if (strncmp(entry.parent, parent.c_str(), sizeof(entry.parent)) == 0 && strncmp(entry.name, name.c_str(), sizeof(entry.name)) == 0)
		{
			break;
		}
	}
	if (i == MAX_ENTRIES) throw std::runtime_error("File Not Found");

	char data[entry.file_size + 1] = {};
	blkdevsim->read(entry.addr, entry.file_size, data);
	data[entry.file_size] = 0;
	std::string file_contents(data);

	return file_contents;
}

void MyFs::set_content(std::string path_str, std::string content) 
{
	std::string parent = "/";
	std::string name;

	parse_path(path_str, parent, name);

	struct myfs_entry entry;
	int i = 0;

	for (i = 0; i < MAX_ENTRIES; i++) 
	{
		blkdevsim->read(sizeof(myfs_header) + i * sizeof(myfs_entry), sizeof(entry), (char*)&entry);
		if (strncmp(entry.parent, parent.c_str(), sizeof(entry.parent)) == 0 && strncmp(entry.name, name.c_str(), sizeof(entry.name)) == 0)
		{
			break;
		}
	}
	if (i == MAX_ENTRIES) throw std::runtime_error("File Not Found");

	blkdevsim->write(entry.addr, strlen(content.c_str()), content.c_str());

	entry.file_size = strlen(content.c_str());

	blkdevsim->write(sizeof(myfs_header) + i * sizeof(myfs_entry), sizeof(entry), (char*)&entry);
}

MyFs::dir_list MyFs::list_dir(std::string path_str) 
{
	if (path_str.size() > 1 && path_str.back() == '/')
    	path_str.pop_back();

	dir_list ans;

	int i = 0;
	for (i = 0; i < MAX_ENTRIES; i++) 
	{
		struct myfs_entry entry;
		blkdevsim->read(sizeof(myfs_header) + i * sizeof(myfs_entry), sizeof(entry), (char*)&entry);

		if ((entry.name[0]) != 0 && strcmp(entry.parent, path_str.c_str()) == 0)
		{
			struct dir_list_entry dir_entry = { std::string(entry.name), entry.is_dir, entry.file_size };

			ans.emplace_back(dir_entry);
		}
	}

	return ans;
}

