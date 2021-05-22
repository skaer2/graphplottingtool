#include <stdio.h>
#include <memory>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <windows.h>

#include <GL/glew.h>
#include <GL/glut.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader_utils.h"

#include "muParser.h"

using namespace std;

GLuint program;
GLint attribute_coord2d;
GLint uniform_color;
GLint uniform_transform;

float offset_x = 0;
float offset_y = 0;
float scale_x = 1;
float scale_y = 1;
float scale = 1.0f;
float scalingFactor = 1.0f;

int oldMousecoords_x = 0;
int oldMousecoords_y = 0;
float mouseOffset_x = 0;
float mouseOffset_y = 0;
bool mouseState = 0;

struct point {
	GLfloat x;
	GLfloat y;
};

GLuint vbo[3];

const int ticksize = 10;

double MySqr(double a_fVal) {  return a_fVal*a_fVal; }

point graph[2000];
bool needToUpdate = false;

int init_resources() {
	program = create_program("graph.v.glsl", "graph.f.glsl");
	if (program == 0)
		return 0;

	attribute_coord2d = get_attrib(program, "coord2d");
	uniform_transform = get_uniform(program, "transform");
	uniform_color = get_uniform(program, "color");

	if (attribute_coord2d == -1 || uniform_transform == -1 || uniform_color == -1)
		return 0;

	// Create the vertex buffer object
	glGenBuffers(3, vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

	// Create our own temporary buffer
	point graphInit[2000];

	// Fill it in just like an array
	for (int i = 0; i < 2000; i++) {
		float x = (i - 1000.0) / 100.0;

		graphInit[i].x = x;
		graphInit[i].y = sin(x * 10.0) / (1.0 + x * x);
	}

	// Tell OpenGL to copy our array to the buffer object
	glBufferData(GL_ARRAY_BUFFER, sizeof graphInit, graphInit, GL_DYNAMIC_DRAW);

	// Create a VBO for the border
	static const point border[4] = { {-1, -1}, {1, -1}, {1, 1}, {-1, 1} };
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof border, border, GL_DYNAMIC_DRAW);

	return 1;
}

// Create a projection matrix that has the same effect as glViewport().
// Optionally return scaling factors to easily convert normalized device coordinates to pixels.
//{{{
glm::mat4 viewport_transform(float x, float y, float width, float height, float *pixel_x = 0, float *pixel_y = 0) {
	// Map OpenGL coordinates (-1,-1) to window coordinates (x,y),
	// (1,1) to (x + width, y + height).

	// First, we need to know the real window size:
	float window_width = glutGet(GLUT_WINDOW_WIDTH);
	float window_height = glutGet(GLUT_WINDOW_HEIGHT);

	// Calculate how to translate the x and y coordinates:
	float offset_x = (2.0 * x + (width - window_width)) / window_width;
	float offset_y = (2.0 * y + (height - window_height)) / window_height;

	// Calculate how to rescale the x and y coordinates:
	float scale_x = width / window_width;
	float scale_y = height / window_height;

	// Calculate size of pixels in OpenGL coordinates
	if (pixel_x)
		*pixel_x = 2.0 / width;
	if (pixel_y)
		*pixel_y = 2.0 / height;

	return glm::scale(glm::translate(glm::mat4(1), glm::vec3(offset_x, offset_y, 0)), glm::vec3(scale_x, scale_y, 1));
}
//}}}

void display() {

	glUseProgram(program);

	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	/* ---------------------------------------------------------------- */
	/* Draw the graph */

	// Set our coordinate transformation matrix
    scale_x = scale;
    scale_y = scale;
    float finalOffset_x = offset_x + mouseOffset_x;
    float finalOffset_y = offset_y - mouseOffset_y;
	glm::mat4 transform = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(scale_x, scale_y, 1)), glm::vec3(finalOffset_x, finalOffset_y, 0));
	glUniformMatrix4fv(uniform_transform, 1, GL_FALSE, glm::value_ptr(transform));

	// Set the color to red
	GLfloat red[4] = { 1, 0, 0, 1 };
	glUniform4fv(uniform_color, 1, red);

	// Draw using the vertices in our vertex buffer object
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);

    //If the function has been changed tell OpenGL to copy our array to the buffer object
    if (needToUpdate){
        needToUpdate = false;
        glBufferData(GL_ARRAY_BUFFER, sizeof graph, graph, GL_DYNAMIC_DRAW);
    }

	glEnableVertexAttribArray(attribute_coord2d);
	glVertexAttribPointer(attribute_coord2d, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_LINE_STRIP, 0, 2000);

    //End of drawing
	glDisableVertexAttribArray(attribute_coord2d);
	glutSwapBuffers();
}

