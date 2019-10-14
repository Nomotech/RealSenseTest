#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <time.h>
#include <windows.h>

#include <opencv2/opencv.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>

#include "../utility/shader.hpp"


clock_t clk = clock();
boolean flag = true;
boolean recodeFlag = false;
int captureFrame = 0;
const int WIDTH = 848;
const int HEIGHT = 480;
int counter = 0;
int timer = 0;
int type = 0;  // draw type

void keyFunCB(GLFWwindow* window, int key, int scancode, int action, int mods) {
	printf("keyFunCB %d %d %d %d\n", key, scancode, action, mods);
	if (key >= 0 + 48 && key <= 9 + 48) type = key - 48;
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			flag = false;
			break;
		case GLFW_KEY_R:
			recodeFlag = !recodeFlag;
			break;
		}
	}
}

int main() {
	cv::Mat depthImg = cv::Mat::ones(HEIGHT, WIDTH, CV_16U);

	std::thread captureThr([&] {
		rs2::colorizer color_map;
		rs2::pipeline pipe;
		rs2::config conf;
		//conf.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, 30);
		// conf.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 90);
		conf.enable_stream(RS2_STREAM_DEPTH, WIDTH, HEIGHT, RS2_FORMAT_Z16, 90);

		rs2::hole_filling_filter hole_filling(1);
		rs2::decimation_filter dec_filter;  // Decimation - reduces depth frame density
		rs2::spatial_filter spat_filter;    // Spatial    - edge-preserving spatial smoothing
		rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise

		// Configure filter parameters
    	dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 3);
	    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.55f);

		rs2::pipeline_profile profile = pipe.start(conf);

		rs2_intrinsics intrinsics = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>().get_intrinsics();
		std::stringstream ss;
		ss	<< std::setw(10) << "Width" << ": " << intrinsics.width << std::endl 
			<< std::setw(10) << "Height" << ": " << intrinsics.height << std::endl
			<< std::setw(10) << "PPX" << ": " << std::setprecision(15) << intrinsics.ppx << std::endl
			<< std::setw(10) << "PPY" << ": " << std::setprecision(15) << intrinsics.ppy << std::endl
			<< std::setw(10) << "Fx" << ": " << std::setprecision(15) << intrinsics.fx << std::endl
			<< std::setw(10) << "Fy" << ": " << std::setprecision(15) << intrinsics.fy << std::endl
			<< std::setw(10) << "coef[0]" << ": " << std::setprecision(15) << intrinsics.coeffs[0] << std::endl
			<< std::setw(10) << "coef[1]" << ": " << std::setprecision(15) << intrinsics.coeffs[1] << std::endl
			<< std::setw(10) << "coef[2]" << ": " << std::setprecision(15) << intrinsics.coeffs[2] << std::endl
			<< std::setw(10) << "coef[3]" << ": " << std::setprecision(15) << intrinsics.coeffs[3] << std::endl
			<< std::setw(10) << "coef[4]" << ": " << std::setprecision(15) << intrinsics.coeffs[4] << std::endl
			<< std::setw(10) << "Distortion" << ": " << rs2_distortion_to_string(intrinsics.model) << std::endl;
			std::cout << ss.str() << std::endl;


		rs2::device selected_device = profile.get_device();
		auto depth_sensor = selected_device.first<rs2::depth_sensor>();

		if (depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED)) {
			depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f); // Enable emitter
			// depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0.f); // Disable emitter
		}

		//rs2::video_stream_profile depth_stream = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
		rs2::frameset frames;


		while (flag) {

			if (pipe.poll_for_frames(&frames)) {
				counter++;
				captureFrame++;

				rs2::frame depth_frame = frames.first(RS2_STREAM_DEPTH);
				//depth_frame.get_data();
				depth_frame = hole_filling.process(depth_frame);
				//depth_frame = dec_filter.process(depth_frame);
				// depth_frame = spat_filter.process(depth_frame);
				// depth_frame = temp_filter.process(depth_frame);
				
				const int w = depth_frame.as<rs2::video_frame>().get_width();
				const int h = depth_frame.as<rs2::video_frame>().get_height();

				// depthImg = cv::Mat(cv::Size(w, h), CV_8UC3, (void*)depth_frame.apply_filter(color_map).get_data(), cv::Mat::AUTO_STEP);
				depthImg = cv::Mat(cv::Size(w, h), CV_16U, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);
				if (recodeFlag) {
					std::stringstream ss;
					ss << "../data/201910140422/" << counter << ".png";
					std::vector<int> params(2);
					params[0] = cv::IMWRITE_PNG_COMPRESSION;
					params[1] = 0;
					cv::imwrite(ss.str(), depthImg, params);
				}


			}
		}
	});

	std::thread viewThr([&] {
		static const GLfloat vertex[] = {
			-1.0f,	-1.0f,	0.0f,
			 1.0f,	-1.0f,	0.0f,
			 1.0f,	 1.0f,	0.0f,
			-1.0f,	 1.0f,	0.0f
		};

		// -----------------------------------------< setup opengl >-----------------------------------------
		if (glfwInit() == GL_FALSE) {
			std::cerr << "Can't initilize GLFW" << std::endl;
			return 1;
		}

		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* const window = glfwCreateWindow(WIDTH, HEIGHT, "depth", NULL, NULL);
		if (window == nullptr) {
			std::cerr << "Can't create GLFW window." << std::endl;
			getchar();
			glfwTerminate();
			return 1;
		}
		glfwMakeContextCurrent(window);

		// Initialize GLEW
		glewExperimental = true;  // Needed for core profile
		if (glewInit() != GLEW_OK) {
			fprintf(stderr, "Failed to initialize GLEW\n");
			getchar();
			glfwTerminate();
			return -1;
		}

		// Ensure we can capture the escape key being pressed below
		glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		// texture
		const int textureNum = 3;
		GLuint textures[textureNum];
		glGenTextures(textureNum, textures);

		for (int i = 0; i < textureNum; i++) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glBindTexture(GL_TEXTURE_2D, textures[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, WIDTH, HEIGHT, 0, GL_RED, GL_SHORT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// -----------------------------------------< compute shader >-----------------------------------------
//		GLuint demosaicProgramID = LoadCompShader("./shader/demosaic.comp");
//		GLuint phaseShiftProgramID = LoadCompShader("./shader/phaseShift.comp");

		// -----------------------------------------< output shader >-----------------------------------------
		GLuint imgViewProgramID = LoadShaders("./shader/simple.vert", "./shader/view.frag");
		GLuint VertexArrayID;
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);


		// attribute
		GLuint vertexID;
		glGenBuffers(1, &vertexID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

		// uniform
		GLuint resolutionID = glGetUniformLocation(imgViewProgramID, "resolution");
		GLuint timeID = glGetUniformLocation(imgViewProgramID, "time");
		GLuint textureID = glGetUniformLocation(imgViewProgramID, "image");
		GLuint typeID = glGetUniformLocation(imgViewProgramID, "type");

		glViewport(0, 0, WIDTH, HEIGHT);
		glfwSwapInterval(0);

		glfwSetKeyCallback(window, keyFunCB);

		// -----------------------------------------< loop >-----------------------------------------

		while (flag) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(imgViewProgramID);
			// input
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, WIDTH, HEIGHT, 0, GL_RED, GL_SHORT, depthImg.data);
			//			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RED_INTEGER, GL_SHORT, depthImg.data);

			glUniform1i(textureID, 0);
			glUniform2f(resolutionID, WIDTH, HEIGHT);
			glUniform1i(timeID, timer);
			glUniform1i(typeID, recodeFlag);

			// attribute
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexID);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			glDisableVertexAttribArray(0);

			// Swap buffers
			glfwSwapBuffers(window);
			glfwPollEvents();
			//Sleep(1000 / 60);

			cv::imshow("Depth", depthImg);
			char key = cv::waitKey(1);
		}
		});

	while (flag) {
		clock_t now = clock();
		double time = double(now - clk) / CLOCKS_PER_SEC;
		if (time > 1.0) {
			std::cout << "tracking : " << captureFrame / time << " fps " << std::endl;
			clk = now;
			captureFrame = 0;
		}
		Sleep(100);
	}
	captureThr.join();
	viewThr.join();
}
