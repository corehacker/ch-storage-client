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
#include <glog/logging.h>

#include "storage-client.hpp"
#include "camera-capture.hpp"

using SC::StorageClient;
using SC::Config;
using SC::CameraCapture;

static Config *config = nullptr;
static StorageClient *client = nullptr;

static void initEnv();
static void deinitEnv();

static void initClient();
static void initEncode();
static void initCapture();

static void initClient() {
	client = new StorageClient(config);
	client->start();
}

static void initEncode() {
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		LOG(ERROR) << "Forking encode process failed. Errno: " << errno;
		exit(1);
	} else if (pid > 0) {
		LOG(INFO) << "Forking encode process success. In parent: " << getpid();
		initClient();

		int status;
		waitpid(pid, &status, 0);
		LOG(INFO) << "Encode process exited. In parent: " << getpid();
	} else {
		LOG(INFO) << "Forking encode process success. In child: " << getpid();
		char **args = config->getCameraEncodeCharsPtrs();
		if (execvp(*args, args) < 0) {     /* execute the command  */
			perror("execvp-encode");
			LOG(ERROR) << "Encode process exec failed. In child: " << getpid();
			exit(1);
		}
		LOG(INFO) << "Encode process exiting. In child: " << getpid();
	}
}

static void initCapture() {

	int fifo = mkfifo(config->getPipeFile().data(), S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if(fifo < 0) {
		if(EEXIST != errno) {
			perror("mkfifo");
			LOG(ERROR) << "Creating fifo file " << config->getPipeFile() <<
					" failed. Errno: " << errno;
			exit(1);
		} else {
			LOG(ERROR) << "Fifo file " << config->getPipeFile() <<
				" already exists.";
		}
	}
	LOG(ERROR) << "Creating fifo file " << config->getPipeFile() <<
			" success.";

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		LOG(ERROR) << "Forking capture process failed. Errno: " << errno;
		exit(1);
	} else if (pid > 0) {
		LOG(INFO) << "Forking capture process success. In parent: " << getpid();
		int status;

		initEncode();

		waitpid(pid, &status, 0);
		LOG(INFO) << "Capture process exited. In parent: " << getpid();
	} else {
		LOG(INFO) << "Forking capture process success. In child: " << getpid();
		char **args = config->getCameraCaptureCharsPtrs();
		if (execvp(*args, args) < 0) {     /* execute the command  */
			perror("execvp-capture");
			LOG(ERROR) << "Capture process exec failed. In child: " << getpid();
			exit(1);
		}
		LOG(INFO) << "Capture process exiting. In child: " << getpid();
	}
}

static void initEnv() {
	config = new Config();
	config->init();

	if (config->isCameraEnabled() && config->hasCameraCaptureCharsPtrs()
			&& config->hasCameraEncodeCharsPtrs()) {
		initCapture();
	} else {
		initClient();
	}
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

	THREAD_SLEEP_FOREVER;

	deinitEnv();

	return 0;
}


