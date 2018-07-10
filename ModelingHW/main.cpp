// Include standard headers
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


// Include others OpenGL helper 
#include <common/shader.hpp>
#include <common/controls.hpp>


// -----------------------------------------------------
// windows variable
// -----------------------------------------------------

double mouseX, mouseY;
double currTime, prevTime;
int width, height;
bool Ldown;


// -----------------------------------------------------
// Curve variable
// -----------------------------------------------------

#define MAX_CONTROL_POINTS	100
#define MAX_CURVE_POINTS	100

int numPoints;
glm::vec3** controlPoints;
glm::vec3* curvePoints;
float red, green, blue;


// -----------------------------------------------------
//+ De Castelijeau algorithm
//	- controlPoints[y][x] = control points x of iteration y
// -----------------------------------------------------

void FindSecondaryControlPoints(glm::vec3** control_points, int num_point, int t) {
	int num = num_point;
	float nT = t / 100.0f;
	
	//printf("%f", nT);
	for (int i = 1; i < num_point;i++ ) {
		num--;
		for (int j = 0; j < num; j++) {
			/*std::cout << "C00.x"<<controlPoints[0][0].x << std::endl;
			std::cout << "C00.y" <<controlPoints[0][0].y << std::endl;

			std::cout << "C01.x" << controlPoints[0][1].x << std::endl;
			std::cout << "C01.y" << controlPoints[0][1].y << std::endl;*/
			
			controlPoints[i][j].x = ((1.0 - nT)*(control_points[i - 1][j].x)) + (nT*(control_points[i - 1][j + 1].x));
			controlPoints[i][j].y = ((1.0 - nT)*(control_points[i - 1][j].y)) + (nT*(control_points[i - 1][j + 1].y));
			//controlPoints[i][j].z = ((1 - t)*(control_points[i - 1][j].z)) + (t*(control_points[i - 1][j + 1].z));
			/*std::cout << "C10.x" << controlPoints[1][0].y << std::endl;
			std::cout << "C10.y" << controlPoints[1][0].y << std::endl;*/
		}
	}

	
}




int main(void)
{
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	srand(time(NULL));
	width = 1024;
	height = 768;
	Ldown = false;

	//glfwWindowHint(GLFW_SAMPLES, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(width, height, "De Castelijeau algorithm", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.2 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(window, width / 2, height / 2);

	// Dark background
	glViewport(0, 0, width, height);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glLineWidth(3.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// Create and compile our GLSL program from the shaders
	GLuint program_2Dline = LoadShaders("2Dline.vert", "2Dline.frag");
	glUseProgram(program_2Dline);

	//Create VBO
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	//Create+set VAO
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Compute the MVP matrix
	glm::mat4 ProjectionMatrix = glm::ortho(-(width / 2)*1.0f, (width / 2)*1.0f, -(height / 2)*1.0f, (height / 2)*1.0f, -10.0f, 10.0f);
	glm::mat4 ViewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	glm::mat4 ModelViewMatrix = ViewMatrix * ModelMatrix;
	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program_2Dline, "MVP"), 1, GL_FALSE, &MVP[0][0]);

	// Init 2d array, array	
	numPoints = 0;
	controlPoints = new glm::vec3*[MAX_CONTROL_POINTS];
	for (int i = 0; i < MAX_CONTROL_POINTS; i++) {
		controlPoints[i] = new glm::vec3[MAX_CONTROL_POINTS - i];
	}
	curvePoints = new glm::vec3[MAX_CURVE_POINTS];

	// frame rate
	glfwSwapInterval(1);
	prevTime = glfwGetTime();

	do {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (int t = 0; t < MAX_CURVE_POINTS; t++) {

			// Left click to add Control point, Right click to clear all Control points	
			glfwPollEvents();
			if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) && !Ldown) {
				if (numPoints < (MAX_CONTROL_POINTS)) {
					glfwGetCursorPos(window, &mouseX, &mouseY);
					controlPoints[0][numPoints] = glm::vec3(mouseX - (width / 2), -mouseY + (height / 2), 0);
					numPoints++;
				}
				Ldown = true;
			}
			if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE) && Ldown) { Ldown = false; }
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) { numPoints = 0; }

			// We draw something when we have more than 1 control point
			if (numPoints >= 1) {

				// Fill all secondary control points
				FindSecondaryControlPoints(controlPoints, numPoints, t);

				// Clear the screen
				currTime = glfwGetTime();
				double result = 1.0f / (currTime - prevTime);
				printf("#control point> %i \n", numPoints);
				printf("at t> %i \n", t);
				printf("fps> %f \n", result);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				// Draw control points, i = iteration index
				for (int i = 0; i < (numPoints - 1); i++){		// Change to this line to draw all control points
				//for (int i = 0; i < 1; i++) {
					glBufferData(GL_ARRAY_BUFFER, (numPoints - i) * sizeof(glm::vec3), &controlPoints[i][0], GL_DYNAMIC_DRAW);
					red = 0.2f * (i % 5) + 0.2f;
					green = 0.2f * ((i % 25) / 5) + 0.2f;
					blue = 0.2f * (i / 25) + 0.2f;
					glUniform3f(glGetUniformLocation(program_2Dline, "Color"), red, green, blue);
					glDrawArrays(GL_LINE_STRIP, 0, (numPoints - i));
				}

				// Draw curve points
				//+ curvePoints = ???;
				curvePoints[t] = controlPoints[numPoints-1][0];
				
				// Uncomment these 3 lines if you know how to set curvePoints
				glBufferData(GL_ARRAY_BUFFER, (t+1) * sizeof(glm::vec3), &curvePoints[0], GL_DYNAMIC_DRAW);
				glUniform3f(glGetUniformLocation(program_2Dline, "Color"), 1,1,1);
				glDrawArrays(GL_LINE_STRIP, 0, (t+1));

				// Swap buffers
				glfwSwapBuffers(window);
				prevTime = currTime;
			}
		}

		glfwSwapBuffers(window);


	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);


	// free 2d array, array
	for (int i = 0; i < MAX_CONTROL_POINTS; i++) {
		delete[] controlPoints[i];
	}
	delete[] controlPoints;
	delete[] curvePoints;

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteVertexArrays(1, &VertexArrayID);
	glDeleteProgram(program_2Dline);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}







