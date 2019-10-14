
#include "faceTracking.hpp"

FaceTrack::FaceTrack(int minw, int minh) {
    // setup cascade 
    cascade.load("../Utility/haarcascades/haarcascade_frontalface_alt.xml");
    


    rs2::frameset data = pipe.wait_for_frames();
    rs2::align align_to_depth(RS2_STREAM_DEPTH);
    data = align_to_depth.process(data);
    rs2::depth_frame  depth = data.get_depth_frame();
    
    int centerX = depth.as<rs2::video_frame>().get_width() / 2.0;
    int centerY = depth.as<rs2::video_frame>().get_height() / 2.0;
    minW = minw;
    minH = minh;

    roi = {
        double(centerX),
        double(centerY),
        centerY - minH,
        centerY + minH,
        centerX - minW,
        centerX + minW
    };
}

int FaceTrack::track() {
    int status = 0;
    rs2::frameset data = pipe.wait_for_frames();
    rs2::align align_to_depth(RS2_STREAM_DEPTH);
    data = align_to_depth.process(data);

    rs2::depth_frame  depth = data.get_depth_frame();
    rs2::video_frame  color = data.get_color_frame();

    // Query frame size (width and height)
    const int w = color.as<rs2::video_frame>().get_width();
    const int h = color.as<rs2::video_frame>().get_height();

    colorImg = cv::Mat(cv::Size(w, h), CV_8UC3, (void*)color.get_data(), cv::Mat::AUTO_STEP);
    depthImg = cv::Mat(cv::Size(w, h), CV_8UC3, (void*)depth.apply_filter(color_map).get_data(), cv::Mat::AUTO_STEP);
    cv::cvtColor(colorImg, colorImg, cv::COLOR_BGR2RGB);


    cv::Mat clipped(colorImg, cv::Rect(roi.left, roi.top, roi.right - roi.left, roi.bottom - roi.top));

    cascade.detectMultiScale(clipped, faces, 1.1, 3, 0, cv::Size(minW, minH));


    rectangle(depthImg, cv::Point(roi.left, roi.top), cv::Point(roi.right, roi.bottom), cv::Scalar(255, 0, 0), 2);
    if (faces.size() == 0) {
        status = 0;
        float r = 10.0;
        roi.vX = 0.0;
        roi.vY = 0.0;
        roi.top = std::max(int(roi.top - r), 0);
        roi.bottom = std::min(int(roi.bottom + r), h);
        roi.left = std::max(int(roi.left - r), 0);
        roi.right = std::min(int(roi.right + r), w);
        if (roi.bottom - roi.top >= h) cv::waitKey(100);
    } else {
        status = 1;
        // face detection
        double x = roi.left + faces[0].x + faces[0].width / 2;
        double y = roi.top + faces[0].y + faces[0].height / 2;

        roi.vX = x - roi.x;
        roi.vY = y - roi.y;
        roi.x = x;
        roi.y = y;


        float pixel[2] = { float(roi.x), float(roi.y) };
        float d = depth.get_distance(roi.x, roi.y);
        rs2_deproject_pixel_to_point(pos, &depth_intr, pixel, d);
        pos[0] = pos[0] * -1000.0 * 3.0 / 2.0 + offset[0];
        pos[1] = pos[1] * -1000.0 * 3.0 / 2.0 + offset[1];
        pos[2] = pos[2] * 1000.0 + offset[2];
        

        // print 
        cv::Point p0 = cv::Point(roi.left + faces[0].x, roi.top + faces[0].y);
        cv::Point p1 = cv::Point(roi.left + faces[0].x + faces[0].width, roi.top + faces[0].y + faces[0].height);
        rectangle(depthImg, p0, p1, cv::Scalar(0, 255, 0), 2);

        std::stringstream ss;
        ss << "(" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")";
        cv::putText(depthImg, ss.str(), p0, cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(100, 255, 100), 2);


        // update roi
        roi.top = std::max(int(roi.y - faces[0].height * 0.6 + std::min(roi.vY * 2.0, 0.0)), 0);
        roi.bottom = std::min(int(roi.y + faces[0].height * 0.6 + std::max(roi.vY * 2.0, 0.0)), h);
        roi.left = std::max(int(roi.x - faces[0].width * 0.6 + std::min(roi.vX * 2.0, 0.0)), 0);
        roi.right = std::min(int(roi.x + faces[0].width * 0.6 + std::max(roi.vX * 2.0, 0.0)), w);
    }
    return status;
}
