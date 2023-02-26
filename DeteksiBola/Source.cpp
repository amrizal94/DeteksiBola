/**
  @file videocapture_basic.cpp
  @brief A very basic sample for using VideoCapture and VideoWriter
  @author PkLab.net
  @date Aug 24, 2016
*/
//#include <opencv2/core.hpp>
//#include <opencv2/videoio.hpp>
//#include <opencv2/imgproc.hpp>
//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <WS2tcpip.h>
#include <opencv2/highgui.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>
//#include <opencv2/features2d.hpp>
#include <iostream>
#include <msclr\marshal_cppstd.h>

using namespace System;
using namespace cv;
using namespace std;
using namespace System::IO;
using namespace System::IO::Ports;
//for file or text management
//nt derajat2;
//int sisiC2;
//string arahputar, pwmmaju;
Mat HSV;
Mat thresholdHSV;
int H_MIN = 0;
int S_MIN = 0;
int V_MIN = 0;
int H_MAX = 255;
int S_MAX = 255;
int V_MAX = 255;
int nilaiopening = 3;
int nilaiclosing = 3;
int nilaierotionbola;
int nilaidilationbola;
int nilaigaussionblur;
string namaport;

//default capture width and height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 3.14159265358979323846 * pow(10, 2);
const int MAX_OBJECT_AREA = FRAME_HEIGHT * FRAME_WIDTH / 1.5;
//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;
const Point CENTER_FRAME(FRAME_WIDTH / 2, FRAME_HEIGHT / 2);
//names that will appear at the top of each window
const string trackbarWindowName = "Trackbars HSV";
bool mouseIsDragging;//used for showing a rectangle on screen as user clicks and drags mouse
bool mouseMove;
bool rectangleSelected;
cv::Point initialClickPoint, currentMousePoint; //keep track of initial point clicked and current position of mouse
cv::Rect rectangleROI; //this is the ROI that the user has selected
vector<int> H_ROI, S_ROI, V_ROI;// HSV values from the click/drag ROI region stored in separate vectors so that we can sort them easily

void simpandata()
{
	StreamWriter^ tulis_file = gcnew StreamWriter("nilai_HSV.txt");
	tulis_file->WriteLine("Data HSV deteksi bola, erosi, dilasi, dan smooting filter");
	tulis_file->WriteLine("urutan H_MIN, S_MIN, V_MIN, H_MAX, S_MAX, V_MAX, erosi, dilasi dan smooting");
	tulis_file->WriteLine(H_MIN);
	tulis_file->WriteLine(S_MIN);
	tulis_file->WriteLine(V_MIN);
	tulis_file->WriteLine(H_MAX);
	tulis_file->WriteLine(S_MAX);
	tulis_file->WriteLine(V_MAX);
	tulis_file->WriteLine(nilaiopening);
	tulis_file->WriteLine(nilaierotionbola);
	tulis_file->WriteLine(nilaidilationbola);
	tulis_file->WriteLine(nilaiclosing);
	tulis_file->WriteLine(nilaigaussionblur);
	tulis_file->WriteLine(DateTime::Now);
	tulis_file->Close();
}

void on_trackbar(int, void*)
{//This function gets called whenever a
	// trackbar position is changed

	//for now, this does nothing.

	//if (nilaierotionbola == 0) nilaierotionbola = 1;
	//if (nilaidilationbola == 0) nilaidilationbola = 1;
	//if (nilaigaussionblur == 0) nilaigaussionblur = 1;

	simpandata();
}
void createTrackbars()
{
	//create window for trackbars
	namedWindow(trackbarWindowName);
	resizeWindow(trackbarWindowName, 400, 400);
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, 255, on_trackbar);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, 255, on_trackbar);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, 255, on_trackbar);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, 255, on_trackbar);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, 255, on_trackbar);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, 255, on_trackbar);
	createTrackbar("Opening", trackbarWindowName, &nilaiopening, 8, on_trackbar);
	createTrackbar("Erode", trackbarWindowName, &nilaierotionbola, 8, on_trackbar);
	createTrackbar("Dilate", trackbarWindowName, &nilaidilationbola, 8, on_trackbar);
	createTrackbar("Closing", trackbarWindowName, &nilaiclosing, 8, on_trackbar);
	createTrackbar("GaussionBlur", trackbarWindowName, &nilaigaussionblur, 8, on_trackbar);
	simpandata();
}

