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
 * \file   storage-client.hpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/
#include <vector>
#include <string>
#include <ch-cpp-utils/timer.hpp>
#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/fs-watch.hpp>
#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/http/client/http.hpp>
#include <ch-cpp-utils/timer.hpp>
#include <ch-cpp-utils/proc-stat.hpp>

#include "config.hpp"

#ifndef SRC_STORAGE_CLIENT_HPP_
#define SRC_STORAGE_CLIENT_HPP_

using std::vector;
using std::string;
using std::unordered_map;
using namespace std::chrono;

using json = nlohmann::json;

using ChCppUtils::FsWatch;
using ChCppUtils::OnFileData;
using ChCppUtils::Fts;
using ChCppUtils::FtsOptions;
using ChCppUtils::Timer;
using ChCppUtils::TimerEvent;
using ChCppUtils::ProcStat;

using ChCppUtils::Http::Client::HttpRequest;
using ChCppUtils::Http::Client::HttpRequestLoadEvent;

namespace SC {

class StorageClient;

class UploadContext {
public:
	UploadContext(StorageClient *client, HttpRequest *request);
	~UploadContext();
	StorageClient *getClient();
	HttpRequest *getRequest();
private:
	StorageClient *client;

	HttpRequest *request;
};

class StorageClient {
private:
	Config *mConfig;
	vector<Fts *> mFts;
	vector<FsWatch *> mFsWatch;
	Timer *mTimer;
	TimerEvent *mTimerEvent;
	string uploadPrefix;
	vector<OnFileData> uploadQueue;
	ProcStat *procStat;
	unordered_map<string, bool> mWatchDirMap;

	bool shouldUpload(OnFileData &data);

	static void _onFile(OnFileData &data, void *this_);
	void onFile(OnFileData &data);

	static void _onLoad(HttpRequestLoadEvent *event, void *this_);
	void onLoad(HttpRequestLoadEvent *event);

	static void _onFilePurge (OnFileData &data, void *this_);
	void onFilePurge (OnFileData &data);

	static void _onFilePurgeForced (OnFileData &data, void *this_);
	void onFilePurgeForced (OnFileData &data);

	void purge(bool force);

	static void _onTimerEvent(TimerEvent *event, void *this_);
	void onTimerEvent(TimerEvent *event);

	void upload(OnFileData &data, string &infValue, string &targetDuration);
public:
	StorageClient(Config *config);
	~StorageClient();
	void start();
};

} // End namespace SC.


#endif /* SRC_STORAGE_CLIENT_HPP_ */
