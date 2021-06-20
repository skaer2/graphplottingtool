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

#define N 101

using namespace std;

//mouse related variables
int oldMousecoords_x = 0;
int oldMousecoords_y = 0;
float mouseOffset_x = 0;
float mouseOffset_y = 0;
bool leftclickState = false;
bool rightclickState = false;

//2d part shader variables
GLuint program;
GLint attribute_coord2d;
GLint uniform_color;
GLint uniform_transform;

//2d part variables for moving the graph
float offset_x = 0;
float offset_y = 0;
float scale_x = 1;
float scale_y = 1;
float scale_z = 1;
float scale = 1.0f;
float scalingFactor = 1.0f;

//3d part shader variables
GLuint program3d;
GLint attribute_coord3d;
GLint attribute_coordz;
GLint uniform_vertex_transform;
GLint uniform_texture_transform;
GLint uniform_color3d;
GLuint texture_id;

//3d part variables for moving the camera
glm::vec3 cameraPos; //3d
glm::vec3 cameraFront; //3d
glm::vec3 cameraUp; //3d
glm::vec3 cameraPosBuff;
float rotationH;
float rotationV;
float rotationHBuff = rotationH;
float rotationVBuff = rotationV;
float tex_scale = 1.0;

struct point {
    GLfloat x;
    GLfloat y;
};

// vbo2d for the 2d buffer array
// index 0 for the 3d buffer array 1
// index 1 for the 3d buffer array 2
// index 2 for the 3d buffer array 3
GLuint vbo2d;
GLuint vbo[3]; 
GLuint vbocoordz;

double MySqr(double a_fVal) {  return a_fVal*a_fVal; }

point graph[2000];
GLfloat graph3d[N][N];
bool needToUpdate = false;
bool mode = false; //mode == true then it is set to 2d
string func3d = "";
string func2d = "";

void resetCameraPos(){
    cameraPos   = glm::vec3(0.0,  1.5, 1.0);
    cameraFront = glm::vec3(0.0, -1.0, 0.0);
    cameraUp    = glm::vec3(0.0,  0.0, 1.0);
    cameraPosBuff = cameraPos;
    rotationH = 0.0f;
    rotationV = 0.0f;
    rotationHBuff = rotationH;
    rotationVBuff = rotationV;
}