void clickAndDrag_Rectangle(int event, int x, int y, int flags, void* param) {
	//only if calibration mode is true will we use the mouse to change HSV values

	//get handle to video feed passed in as "param" and cast as Mat pointer
	Mat* videoFeed = (Mat*)param;

	if (event == EVENT_LBUTTONDOWN && mouseIsDragging == false)
	{
		//keep track of initial point clicked
		initialClickPoint = cv::Point(x, y);
		//user has begun dragging the mouse
		mouseIsDragging = true;
	}
	/* user is dragging the mouse */
	if (event == EVENT_MOUSEMOVE && mouseIsDragging == true)
	{
		//keep track of current mouse point
		currentMousePoint = cv::Point(x, y);
		//user has moved the mouse while clicking and dragging
		mouseMove = true;
	}
	/* user has released left button */
	if (event == EVENT_LBUTTONUP && mouseIsDragging == true)
	{
		//set rectangle ROI to the rectangle that the user has selected
		rectangleROI = Rect(initialClickPoint, currentMousePoint);

		//reset boolean variables
		mouseIsDragging = false;
		mouseMove = false;
		rectangleSelected = true;
	}

	if (event == EVENT_RBUTTONDOWN) {
		//user has clicked right mouse button
		//Reset HSV Values
		H_MIN = 0;
		S_MIN = 0;
		V_MIN = 0;
		H_MAX = 255;
		S_MAX = 255;
		V_MAX = 255;
		createTrackbars();
	}
	if (event == EVENT_MBUTTONDOWN) {

		//user has clicked middle mouse button
		//enter code here if needed.
	}

}

void recordHSV_Values(cv::Mat frame, cv::Mat hsv_frame) {

	//save HSV values for ROI that user selected to a vector
	if (mouseMove == false && rectangleSelected == true) {

		//clear previous vector values
		if (H_ROI.size() > 0) H_ROI.clear();
		if (S_ROI.size() > 0) S_ROI.clear();
		if (V_ROI.size() > 0)V_ROI.clear();
		//if the rectangle has no width or height (user has only dragged a line) then we don't try to iterate over the width or height
		if (rectangleROI.width < 1 || rectangleROI.height < 1) cout << "Please drag a rectangle, not a line" << endl;
		else {
			for (int i = rectangleROI.x; i < rectangleROI.x + rectangleROI.width; i++) {
				//iterate through both x and y direction and save HSV values at each and every point
				for (int j = rectangleROI.y; j < rectangleROI.y + rectangleROI.height; j++) {
					//save HSV value at this point
					H_ROI.push_back((int)hsv_frame.at<cv::Vec3b>(j, i)[0]);
					S_ROI.push_back((int)hsv_frame.at<cv::Vec3b>(j, i)[1]);
					V_ROI.push_back((int)hsv_frame.at<cv::Vec3b>(j, i)[2]);
				}
			}
		}
		//reset rectangleSelected so user can select another region if necessary
		rectangleSelected = false;
		//set min and max HSV values from min and max elements of each array

		if (H_ROI.size() > 0) {
			//NOTE: min_element and max_element return iterators so we must dereference them with "*"
			H_MIN = *std::min_element(H_ROI.begin(), H_ROI.end());
			H_MAX = *std::max_element(H_ROI.begin(), H_ROI.end());
			cout << "MIN 'H' VALUE: " << H_MIN << endl;
			cout << "MAX 'H' VALUE: " << H_MAX << endl;
		}
		if (S_ROI.size() > 0) {
			S_MIN = *std::min_element(S_ROI.begin(), S_ROI.end());
			S_MAX = *std::max_element(S_ROI.begin(), S_ROI.end());
			cout << "MIN 'S' VALUE: " << S_MIN << endl;
			cout << "MAX 'S' VALUE: " << S_MAX << endl;
		}
		if (V_ROI.size() > 0) {
			V_MIN = *std::min_element(V_ROI.begin(), V_ROI.end());
			V_MAX = *std::max_element(V_ROI.begin(), V_ROI.end());
			cout << "MIN 'V' VALUE: " << V_MIN << endl;
			cout << "MAX 'V' VALUE: " << V_MAX << endl;
		}
		//create slider bars for HSV filtering
		createTrackbars();
	}

	if (mouseMove == true) {
		//if the mouse is held down, we will draw the click and dragged rectangle to the screen
		rectangle(frame, initialClickPoint, cv::Point(currentMousePoint.x, currentMousePoint.y), cv::Scalar(0, 255, 0), 1, 8, 0);
	}
}