void idle(){
}

void mouseMotionFunc(int x, int y){
    if(mouseState){
        mouseOffset_x = (oldMousecoords_x - x)/100.0f;
        mouseOffset_y = (oldMousecoords_y - y)/100.0f;

        printf("moving x=%d, y=%d\n", x, y);
        printf("offset_x = %f, offset_y = %f\n", mouseOffset_x, mouseOffset_y);
        glutPostRedisplay();
    }
}

void mouseFunc(int button, int state, int x, int y){
    if(glutGetModifiers() == GLUT_ACTIVE_SHIFT) scalingFactor = 10.0f;
        else scalingFactor = 1.0f;
    if(button == 3 && state == 1){
        scale *= 1.1f * scalingFactor;
    }
    if(button == 4 && state == 1){
        scale /= 1.1f * scalingFactor;
    }
    if(button == 0 && state == 1){
        mouseState = 0;
        offset_x += mouseOffset_x;
        offset_y -= mouseOffset_y;
        mouseOffset_x = 0;
        mouseOffset_y = 0;
        printf("click end x=%d, y=%d\n", x, y);
    }

    if(button == 0 && state == 0){
        mouseState = 1;
        oldMousecoords_x = x;
        oldMousecoords_y = y;
        printf("click x=%d, y=%d\n", x, y);
    }

    printf("button = %d, state = %d\n", button, state);
    //printf("scalingFactor = %f\n", (float) scalingFactor);
	glutPostRedisplay();
}

void special(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT: //to move left on the graph
		offset_x += 0.03;
		break;
	case GLUT_KEY_RIGHT: //to move right on the graph
		offset_x -= 0.03;
		break;
	case GLUT_KEY_UP: //to move up on the graph
		offset_y -= 0.3;
		break;
	case GLUT_KEY_DOWN: //to move down on the graph
		offset_y += 0.3;
		break;
	case GLUT_KEY_HOME: //to move to the default position on the graph
		offset_x = 0.0;
		offset_y = 0.0;
		scale = 1.0;
		break;
	}

	glutPostRedisplay();
}

void free_resources() {
	glDeleteProgram(program);
}

DWORD WINAPI textIOthread(LPVOID param){
    while(true){
        string test = "";
        cout << "Enter new function\n";
        cin >> test;
        cout << "Plotting new function: " << test << endl;

        try
        {
            double var_x = 1;
            double var_y = 1;
            mu::Parser p;
            p.DefineVar("x", &var_x);
            p.DefineVar("y", &var_y);
            p.DefineFun("MySqr", MySqr);
            p.SetExpr(test);

            auto myMap = p.GetUsedVar();
            for(auto elem : myMap)
            {
                cout << elem.first << " " << elem.second << "\n";
            }

            // Fill in and evaluate the values of the function
            for (int i = 0; i < 2000; i++) {
                double x = (i - 1000.0) / 100.0;
                double y = (i - 1000.0) / 100.0;

                var_x = x;
                var_y = y;
                graph[i].x = (float) x;
                graph[i].y = (float) p.Eval();
            }

            //set the flag meaning that the function has been changed
            needToUpdate = true;
            //glutPostRedisplay();
        }
        catch (mu::Parser::exception_type &e) //parsers errors
        {
            cout << e.GetMsg() << endl;
        }
    }




    return 0;
}

int main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowSize(640, 480);
	glutCreateWindow("Graph plotting tool");

	GLenum glew_status = glewInit();

	if (GLEW_OK != glew_status) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
		return 1;
	}

	if (!GLEW_VERSION_2_0) {
		fprintf(stderr, "No support for OpenGL 2.0 found\n");
		return 1;
	}

	printf("Use left/right to move horizontally.\n");
	printf("Use up/down to change the horizontal scale.\n");
    printf("Press home to reset the position and scale.\n");

	//create thread for user console interaction
	DWORD threadID;
    CreateThread(0, 0, textIOthread, NULL, 0, &threadID);

	if (init_resources()) {
		glutDisplayFunc(display);
        glutIdleFunc(idle);
        glutMotionFunc(mouseMotionFunc);
        glutMouseFunc(mouseFunc);
		glutSpecialFunc(special);
		glutMainLoop();
	}

	free_resources();
	return 0;
}
