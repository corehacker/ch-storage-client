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
 * \file   main.cpp
 *
 * \author Sandeep Prakash
 *
 * \date   Nov 02, 2017
 *
 * \brief
 *
 ******************************************************************************/

#include <stdlib.h>
#include <signal.h>
#include <csignal>
#include <iostream>
#include <sys/wait.h>
#include <event2/event.h>
#include <glog/logging.h>
#include <ch-cpp-utils/utils.hpp>

#include "storage-client.hpp"

using ChCppUtils::directoryListing;

using SC::StorageClient;
using SC::Config;

static Config *config = nullptr;
static StorageClient *client = nullptr;

static void initEnv();
static void deinitEnv();
static void initClient();

static void initClient() {
	client = new StorageClient(config);
	client->start();
}

static void initEnv() {
	config = new Config();
	config->init();

	// Initialize Google's logging library.
	if(config->shouldLogToConsole()) {
		LOG(INFO) << "LOGGING to console.";
	} else {
		LOG(INFO) << "Not LOGGING to console.";
		google::InitGoogleLogging("ch-storage-server");
	}

	initClient();
}

static void deinitEnv() {
	LOG(INFO) << "Stopping client...";
//	client->stop();
	LOG(INFO) << "Stopped client...";
	delete client;
	LOG(INFO) << "Deleted client...";
	delete config;
	LOG(INFO) << "Deleted config...";
}

int main(int argc, char **argv) {
	initEnv();

	if(config->shouldRunForever()) {
		THREAD_SLEEP_FOREVER;
	} else {
		THREAD_SLEEP(config->getRunFor());
	}


	deinitEnv();

	return 0;
}