string intToString(int number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

string doubleToString(double number)
{
	std::stringstream ss;
	ss << number;
	return ss.str();
}

void anglemeasuring(int x, int y, int& sisiC, int& derajat, Mat& frame)
{
	line(frame, Point(x, y), CENTER_FRAME, Scalar(0, 255, 0), 2);
	rectangle(frame, Point(CENTER_FRAME.x - 40, CENTER_FRAME.y - 40), Point(CENTER_FRAME.x + 40, CENTER_FRAME.y + 40), Scalar(255, 0, 0), 2);
	int sisiA = CENTER_FRAME.x - x;
	if (sisiA < 0)sisiA = sisiA * -1;
	int sisiB = CENTER_FRAME.y - y;
	if (sisiB < 0)sisiB = sisiB * -1;
	sisiC = sqrt(pow(sisiA, 2) + pow(sisiB, 2));

	//int sisiA2 = 320 - x;
	//if (sisiA2 < 0)sisiA2 = sisiA2 * -1;
	//int sisiB2 = 378 - y;
	//if (sisiB2 < 0)sisiB2 = sisiB2 * -1;
	//sisiC2 = sqrt(pow(sisiA2, 2) + pow(sisiB2, 2));

	if (x > CENTER_FRAME.x)
	{
		if (y > CENTER_FRAME.y)derajat = ((sisiB * 90) / sisiC) + 90;
		else
		{
			derajat = (sisiA * 90) / sisiC;
			//derajat2 = (sisiA2 * 90) / sisiC2;
			//arahputar = "ee";
		}
	}
	else
	{
		if (y > CENTER_FRAME.y)derajat = ((sisiA * 90) / sisiC) + 180;
		else
		{
			//SimpleBlobDetector::Params params;
			derajat = ((sisiB * 90) / sisiC) + 270;
			//derajat2 = (sisiA2 * 90) / sisiC2;
			//arahputar = "e";
		}

	}
	if (derajat == 360)derajat = 0;
	putText(frame, intToString(derajat), Point(CENTER_FRAME.x - 40, CENTER_FRAME.y - 40), 1.5, 1.5, Scalar(0, 255, 0), 2);
}

void drawObject(int x, int y, int r, int& sisiC, int& derajat, Mat& frame) {

	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!


	//'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window)

	circle(frame, Point(x, y), r, Scalar(255, 0, 0), 2);
	if (y - r > 0)
		line(frame, Point(x, y), Point(x, y - r), Scalar(255, 0, 0), 2);
	else line(frame, Point(x, y), Point(x, 0), Scalar(255, 0, 0), 2);
	if (y + r < FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + r), Scalar(255, 0, 0), 2);
	else line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(255, 0, 0), 2);
	if (x - r > 0)
		line(frame, Point(x, y), Point(x - r, y), Scalar(255, 0, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(255, 0, 0), 2);
	if (x + r < FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + r, y), Scalar(255, 0, 0), 2);
	else line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(255, 0, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);

	anglemeasuring(x, y, sisiC, derajat, frame);
}

void trackFilteredObject(int& x, int& y, int& r, int& sisiC, int& derajat, Mat threshold, Mat& cameraFeed) {
	// Set up the detector with default parameters.
	//SimpleBlobDetector detector;
	Mat temp(threshold);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	vector<Point> approxCircle;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));
	
	//use moments method to find our filtered object
	double refArea = 0;
	//int largestIndex = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0) {
		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (hierarchy.size() < MAX_NUM_OBJECTS) {
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Moments moment = moments((cv::Mat)contours[index]);
				approxPolyDP(contours[index], approxCircle, arcLength(Mat(contours[index]), true) * 0.05, true);
				if (approxCircle.size() > 4)
				{
					//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we save a reference area each
				//iteration and compare it to the area in the next iteration.
				//vector<Point> approxCircle;
				//approxPolyDP(contours[index], approxCircle, arcLength(Mat(contours[index]), true) * 0.05, true);
				//r = approxCircle.size();
					if (moment.m00 > MIN_OBJECT_AREA&& moment.m00<MAX_OBJECT_AREA && moment.m00 > refArea) {
						// && approxCircle.size() > 75
						x = moment.m10 / moment.m00;
						y = moment.m01 / moment.m00;
						refArea = moment.m00;
						objectFound = true;

						//save index of largest contour to use with drawContours
						//largestIndex = index;
					}
					else
					{
						sisiC = 0;
						derajat = 0;
						objectFound = false;
						//arahputar = "b";
					}
				}
				
			}
			r = sqrt(refArea / 3.14159265358979323846);
			//let user know you found an object
			if (objectFound) {
				putText(cameraFeed, "Tracking Object ", Point(0, 80), 2, 1, Scalar(0, 255, 0), 2);
				//draw object location on screen
				drawObject(x, y, r, sisiC, derajat, cameraFeed);
				//draw largest contour
				//drawContours(cameraFeed, contours, largestIndex, Scalar(0, 255, 255), 2);
			}
		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 80), 1, 2, Scalar(0, 0, 255), 2);
	}
}

