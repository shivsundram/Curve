// Curve.cpp : Defines the entry point for the console application.
//
//andrey, leave stdafx.h out of your code. this is a stupid windows thing
#include "stdafx.h"
// Curve.cpp : Defines the entry point for the console application.
//
//andrey, leave stdafx.h out of your code. this is a stupid windows thing

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
#include <list>
#include <queue>


#include "Shape.h"
// #include "Shape.cpp"



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

using namespace std;
using namespace Eigen;


class Viewport {
public:
	int w, h; // width and height
};


class Patch{
public:
	vector <Vector3d> q;
	Patch(vector <Vector3d> q);
};

Patch::Patch(vector <Vector3d> q1){
	q = q1;
}

//****************************************************
// Global Variables
//****************************************************
Viewport    viewport;
int numPatches;
vector <Vector3d> coordinates;
vector <Patch> bezpatches;
vector <Shape*> faces;

int rotateX = 0;
int rotateY = 0;
double tessArg = 0.01;
double translateX = 0;
double translateY = 0;
double scale = 1;

bool isWireframe = false;
bool isSmoothShading = false;

const double TRANSLATE_DELTA = 0.04;
const int ROTATE_DELTA = 3;
const double SCALE_DELTA = 0.5;



void mySpecialKeys(int key, int x, int y) {



	int mod = glutGetModifiers();
	bool isShift = false;

	switch (mod) {
	case GLUT_ACTIVE_SHIFT:
		isShift = true;
		break;
	}


	switch (key) {

	case GLUT_KEY_LEFT:
		if (isShift) {
			translateX -= TRANSLATE_DELTA;
		}
		else {
			rotateY -= ROTATE_DELTA;
		}
		break;
	case GLUT_KEY_RIGHT:
		if (isShift) {
			translateX += TRANSLATE_DELTA;
		}
		else {
			rotateY += ROTATE_DELTA;
		}
		break;
	case GLUT_KEY_UP:
		if (isShift) {
			translateY += TRANSLATE_DELTA;
		}
		else {
			rotateX -= ROTATE_DELTA;
		}
		break;
	case GLUT_KEY_DOWN:
		if (isShift) {
			translateY -= TRANSLATE_DELTA;
		}
		else {
			rotateX += ROTATE_DELTA;
		}
		break;
	default:
		cout << "Special key " << key << endl;
	}

	glutPostRedisplay();
}

//---+----3----+----2----+----1----+---<>---+----1----+----2----+----3----+----4

void myKeyboard(unsigned char key, int x, int y) {
	switch (key) {
	case '+':
		scale /= SCALE_DELTA;
		break;
	case '-':
		scale *= SCALE_DELTA;
		break;
	case 'w':
		isWireframe = isWireframe ? false : true;
		break;
	case 's':
		isSmoothShading = isSmoothShading ? false : true;
		break;
	default:
		cout << "myKeyboard " << key << endl;
	}

	glutPostRedisplay();
}



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
	glOrtho(-w / 400.0, w / 400.0, -h / 400.0, h / 400.0, 2, -2); // resize type = center

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


Vector3d deCasteljau(const Vector3d& v0, const Vector3d& v1, const Vector3d& v2, const Vector3d& v3, double u){
	return pow((1 - u), 3)*v0 + 3 * pow((1 - u), 2)*u*v1 + 3 * (1 - u)*pow(u, 2)*v2 + pow(u, 3)*v3;
}


void glgenCurve(const Vector3d& v0, const Vector3d& v1, const Vector3d& v2, const Vector3d& v3, double u){
	glBegin(GL_LINE_STRIP);
	for (unsigned int i = 0; i <= (unsigned int)1 / u; ++i){
		Vector3d result = deCasteljau(v0, v1, v2, v3, (double)(u*i));
		glVertex3f(result[0], result[1], result[2]);
	}
	glEnd();
}



LocalInfo curveInterp(Vector3d v0, Vector3d v1, Vector3d v2, Vector3d v3, double u) {
	Vector3d A = v0 * (1.0 - u) + v1 * u;
	Vector3d B = v1 * (1.0 - u) + v2 * u;
	Vector3d C = v2 * (1.0 - u) + v3 * u;

	Vector3d D = A * (1.0 - u) + B * u;
	Vector3d E = B * (1.0 - u) + C * u;

	Vector3d p = D * (1.0 - u) + E * u;

	Vector3d np = 3.0 * (E - D);

	return LocalInfo(p, np);
}



