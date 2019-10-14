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

#include "../Utility/shader.hpp"
#include "../Utility/faceTracking.hpp"

#define TRAC 1

// int winW = 640;
// int winH = 480;
int winW = 1920;
int winH = 1080;
float face[3] = { 0.0, 0.0, 800.0 };
float offset[3] = {0.0, 157.0, 30.0};

boolean flag = true;
boolean trackFlag = true;
float count = 0.0;
int status = 0;

clock_t timer = clock();
int trackingFrame = 0;
int displayFrame = 0;


static const GLfloat vertex[] = {
	1.0f,	 1.0f,	0.0f,
	-1.0f,	 1.0f,	0.0f,
	-1.0f,	-1.0f,	0.0f,
	1.0f,	-1.0f,	0.0f
};

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_ESCAPE:
			flag = false;
			break;
		case GLFW_KEY_T:
			trackFlag = !trackFlag;
			break;
		}
	}
	else {
		switch (key) {
		case GLFW_KEY_LEFT:
			face[0] -= 5.0;
			break;
		case GLFW_KEY_RIGHT:
			face[0] += 5.0;
			break;
		case GLFW_KEY_UP:
			face[2] -= 5.0;
			break;
		case GLFW_KEY_DOWN:
			face[2] += 5.0;
			break;

		}
		std::cout << "(" << face[0] << ", " << face[1] << ", " << face[2] << ")" << std::endl;
	}
}

int main() {
	std::thread trackingThr([&] {
		FaceTrack FT(70, 70);
		FT.offset[0] = offset[0];
		FT.offset[1] = offset[1];
		FT.offset[2] = offset[2];

		while (flag) {
			trackingFrame++;
			status = FT.track();
			if (trackFlag && status) {
				face[0] = FT.pos[0];
				face[1] = FT.pos[1];
				face[2] = FT.pos[2];
			}
			imshow("Depth", FT.depthImg);
			char key = cv::waitKey(1);
			if (key == 27) flag = false; // ESC
		}
	});

	std::thread viewThr([&] {
		// ------------------------------------< initialize >------------------------------------
		if (glfwInit() == GL_FALSE) {
			std::cerr << "Can't initilize GLFW" << std::endl;
			return 1;
		}
		glfwWindowHint(GLFW_DECORATED, false);
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		int monitorNum;
		GLFWmonitor** monitors = glfwGetMonitors(&monitorNum);


		// ------------------------------------< image >------------------------------------
		GLFWwindow *const window = glfwCreateWindow(winW, winH, "Image", NULL, NULL);
		glfwGetFramebufferSize(window, &winW, &winH);
		if (window == nullptr) {
			std::cerr << "Can't create GLFW window." << std::endl;
			glfwTerminate();
			return 1;
		}
		glfwMakeContextCurrent(window);

		glewExperimental = GL_TRUE;
		if (GLEW_OK != glewInit()) {
			std::cout << "Failed to initialise GLEW" << std::endl;
			return EXIT_FAILURE;
		}

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

		GLuint VertexArrayID;
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		GLuint programID = LoadShaders("./shader/plane.vert", "./shader/spheres.frag");

		// attribute
		GLuint vertexID;
		glGenBuffers(1, &vertexID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);

		// uniform
		GLuint resolutionID = glGetUniformLocation(programID, "resolution");
		GLuint timeID = glGetUniformLocation(programID, "time");
		GLuint posID = glGetUniformLocation(programID, "eye");
        GLuint statusID = glGetUniformLocation(programID, "status");
		glViewport(0, 0, winW, winH);

		glfwSwapInterval(1);
		glfwSetKeyCallback(window, keyCallback);
		
		while (flag) {
			displayFrame++;
	
			// ----------------------------< image >----------------------------
			glUseProgram(programID);
			glfwMakeContextCurrent(window);
			glClear(GL_COLOR_BUFFER_BIT);

			// uniform
			glUniform2f(resolutionID, winW, winH);
			glUniform1i(timeID, int(count) % (4 * 100000000000000));
            glUniform1i(statusID, status);
			glUniform3f(posID, face[0], face[1], face[2]);
		
			// attribute
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, vertexID);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			glDisableVertexAttribArray(0);


			// ----------------------------< swap >----------------------------
			
			glfwSwapBuffers(window);
			
			glfwPollEvents();
			// Sleep(1000.0 / 60.0);
		}

		glDeleteBuffers(1, &vertexID);
		glDeleteVertexArrays(1, &VertexArrayID);
		glDeleteProgram(programID);

		glfwTerminate();
	});

	while (flag) {
		clock_t now = clock();
		double time = double(now - timer) / CLOCKS_PER_SEC;
		if (time > 1.0) {
//			std::cout << "pos: (" << face[0] << ", " << face[1] << ", " << face[2] << ")\t";
			std::cout << "tracking : " << trackingFrame / time << " fps ";
			std::cout << "display : " << displayFrame / time << " fps" << std::endl;

			//timer = now;
			trackingFrame = 0;
			displayFrame = 0;
		}
		Sleep(100);
	}
	trackingThr.join();
	viewThr.join();
}
