#!/bin/bash

PWD=`pwd`

TEMP_DIR="$PWD/temp"
INSTALLATION_TARGET="$TEMP_DIR/target"

rm -rf $TEMP_DIR

mkdir -p $TEMP_DIR && cd $TEMP_DIR

echo "Temporary Dir   : $TEMP_DIR"
echo "Installation Dir: $INSTALLATION_TARGET"

function install_libevent {
  wget https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz && \
  tar xvf libevent-2.1.8-stable.tar.gz && \
  cd libevent-2.1.8-stable && \
  ./configure --prefix=$INSTALLATION_TARGET && make && sudo make install && \
  cd ..
}

function install_glog {
  git clone https://github.com/google/glog.git && \
  cd glog && \
  git checkout v0.4.0
  ./autogen.sh
  ./autogen.sh && ./configure && make && sudo make install && \
  cd ..
}

function install_gperftools {
  git clone https://github.com/gperftools/gperftools.git && \
  cd gperftools && \
  git checkout gperftools-2.7
  ./autogen.sh
  ./autogen.sh && ./configure && make && sudo make install && \
  cd ..
}

function install_ch_cpp_utils {
  git clone https://github.com/corehacker/ch-cpp-utils.git && \
  cd ch-cpp-utils && \
  ./autogen.sh && ./configure && make && sudo make install && \
  cd ..
}

function install_ch_storage_client {
  git clone https://github.com/corehacker/ch-storage-client.git && \
  cd ch-storage-client && \
  ./autogen.sh && ./configure && make && sudo make install && \
  cd ..
}

#install_libevent
#install_glog
#install_gperftools
#install_ch_cpp_utils


install_libevent && install_glog && install_gperftools && install_ch_cpp_utils && install_ch_storage_client