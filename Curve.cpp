// Curve.cpp : Defines the entry point for the console application.
//
//andrey, leave stdafx.h out of your code. this is a stupid windows thing
#include "stdafx.h"


#include <vector>
#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h> 
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <vector>
#include <time.h>
#include <limits>
using namespace std;


#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <Eigen/Dense>
#include <Eigen/StdVector>
using namespace Eigen;



class Viewport {
public:
	int w, h; // width and height
};


class Patch{
public:
	std::vector <Eigen::Vector3d> q;
	Patch(std::vector <Eigen::Vector3d> q); 
};

Patch::Patch(std::vector <Eigen::Vector3d> q1){
	q = q1; 
}

//****************************************************
// Global Variables
//****************************************************
Viewport    viewport;
int patches; 
vector <Eigen::Vector3d> coordinates;
vector <Patch> bezpatches;

//****************************************************
// reshape viewport if the window is resized
//****************************************************



void myReshape(int w, int h) {
	viewport.w = w;
	viewport.h = h;

	glViewport(0, 0, viewport.w, viewport.h);// sets the rectangle that will be the window
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();                // loading the identity matrix for the screen

	//----------- setting the projection -------------------------
	// glOrtho sets left, right, bottom, top, zNear, zFar of the chord system


	// glOrtho(-1, 1 + (w-400)/200.0 , -1 -(h-400)/200.0, 1, 1, -1); // resize type = add
	 glOrtho(-w/400.0, w/400.0, -h/400.0, h/400.0, -10, 10); // resize type = center

	//glOrtho(-4, 4, -4, 4, 2, -2);    // resize type = stretch

	//------------------------------------------------------------
}


//****************************************************
// sets the window up
//****************************************************
void initScene(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent

	myReshape(viewport.w, viewport.h);
}


Eigen::Vector3d curveInterp(Eigen::Vector3d v0, Eigen::Vector3d v1, Eigen::Vector3d v2, Eigen::Vector3d v3, double u){
	
	Vector3d A = v0*(1.0 - u) + v1*u;
	Vector3d B = v1*(1.0 - u) + v2*u;
	Vector3d C = v2*(1.0 - u) + v3*u;

	Vector3d D = A*(1 - u) + B*u; 
	Vector3d E = B*(1 - u) + C*u;

	Vector3d p = D*(1 - u) + E*u;

	return p; 
}

Eigen::Vector3d patchInterp(Patch patch, double u, double v){

	Vector3d vcurve0 = curveInterp(patch.q[0], patch.q[1], patch.q[2], patch.q[3], u);
	Vector3d vcurve1 = curveInterp(patch.q[4], patch.q[5], patch.q[6], patch.q[7], u);
	Vector3d vcurve2 = curveInterp(patch.q[8], patch.q[9], patch.q[10], patch.q[11], u);
	Vector3d vcurve3 = curveInterp(patch.q[12], patch.q[13], patch.q[14], patch.q[15], u);

	Vector3d ucurve0 = curveInterp(patch.q[0], patch.q[4], patch.q[8], patch.q[12], v);
	Vector3d ucurve1 = curveInterp(patch.q[1], patch.q[5], patch.q[9], patch.q[13], v);
	Vector3d ucurve2 = curveInterp(patch.q[2], patch.q[6], patch.q[10], patch.q[14], v);
	Vector3d ucurve3 = curveInterp(patch.q[3], patch.q[7], patch.q[11], patch.q[15], v);

	Vector3d p = curveInterp(vcurve0, vcurve1, vcurve2, vcurve3, v);
	return p; 
}

void glgenCurve(Eigen::Vector3d v0, Eigen::Vector3d v1, Eigen::Vector3d v2, Eigen::Vector3d v3, double u){
	glBegin(GL_LINE_STRIP);
	for (unsigned int i = 0; i <= (unsigned int) 1/u; ++i){
		Vector3d result = curveInterp(v0, v1, v2, v3, (double)(u*i));
		glVertex3f(result[0], result[1], result[2]);
	}
	glEnd();

}

