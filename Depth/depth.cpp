#include <iostream>
#include <iostream>
#include <vector>
#include <thread>
#include <time.h>
#include <windows.h>

#include <opencv2/opencv.hpp>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>

#include "../utility/shader.hpp"
#include "../utility/file.hpp"


#define USE_REALSENSE 0

clock_t clk = clock();
boolean flag = true;
boolean capFlag = true;
int captureFrame = 0;
int processingFrame = 0;
int glFrame = 0;

const int WIDTH = 848;
const int HEIGHT = 480;
const float PI = 3.14159265;
int timer = 0;
int type = 0;  // draw type
float mx = 1, my = 1;

struct Mouse {
	double x = 0;
	double y = 0;
	double scroll = 0;
	double x_ = 0;	// drag start point
	double y_ = 0;	// drag start point
	double dx = 0;
	double dy = 0;
	boolean drag = false;
};
Mouse mouse;


void mouseButtonCB(GLFWwindow *window, int button, int action, int mods) {
	// printf("mouseButtonCB %d %d %d\n", button, action, mods);
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action) {
			mouse.drag = true;
			mouse.x_ = mouse.x;
			mouse.y_ = mouse.y;
		} else {
			mx += mouse.dx;
			my += mouse.dy;
			mouse.dx = mouse.dy = 0.0;
			mouse.drag = false;
		}
	}
}

void mousePosCB(GLFWwindow *window, double x, double y) {
	mouse.x = x;
	mouse.y = y;
	if (mouse.drag) {
		mouse.dx = (mouse.x - mouse.x_);
		mouse.dy = (mouse.y - mouse.y_);
	} else {
		mouse.dx = 0;
		mouse.dy = 0;
	}

 	//printf("mousePosCB %.1lf %.1lf : (%.1lf %.1lf)\n", x, y, mouseX, mouseY);
}

void mouseScrollCB(GLFWwindow *window, double x, double y) {
	mouse.scroll = y;
	//printf("mouseScrollCB %.1lf %.1lf : %.1lf\n", x, y, mouseScroll);
}

void keyFunCB(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// printf("keyFunCB %d %d %d %d\n", key, scancode, action, mods);
	if (key >= 0 + 48 && key <= 9 + 48) type = key - 48;
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_C:
			capFlag = true;
			break;
		case GLFW_KEY_T:
			timer++;
			break;
		case GLFW_KEY_ESCAPE:
			flag = false;
			break;
		}
	}
}


void saveScreen(int w, int h, std::string filename) {
	glReadBuffer(GL_FRONT);
	cv::Mat img(h, w, CV_8UC3);;
	glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, img.data);
	cv::flip(img, img, 0);
	cv::imwrite(filename, img);
}

template< typename T, typename Alloc >
GLuint makeGLBuffer(GLenum const type, GLenum const usage, std::vector< T, Alloc > const& vec) {
	GLuint id;
	glGenBuffers(1, &id);
	glBindBuffer(type, id);
	auto const size = std::size(vec);
	glBufferData(type, sizeof(vec[0]) * size, size ? &vec[0] : nullptr, usage);
	return id;
}