void ambildatatersimpan()
{
	StreamReader^ baca_file = gcnew StreamReader("nilai_HSV.txt");
	System::String^ bantu;
	bantu = baca_file->ReadLine();	// keterangan atas
	bantu = baca_file->ReadLine();	// keterangan atas
	H_MIN = Convert::ToInt16(baca_file->ReadLine());
	S_MIN = Convert::ToInt16(baca_file->ReadLine());
	V_MIN = Convert::ToInt16(baca_file->ReadLine());
	H_MAX = Convert::ToInt16(baca_file->ReadLine());
	S_MAX = Convert::ToInt16(baca_file->ReadLine());
	V_MAX = Convert::ToInt16(baca_file->ReadLine());
	nilaiopening = Convert::ToInt16(baca_file->ReadLine());
	nilaierotionbola = Convert::ToInt16(baca_file->ReadLine());
	nilaidilationbola = Convert::ToInt16(baca_file->ReadLine());
	nilaiclosing = Convert::ToInt16(baca_file->ReadLine());
	nilaigaussionblur = Convert::ToInt16(baca_file->ReadLine());
	baca_file->Close();
}



void morphOps(Mat& thresh) {
	cuda::GpuMat d_thresh(thresh);
	if (nilaiopening > 0)
	{
		Mat openingElement = cv::getStructuringElement(MORPH_ELLIPSE, Size(2 * nilaiopening + 1, 2 * nilaiopening + 1), Point(nilaiopening, nilaiopening));
		Ptr<cv::cuda::Filter> openingfilter = cuda::createMorphologyFilter(MORPH_OPEN, d_thresh.type(), openingElement);
		openingfilter->apply(d_thresh, d_thresh);
	}

	if (nilaierotionbola > 0)
	{
		Mat erodeElement = cv::getStructuringElement(MORPH_ELLIPSE, Size(2 * nilaierotionbola + 1, 2 * nilaierotionbola + 1), Point(nilaierotionbola, nilaierotionbola));
		Ptr<cuda::Filter> erodeFilter = cuda::createMorphologyFilter(MORPH_ERODE, d_thresh.type(), erodeElement);
		erodeFilter->apply(d_thresh, d_thresh);
	}
	
	if (nilaidilationbola > 0)
	{
		Mat dilateElement = cv::getStructuringElement(MORPH_ELLIPSE, Size(2 * nilaidilationbola + 1, 2 * nilaidilationbola + 1), Point(nilaidilationbola, nilaidilationbola));
		Ptr<cuda::Filter> dilateFilter = cuda::createMorphologyFilter(MORPH_DILATE, d_thresh.type(), dilateElement);
		dilateFilter->apply(d_thresh, d_thresh);
	}
	
	if (nilaigaussionblur > 0)
	{
		if (nilaigaussionblur % 2 == 0)nilaigaussionblur = nilaigaussionblur + 1;
		Ptr<cv::cuda::Filter> GaussianBlurfilter = cv::cuda::createGaussianFilter(d_thresh.type(), d_thresh.type(), Size(nilaigaussionblur, nilaigaussionblur), 0);
		GaussianBlurfilter->apply(d_thresh, d_thresh);
	}

	if (nilaiclosing > 0)
	{
		Mat closingElement = cv::getStructuringElement(MORPH_ELLIPSE, Size(2 * nilaiclosing + 1, 2 * nilaiclosing + 1), Point(nilaiclosing, nilaiclosing));
		Ptr<cv::cuda::Filter> closingfilter = cuda::createMorphologyFilter(MORPH_CLOSE, d_thresh.type(), closingElement);
		closingfilter->apply(d_thresh, d_thresh);
	}
	
	
	d_thresh.download(thresh);
}

void cariports(void)
{

	cli::array<System::String^, 1>^ myPort;
	//myPort->Clear;
	myPort = System::IO::Ports::SerialPort::GetPortNames();
	System::String^ start_param = System::String::Concat(myPort);
	namaport = msclr::interop::marshal_as<std::string>(start_param);
}


