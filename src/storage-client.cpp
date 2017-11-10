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

#include <glog/logging.h>

#include "storage-client.hpp"

using std::ifstream;

using ChCppUtils::mkPath;
using ChCppUtils::directoryListing;
using ChCppUtils::fileExpired;
using ChCppUtils::fileExists;

namespace SC {

StorageClient::StorageClient(Config *config) {
	mConfig = config;
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
}

void StorageClient::_onFile(string name, string ext, string path, void *this_) {
	StorageClient *client = (StorageClient *) this_;
	client->onFile(name, ext, path);
}

void StorageClient::onFile(string name, string ext, string path) {
	upload(name, ext, path);
}

void StorageClient::_onLoad(HttpRequestLoadEvent *event, void *this_) {
	StorageClient *client = (StorageClient *) this_;
	client->onLoad(event);
}

void StorageClient::onLoad(HttpRequestLoadEvent *event) {
	LOG(INFO) << "Request Complete";
}

void StorageClient::_onTimerEvent(TimerEvent *event, void *this_) {

}

void StorageClient::onTimerEvent(TimerEvent *event) {

}

void StorageClient::upload(string name, string ext, string path) {
	LOG(INFO) << "File to be uploaded: name: " << name << ", path: " << path;

	std::ifstream file(path, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer(size);
	if (file.read(buffer.data(), size))
	{
		LOG(INFO) << "Read " << size << " bytes";
		string url = uploadPrefix + "/" + name;
		LOG(INFO) << "Target URL: " << url;
		HttpRequest *request = new HttpRequest();
		request->onLoad(StorageClient::_onLoad).bind(this);
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