int main() {
	cv::Mat preImg = cv::Mat::ones(HEIGHT, WIDTH, CV_16U);
	cv::Mat depthImg = cv::Mat::ones(HEIGHT, WIDTH, CV_16U);
	cv::Mat diffImg = cv::Mat::ones(HEIGHT, WIDTH, CV_16U);
	cv::Mat labelImg = cv::Mat::ones(HEIGHT, WIDTH, CV_16U);
	cv::Mat img_8bit = cv::Mat::ones(HEIGHT, WIDTH, CV_8U);
	
	
	double theta = 0.0, phi = 0.0, zoom = 1.0;

#if USE_REALSENSE
	std::thread captureThr([&] {	
		rs2::colorizer color_map;
		rs2::pipeline pipe;
		rs2::config conf;
		// conf.enable_stream(RS2_STREAM_COLOR, WIDTH, HEIGHT, RS2_FORMAT_RGB8, 60);
		// conf.enable_stream(RS2_STREAM_DEPTH, WIDTH, HEIGHT, RS2_FORMAT_Z16, 60);
		conf.enable_stream(RS2_STREAM_DEPTH, WIDTH, HEIGHT, RS2_FORMAT_Z16, 90);

		rs2::pipeline_profile profile = pipe.start(conf);

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
				captureFrame++;
	
				rs2::frame depth_frame = frames.first(RS2_STREAM_DEPTH);
				//depth_frame.get_data();
				const int w = depth_frame.as<rs2::video_frame>().get_width();
				const int h = depth_frame.as<rs2::video_frame>().get_height();

				// depthImg = cv::Mat(cv::Size(w, h), CV_8UC3, (void*)depth_frame.apply_filter(color_map).get_data(), cv::Mat::AUTO_STEP);
				//memcpy(preImg.data, depthImg.data, 2 * depthImg.rows*depthImg.cols);
				depthImg = cv::Mat(cv::Size(w, h), CV_16U, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);
				if (capFlag) {
					memcpy(preImg.data, depthImg.data, 2 * depthImg.rows*depthImg.cols);
					capFlag = false;
				}

				cv::absdiff(depthImg, preImg, diffImg);


				// rs2::frame color_frame = frames.first(RS2_STREAM_COLOR);
				// preImg = cv::Mat(cv::Size(w, h), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);
			}
		}
	});
#else
	std::thread readThr([&] {
		int num = 0;
		std::string path = "../data/201910140422/";
		std::vector<std::string> fileList = getImageList(path);
		std::vector<cv::Mat> inputData;
		for (auto f : fileList) {
			std::stringstream ss;
			ss << path << f;
			inputData.push_back(cv::imread(ss.str(), cv::IMREAD_ANYDEPTH));
		}


		while (flag) {
			captureFrame++;
			num++;

			if (num > fileList.size() - 1) num = 0;

			// -------------------------< straming >-------------------------
			// std::stringstream ss;
			// ss << "../data/201910140126/" << fileList[num];
			// cv::Mat input = cv::imread(ss.str(), cv::IMREAD_ANYDEPTH);
			// memcpy(depthImg.data, input.data, 2 * depthImg.rows * depthImg.cols);

			// -------------------------< pre load >-------------------------
			memcpy(depthImg.data, inputData[num].data, 2 * depthImg.rows * depthImg.cols);

			if (capFlag) {
				memcpy(preImg.data, depthImg.data, 2 * depthImg.rows * depthImg.cols);
				capFlag = false;
			}

			cv::absdiff(depthImg, preImg, diffImg);

			Sleep(1000 / 30);
		}
	});