int main(int, char**)
{
	ambildatatersimpan();
	bool terputus = false;
	int portarduino = 2;
	int idkamera = 0;
	int idmode = 0;
	//int timetext = 0;
	int sisiC = 0;
	int derajat = 0;
	double fps = 0;
	double tpf = 0;
	bool flipkamera = false;
	bool kalibrasideteksibola = false;
	const int N = 1;
	int64 t0 = cv::getTickCount();
	System::String^ str2;
	//Matrix to store each frame of the webcam feed
	Mat frame;
	//matrix storage for HSV image
	cuda::GpuMat gpu_HSV;

	//matrix storage for binary threshold image
	Mat threshold;
	//x and y values for the location of the object
	int x = 0, y = 0, r = 0;
	//must create a window before setting mouse callback
	cv::namedWindow("Live");
	//set mouse callback function to be active on "Webcam Feed" window
	//we pass the handle to our "frame" matrix so that we can draw a rectangle to it
	//as the user clicks and drags the mouse
	cv::setMouseCallback("Live", clickAndDrag_Rectangle, &frame);
	//initiate mouse move and drag to false 
	mouseIsDragging = false;
	mouseMove = false;
	rectangleSelected = false;
	//video capture object to acquire webcam feed
	//--- INITIALIZE VIDEOCAPTURE
	VideoCapture cap;
gantiidkamera:
	// open the default camera using default API
	// cap.open(0);
	// OR advance usage: select any API backend
	int deviceID = idkamera;             // 0 = open default camera
	int apiID = cv::CAP_ANY;      // 0 = autodetect default API
	// open selected camera using selected API
	cap.open(deviceID + apiID);
	// check if we succeeded
	//set height and width of capture frame
	cap.set(CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	if (!cap.isOpened()) {
		cerr << "ERROR! Unable to open camera\n";
		return -1;
	}

	SerialPort Laptop2Arduino;
	
koneksiulang:
	namaport = "";
	cariports();
	if (terputus == true)
	{
		if (namaport.length() == (4 * portarduino) )
		{
			portarduino ++;
			namaport = "COM6";
		}
		else if (namaport == "COM6")namaport = "";
	}
	
	if (namaport != "")
	{
		str2 = gcnew System::String(namaport.c_str());
		if (Laptop2Arduino.IsOpen == false)
		{
			Laptop2Arduino.PortName = str2;
			Laptop2Arduino.BaudRate = 9600;
			if (namaport.length() < 5)Laptop2Arduino.Open();
			int e = namaport.length() - 4;
			namaport.erase(4, e);
		}
		//else Laptop2Arduino.Close();
	}
	terputus == false;
	//if (str2 == "COM6")
	//else namaport = msclr::interop::marshal_as<std::string>(str2);
	for (;;)
	{
		// wait for a new frame from camera and store it into 'frame'
		cap >> frame;
		// check if we succeeded
		if (frame.empty()) {
			cerr << "ERROR! blank frame grabbed\n";
			break;
		}
		if (flipkamera) flip(frame, frame, 1);
		cuda::GpuMat d_frame(frame);
		//convert frame from BGR to HSV colorspace
		cuda::cvtColor(d_frame, gpu_HSV, COLOR_BGR2HSV);
		//filter HSV image between values and store filtered image to
		//threshold matrix
		Mat HSV(gpu_HSV);
		inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);
		morphOps(threshold);
		trackFilteredObject(x, y, r, sisiC, derajat, threshold, frame);

		if (kalibrasideteksibola)
		{
			cv::imshow("HSV", HSV);
			cv::imshow("THRESHOLDING", threshold);
			recordHSV_Values(frame, HSV);
			putText(frame, "Klik Kanan Pada Mouse Untuk Mereset Nilai HSV", Point(20, 450), 1.5, 1.5, Scalar(255, 255, 255), 2);
		}
		else
		{
			//if (timetext < 20)
			//{
				putText(frame, "'B' / 'b'", Point(110, 450), 1.5, 2, Scalar(0, 255, 0), 2);
				putText(frame, "Bola", Point(510, 450), 1.5, 2, Scalar(0, 255, 0), 2);
			//}
			//else if (timetext > 20 && timetext < 40)
			//{
				//putText(frame, "'G' / 'g'", Point(110, 450), 1.5, 2, Scalar(255, 255, 255), 2);
				//putText(frame, "Gawang", Point(510, 450), 1.5, 2, Scalar(255, 255, 255), 2);
			//}
			//else if (timetext > 40)
			//{
				//timetext = 0;
			//}
			//timetext++;
			putText(frame, "Tekan ", Point(1, 450), 1.5, 2, Scalar(0, 0, 255), 2);
			putText(frame, " untuk Kalibrasi ", Point(230, 450), 1.5, 2, Scalar(255, 0, 0), 2);
			rectangle(frame, Point(380, 60), Point(640, 80), cv::Scalar(0, 0, 0), -1);
			putText(frame, "FPS = " + doubleToString(fps), Point(380, 80), 1, 2, Scalar(255, 255, 255), 2);
		}

		if (idmode == 0/* Tampilan Utama*/)
		{
			putText(frame, "Flip Kamera = 'F' / 'f'", Point(1, 15), 1, 1, Scalar(255, 255, 255), 2);
			putText(frame, "Ganti Kamera = 'C' / 'c'", Point(1, 35), 1, 1, Scalar(255, 255, 255), 2);
			putText(frame, "Tampilkan HSV = 'H' / 'h'", Point(1, 55), 1, 1, Scalar(255, 255, 255), 2);
			putText(frame, "Informasi Output = 'I' / 'i'", Point(320, 15), 1, 1, Scalar(255, 255, 255), 2);
			putText(frame, "Press 'ESC' key to terminate", Point(320, 35), 1, 1, Scalar(255, 255, 255), 2);
		}
		else if (idmode == 1/*TAMPILKAN HSV*/)
		{
			putText(frame, "H MIN Bola = " + intToString(H_MIN), Point(10, 15), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "S MIN Bola = " + intToString(S_MIN), Point(10, 35), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "V MIN Bola = " + intToString(V_MIN), Point(10, 55), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "H MAX Bola = " + intToString(H_MAX), Point(180, 15), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "S MAX Bola = " + intToString(S_MAX), Point(180, 35), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "V MAX Bola = " + intToString(V_MAX), Point(180, 55), 1, 1, Scalar(0, 255, 0), 2);
		}
		else if (idmode == 2 /*TAMPILKAN INFORMASI OUTPUT*/)
		{
			putText(frame, "Derajat Bola = " + intToString(derajat) + " derajat", Point(1, 15), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "Revolusi = " + intToString(FRAME_WIDTH) + "x" + intToString(FRAME_HEIGHT) + " pixel", Point(1, 35), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "Jari - Jari = " + intToString(r) + " pixel", Point(1, 55), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "Jarak Bola = " + intToString(sisiC) + " pixel", Point(320, 15), 1, 1, Scalar(0, 255, 0), 2);
		}
		
		if (Laptop2Arduino.IsOpen)putText(frame, "Terhubung " + namaport, Point(320, 55), 1, 1, Scalar(0, 255, 0), 2);
		else
		{
			if(namaport!="")terputus = true;
			putText(frame, "Terputus", Point(320, 55), 1, 1, Scalar(0, 255, 0), 2);
			putText(frame, "Reconnect = 'R'/'r'", Point(400, 55), 1, 1, Scalar(255, 255, 255), 2);
		}
			
		fps = (double)getTickFrequency() * N / (getTickCount() - t0);
		tpf = (double)(getTickCount() - t0) * 1000.0f / (N * getTickFrequency());
		t0 = getTickCount();

		cv::imshow("Live", frame);

		int key = waitKey(1);
		if (key == 27/*ESC*/)break;
		else if (key == 66/*B*/ || key == 98/*b*/)
		{
			kalibrasideteksibola = !kalibrasideteksibola;
			destroyWindow("HSV");
			destroyWindow("THRESHOLDING");
			destroyWindow(trackbarWindowName);
			if (kalibrasideteksibola)createTrackbars();
		}
		else if (key == 67/*C*/ || key == 99/*c*/)
		{
			cout << "ID Kamera? ";
			cin >> idkamera;
			goto gantiidkamera;
		}
		else if (key == 70/*F*/ || key == 102/*h*/)flipkamera = !flipkamera;
		else if (key == 72/*H*/ || key == 104/*h*/)
		{
			if (idmode == 0)
			{
				idmode = 1;
			}
			else
			{
				idmode = 0;
			}
		}
		else if (key == 73/*I*/ || key == 105/*i*/)
		{
			if (idmode == 0)
			{
				idmode = 2;
			}
			else
			{
				idmode = 0;
			}
		}
		else if (key == 82/*R*/ || key == 114/*r*/)
		{
			goto koneksiulang;
		}
	}
}