int init_resources() {
    //2d initialization
    program = create_program("graph.v.glsl", "graph.f.glsl");
    if (program == 0)
        return 0;

    attribute_coord2d = get_attrib(program, "coord2d");
    uniform_transform = get_uniform(program, "transform");
    uniform_color = get_uniform(program, "color");

    if (attribute_coord2d == -1 || uniform_transform == -1 || uniform_color == -1)
        return 0;

    // Create the vertex buffer object
    glGenBuffers(1, &vbo2d);

    // Fill it in just like an array
    for (int i = 0; i < 2000; i++) {
        float x = (i - 1000.0) / 100.0;

        graph[i].x = x;
        graph[i].y = sin(x * 10.0) / (1.0 + x * x); //initial function
    }

    // Tell OpenGL to copy our array to the buffer object
    glBindBuffer(GL_ARRAY_BUFFER, vbo2d);
    glBufferData(GL_ARRAY_BUFFER, sizeof graph, graph, GL_DYNAMIC_DRAW);
    //end of 2d initialization




    //3d initialization
    program3d = create_program("graph3d.v.glsl", "graph3d.f.glsl");
    if(program3d == 0)
        return 0;

    attribute_coord3d = get_attrib(program3d, "coord3d");
    attribute_coordz = get_attrib(program3d, "coordz");
    uniform_vertex_transform = get_uniform(program3d, "vertex_transform");
    uniform_texture_transform = get_uniform(program3d, "texture_transform");
    uniform_color3d = get_uniform(program3d, "color3d");

    if(attribute_coord3d == -1 || attribute_coordz == -1 || uniform_texture_transform == -1 || uniform_vertex_transform == -1 || uniform_color3d == -1)
        return 0;

    glm::vec2 vertices[101][101];
    for (int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            float x = (i - N / 2) / (N / 2.0);
            float y = (j - N / 2) / (N / 2.0);
            vertices[i][j].x = x;
            vertices[i][j].y = y;
             
            float d = hypotf(x, y) * 4.0; 
            float z = (1 - d * d) * expf(d * d / -2.0); //initial 3d function
            //float z = x * y; //initial 3d function

            graph3d[i][j] = z;
            //cout << "coord = " << x << endl;
        }
    }

    glGenBuffers(1, &vbocoordz);
    glBindBuffer(GL_ARRAY_BUFFER, vbocoordz);
    glBufferData(GL_ARRAY_BUFFER, sizeof graph3d, graph3d, GL_DYNAMIC_DRAW);

    //upload the texture

    glGenBuffers(3, vbo);

    //create an array for 101 * 101 verticers

    //for (int i = 0; i < 101; i++){
        //for (int j = 0; j < 101; j++){
            //vertices[i][j].x = (j - 50) / 50.0;
            //vertices[i][j].y = (i - 50) / 50.0;
        //}
    //}

    //Copy the array to the buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);

    //Create an arrray of indices into the vertex array that traces both horizontal and vertical lines
    GLushort indices[100 * 100 * 6];
    int i = 0;

    for (int y = 0; y < 101; y++) {
        for (int x = 0; x < 100; x++) {
            indices[i++] = y * 101 + x;
            indices[i++] = y * 101 + x + 1;
        }
    }

    for (int x = 0; x < 101; x++) {
        for (int y = 0; y < 100; y++) {
            indices[i++] = y * 101 + x;
            indices[i++] = (y + 1) * 101 + x;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 100 * 101 * 4 * sizeof *indices, indices, GL_STATIC_DRAW);

    //fill an array of indices that describes all the trianlges needed to create a completely filled surface
    i = 0;

    for (int y = 0; y < 100; y++){
        for (int x = 0; x < 100; x++){
            indices[i++] = y * 101 + x;
            indices[i++] = y * 101 + x + 1;
            indices[i++] = (y + 1) * 101 + x + 1;

            indices[i++] = y * 101 + x;
            indices[i++] = (y + 1) * 101 + x + 1;
            indices[i++] = (y + 1) * 101 + x;
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);

    resetCameraPos(); // set the camera

    //end of 3d initialization
    
    return 1;
}

void display() {

    //if mode is 2d draw the 2d part
    if(mode){
        //2d drawing start
        glUseProgram(program);
        glDisable(GL_DEPTH_TEST);

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

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
        glBindBuffer(GL_ARRAY_BUFFER, vbo2d);

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
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glutSwapBuffers();

        //2d drawing end
    }
    else{
        //3d drawing start
        glUseProgram(program3d);

        if (needToUpdate){
            needToUpdate = false;
            //for (int i = 0; i < N; i++){
                //for(int j = 0; j < N; j++){
                    //cout << "coord = " << graph3d[i][j] << endl;
                //}
            //}
            glBindBuffer(GL_ARRAY_BUFFER, vbocoordz);
            glBufferData(GL_ARRAY_BUFFER, sizeof graph3d, graph3d, GL_DYNAMIC_DRAW);
        }

        //Model matrix for rotating the graph
        glm::mat4 model;

        model = glm::rotate(glm::mat4(1.0f), glm::radians(rotationHBuff), glm::vec3(0.0f, 0.0f, 1.0f)); //horizontal rotation 
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), -glm::radians(rotationHBuff), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 rotationAxis = rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        model = glm::rotate(model
                , glm::radians(rotationVBuff)
                , glm::vec3(rotationAxis)
                ); //vertical rotation 

        //Scaling the graph
        scale_x = scale;
        scale_y = scale;
        scale_z = scale;
        model = glm::scale(model, glm::vec3(scale_x, scale_y, scale_z));

        //View matrix - the camera tranformation
        glm::mat4 view = glm::lookAt(cameraPosBuff, cameraPosBuff + cameraFront, cameraUp);
        //3d Projection matrix
        glm::mat4 projection = glm::perspective(45.0f, 1.0f * 640 / 480, 0.01f, 10.0f);

        glm::mat4 vertex_transform = projection * view * model;
        glm::mat4 texture_transform = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(tex_scale, tex_scale, 1)), glm::vec3(0, 0, 0));

        glUniformMatrix4fv(uniform_vertex_transform, 1, GL_FALSE, glm::value_ptr(vertex_transform));
        glUniformMatrix4fv(uniform_texture_transform, 1, GL_FALSE, glm::value_ptr(texture_transform));

        //glClearColor(0,0,0,0);
        glClearColor(1,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Set texture interpolation mode

        //Set the color for the triangles
        GLfloat grey[4] = { 0.5, 0.5, 0.5, 1 };
        glUniform4fv(uniform_color3d, 1, grey);

        glEnable(GL_DEPTH_TEST);

        bool polygonoffset = true;
        if (polygonoffset) {
            glPolygonOffset(1, 0);
            glEnable(GL_POLYGON_OFFSET_FILL);
        }

        glEnableVertexAttribArray(attribute_coordz);
        glBindBuffer(GL_ARRAY_BUFFER, vbocoordz);
        glVertexAttribPointer(attribute_coordz, 1, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(attribute_coord3d);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glVertexAttribPointer(attribute_coord3d, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
        glDrawElements(GL_TRIANGLES, 100 * 100 * 6, GL_UNSIGNED_SHORT, 0);

        glPolygonOffset(0, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);

        //Draw the grid, very bright 
        GLfloat bright[4] = { 2, 2, 2, 1 };
        glUniform4fv(uniform_color3d, 1, bright);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
        glDrawElements(GL_LINES, 100 * 101 * 4, GL_UNSIGNED_SHORT, 0);

        //Stop using the vertex buffer object
        glDisableVertexAttribArray(attribute_coord3d);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glutSwapBuffers();

        //3d drawing end
    }
}

void mouseMotionFunc(int x, int y){
    if(leftclickState && !rightclickState){
        mouseOffset_x = (oldMousecoords_x - x)/100.0f;
        mouseOffset_y = (oldMousecoords_y - y)/100.0f;

        if(!mode){ // not 2d mode
            cameraPosBuff = cameraPos;
            cameraPosBuff += (glm::normalize(glm::cross(cameraFront, cameraUp)) * mouseOffset_x);
            if(glutGetModifiers() == GLUT_ACTIVE_SHIFT) cameraPosBuff -= cameraUp * mouseOffset_y;
                else cameraPosBuff -= cameraFront * mouseOffset_y;
        }

        //printf("moving x=%d, y=%d\n", x, y);
        //printf("offset_x = %f, offset_y = %f\n", mouseOffset_x, mouseOffset_y);
        glutPostRedisplay();
    }

    if(rightclickState && !leftclickState && !mode){ // not 2d mode
        mouseOffset_x = (oldMousecoords_x - x)/100.0f;
        mouseOffset_y = (oldMousecoords_y - y)/100.0f;

        rotationHBuff = rotationH;
        rotationVBuff = rotationV;
        rotationHBuff += mouseOffset_x * 10;
        rotationVBuff += mouseOffset_y * 10;

        //printf("moving x=%d, y=%d\n", x, y);
        //printf("offset_x = %f, offset_y = %f\n", mouseOffset_x, mouseOffset_y);
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

    //left mouse button
    if(button == 0 && state == 1){
        leftclickState = 0;
        if(mode){
            offset_x += mouseOffset_x;
            offset_y -= mouseOffset_y;
        }
        else{
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * mouseOffset_x;
            if(glutGetModifiers() == GLUT_ACTIVE_SHIFT) cameraPos -= cameraUp * mouseOffset_y;
                else cameraPos -= cameraFront * mouseOffset_y ;
        }
        
        mouseOffset_x = 0;
        mouseOffset_y = 0;
        //printf("click end x=%d, y=%d\n", x, y);
    }

    if(button == 0 && state == 0){
        leftclickState = 1;
        oldMousecoords_x = x;
        oldMousecoords_y = y;
        //printf("click x=%d, y=%d\n", x, y);
    }

    //right mouse button
    if(button == 2 && state == 1){
        rightclickState = 0;
        if(mode){
            //in 2d don't do anything yet
        }
        else{
            rotationH += mouseOffset_x * 10;
            rotationV += mouseOffset_y * 10;
        }
        
        mouseOffset_x = 0;
        mouseOffset_y = 0;
        //printf("click end x=%d, y=%d\n", x, y);
    }

    if(button == 2 && state == 0){
        rightclickState = 1;
        oldMousecoords_x = x;
        oldMousecoords_y = y;
        //printf("click x=%d, y=%d\n", x, y);
    }
    //printf("button = %d, state = %d\n", button, state);
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
        resetCameraPos();
        scale = 1.0;
        glutPostRedisplay();
        break;
    case GLUT_KEY_INSERT:
        mode = !mode;
        if(mode) 
            cout << "Plotting the function: " << func2d << endl;
        else cout << "Plotting the function: " << func3d << endl;
        break;
    case GLUT_KEY_PAGE_UP:
        tex_scale *= 1.5;
        break;
    case GLUT_KEY_PAGE_DOWN:
        tex_scale /= 1.5;
        break;

    }

    glutPostRedisplay();
}

void free_resources() {
    glDeleteProgram(program);
    glDeleteProgram(program3d);
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
            int amountOfVars = myMap.size();

            // Fill in and evaluate the values of the function
            if (amountOfVars == 1){
                func2d = test;
                for (int i = 0; i < 2000; i++) {
                    double x = (i - 1000.0) / 100.0;
                    //double y = (i - 1000.0) / 100.0;

                    var_x = x;
                    graph[i].x = (float) x;
                    graph[i].y = (float) p.Eval();
                }
            }
            if(amountOfVars > 1){
                func3d = test;
                for (int i = 0; i < N; i++){
                    for(int j = 0; j < N; j++){
                        double x = (i - N / 2) / (N / 2.0);
                        double y = (j - N / 2) / (N / 2.0);

                        var_x = x;
                        var_y = y;
                        graph3d[i][j] = ((float) p.Eval());
                        //cout << "coord = " << x << endl;
                    }
                }

            }

            //set the flag meaning that the function has been changed
            needToUpdate = true;
            glutPostRedisplay();
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
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
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

    printf("Use left mouse button to move on the x and y axis.\n");
    printf("Use right mouse button to rotate the graph in 3d mode.\n");
    printf("Use left mouse button while holding shift to move on the z axis in 3d mode.\n");
    printf("Use the mouse wheel to zoom in and out. Use the mouse wheel while holding shift to zoom in and out faster\n");
    printf("Press insert to switch between 2d and 3d vizualization modes\n");
    printf("Press home to reset the position and scale.\n");

    func2d = "sin(10*x)/(1+x^2)";
    func3d = "(1-(sqrt(x^2+y^2)*4.0)^2)*exp((sqrt(x^2+y^2)*4.0)^2/-2)";
    if(mode) 
        cout << "Plotting the function: " << func2d << endl;
    else cout << "Plotting the function: " << func3d << endl;

    
    //create thread for user console interaction
    DWORD threadID;
    CreateThread(0, 0, textIOthread, NULL, 0, &threadID);

    if (init_resources()) {
        glutDisplayFunc(display);
        glutMotionFunc(mouseMotionFunc);
        glutMouseFunc(mouseFunc);
        glutSpecialFunc(special);
        glutMainLoop();
    }

    free_resources();
    return 0;
}