LocalInfo patchInterp(Patch patch, double u, double v) {

	Vector3d vcurve0 = curveInterp(patch.q[0], patch.q[1], patch.q[2], patch.q[3], u).point;
	Vector3d vcurve1 = curveInterp(patch.q[4], patch.q[5], patch.q[6], patch.q[7], u).point;
	Vector3d vcurve2 = curveInterp(patch.q[8], patch.q[9], patch.q[10], patch.q[11], u).point;
	Vector3d vcurve3 = curveInterp(patch.q[12], patch.q[13], patch.q[14], patch.q[15], u).point;

	Vector3d ucurve0 = curveInterp(patch.q[0], patch.q[4], patch.q[8], patch.q[12], v).point;
	Vector3d ucurve1 = curveInterp(patch.q[1], patch.q[5], patch.q[9], patch.q[13], v).point;
	Vector3d ucurve2 = curveInterp(patch.q[2], patch.q[6], patch.q[10], patch.q[14], v).point;
	Vector3d ucurve3 = curveInterp(patch.q[3], patch.q[7], patch.q[11], patch.q[15], v).point;

	LocalInfo infv = curveInterp(vcurve0, vcurve1, vcurve2, vcurve3, v);
	LocalInfo infu = curveInterp(ucurve0, ucurve1, ucurve2, ucurve3, u);

	Vector3d p = infu.point;
	Vector3d dPdv = infv.normal;
	Vector3d dPdu = infu.normal;

	Vector3d n = dPdu.cross(dPdv);
	n.normalize();

	return LocalInfo(p, n);
}


void uniformTesselate(Patch patch, double step){
	int numdiv = 1.0 / step;
	for (int u = 0; u < numdiv; ++u){
		for (int v = 0; v < numdiv; ++v){
			LocalInfo A = patchInterp(patch, u*step, v*step);
			LocalInfo B = patchInterp(patch, (1 + u) * step, v * step);
			LocalInfo C = patchInterp(patch, (1 + u) * step, (1 + v) * step);
			LocalInfo D = patchInterp(patch, u*step, (1 + v) * step);

			Quad* q = new Quad(A.point, B.point, C.point, D.point, A.normal, B.normal, C.normal, D.normal);
			faces.push_back(q);
		}
	}
}


