#ifndef UTILS_H
#define UTILS_H

#ifndef NETWORK_MOCK
#include <curl/curl.h>
#include <curl/easy.h>
#endif

#include <algorithm>
#include <stdio.h>
#include <string>

// the struct to be passed in the write function.
typedef struct
{
	uint8_t* data;
	size_t data_size;
	uint64_t offset;
	FILE* out;
} ntwrk_struct_t;

#define STATUS_DOWNLOADING 0
#define STATUS_INSTALLING 1
#define STATUS_REMOVING 2
#define STATUS_RELOADING 3
#define STATUS_UPDATING_STATUS 4
#define STATUS_ANALYZING 5

// folder stuff
bool mkpath(const std::string& path);
bool CreateSubfolder(std::string_view path);

// networking stuff
int init_networking();
int deinit_networking();
bool downloadFileToMemory(const std::string& path, std::string* buffer);				// writes to disk in BUF_SIZE chunks.
bool downloadFileToDisk(const std::string& remote_path, const std::string& local_path); // saves file to local_path.

#ifndef NETWORK_MOCK
void setPlatformCurlFlags(CURL* c);
#endif

// for cross platform dir creation
int my_mkdir(const std::string& path, int perms = 0700);
char* my_strptime(const char* s, const char* f, struct tm* tm);


// callback for networking progress
// if set, will be invoked during the download
extern int (*networking_callback)(void*, double, double, double, double);
extern int (*libget_status_callback)(int, int, int);
void setUserAgent(const char* data);

// helper methods
const char* plural(int amount);
void cp(const char* from, const char* to);

template <typename CharT>
inline std::basic_string<CharT> toLower(const std::basic_string<CharT>& str)
{
	std::basic_string<CharT> lower;
	std::transform(str.begin(), str.end(), std::back_inserter(lower), ::tolower);
	return lower;
}

int remove_empty_dirs(const char* name, int count);
bool libget_reset_data(const char* path);

std::string dir_name(const std::string& file_path);
bool compareLen(const std::string& a, const std::string& b);
bool is_dir(std::string_view path, struct dirent* ent);
std::string getHumanReadableBytes(uint64_t bytes);
#endif