/*
 * camera-capture.hpp
 *
 *  Created on: Nov 9, 2017
 *      Author: corehacker
 */

#include "config.hpp"

#ifndef SRC_CAMERA_CAPTURE_HPP_
#define SRC_CAMERA_CAPTURE_HPP_

namespace SC {

class CameraCapture {
private:
	Config *mConfig;
public:
	CameraCapture(Config *config);
	~CameraCapture();
};

} // End namespace SC.


#endif /* SRC_CAMERA_CAPTURE_HPP_ */
