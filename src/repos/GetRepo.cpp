#include "GetRepo.hpp"
#include "../Utils.hpp"
#include "constants.h"
#include "rapidjson/document.h"
#include <regex>
#include <sstream>
#include <iomanip>

using namespace rapidjson;

std::vector<std::unique_ptr<Package>> GetRepo::loadPackages()
{
	std::vector<std::unique_ptr<Package>> result;
	std::string directoryUrl = this->url + "/repo.json";

	// fetch current repository json
	std::string response;
	bool success = downloadFileToMemory(directoryUrl, &response);

#ifdef NETWORK_MOCK
	mockPopulatePackages(&response);
#endif

	// attempt fallback to http in case of https repos failure
	if (!success && (this->url.rfind("https", 0) == 0))
	{
		printf("--> Attempting http fallback for https repo \"%s\" after loading failure\n", this->name.c_str());

		// update repo url
		this->url.replace(0, 5, "http");
		directoryUrl = this->url + "/repo.json";

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
		libget_status_callback(STATUS_UPDATING_STATUS, 1, 1);

	// extract out packages, append to package list
	Document doc;
	ParseResult ok = doc.Parse(response.c_str());

	if (!ok || !doc.IsObject() || !doc.HasMember("packages"))
	{
		printf("--> Invalid format in downloaded repo.json for %s\n", this->url.c_str());
		this->loaded = false;
		return {};
	}

	const Value& packages_doc = doc["packages"];

	// for every repo
	auto total = packages_doc.Size();
	for (int i = 0; i < (int)total; i++)
	{
		if (networking_callback != nullptr)
			networking_callback(nullptr, total, i + 1, 0, 0);

		auto package = std::make_unique<Package>(GET);

		// TODO: use arrays and loops for parsing this info, and also check the type first

		auto& cur = packages_doc[i];

		// mostly essential attributes
		if (cur.HasMember("name"))
		{
			package->pkg_name = cur["name"].GetString();
		}
		else
		{
			printf("Missing name for package on repo, skipping\n");
			continue;
		}
		if (cur.HasMember("title"))
			package->title = cur["title"].GetString();
		else
			package->title = package->pkg_name;

		if (cur.HasMember("author"))
			package->author = cur["author"].GetString();
		if (cur.HasMember("description"))
			package->short_desc = cur["description"].GetString();
		if (cur.HasMember("details"))
			package->long_desc = std::regex_replace(cur["details"].GetString(), std::regex("\\\\n"), "\n");
		if (cur.HasMember("version"))
			package->version = cur["version"].GetString();

		// more information and details
		if (cur.HasMember("license"))
			package->license = cur["license"].GetString();
		if (cur.HasMember("changelog"))
			package->changelog = std::regex_replace(cur["changelog"].GetString(), std::regex("\\\\n"), "\n");
		if (cur.HasMember("url"))
		{
			package->url = cur["url"].GetString();
			package->sourceUrl = package->url;
		}
		if (cur.HasMember("updated"))
		{
			package->updated = cur["updated"].GetString();
			struct tm tm {};

			auto res = my_strptime(package->updated.c_str(), "%d/%m/%Y", &tm);
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

		// even more details
		if (cur.HasMember("app_dls"))
			package->downloads += cur["app_dls"].GetInt();
		if (cur.HasMember("web_dls"))
			package->downloads += cur["web_dls"].GetInt();
		if (cur.HasMember("extracted"))
			// convert KB to bytes
			package->extracted_size += cur["extracted"].GetInt() * 1000;
		if (cur.HasMember("filesize"))
			package->download_size += cur["filesize"].GetInt() * 1000; // ibid

		if (cur.HasMember("category"))
			package->category = cur["category"].GetString();
		if (cur.HasMember("binary"))
			package->binary = cur["binary"].GetString();
		if (cur.HasMember("screens"))
			package->screens = cur["screens"].GetInt();

		result.push_back(std::move(package));
	}
	return result;
}

std::string GetRepo::getType() const
{
	return "get";
}

std::string GetRepo::getZipUrl(const Package& package) const
{
	// Get packages are in the /zips folder under the package name
	return this->url + "/zips/" + package.getPackageName() + ".zip";
}

std::string GetRepo::getIconUrl(const Package& package) const
{
	// Get icons are also just in the /packages folder
	return this->url + "/packages/" + package.getPackageName() + "/icon.png";
}