class PatchTri{
	//ACTUALLY THESE ARE 2D VECTORS. BUT I GET ALIGNMENT ERRORS WHEN I USE VECTOR2D
public: 
	Vector3d vertexA; 
	Vector3d vertexB;
	Vector3d vertexC;
	PatchTri(const Vector3d&, const Vector3d&, const Vector3d&);
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

PatchTri::PatchTri(const Vector3d& A, const Vector3d& B, const Vector3d& C){
	vertexA = A; 
	vertexB = B; 
	vertexC = C; 
}

class testResults{
public:
	bool flatEnough; 
	vector <PatchTri> newTriangles; 
	testResults(bool isFlat){ flatEnough = isFlat;}
	testResults(bool isFlat, vector <PatchTri> triangles);
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

testResults::testResults(bool isFlat, vector <PatchTri> triangles){
	flatEnough = isFlat;
	newTriangles = triangles;
}

testResults edgeTests(Patch patch,const PatchTri& tri, double error){
	//pass in triangle u,v values
	//		C		
	//     / \
	//	  /   \
	//   A --- B
	
	//evaluate corners of triangle. remember patch tri has 2d vectors of (u,v) values
	Vector3d A = patchInterp(patch, tri.vertexA[0], tri.vertexA[1]).point;
	Vector3d B = patchInterp(patch, tri.vertexB[0], tri.vertexB[1]).point;
	Vector3d C = patchInterp(patch, tri.vertexC[0], tri.vertexC[1]).point;

	//calculate midpoints of triangle by averaging evaluated corners
	Vector3d Trileft = (A + C) / 2; 
	Vector3d Triright = (B + C) / 2;
	Vector3d Tribottom = (A + B) / 2;
	Vector3d Tricenter = (A + B+ C) / 3;

	//generate midpoint u,v values of triangle for evaluation. all 2d vectors
	Vector3d left = (tri.vertexA + tri.vertexC) / 2;
	Vector3d right = (tri.vertexB + tri.vertexC) / 2;
	Vector3d bottom = (tri.vertexA + tri.vertexB) / 2;
	Vector3d center = (tri.vertexA + tri.vertexB + tri.vertexC) / 3;

	//evaluate at midpoints using u,v values
	Vector3d Bezleft =  patchInterp(patch, left[0], left[1]).point;
	Vector3d Bezright = patchInterp(patch, right[0], right[1]).point;
	Vector3d Bezbottom = patchInterp(patch, bottom[0], bottom[1]).point;
	Vector3d Bezcenter = patchInterp(patch, center[0], center[1]).point;

	//do tests, comparing triangle midpoints(TRI) and evaluated midpoints (Bez)
	bool lefttest = (Trileft - Bezleft).norm() < error ;
	bool righttest = (Trileft - Bezleft).norm() < error;
	bool bottomtest = (Trileft - Bezleft).norm() < error;
	bool centertest = (Trileft - Bezleft).norm() < error;
	
	
	vector <PatchTri> newTriangles;
	//case 1
	
	if (lefttest && righttest && bottomtest){
		return testResults(true);
		cout << "done" << endl;
	}
	//case 2.1
	else if (!lefttest && righttest && bottomtest){
		newTriangles.push_back(PatchTri(tri.vertexA, tri.vertexB, left));
		newTriangles.push_back(PatchTri(tri.vertexB, tri.vertexC, left));
	}

	//case 2.2
	else if (lefttest && !righttest && bottomtest){
		newTriangles.push_back(PatchTri(tri.vertexA, right, tri.vertexB));
		newTriangles.push_back(PatchTri(tri.vertexA, tri.vertexC, right));
	}

	//case 2.2
	else if (lefttest && righttest && !bottomtest){
		newTriangles.push_back(PatchTri(tri.vertexA, tri.vertexC, bottom));
		newTriangles.push_back(PatchTri(tri.vertexB, tri.vertexC, bottom));
	}


	//case 3
	
	else if (!lefttest && !righttest && bottomtest ){
		newTriangles.push_back(PatchTri(tri.vertexB, left, tri.vertexA));
		newTriangles.push_back(PatchTri(tri.vertexB,right, left));
		newTriangles.push_back(PatchTri(right, tri.vertexC, left));
	}

	else if (!lefttest && righttest && !bottomtest){
		newTriangles.push_back(PatchTri(tri.vertexA, bottom, left));
		newTriangles.push_back(PatchTri(tri.vertexC, left, bottom));
		newTriangles.push_back(PatchTri(bottom, tri.vertexB, tri.vertexC));
	}


	else if (lefttest && !righttest && !bottomtest){
		newTriangles.push_back(PatchTri(tri.vertexA, bottom, right));
		newTriangles.push_back(PatchTri(bottom, tri.vertexB, right));
		newTriangles.push_back(PatchTri(left, right, tri.vertexC));
	}


	//case 4
	else if (!lefttest && !righttest && !bottomtest){
		newTriangles.push_back(PatchTri(tri.vertexA, bottom, left));
		newTriangles.push_back(PatchTri(bottom, right, left));
		newTriangles.push_back(PatchTri(bottom, tri.vertexB, right));
		newTriangles.push_back(PatchTri(left, right, tri.vertexC));
		
	}

	
	else{
		 return testResults(true);
	 }

	//case 5
	
//	else if (lefttest && righttest && bottomtest && !centertest){
	//	newTriangles.push_back(PatchTri(tri.vertexA, tri.vertexB, center));
	//	newTriangles.push_back(PatchTri(tri.vertexB, tri.vertexC, center));
	//	newTriangles.push_back(PatchTri(tri.vertexA, center, tri.vertexC));
	
		
		return testResults(false, newTriangles);
		//}
	/*
	//case 6
	else if (!lefttest && righttest && bottomtest && !centertest){
		newTriangles.push_back(PatchTri(tri.vertexA, tri.vertexB, center));
		newTriangles.push_back(PatchTri(tri.vertexB, tri.vertexC, center));
		newTriangles.push_back(PatchTri(tri.vertexA, center, left));
		newTriangles.push_back(PatchTri(left, center, tri.vertexC));
	}

	//case 7
	else if (!lefttest && !righttest && bottomtest && !centertest){
		newTriangles.push_back(PatchTri(tri.vertexA, tri.vertexB, center));

		newTriangles.push_back(PatchTri(tri.vertexA, center, left));
		newTriangles.push_back(PatchTri(tri.vertexA, center, left));

		newTriangles.push_back(PatchTri(tri.vertexA, center, left));
		newTriangles.push_back(PatchTri(left, center, tri.vertexC));
	}

	//case 8
	*/


}

void adaptiveTriangulate(Patch patch, double error){
	std::queue<PatchTri> queue;
	vector<PatchTri> triangulation; 

	//create triangle tri,s add to patch tri vector, run tesselation, passing in 
	PatchTri first(Vector3d(0, 0,0), Vector3d(1, 0,0), Vector3d(0, 1,0)); 
	PatchTri second(Vector3d(1, 1,0), Vector3d(0, 1,0), Vector3d(1, 0,0)); //THIS IS WHERE THE BUG WAS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	queue.push(first);
	queue.push(second);
	unsigned int j = 0; 
	while (!queue.empty()){
		PatchTri popped = queue.front();
		testResults result = edgeTests(patch, popped, error);
		
		if (result.flatEnough){
			queue.pop();
			triangulation.push_back(popped);
		}
		else{
			queue.pop();
			for (unsigned int i = 0; i < result.newTriangles.size(); ++i){				
				triangulation.push_back(result.newTriangles[i]);
				cout << "popped" << endl; 
			}
		}
		++j; 
	}

	for (unsigned int i=0; i < triangulation.size(); ++i){
		PatchTri first = triangulation[i];
		LocalInfo A = patchInterp(patch, first.vertexA[0], first.vertexA[1]);
		LocalInfo B = patchInterp(patch, first.vertexB[0], first.vertexB[1]);
		LocalInfo C = patchInterp(patch, first.vertexC[0], first.vertexC[1]);

		Triangle* t = new Triangle(A.point, B.point, C.point, A.normal, B.normal, C.normal);
		faces.push_back(t);
	}
}


void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

