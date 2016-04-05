#include <ctime>
#include <iostream>
#include <raspicam/raspicam_cv.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace std; 
using namespace cv; 

#define VIS 1

#define BLOB_SIZE_THRESHOLD 100
#define METHOD_COM 0
#define METHOD_CONTOUR 1
#define METHOD  METHOD_CONTOUR
#define LPF_TRUST_RATIO_NEW   0.6

int main ( int argc,char **argv ) {
   
    clock_t t0,t1;
    raspicam::RaspiCam_Cv Camera;
    cv::Mat original, imgHSV, imgThresh;
    uint8_t loH = 140;
    uint8_t loS = 40;
    uint8_t loV = 90;
    uint8_t hiH = 180;
    Moments moment;
    double oldX, oldY, newX, newY;
    vector< vector<Point> > contours;

    cv::namedWindow("CamOrig", WINDOW_AUTOSIZE);
    cv::namedWindow("Cam", WINDOW_AUTOSIZE);

    //set camera params
    Camera.set( CV_CAP_PROP_FORMAT, CV_8UC3 );
    Camera.set( CV_CAP_PROP_FRAME_WIDTH, 1024);
    Camera.set( CV_CAP_PROP_FRAME_HEIGHT, 567);
    
    //Open camera
    cout<<"Opening Camera..."<<endl;
    if (!Camera.open()) {cerr<<"Error opening the camera"<<endl;return -1;}
    
    //Start capture
    t0 = clock();
    while(1) {
        Camera.grab();
        Camera.retrieve (original);
        flip(original, original, 1);
        
        // Use HSV color space
        cvtColor(original, imgHSV, COLOR_BGR2HSV); 

        // Apply threshold
        inRange(imgHSV, Scalar(loH, loS, loV), Scalar(hiH, 255, 255), imgThresh);

        // Open and close to reduce noise
        erode(imgThresh, imgThresh, getStructuringElement(MORPH_RECT, Size(3,3)));
        morphologyEx(imgThresh, imgThresh, MORPH_CLOSE, 
                     getStructuringElement(MORPH_RECT, Size(3,3)));

#if METHOD == METHOD_COM
        // Find center of mass
        moment = moments(imgThresh, true);
        if (moment.m00 > BLOB_SIZE_THRESHOLD) {
            oldX = newX;
            oldY = newY;
            newX = moment.m10 / moment.m00;
            newY = moment.m01 / moment.m00;
            // TODO: low pass filter the newly obtained target?
        }

#elif METHOD == METHOD_CONTOUR
        findContours(imgThresh, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        if (contours.size() > 0) {
            // Only take the largest contour (assume that's our target blbo)
            int maxSize = 0;
            int maxIdx = 0;
            for (int i = 0; i < contours.size(); i++) {
                if (contours[i].size() >= maxSize) {
                    maxSize = contours[i].size();
                    maxIdx = i;
                }
            }
            // get the center
            Rect rect = boundingRect(contours[maxIdx]);
            oldX = newX;
            oldY = newY;
            newX = rect.x + rect.width / 2;
            newY = rect.y + rect.height / 2;
#if VIS == 1
            drawContours(original, contours, maxIdx, CV_RGB(155,155,0), 2);
#endif
        }
#endif

        // low pass filter the coordinate
        newX = LPF_TRUST_RATIO_NEW * newX + (1 - LPF_TRUST_RATIO_NEW) * oldX;
        newY = LPF_TRUST_RATIO_NEW * newY + (1 - LPF_TRUST_RATIO_NEW) * oldY;

#if VIS == 1
        line(original, Point(oldX, oldY), Point(newX, newY), CV_RGB(255,255, 0), 4);
        cv::imshow("CamOrig", original);
        cv::imshow("Cam", imgThresh);
#endif

        // Frame rate
        char key = waitKey(5);
        if (key==27) break;
        t1 = clock();
        cout<<"\tFPS: "<<((double)CLOCKS_PER_SEC/(t1-t0))<<endl;
        t0 = t1;
    }
    cout<<"Stop camera..."<<endl;
    Camera.release();
}
