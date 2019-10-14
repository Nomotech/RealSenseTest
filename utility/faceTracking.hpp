#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>

struct ROI {
	double x = 0.0;
	double y = 0.0;
	int top		= 0;
	int bottom	= 0;
	int left	= 0;
	int right	= 0;
	double vX = 0.0;
	double vY = 0.0;
};


class FaceTrack {
private:
    ROI roi;
    int minW = 70;
    int minH = 70;
    

    cv::CascadeClassifier cascade;
    std::vector<cv::Rect> faces;

    rs2::colorizer color_map;
    rs2::pipeline pipe;
	rs2::pipeline_profile profile = pipe.start();
	rs2::video_stream_profile depth_stream = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
	rs2_intrinsics depth_intr = depth_stream.get_intrinsics();
public:
	float offset[3] = { 0.0, 0.0, 0.0 };
	cv::Mat colorImg;
	cv::Mat depthImg;
	float pos[3];
    FaceTrack(int minw, int minh);
    int track();
};