void glUniformTesselate(Patch patch, unsigned int numdiv){
	double step = numdiv; 
	double stepsize = (double)(1 / step); 
	for (unsigned int u = 0; u<numdiv; ++u){
		for (unsigned int v = 0; v < numdiv; ++v){
			Vector3d A = patchInterp(patch, u*stepsize, v*stepsize);
			Vector3d B = patchInterp(patch, (1+u)*(stepsize), v*stepsize);
			Vector3d C = patchInterp(patch, (1+u)*(stepsize), (1+v)*(stepsize));
			Vector3d D = patchInterp(patch, u*stepsize, (1+v)*(stepsize));

			glBegin(GL_LINE_STRIP);
			glVertex3d(A[0], A[1], A[2]);
			glVertex3d(B[0], B[1], B[2]);
			glVertex3d(C[0], C[1], C[2]);
			glVertex3d(D[0], D[1], D[2]);
			glEnd();
		}
	}
}

void glgenWirePatch(Patch patch, double u){
	for (unsigned int i = 0; i < 4; ++i){
		glgenCurve(patch.q[4 * i], patch.q[4 * i + 1], patch.q[4 * i + 2], patch.q[4 * i + 3], u);
	}
	
	for (unsigned int i = 0; i < 4; ++i){
		glgenCurve(patch.q[i], patch.q[4 + i], patch.q[8 + i], patch.q[12 + i], u);
	}
	
}

void display() // adapted from http://stackoverflow.com/questions/13159444/opengl-draw-polygon-with-gl-lines-and-angle-between-lines
{
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glShadeModel(GL_FLAT);
	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	glRotatef(3, 1.0, 0.0, 0.0);
	glRotatef(.3, 0.0, 1.0, 0.0);
	glRotatef(.7, 0.0, 1.0, 1.0);

	vector<Eigen::Vector3d> tests; 

	glColor3ub(255, 0, 0);

	
	Vector3d v0(0.0, 0.0, 0.0);
	Vector3d v1(0.33, 0.0, 0.0);
	Vector3d v2(0.66, 0.0, 0.0);
	Vector3d v3(1.0, 0.0, 0.0);

	tests.push_back(v0);
	tests.push_back(v1);
	tests.push_back(v2);
	tests.push_back(v3);
	

	Vector3d v4(0.0, 0.0, .33);
	Vector3d v5(0.33, 0.33, .33);
	Vector3d v6(0.66, 0.33, .33);
	Vector3d v7(1.0, 0.00, .33);

	tests.push_back(v4);
	tests.push_back(v5);
	tests.push_back(v6);
	tests.push_back(v7);
	
	Vector3d v8(0.0, 0.0, .66);
	Vector3d v9(0.33, 0.33, .66);
	Vector3d v10(0.66, 0.33, .66);
	Vector3d v11(1.0, 0.0, .66);


	tests.push_back(v8);
	tests.push_back(v9);
	tests.push_back(v10);
	tests.push_back(v11);

	Vector3d v12(0.0, 0.0, 1);
	Vector3d v13(0.33, 0.0, 1);
	Vector3d v14(0.66, 0.0, 1);
	Vector3d v15(1.0, 0.0, 1);


	tests.push_back(v12);
	tests.push_back(v13);
	tests.push_back(v14);
	tests.push_back(v15);

	//glgenCurve(v0, v1, v2, v3, .01);
	
	glColor3f(1.0f, 0.0f, 0.0f);

	glUniformTesselate(tests, 20);

	//glgenWirePatch(tests, .01);

	/*
	glColor3f(0.0f, 0.0f, 1.0f);
	glBegin(GL_POINTS);

	glVertex3d(0.0, 0.0, 0);
	glVertex3d(0.33, 0.33, 0);
	glVertex3d(0.66, 0.33, 0);
	glVertex3d(1.0, 0.0, 0);


	glVertex3d(0.0, 0.1, 1);
	glVertex3d(0.33, 0.43, 1);
	glVertex3d(0.66, 0.43, 1);
	glVertex3d(1.0, 0.1, 1);
	glEnd();
	*/

	glutSwapBuffers();


}