#endif


	std::thread processingThr([&] {
		while (flag) {
			processingFrame++;
			
			diffImg.convertTo(img_8bit, CV_8U, 1 / 256.0);
			cv::Mat bin;
			cv::threshold(img_8bit, bin, 0, 255, cv::THRESH_BINARY);
			// --------------------------------< find contours >----------------------------
			// std::vector<std::vector<cv::Point> > contours;
			// cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
			// std::vector<int> arr;
			// int max = 10000;
			// int min = 500;
			// for(int i = 0; i < contours.size(); i++){
			// 	double area = cv::contourArea(contours.at(i));
			// 	if(area > min && area < max){
			// 		arr.push_back(i);
			// 		//std::cout << arr.size() << " : " << area << std::endl;
			// 	}
			// }
			// for (int i = 0; i < arr.size(); i++) {
			// 	int count = contours.at(arr[i]).size();
			// 	double sumX=0;
			// 	double sumY=0;
			// 	int minX = HEIGHT;
			// 	int minY = WIDTH;
			// 	int maxX = 0;
			// 	int maxY = 0;
			// 	for(int k = 0;k < count; k++){
			// 		int x = contours.at(arr[i]).at(k).x;
			// 		int y = contours.at(arr[i]).at(k).y;
			// 		if (minX > x) minX = x;
			// 		if (minY > y) minY = y;
			// 		if (maxX < x) maxX = x;
			// 		if (maxY < y) maxY = y;
			// 		sumX += x;
			// 		sumY += y;
			// 	}
			// 	sumX /= count;
			// 	sumY /= count;
			// 	double asp = double(maxX - minX) / (maxY - minY);
			// 	double dx = maxX - minX;
			// 	double dy = maxY - minY;

			// 	cv::Point centroid = cv::Point(sumX, sumY);
			// 	cv::Point center = cv::Point(maxX + minX, maxY + minY) / 2.0;

			// 	if (	centroid.x < center.x + dx / 4.0
			// 		&&	centroid.x > center.x - dx / 4.0
			// 		&&	centroid.y < center.y + dy / 4.0
			// 		&&	centroid.y > center.y - dy / 4.0) {
			// 		cv::rectangle(depthImg, cv::Point(minX, minY), cv::Point(maxX, maxY), cv::Scalar(0, 0, 200), 3, 4);
			// 		cv::circle(depthImg, centroid, 5, cv::Scalar(0, 0, 255), 3, 4);
			// 	}
			// }

			// ----------------------------------< connectedComponents >--------------------------------
			
			cv::erode(bin, bin, cv::Mat(), cv::Point(-1, -1), 6);
			int nLabels = cv::connectedComponents(bin, labelImg, 4, CV_16U);
			
			// cv::Mat stats;
			// cv::Mat centroids;
			// int nLabels = cv::connectedComponentsWithStats(bin, labelImg, stats, centroids, 4, CV_16U);
			// for (int i = 1; i < nLabels; ++i) {
			// 	int *param = stats.ptr<int>(i);

			// 	//ROIの左上に番号を書き込む
			// 	int area = param[cv::ConnectedComponentsTypes::CC_STAT_AREA];
			// 	int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
			// 	int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
			// 	if (area > 10) {
			// 		std::stringstream num;
			// 		num << i;
			// 		cv::putText(labelImg, num.str(), cv::Point(x+5, y+20), cv::FONT_HERSHEY_COMPLEX, 0.7, cv::Scalar(10, 0, 0), 2);
			// 	}
			// }

			// std::vector<cv::Vec3b> colors(nLabels);
			// colors[0] = cv::Vec3b(0, 0, 0);
			// for(int label = 1; label < nLabels; ++label) {
			// 	colors[label] = cv::Vec3b((rand()&255), (rand()&255), (rand()&255));
			// }

			// // ラベリング結果の描画
			// cv::Mat dst(labelImg.size(), CV_8UC3);
			// for(int y = 0; y < dst.rows; ++y) {
			// 	for(int x = 0; x < dst.cols; ++x) {
			// 		int label = labelImg.at<int>(y, x);
			// 		cv::Vec3b &pixel = dst.at<cv::Vec3b>(y, x);
			// 		pixel = colors[label];
			// 	}
			// }
			// cv::imshow("depth", depthImg);
			// cv::imshow("diff", diffImg);
			// cv::imshow("bin", bin);
			// cv::imshow("pre", preImg);
			//cv::imshow("label", dst);
			char key = cv::waitKey(1);
		}
	});

	std::thread viewThr([&] {
		int w = WIDTH;
		int h = HEIGHT;
		// -----------------------------------------< setup opengl >-----------------------------------------
		if (glfwInit() == GL_FALSE) {
			std::cerr << "Can't initilize GLFW" << std::endl;
			return 1;
		}

		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* const window = glfwCreateWindow(w, h, "Normal", NULL, NULL);
		if (window == nullptr) {
			std::cerr << "Can't create GLFW window." << std::endl;
			getchar();
			glfwTerminate();
			return 1;
		}
		glfwMakeContextCurrent(window);

		// Initialize GLEW
		glewExperimental = true; // Needed for core profile
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

		GLuint VertexArrayID;
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		// Create and compile our GLSL program from the shaders
		GLuint programID = LoadShaders("./shader/simple.vert", "./shader/view.frag");
	
		GLuint resolutionID = glGetUniformLocation(programID, "resolution");
		GLuint timeID = glGetUniformLocation(programID, "time");
		GLuint textureID = glGetUniformLocation(programID, "image");
		GLuint labelTexID = glGetUniformLocation(programID, "label");
		GLuint typeID = glGetUniformLocation(programID, "type");
		GLuint MatrixID = glGetUniformLocation(programID, "MVP");

		glm::mat4 Projection = glm::perspective(glm::radians(45.0f), float(WIDTH) / HEIGHT, 0.1f, 1000000.0f);

	
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


		std::vector< float > vertices(w * h * 3);
		std::vector< unsigned int > indices(w * h);
		for (int x = 0; x < w; x++) {
			for (int y = 0; y < h; y++) {
				vertices[3 * (w * y + x) + 0] = x;
				vertices[3 * (w * y + x) + 1] = y;
				vertices[3 * (w * y + x) + 2] = 0;
				indices[w * y + x] = w * y + x;
			}
		}
		GLuint vertex_id = makeGLBuffer(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, vertices);
		GLuint index_id = makeGLBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, indices);

		glfwSetMouseButtonCallback(window, mouseButtonCB);
		glfwSetCursorPosCallback(window, mousePosCB);
		glfwSetScrollCallback(window, mouseScrollCB);
		glfwSetKeyCallback(window, keyFunCB);
		// glfwSwapInterval(0);

		// -----------------------------------------< loop >-----------------------------------------
		while (flag) {
			glFrame++;

			float cx = (mx + mouse.dx) / w;
			float cy = (my + mouse.dy) / h;
			float sq = sqrt(cx * cx + cy * cy);
			float r = sq * PI;
			if (sq != 1.0) {
				sq = 1.0 / sq;
				cx *= sq;
				cy *= sq;
			}
			zoom -= mouse.scroll * 0.1;
			mouse.scroll = 0.0;


			glm::quat q = glm::angleAxis(r, glm::vec3(cy, cx, 0));
			q = glm::normalize(q);
		

			// Camera matrix
			glm::mat4 View = glm::lookAt(
				//camPos,
				glm::vec3(0, 0, 100 * zoom),
				glm::vec3(0, 0, 0), // and looks at the origin
				glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
			);
			View *= glm::mat4_cast(q);
			glm::mat4 Model = glm::mat4(1.0f);
			glm::mat4 MVP = Projection * View * Model; // ModelViewProjection

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glUseProgram(programID);

			// uniform
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, WIDTH, HEIGHT, 0, GL_RED, GL_SHORT, depthImg.data);
			glUniform1i(textureID, 0);

			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, textures[1]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, WIDTH, HEIGHT, 0, GL_RED, GL_SHORT, labelImg.data);
			glUniform1i(labelTexID, 1);

			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniform2f(resolutionID, WIDTH, HEIGHT);
			glUniform1i(timeID, timer);
			glUniform1i(typeID, type);


			glEnableClientState(GL_VERTEX_ARRAY);

			//vertPos_M()
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ARRAY_BUFFER, vertex_id);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices[0])* vertices.size(), &vertices[0]);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_id);
			glDrawElements(GL_POINTS, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
			glPointSize(1.0);

			glDisableClientState(GL_VERTEX_ARRAY);


			// Swap buffers
			glfwSwapBuffers(window);
			glfwPollEvents();
			Sleep(1000 / 60);
		}

		glDeleteVertexArrays(1, &VertexArrayID);
		glDeleteProgram(programID);

		// Close OpenGL window and terminate GLFW
		glfwTerminate();
	});

	while (flag) {
		clock_t now = clock();
		double time = double(now - clk) / CLOCKS_PER_SEC;
		if (time > 1.0) {
			std::cout 
				<< "cap : " << captureFrame / time << " fps " 
				<< "proce : " << processingFrame / time << " fps "
				<< "gl : " << glFrame / time << " fps " 
				<< std::endl;
			clk = now;
			captureFrame = 0;
			processingFrame = 0;
			glFrame = 0;
		}
		Sleep(100);
	}


	processingThr.join();
	viewThr.join();
	#if USE_REALSENSE
		captureThr.join();
	#else
		readThr.join();
	#endif

}


