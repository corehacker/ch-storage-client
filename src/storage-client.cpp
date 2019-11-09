/*******************************************************************************
 *
 *  BSD 2-Clause License
 *
 *  Copyright (c) 2017, Sandeep Prakash
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * Copyright (c) 2017, Sandeep Prakash <123sandy@gmail.com>
 *
 * \file   storage-client.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <chrono>
#include <cstdio>
#include <dirent.h>

#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/http/client/http.hpp>
#include <glog/logging.h>

#include "storage-client.hpp"

using std::ifstream;
using std::chrono::system_clock;

using ChCppUtils::mkPath;
using ChCppUtils::directoryListing;
using ChCppUtils::fileExpired;
using ChCppUtils::fileExists;
using ChCppUtils::getEpochNano;
using ChCppUtils::getDateTime;
using ChCppUtils::replace;

using ChCppUtils::Http::Client::HttpClientImpl;

namespace SC {

static int get_num_fds(void);

static int get_num_fds()
{
     int fd_count;
     char buf[64];
     struct dirent *dp;
       DIR *dir = NULL;

     snprintf(buf, 64, "/proc/%i/fd/", getpid());

     fd_count = 0;
     
       dir = opendir(buf);
     while ((dp = readdir(dir)) != NULL) {
          fd_count++;
     }
     closedir(dir);
     return fd_count;
}

UploadContext::UploadContext(StorageClient *client, HttpRequest *request) {
	this->client = client;
	this->request = request;
}

UploadContext::~UploadContext() {

}

StorageClient *UploadContext::getClient() {
	return client;
}

HttpRequest *UploadContext::getRequest() {
	return request;
}

StorageClient::StorageClient(Config *config) {
	mConfig = config;

	procStat = new ProcStat();

	purge(true);

	for(auto watch : mConfig->getWatchDirs()) {
		FtsOptions options = {false};
		options.bIgnoreRegularFiles = false;
		options.bIgnoreHiddenFiles = true;
		options.bIgnoreHiddenDirs = true;
		options.bIgnoreRegularDirs = true;
		options.filters = mConfig->getFilters();
		Fts *fts = new Fts(watch, &options);
		mFts.emplace_back(fts);

		FsWatch *fsWatch = new FsWatch(watch);
		mFsWatch.emplace_back(fsWatch);
	}

	uploadPrefix = "http://" + mConfig->getHostname() + ":" +
				std::to_string(mConfig->getPort()) + mConfig->getPrefix() + "/" +
				mConfig->getName();

	mTimer = new Timer();
	struct timeval tv = {0};
	tv.tv_sec = mConfig->getPurgeIntervalSec();
	mTimerEvent = mTimer->create(&tv, StorageClient::_onTimerEvent, this);
}

StorageClient::~StorageClient() {
	for(auto fts : mFts) {
		delete fts;
	}
	for(auto watch : mFsWatch) {
		delete watch;
	}
	mTimer->destroy(mTimerEvent);
	delete mTimer;
}

void StorageClient::_onFile(OnFileData &data, void *this_) {
	StorageClient *client = (StorageClient *) this_;
	client->onFile(data);
}

void StorageClient::onFile(OnFileData &data) {
	if(data.ext == "ts") {
		std::ifstream segmentFile(data.path);
		if(!segmentFile.is_open()) {
			LOG(WARNING) << "Segment file open failed. File may not exist. "
					"Might be deleted on startup which got detected by "
					"fs watch: " << data.path;
			return;
		}
		LOG(INFO) << "Segment file exists. Will add it to the queue: " <<
				data.path;
		uploadQueue.emplace_back(data);
	} else if(data.ext == "m3u8") {
		if(uploadQueue.size() == 0) {
			return;
		}
		std::vector<std::string> playlist;
		std::ifstream playlistFile(data.path);
		std::string line;

		while(std::getline(playlistFile, line)) {
			string filename = line.substr(line.find_last_of('/') + 1);
			LOG(INFO) << " + " << filename;
		    playlist.push_back(filename);
		}
		OnFileData tsData = uploadQueue.at(0);
		LOG(INFO) << "Queue head: " << tsData.name;
		bool found = false;
		string inf;
		string targetDuration;
		for(uint32_t i = 0; i < playlist.size(); i++) {
			string filename = playlist[i];

			if(filename.find("#EXT-X-TARGETDURATION") != string::npos) {
				targetDuration = filename.substr(filename.find_first_of(":") + 1);
			}

			if(filename == tsData.name) {
				found = true;
				inf = playlist[i - 1];
				break;
			}
		}
		if(found) {
			uploadQueue.erase(uploadQueue.begin());
			string infValue = inf.substr(inf.find_last_of(":") + 1);
			if(infValue.find_last_of(",") != string::npos) {
				infValue.erase(infValue.find_last_of(","), 1);
			}
			LOG(INFO) << "File found: " << tsData.name << ":" << infValue;
			upload(tsData, infValue, targetDuration);
		}
	} else {
	}
}

void StorageClient::_onLoad(HttpRequestLoadEvent *event, void *this_) {
	UploadContext *context = (UploadContext *) this_;
	StorageClient *client = context->getClient();
	client->onLoad(event);

	HttpRequest *request = context->getRequest();
	delete request;
	delete context;
}

void StorageClient::onLoad(HttpRequestLoadEvent *event) {
	LOG(INFO) << "Request Complete";
}

void StorageClient::_onFilePurge (OnFileData &data, void *this_) {
	StorageClient *client = (StorageClient *) this_;
	client->onFilePurge(data);
}

void StorageClient::onFilePurge (OnFileData &data) {
	bool markForDelete = fileExpired(data.path, mConfig->getPurgeTtlSec());
	if(markForDelete) {
		if(0 != std::remove(data.path.data())) {
			LOG(ERROR) << "File: " << data.path << ", marked for Delete! failed to delete";
			perror("remove");
		} else {
			LOG(INFO) << "File: " << data.path << ", marked for Delete! Deleted successfully";
		}
	}
}

void StorageClient::_onFilePurgeForced (OnFileData &data, void *this_) {
	StorageClient *client = (StorageClient *) this_;
	client->onFilePurgeForced(data);
}

void StorageClient::onFilePurgeForced (OnFileData &data) {
	if(0 != std::remove(data.path.data())) {
		LOG(ERROR) << "File: " << data.path << ", marked for Delete! failed to delete";
		perror("remove");
	} else {
		LOG(INFO) << "File: " << data.path << ", marked for Delete! Deleted successfully";
	}
}

void StorageClient::purge(bool force) {
	for(auto watchDir : mConfig->getWatchDirs()) {
		FtsOptions options;
//		LOG(INFO) << "Purging files in: " << watchDir;
		memset(&options, 0x00, sizeof(FtsOptions));
		options.bIgnoreRegularFiles = false;
		options.bIgnoreHiddenFiles = true;
		options.bIgnoreHiddenDirs = true;
		options.bIgnoreRegularDirs = true;
		Fts fts(watchDir, &options);
		if(!force) {
			fts.walk(StorageClient::_onFilePurge, this);
		} else {
			fts.walk(StorageClient::_onFilePurgeForced, this);
		}
	}
}

void StorageClient::_onTimerEvent(TimerEvent *event, void *this_) {
	StorageClient *server = (StorageClient *) this_;
	server->onTimerEvent(event);
}

void StorageClient::onTimerEvent(TimerEvent *event) {
	uint32_t rss = procStat->getRSS();
   LOG(INFO) << "Number of open fd(s): " << get_num_fds();
	LOG(INFO) << "RSS: " << rss / 1024 << " KB, Max RSS Configured: " << 
		(mConfig->getMaxRss() / 1024) << " KB";

	if(rss > mConfig->getMaxRss()) {
		LOG(ERROR) << "*********FATAL**********";
		LOG(ERROR) << "Too much memory in use. Exiting process.";
		LOG(FATAL) << "*********FATAL**********";
		exit(1);
	}


	purge(false);
	mTimer->restart(event);
}

void StorageClient::upload(OnFileData &data, string &infValue, string &targetDuration) {
	LOG(INFO) << "File to be uploaded: name: " << data.name << ", path: " << data.path;

	string targetName = getDateTime() + "-" + std::to_string(getEpochNano()) +
			"." + data.ext;
	std::replace(targetName.begin(), targetName.end(), ' ', '-');
	std::replace(targetName.begin(), targetName.end(), ':', '-');

	LOG(INFO) << "Will rename to: " << targetName;

	std::ifstream file(data.path, std::ios::binary | std::ios::ate);
	if(!file.is_open()) {
		LOG(WARNING) << "File open failed: " << data.path;
		return;
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		LOG(INFO) << "File content read " << size << " bytes";
		string url = uploadPrefix + "/" + targetName +
				"?extinf=" + infValue +
				"&targetduration=" + targetDuration;
		LOG(INFO) << "Target URL: " << url;
		HttpRequest *request = new HttpRequest();

		UploadContext *context = new UploadContext(this, request);
		request->onLoad(StorageClient::_onLoad).bind(context);
		request->open(EVHTTP_REQ_POST, url).send(buffer.data(), buffer.size());
	}
}

void StorageClient::start() {
	for(auto watch : mFsWatch) {
		watch->init();
		watch->OnNewFileCbk(StorageClient::_onFile, this);
		watch->start(mConfig->getFilters());
	}
	for(auto fts : mFts) {
		fts->walk(StorageClient::_onFile, this);
	}
}

} // End namespace SS.