//****************************************************
// called by glut when there are no messages to handle
//****************************************************
void myFrameMove() {
	//nothing here for now
#ifdef _WIN32
	Sleep(10);                                   //give ~10ms back to OS (so as not to waste the CPU)
#endif
	glutPostRedisplay(); // forces glut to call the display function (myDisplay())
}


//****************************************************
// the usual stuff, nothing exciting here
//****************************************************

void parseLine(const std::string& line) {
	istringstream iss(line);
	vector<string> tokens((istream_iterator<string>(iss)), istream_iterator<string>());


	if (tokens.size() == 0) {
		return;
	}
	else if (tokens.size() == 1) {
		patches = atoi(tokens[0].c_str());
	}

	else if (tokens.size() == 12) {
		for (int i = 0; i < 4; i++){
			coordinates.push_back(Eigen::Vector3d(atof(tokens[3 * i+0].c_str()), atof(tokens[3 * i + 1].c_str()), atof(tokens[3 * i + 2].c_str())));
		}	
	}
}

const char* filename;
int main(int argc, char *argv[]) {
	
	std::ifstream fin("test.bez");
	std::string line;
 
	if (!fin.good())
	{
		cout << "bad file name" << endl; 
	}
	else{
		//first load all text file data into "coordinates" buffer during parse stage. 
		//after parsing is done, transfer data from buffer into patches, and then clumb all patches together in "bezpatches" vector
		while (getline(fin, line)) {
			try {
				parseLine(line);

			}
			catch (invalid_argument& e) {
				cerr << e.what() << endl;
			}
		}
	}
	cout << patches << endl; 
	for (int j = 0; j < patches; j++){
		std::vector <Eigen::Vector3d> curve;
		for (int i = 0; i < 16; i++){
			curve.push_back(coordinates[j * 16 + i]);
		}
		Patch next(curve); 
		//just for testing
		//cout << "new patch" << endl;
		//for (int k = 0; k < 16;  k++){
		//	cout << curve[k] << endl;
		//}
		bezpatches.push_back(next);
	}




	glutInit(&argc, argv);

	//This tells glut to use a double-buffered window with red, green, and blue channels 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

	// Initalize theviewport size
	viewport.w = 400;
	viewport.h = 400;

	//The size and position of the window
	glutInitWindowSize(viewport.w, viewport.h);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("CS184!");

	initScene();                                 // quick function to set up scene

	glutDisplayFunc(display);                  // function to run when its time to draw something
	glutReshapeFunc(myReshape);                  // function to run when the window gets resized
	glutIdleFunc(myFrameMove);                   // function to run when not handling any other task
	glutMainLoop();                              // infinite loop that will keep drawing and resizing and whatever else

	return 0;
}














