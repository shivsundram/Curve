// Curve.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <vector>
#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h> 
#include <string>
#include <fstream>
using std::ifstream;



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
	 glOrtho(-w/400.0, w/400.0, -h/400.0, h/400.0, 2, -2); // resize type = center

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


//***************************************************
// function that does the actual drawing
//***************************************************
void myDisplay() {

	glClear(GL_COLOR_BUFFER_BIT);                // clear the color buffer (sets everything to black)

	glMatrixMode(GL_MODELVIEW);                  // indicate we are specifying camera transformations
	glLoadIdentity();                            // make sure transformation is "zero'd"

	//----------------------- code to draw objects --------------------------
	// Rectangle Code
	//glColor3f(red component, green component, blue component);
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

	glFlush();
	glutSwapBuffers();                           // swap buffers (we earlier set double buffer)
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
		cout << patches << endl; 
		cout << "cool" << endl;
	}

	else if (tokens.size() == 12) {
		for (int i = 0; i < 4; i++){
			coordinates.push_back(Eigen::Vector3d(atof(tokens[3 * i+0].c_str()), atof(tokens[3 * i + 1].c_str()), atof(tokens[3 * i + 2].c_str())));
		}	
	}
}

const char* filename;
int main(int argc, char *argv[]) {
	//This initializes glut


	
	std::ifstream fin("test.bez");
	std::string line;
 
	if (!fin.good())
	{
		cout << "wtf" << endl; 
	}
	else{
		while (getline(fin, line)) {
			try {
				parseLine(line);

			}
			catch (invalid_argument& e) {
				cerr << e.what() << endl;
			}
		}
	}
	cout << "shit" << endl;
	cout << patches << endl; 
	for (int j = 0; j < patches; j++){
		std::vector <Eigen::Vector3d> curve;
		for (int i = 0; i < 16; i++){
			curve.push_back(coordinates[j * 16 + i]);
			cout << "deon" << endl;
		}
		Patch next(curve); 
		cout << "new patch" << endl;
		for (int k = 0; k < 16;  k++){
			cout << curve[k] << endl;
		}
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

	glutDisplayFunc(myDisplay);                  // function to run when its time to draw something
	glutReshapeFunc(myReshape);                  // function to run when the window gets resized
	glutIdleFunc(myFrameMove);                   // function to run when not handling any other task
	glutMainLoop();                              // infinite loop that will keep drawing and resizing and whatever else

	return 0;
}


