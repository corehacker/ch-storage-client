#!/bin/bash

PWD=`pwd`

LOG_FILE="$PWD/install.log"
TEMP_DIR="$PWD/temp"
INSTALLATION_TARGET="/usr/local"

LIBEVENT_DOWNLOAD="https://github.com/libevent/libevent/releases/download/release-2.1.8-stable/libevent-2.1.8-stable.tar.gz"
GLOG_GIT_DOWNLOAD="https://github.com/google/glog.git"
GLOG_GIT_TAG="v0.4.0"
GPERFTOOLS_GIT_DOWNLOAD="https://github.com/gperftools/gperftools.git"
GPERFTOOLS_GIT_TAG="gperftools-2.7"
CH_CPP_UTILS_GIT_DOWNLOAD="https://github.com/corehacker/ch-cpp-utils.git"
CH_STORAGE_CLIENT_GIT_DOWNLOAD="https://github.com/corehacker/ch-storage-client.git"

function cleanup {
  sudo rm -rf $TEMP_DIR ltmain.sh
  echo "----------------------------------------------------------------------------" >> $LOG_FILE
  echo "                        Ending Installation" >> $LOG_FILE
  echo "----------------------------------------------------------------------------" >> $LOG_FILE
  log "----------------------------------------------------------------------------"
  log "                        Ending Installation"
  log "----------------------------------------------------------------------------"
}

function log {
  echo "[$0] $1"
}

function init {
  mkdir -p $TEMP_DIR && cd $TEMP_DIR

  echo "" > $LOG_FILE
  echo "----------------------------------------------------------------------------" >> $LOG_FILE
  echo "                        Starting Installation" >> $LOG_FILE
  echo "----------------------------------------------------------------------------" >> $LOG_FILE
  log "----------------------------------------------------------------------------"
  log "                        Starting Installation"
  log "----------------------------------------------------------------------------"

  echo "Temporary Dir   : $TEMP_DIR"
  echo "Installation Dir: $INSTALLATION_TARGET"
}

function configure_make_make_install {
  log "[$1] configuring..." && \
  ./configure --prefix=$INSTALLATION_TARGET &>> $LOG_FILE && \
  log "[$1] make..." && \
  make -j8 &>> $LOG_FILE && \
  log "[$1] make install..." && \
  sudo make -j8 install &>> $LOG_FILE
}

function remove_existing_libevent {
  if apt list --installed | grep libevent; then
    log "[libevent] Already installed through apt. Uninstalling..."
    uninstall_package=`apt list --installed | grep libevent | cut -d'/' -f1`
    sudo apt -y remove $uninstall_package &>> $LOG_FILE
    log "[libevent] Already installed through apt. Uninstalled."
  else
    log "[libevent] No libevent installation found through apt."
  fi
}

function install_libevent {

  remove_existing_libevent  

  if ldconfig -p | grep libevent; then
    log "[libevent] Already installed based on ldconfig."
  elif test -f /usr/local/lib/libevent.so; then
    log "[libevent] Already installed @ /usr/local/lib/libevent.so."
    ls -lh /usr/local/lib/libevent.so
  else
    log "[libevent] Downloading $LIBEVENT_DOWNLOAD..."
    wget -O libevent.tar.gz $LIBEVENT_DOWNLOAD &>> $LOG_FILE && \
    mkdir -p libevent && \
    log "[libevent] Untar libevent..."
    tar xf libevent.tar.gz -C libevent --strip-components=1 && \
    cd libevent && \
    configure_make_make_install "libevent" && \
    cd ..
  fi
}

function install_glog {
  if ldconfig -p | grep libglog; then
    log "[glog] Already installed."
  else
    log "[glog] Cloning $GLOG_GIT_DOWNLOAD..." && \
    git clone $GLOG_GIT_DOWNLOAD && \
    cd glog && \
    log "[glog] Checkout $GLOG_GIT_TAG..." && \
    git checkout $GLOG_GIT_TAG
    log "[glog] Running autogen.sh..." && \
    ./autogen.sh
    log "[glog] Running autogen.sh..." && \
    ./autogen.sh && configure_make_make_install "glog" && \
  cd ..
  fi
}

function install_gperftools {
  if ldconfig -p | grep libtcmalloc; then
    log "[gperftools] Already installed."
  else
    log "[gperftools] Cloning $GPERFTOOLS_GIT_DOWNLOAD..." && \
    git clone $GPERFTOOLS_GIT_DOWNLOAD && \
    cd gperftools && \
    log "[gperftools] Checkout $GPERFTOOLS_GIT_TAG..." && \
    git checkout $GPERFTOOLS_GIT_TAG 
    log "[gperftools] Running autogen.sh..." && \
    ./autogen.sh
    log "[gperftools] Running autogen.sh..." && \
    ./autogen.sh && configure_make_make_install "gperftools" && \
    cd ..
  fi
}

function install_ch_cpp_utils {
  log "[ch-cpp-utils] Cloning $CH_CPP_UTILS_GIT_DOWNLOAD..." && \
  git clone $CH_CPP_UTILS_GIT_DOWNLOAD && \
  cd ch-cpp-utils && \
  log "[ch-cpp-utils] Running autogen.sh..." && \
  ./autogen.sh && configure_make_make_install "ch-cpp-utils" && \
  cd ..
}

function install_ch_storage_client {
  log "[ch-storage-client] Cloning $CH_STORAGE_CLIENT_GIT_DOWNLOAD..." && \
  git clone $CH_STORAGE_CLIENT_GIT_DOWNLOAD && \
  cd ch-storage-client && \
  log "[ch-storage-client] Running autogen.sh..." && \
  ./autogen.sh && configure_make_make_install "ch-storage-client" && \
  cd ..
}

cleanup

init


# install_libevent
# install_glog
# install_gperftools
# install_ch_cpp_utils
# install_ch_storage_client

install_libevent && install_glog && install_gperftools && install_ch_cpp_utils && install_ch_storage_client

cleanup