//for some reason, this function is shitty and juut doesn't work
//***************************************************
// function that does the actual drawing
//***************************************************
void myDisplay() {

	glClear(GL_COLOR_BUFFER_BIT);                // clear the color buffer (sets everything to black)

	glMatrixMode(GL_MODELVIEW);                  // indicate we are specifying camera transformations
	glLoadIdentity();                            // make sure transformation is "zero'd"


	Vector3d v0(0.0, 0.0, 0.0);
	Vector3d v1(0.33, 0.33, 0.0);
	Vector3d v2(0.66, 0.33, 0.0);
	Vector3d v3(1.0, 0.0, 0.0);

	//glgenCurve(v0, v1, v2, v3, .2);


	//----------------------- code to draw objects --------------------------
	// Rectangle Code
	//glColor3f(red component, green component, blue component);
	/*
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POLYGON);                         // draw rectangle
	//glVertex3f(x val, y val, z val (won't change the point because of the projection type));

	glVertex3f(0.0f, 0.0f, 0.0f);               // bottom left corner of rectangle
	glVertex3f(0.33f, 0.33f, 0.0f);
	glVertex3f(0.66f, 0.33f, 0.0f);
	glVertex3f(1.0f, 0.0f, 0.0f);
	glEnd();
	*/

	/*
	glColor3f(1.0f, 0.0f, 0.0f);                   // setting the color to pure red 90% for the rect
	glBegin(GL_POLYGON);                         // draw rectangle
	//glVertex3f(x val, y val, z val (won't change the point because of the projection type));
	float farl =1.1f;
	glVertex3f(-0.8f, 0.0f, farl);               // bottom left corner of rectangle
	glVertex3f(-0.8f, 0.5f, farl);               // top left corner of rectangle
	glVertex3f(0.0f, 0.5f, farl);               // top right corner of rectangle
	glVertex3f(0.0f, 0.0f, farl);               // bottom right corner of rectangle
	glEnd();
	// Triangle Code
	glColor3f(1.0f, 0.5f, 0.0f);                   // setting the color to orange for the triangle

	float basey = -sqrt(0.48f);                  // height of triangle = sqrt(.8^2-.4^2)
	glBegin(GL_POLYGON);
	glVertex3f(.5f, 0.0f, 0.0f);                // top tip of triangle
	glVertex3f(0.1f, basey, 0.0f);               // lower left corner of triangle
	glVertex3f(0.9f, basey, 0.0f);               // lower right corner of triangle
	glEnd();
	//-----------------------------------------------------------------------
	*/
	glFlush();
	glutSwapBuffers();                           // swap buffers (we earlier set double buffer)
}


/*
void glgenCurve(Eigen::Vector3d v0, Eigen::Vector3d v1, Eigen::Vector3d v2, Eigen::Vector3d v3, double u){
glBegin(GL_LINE_STRIP);
for (unsigned int i = 0; i <= (unsigned int) 1/u; ++i){
Vector3d result = curveInterp(v0, v1, v2, v3, (double) (u*i));
glVertex3f(result[0], result[1], result[2]);
}
glEnd();

}

void glgenWirePatch(Patch patch, double u){
for (unsigned int i = 0; i < 4 ; ++i){
glgenCurve(patch.q[4*i], patch.q[4*i + 1], patch.q[4*i + 2], patch.q[4*i + 3], u);
}

for (unsigned int i = 0; i < 4; ++i){
glgenCurve(patch.q[i], patch.q[4  + i], patch.q[8 + i], patch.q[12 + i], u);
}
}
*/


/*
Vector3d v0(0.0, 0.0, 0.0);
Vector3d v1(0.33, 0.33, 0.0);
Vector3d v2(0.66, 0.33, 0.0);
Vector3d v3(1.0, 0.0, 0.0);

tests.push_back(v0);
tests.push_back(v1);
tests.push_back(v2);
tests.push_back(v3);


Vector3d v4(0.0, 0.1, 1);
Vector3d v5(0.33, 0.43, 1);
Vector3d v6(0.66, 0.43, 1);
Vector3d v7(1.0, 0.1, 1);



tests.push_back(v4);
tests.push_back(v5);
tests.push_back(v6);
tests.push_back(v7);


Vector3d v8(0.0, 0.2, 2);
Vector3d v9(0.33, 0.53, 2);
Vector3d v10(0.66, 0.53, 2);
Vector3d v11(1.0, 0.2, 2);


tests.push_back(v8);
tests.push_back(v9);
tests.push_back(v10);
tests.push_back(v11);

Vector3d v12(0.0, 0.3, 3);
Vector3d v13(0.33, 0.63, 3);
Vector3d v14(0.66, 0.63, 3);
Vector3d v15(1.0, 0.3, 3);


tests.push_back(v12);
tests.push_back(v13);
tests.push_back(v14);
tests.push_back(v15);

*/