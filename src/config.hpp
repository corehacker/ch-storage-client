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
#include <ch-cpp-utils/utils.hpp>
#include <ch-cpp-utils/config.hpp>

#ifndef SRC_CONFIG_HPP_
#define SRC_CONFIG_HPP_

using std::vector;
using std::string;

namespace SC {

class WatchDir {
public:
	string dir;
	bool upload;

	WatchDir();

	WatchDir(string dir, bool upload);
};

class Config : public ChCppUtils::Config {
private:
	vector<WatchDir> watchDirs;
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

	vector<char *> mCameraCaptureChars;
	vector<char *> mCameraEncodeChars;

	bool populateConfigValues();
public:
	Config();
	~Config();
	void init();

	vector<WatchDir> &getWatchDirs();
	vector<string> &getFilters();
	string &getHostname();
	uint16_t getPort();
	string &getPrefix();
	string &getName();
	uint32_t getPurgeTtlSec();
	uint32_t getPurgeIntervalSec();
	bool isCameraEnabled();
	string &getPipeFile();
	vector<string> &getCameraCapture();
	vector<string> &getCameraEncode();
	vector<char *> &getCameraCaptureChars();
	vector<char *> &getCameraEncodeChars();
	bool hasCameraCaptureCharsPtrs();
	bool hasCameraEncodeCharsPtrs();
	char **getCameraCaptureCharsPtrs();
	char **getCameraEncodeCharsPtrs();
};

} // End namespace SC.


#endif /* SRC_CONFIG_HPP_ */
