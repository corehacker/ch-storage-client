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
#include <string>
#include <glog/logging.h>

#include "config.hpp"

using std::ifstream;

namespace SC {

Config::Config() :
		ChCppUtils::Config("/etc/ch-storage-client/ch-storage-client.json",
				"./config/ch-storage-client.json") {
	hostname = "localhost";
	port = 8888;
	prefix = "/";
	name = "storage-client";

	mPurgeIntervalSec = 60;
	mPurgeTtlSec= 60;
	mCameraEnable = false;
}

Config::~Config() {
	LOG(INFO) << "*****************~Config";
}

void Config::init() {
	ChCppUtils::Config::init();

	populateConfigValues();
}

bool Config::populateConfigValues() {
	LOG(INFO) << "<-----------------------Config";

	name = mJson["name"];
	LOG(INFO) << "name : " << name;

	hostname = mJson["server"]["hostname"];
	LOG(INFO) << "server.hostname : " << hostname;

	port = mJson["server"]["port"];
	LOG(INFO) << "server.port : " << port;

	prefix = mJson["server"]["prefix"];
	LOG(INFO) << "server.prefix : " << prefix;

	mPurgeTtlSec = mJson["purge"]["ttl-s"];
	LOG(INFO) << "purge.ttl-s: " << mPurgeTtlSec;

	mPurgeIntervalSec = mJson["purge"]["interval-s"];
	LOG(INFO) << "purge.interval-s: " << mPurgeIntervalSec;

	auto const watches = mJson.find("watch");
	for (auto const& watch : *watches) {
		WatchDir watchDir(watch["dir"].get<string>(), watch.value("upload", false));
		watchDirs.emplace_back(watchDir);
		LOG(INFO) << "watch[\"" << watchDir.dir << "\"]: " << (watchDir.upload ? "true" : "false");
	}

	for(string filter : mJson["filters"]) {
		LOG(INFO) << "Filter: " << filter;
		filters.emplace_back(filter);
	}

	mCameraEnable= mJson["camera"]["enable"];
	if(mCameraEnable) {
		mPipeFile = mJson["camera"]["pipe"];
		for(string command : mJson["camera"]["commands"]["capture"]) {
			mCameraCapture.emplace_back(command);
		}

		for(string command : mJson["camera"]["commands"]["encode"]) {
			mCameraEncode.emplace_back(command);
		}

		// 1 extra for nullptr.
		mCameraCaptureChars.reserve(mCameraCapture.size() + 1);
		for(string str : mCameraCapture) {
			mCameraCaptureChars.push_back(const_cast<char*>(str.c_str()));
		}
		mCameraCaptureChars.push_back(const_cast<char*>((char *) nullptr));

		// 1 extra for nullptr.
		mCameraEncodeChars.reserve(mCameraEncode.size() + 1);
		for(string str : mCameraEncode) {
			mCameraEncodeChars.push_back(const_cast<char*>(str.c_str()));
		}
		mCameraEncodeChars.push_back(const_cast<char*>((char *) nullptr));
	}

	LOG(INFO) << "----------------------->Config";
	return true;
}

vector<WatchDir> &Config::getWatchDirs() {
	return watchDirs;
}

vector<string> &Config::getFilters() {
	return filters;
}

string &Config::getHostname() {
	return hostname;
}

uint16_t Config::getPort() {
	return port;
}

string &Config::getPrefix() {
	return prefix;
}

string &Config::getName() {
	return name;
}

uint32_t Config::getPurgeTtlSec() {
	return mPurgeTtlSec;
}

uint32_t Config::getPurgeIntervalSec() {
	return mPurgeIntervalSec;
}

bool Config::isCameraEnabled() {
	return mCameraEnable;
}

string &Config::getPipeFile() {
	return mPipeFile;
}

vector<string> &Config::getCameraCapture() {
	return mCameraCapture;
}

vector<string> &Config::getCameraEncode() {
	return mCameraEncode;
}

vector<char *> &Config::getCameraCaptureChars() {
	return mCameraCaptureChars;
}

vector<char *> &Config::getCameraEncodeChars() {
	return mCameraEncodeChars;
}

bool Config::hasCameraCaptureCharsPtrs() {
	return (mCameraCaptureChars.size() ? true : false);
}

bool Config::hasCameraEncodeCharsPtrs() {
	return (mCameraEncodeChars.size() ? true : false);
}

char **Config::getCameraCaptureCharsPtrs() {
	return (mCameraCaptureChars.size() ? &mCameraCaptureChars[0] : nullptr);
}

char **Config::getCameraEncodeCharsPtrs() {
	return (mCameraEncodeChars.size() ? &mCameraEncodeChars[0] : nullptr);
}

WatchDir::WatchDir() {
	dir = "";
	upload = false;
}

WatchDir::WatchDir(string dir, bool upload) {
	this->dir = dir;
	this->upload = upload;
}

} // End namespace SS.
