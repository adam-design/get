#include "UniStoreRepo.hpp"
#include "../Utils.hpp"
#include "constants.h"
#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include <iomanip>
#include <regex>
#include <sstream>
#include <map>

using namespace rapidjson;

std::vector<std::unique_ptr<Package>> UniStoreRepo::loadPackages()
{
	std::vector<std::unique_ptr<Package>> result;
	std::string directoryUrl = this->url;

	std::string response;
	bool success = downloadFileToMemory(directoryUrl, &response);

	// attempt fallback to http in case of https repos failure
	// TODO: Abstract this out, and allow to be optional
	if (!success && (this->url.rfind("https", 0) == 0))
	{
		printf("--> Attempting http fallback for https repo \"%s\" after loading failure\n", this->name.c_str());

		// update repo url
		this->url.replace(0, 5, "http");
		directoryUrl = this->url;

		// retry fetch
		success = downloadFileToMemory(directoryUrl, &response);
	}

	if (!success)
	{
		printf("--> Could not update repository metadata for \"%s\" repo!\n", this->name.c_str());
		this->loaded = false;
		return {};
	}

	if (libget_status_callback != nullptr)
	{
		libget_status_callback(STATUS_UPDATING_STATUS, 1, 1);
	}

	// extract out packages, append to package list
	Document doc;
	ParseResult ok = doc.Parse(response.c_str());

	if (!ok || !doc.IsArray())
	{
		printf("--> Invalid format in downloaded data for %s\n", this->url.c_str());
		this->loaded = false;
		return {};
	}

	const Value& packages_doc = doc.GetArray();

	// for every package in the repo
	auto total = (int32_t)packages_doc.Size();
	for (int i = 0; i < total; i++)
	{
		if (networking_callback != nullptr)
			networking_callback(nullptr, total, i + 1, 0, 0);
		
		std::vector<std::string> keys;
		auto start = packages_doc[i].GetObj().MemberBegin();
		auto end = packages_doc[i].GetObj().MemberEnd();
		for (auto it = start; it != end; ++it) {
			keys.push_back(it->name.GetString());
		}

		auto& cur = packages_doc[i];

		if (cur.HasMember("systems")) {
			// if "3DS" isn't in the systems array, skip this package
			// TODO: multi-platform support (WiiU wants vWii, 3DS wants DS, etc)
			const Value& systems = cur["systems"];
			bool found = false;
			std::string plat3ds = "3DS";
			for (SizeType i = 0; i < systems.Size(); i++) {
				if (plat3ds == systems[i].GetString()) {
					found = true;
					break;
				}
			}
			if (!found)
				continue;
		}

		auto package = std::make_unique<Package>(GET);

		if (cur.HasMember("slug")) {
			package->pkg_name = cur["slug"].GetString();
		}
		if (cur.HasMember("title")) {
			package->title = cur["title"].GetString();
			if (package->pkg_name.empty())
				package->pkg_name = package->title; // fallback
		}
		if (cur.HasMember("author"))
			package->author = cur["author"].GetString();
		if (cur.HasMember("description")) {
			package->short_desc = cur["description"].GetString();
			package->long_desc = package->short_desc;
		}
		if (cur.HasMember("version"))
			package->version = cur["version"].GetString();
		if (cur.HasMember("license"))
			package->license = cur["license"].GetString();
		if (cur.HasMember("releasenotes"))
			package->changelog = cur["releasenotes"].GetString();
		if (cur.HasMember("category")) {
			// category is an array of strings in the JSON, we'll take just the first one
			const Value& category = cur["category"];
			if (category.Size() > 0)
				package->category = category[0].GetString();
		}
		if (cur.HasMember("icon")) {
			package->iconUrl = cur["icon"].GetString();
			// printf("Icon URL: %s\n", package->iconUrl.c_str());
		}
		if (cur.HasMember("source")) {
			package->sourceUrl = cur["source"].GetString();
		}
		if (cur.HasMember("updated")) {
			package->updated = cur["updated"].GetString();
			struct tm tm {};

			// convert 2019-12-27T20:44:30Z to timestamp
			auto res = my_strptime(package->updated.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm);
			if (res)
			{
				// make sure that all the time-related fields are set
				tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
				package->updated_timestamp = (int)mktime(&tm);

				// also update the label to a simpler y-m-d format
				std::stringstream ss;
				ss << std::put_time(&tm, "%Y-%m-%d");
				package->updated = ss.str();
			}
		}

		// TODO: screen shots (see below commented out method)
		// TODO: address needing to follow unistore script instructions properly

		bool found = false;
		// For now, we're only going to take a .zip, then a .3dsx, then a .cia
		if (cur.HasMember("downloads")) {
			std::vector<std::string> endings = { ".zip", ".3dsx", ".cia" };
			for (auto& ending : endings)
			{
				auto cur2 = cur["downloads"].GetObj();
				auto start = cur2.MemberBegin();
				auto end = cur2.MemberEnd();
				for (auto it = start; it != end; ++it) {
					// get the object for this key
					std::string name = it->name.GetString();
					const Value& obj = cur2[name.c_str()];
					if (obj.HasMember("url") && obj.HasMember("size")) {
						std::string url = obj["url"].GetString();
						auto size = obj["size"].GetInt();
						if (name.ends_with(ending)) {
							package->url = url;
							package->download_size = size;
							found = true;
							break;
						}
					}
				}
				if (found)
					break;
			}
		}

		if (!found) {
			printf("No valid URL for %s\n", package->pkg_name.c_str());
			continue;
		}
		
		result.push_back(std::move(package));

	}
	return result;
}

std::string UniStoreRepo::getType() const
{
	return "unistore";
}

std::string UniStoreRepo::getZipUrl(const Package& package) const
{
	// OSC packages just use the url field directly
	// printf("Package URL: %s\n", package.url.c_str());
	return package.url;
}

std::string UniStoreRepo::getIconUrl(const Package& package) const
{
	// OSC packages just use the url field directly
	return package.iconUrl;
}

// std::string Package::getScreenShotUrl(int count) const
// {
// 	// TODO: parse screenshots and count from unistore repo
// 	return "";
// }