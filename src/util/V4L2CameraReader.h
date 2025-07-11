#pragma once

#include <opencv2/opencv.hpp>
#include "util/Undistort.h"
#include "util/globalCalib.h"

namespace dso {

class V4L2CameraReader {
public:
    V4L2CameraReader(const std::string &device,
                     const std::string &calibFile,
                     const std::string &gammaFile,
                     const std::string &vignetteFile)
    {
        cap.open(device, cv::CAP_V4L2);
        if(!cap.isOpened()) {
            printf("Cannot open camera %s\n", device.c_str());
            exit(1);
        }
        undistort = Undistort::getUndistorterForFile(calibFile, gammaFile, vignetteFile);
        widthOrg = undistort->getOriginalSize()[0];
        heightOrg = undistort->getOriginalSize()[1];
        cap.set(cv::CAP_PROP_FRAME_WIDTH, widthOrg);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, heightOrg);
    }

    ~V4L2CameraReader()
    {
        cap.release();
        delete undistort;
    }

    void setGlobalCalibration()
    {
        int w_out, h_out;
        Eigen::Matrix3f K;
        undistort->getCalibMono(K, w_out, h_out);
        setGlobalCalib(w_out, h_out, K);
    }

    ImageAndExposure* getImage(double timestamp)
    {
        cv::Mat frame;
        if(!cap.read(frame)) return 0;
        if(frame.cols != widthOrg || frame.rows != heightOrg)
            cv::resize(frame, frame, cv::Size(widthOrg, heightOrg));
        cv::Mat gray;
        if(frame.channels() == 3)
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        else
            gray = frame;
        MinimalImageB imgWrap(gray.cols, gray.rows, gray.data);
        return undistort->undistort<unsigned char>(&imgWrap, 1.0f, timestamp);
    }

    float* getPhotometricGamma()
    {
        return undistort->photometricUndist ? undistort->photometricUndist->getG() : 0;
    }

private:
    cv::VideoCapture cap;
    Undistort* undistort;
    int widthOrg;
    int heightOrg;
};

} // namespace dso

