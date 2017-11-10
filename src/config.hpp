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
#include "../third-party/json/json.hpp"
#include <ch-cpp-utils/timer.hpp>
#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/fs-watch.hpp>
#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/http-request.hpp>
#include <ch-cpp-utils/timer.hpp>

#ifndef SRC_CONFIG_HPP_
#define SRC_CONFIG_HPP_

using std::vector;
using std::string;
using namespace std::chrono;

using json = nlohmann::json;

using ChCppUtils::FsWatch;
using ChCppUtils::Fts;
using ChCppUtils::FtsOptions;
using ChCppUtils::Timer;
using ChCppUtils::TimerEvent;

using ChCppUtils::Http::Client::HttpRequest;
using ChCppUtils::Http::Client::HttpRequestLoadEvent;

namespace SC {

class Config {
private:
	string etcConfigPath;
	string localConfigPath;
	string selectedConfigPath;

	vector<string> watchDirs;
	vector<string> filters;

	string hostname;
	uint16_t port;
	string prefix;
	string name;

	uint32_t mPurgeTtlSec;
	uint32_t mPurgeIntervalSec;

	bool mCameraEnable;
	string mPipeFile;
	vector<string> mCameraCapture;
	vector<string> mCameraEncode;

	bool selectConfigFile();
	bool validateConfigFile();
public:
	json mJson;

	Config();
	~Config();
	void init();

	vector<string> &getWatchDirs();
	vector<string> &getFilters();
	string &getHostname();
	uint16_t getPort();
	string &getPrefix();
	string &getName();
	uint32_t getPurgeTtlSec();
	uint32_t getPurgeIntervalSec();
};

} // End namespace SC.


#endif /* SRC_CONFIG_HPP_ */