	GLfloat lightColor0[] = { 0.5f, 0.5f, 0.5f, 1.0f }; //Color (0.5, 0.5, 0.5)
	GLfloat lightPos0[] = { 4.0f, 4.0f, -4.0f, 1.0f }; //Positioned at (4, 4, 4)
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);


	GLfloat lightColor1[] = { 0.5f, 0.5f, 0.5f, 1.0f }; //Color (0.5, 0.5, 0.5)
	GLfloat lightPos1[] = { -4.0f, -4.0f, -4.0f, 1.0f }; //Positioned at (4, 4, 4)
	glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
	glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);


	if (isSmoothShading) {
		glShadeModel(GL_SMOOTH);
	}
	else {
		glShadeModel(GL_FLAT);
	}

	if (isWireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
	}
	else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}


	// glColor3ub(255, 255, 255);

	glPushMatrix();

	glTranslated(translateX, translateY, 0);
	glRotated(rotateX, 1, 0, 0);  // Up and down arrow keys 'tip' view.
	glRotated(rotateY, 0, 1, 0);  // Right/left arrow keys 'turn' view.
	glScaled(scale, scale, scale);
	for (int i = 0; i < faces.size(); i++) {
		faces[i]->draw();
	}

	glPopMatrix();

	glFlush();
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

void parseLine(const string& line) {
	istringstream iss(line);
	vector<string> tokens((istream_iterator<string>(iss)), istream_iterator<string>());


	if (tokens.size() == 0) {
		return;
	}
	else if (tokens.size() == 1) {
		numPatches = atoi(tokens[0].c_str());
	}
	else if (tokens.size() == 12) {
		for (int i = 0; i < 4; i++){
			coordinates.push_back(Vector3d(atof(tokens[3 * i + 0].c_str()), atof(tokens[3 * i + 1].c_str()), atof(tokens[3 * i + 2].c_str())));
		}
	}
}



int main(int argc, char *argv[]) {

	ifstream fin;
	string line;

	if (argc >= 2) {

		if (argc >= 3) {
			tessArg = atof(argv[2]);
		}

		fin.open(argv[1]);


		if (!fin.good()) {
			cout << "File \"" << argv[1] << "\" does not exists" << endl;
			return 1;
		}

		//after parsing is done, transfer data from buffer into numPatches, and then clumb all numPatches together in "bezpatches" vector
		while (getline(fin, line)) {

			try {
				parseLine(line);
			}
			catch (invalid_argument& e) {
				cerr << e.what() << endl;
			}
		}

		fin.close();
	}
	else {
		cout << "Invalid argument." << endl;
	}


	cout << numPatches << endl;
	for (int j = 0; j < numPatches; j++){
		vector <Vector3d> curve;
		for (int i = 0; i < 16; i++){
			curve.push_back(coordinates[j * 16 + i]);
		}
		Patch next(curve);

		bezpatches.push_back(next);
	}

	for (int i = 0; i < numPatches; i++) {
		//uniformTesselate(bezpatches[i], tessArg);
		adaptiveTriangulate(bezpatches[i], tessArg);
	}


	glutInit(&argc, argv);

	//This tells glut to use a double-buffered window with red, green, and blue channels
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);


	// Initalize theviewport size
	viewport.w = 800;
	viewport.h = 800;

	//The size and position of the window
	glutInitWindowSize(viewport.w, viewport.h);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("CS184 Assignment 3");

	initScene();                                 // quick function to set up scene

	glEnable(GL_DEPTH_TEST);


	glutDisplayFunc(display);                  // function to run when its time to draw something
	glutReshapeFunc(myReshape);                  // function to run when the window gets resized
	glutKeyboardFunc(myKeyboard);
	glutSpecialFunc(mySpecialKeys);
	glutIdleFunc(myFrameMove);                   // function to run when not handling any other task
	glutMainLoop();                              // infinite loop that will keep drawing and resizing and whatever else

	return 0;
}

