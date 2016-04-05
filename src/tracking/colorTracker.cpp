#include <ctime>
#include <csignal>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>
#include <string>

using namespace std;
using namespace cv;

#define PLATFORM_RPI  0
#define PLATFORM_MAC  1
#ifndef PLATFORM
#define PLATFORM      PLATFORM_RPI
#endif

#if PLATFORM == PLATFORM_RPI
#include <raspicam/raspicam_cv.h>

#define OPEN_CAM(cam)         cam.open()
#define GET_FRAME(cam, out)   do {cam.grab(); cam.retrieve(out);} while(0)

#elif PLATFORM == PLATFORM_MAC
#define OPEN_CAM(cam)         cam.open(0)
#define GET_FRAME(cam, out)   cam >> out;
#endif

#define VIS 0
#define GUI_SELECT_COLOR 0

#define FRAME_WIDTH    640 //1024
#define FRAME_HEIGHT   480 //576

#define BLOB_SIZE_THRESHOLD 50
#define METHOD_COM 0
#define METHOD_CONTOUR 1
#define METHOD  METHOD_CONTOUR
#define LPF_TRUST_RATIO_NEW   0.6
#define BLOB_DISTANCE_THRESH 200

#define SERVO_SERVER_HOST  "127.0.0.1"
#define SERVO_SERVER_PORT  5200
#define SERVO_SERVER_CMD_PACKET_SIZE  8

#define PIXEL_TO_DEG_X   ( -1.0 / 30.5 ) // with sign flip
#define PIXEL_TO_DEG_Y   ( 1.0 / 30.5 ) // with sign flip

static bool exitFlag = false;

#if GUI_SELECT_COLOR == 1
static int mousex = 0;
static int mousey = 0;
static bool changed = false;
static void onMouse(int event, int x, int y, int, void*) {
    if( event != EVENT_LBUTTONDOWN )
        return;
    mousex = x;
    mousey = y;
    changed = true;
}

static void selectColorGUI(
#if PLATFORM == PLATFORM_MAC
                           raspicam::RaspiCam_Cv &Cam,
#else
                           VideoCapture &Cam,
#endif
                           uint8_t &loH, uint8_t &hiH,
                           const string &winName, const string &text) {
    cv::Mat original, imgHSV;
    Vec3b pixelOrig;

    // Select colors to track
    while(1) {
        GET_FRAME(Cam, original);

        // Use HSV color space
        cvtColor(original, imgHSV, COLOR_BGR2HSV);

        if (changed) {
            changed = false;
            // grab the pixel selected by mouse and calculate threshold
            Vec3b pixel = imgHSV.at<Vec3b>(mousey, mousex);
            if (pixel[0] < 10) {
                loH = 0;
            } else {
                loH = pixel[0] - 10;
            }
            hiH = pixel[0] + 10;
            pixelOrig = original.at<Vec3b>(mousey, mousex);
        }

        // display
        cv::circle(original, Point(25,50), 20, CV_RGB(pixelOrig[2],pixelOrig[1],pixelOrig[0]), -1);
        cv::putText(original, text, Point(5,20), cv::FONT_HERSHEY_COMPLEX_SMALL,
                    0.8, CV_RGB(255,255,0));
        cv::imshow(winName, original);

        // wait for user to say OK
        char key = waitKey(5);
        if (key==27) break;
    }
}
#endif

static int getHueFromCmdArg(char *arg, uint8_t &hue) {
    int temp;
    temp = atoi(arg);
    if (temp < 0 || temp > 179) {
        // Wrong range for hue
        return -1;
    }
    hue = (uint8_t)temp;
    return 0;
}

void signal_handler(int signal)
{
  exitFlag = true;
}

int main (int argc, char **argv) {
    clock_t t0,t1;
#if PLATFORM == PLATFORM_RPI
    raspicam::RaspiCam_Cv Camera;
#else
    VideoCapture Camera;
#endif
    cv::Mat original, imgHSV, imgThresh, imgHSV2, imgThresh2;
    uint8_t loS = 40;
    uint8_t loV = 90;
    uint8_t loH = 140;
    uint8_t hiH = 180;
    uint8_t loH2 = 170;
    uint8_t hiH2 = 179;
    bool valid;
    Moments moment;
    Moments moment2;
    double oldX1, oldY1, newX1, newY1;
    double oldX2, oldY2, newX2, newY2;
    double oldX, oldY, newX, newY;
    vector< vector<Point> > contours, contours2;

#if GUI_SELECT_COLOR != 1
    if (argc < 5) {
        // Too few arguments
        exit(-1);
    }
    if (getHueFromCmdArg(argv[1], loH) != 0) {
        exit(-1);
    }
    if (getHueFromCmdArg(argv[2], hiH) != 0) {
        exit(-1);
    }
    if (getHueFromCmdArg(argv[3], loH2) != 0) {
        exit(-1);
    }
    if (getHueFromCmdArg(argv[4], hiH2) != 0) {
        exit(-1);
    }
#endif

    // Install signal handler
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // servo server stuff
    int sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVO_SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVO_SERVER_HOST);

    // debug windows
    cv::namedWindow("CamOrig", WINDOW_AUTOSIZE);
    cv::namedWindow("Cam", WINDOW_AUTOSIZE);
    cv::namedWindow("Cam2", WINDOW_AUTOSIZE);

    //set camera params
    Camera.set( CV_CAP_PROP_FORMAT, CV_8UC3 );
    Camera.set( CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    Camera.set( CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

    //Open camera
    cout<<"Opening Camera..."<<endl;
    if (!OPEN_CAM(Camera)) {cerr<<"Error opening the camera"<<endl;return -1;}

#if GUI_SELECT_COLOR == 1
    // Optionally select threshold values using the GUI
    cv::setMouseCallback("CamOrig", onMouse);
    selectColorGUI(Camera, loH, hiH, "CamOrig", "Select first color. Press ESC to finish.");
    selectColorGUI(Camera, loH2, hiH2, "CamOrig", "Select second color. Press ESC to finish.");
#endif

    // Print out the selected threshold values
    std::vector<int> threshvec;
    threshvec.push_back(loH);
    threshvec.push_back(loS);
    threshvec.push_back(loV);
    threshvec.push_back(hiH);
    threshvec.push_back(loH2);
    threshvec.push_back(loS);
    threshvec.push_back(loV);
    threshvec.push_back(hiH2);
    std::cout << "myvector contains:";
    for (std::vector<int>::iterator it = threshvec.begin() ; it != threshvec.end(); ++it)
    std::cout << ' ' << *it;
    std::cout << '\n';

    //Start capture
    t0 = clock();
    while(!exitFlag) {
        GET_FRAME(Camera, original);

        // Use HSV color space
        cvtColor(original, imgHSV, COLOR_BGR2HSV);
        cvtColor(original, imgHSV2, COLOR_BGR2HSV);

        // Apply threshold
        inRange(imgHSV, Scalar(loH, loS, loV), Scalar(hiH, 255, 255), imgThresh);
        inRange(imgHSV2, Scalar(loH2, loS, loV), Scalar(hiH2, 255, 255), imgThresh2);

        // Open and close to reduce noise
        erode(imgThresh, imgThresh, getStructuringElement(MORPH_RECT, Size(3,3)));
        erode(imgThresh2, imgThresh2, getStructuringElement(MORPH_RECT, Size(3,3)));
        morphologyEx(imgThresh, imgThresh, MORPH_CLOSE,
                     getStructuringElement(MORPH_RECT, Size(3,3)));
        morphologyEx(imgThresh2, imgThresh2, MORPH_CLOSE,
                     getStructuringElement(MORPH_RECT, Size(3,3)));

        valid = false;

#if METHOD == METHOD_COM
        // Find 2 centers of mass
        moment = moments(imgThresh, true);
        if (moment.m00 > BLOB_SIZE_THRESHOLD) {
            oldX1 = newX1;
            oldY1 = newY1;
            newX1 = moment.m10 / moment.m00;
            newY1 = moment.m01 / moment.m00;
        }

        moment2 = moments(imgThresh2, true);
        if (moment2.m00 > BLOB_SIZE_THRESHOLD) {
            oldX2 = newX2;
            oldY2 = newY2;
            newX2 = moment2.m10 / moment2.m00;
            newY2 = moment2.m01 / moment2.m00;
        }

        // threshold on distance
        if (sqrt((newX1 - newX2) * (newX1 - newX2) + (newY1 - newY2) * (newY1 - newY2)) < BLOB_DISTANCE_THRESH ){
          oldX = (oldX1 + oldX2) / 2;
          oldY = (oldY1 + oldY2) / 2;
          newX = (newX1 + newX2) / 2;
          newY = (newY1 + newY2) / 2;
          valid = true;
        }


#elif METHOD == METHOD_CONTOUR
        findContours(imgThresh, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        findContours(imgThresh2, contours2, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        int maxSize = 0;
        int maxIdx = 0;
        if (contours.size() > 0) {
            // Only take the largest contour (assume that's our target blbo)
            for (int i = 0; i < contours.size(); i++) {
                if (contours[i].size() >= maxSize) {
                    maxSize = contours[i].size();
                    maxIdx = i;
                }
            }
            // get the center
            Rect rect = boundingRect(contours[maxIdx]);
            oldX1 = newX1;
            oldY1 = newY1;
            newX1 = rect.x + rect.width / 2;
            newY1 = rect.y + rect.height / 2;
        }
        int maxSize2 = 0;
        int maxIdx2 = 0;
        if (contours2.size() > 0) {
            // Only take the largest contour (assume that's our target blbo)
            for (int i = 0; i < contours2.size(); i++) {
                if (contours2[i].size() >= maxSize2) {
                    maxSize2 = contours2[i].size();
                    maxIdx2 = i;
                }
            }
            // get the center
            Rect rect = boundingRect(contours2[maxIdx2]);
            oldX2 = newX2;
            oldY2 = newY2;
            newX2 = rect.x + rect.width / 2;
            newY2 = rect.y + rect.height / 2;
        }
        cout << maxSize << "\t" << maxSize2 << "\t";
        // X1 X2, Y1 and Y2 must have been updated
        if (maxSize >= BLOB_SIZE_THRESHOLD &&
            maxSize2 >= BLOB_SIZE_THRESHOLD &&
            sqrt((newX1 - newX2) * (newX1 - newX2) + (newY1 - newY2) * (newY1 - newY2)) < BLOB_DISTANCE_THRESH ){
            oldX = (oldX1 + oldX2) / 2;
            oldY = (oldY1 + oldY2) / 2;
            newX = (newX1 + newX2) / 2;
            newY = (newY1 + newY2) / 2;
            valid = true;
        }
        // otherwise same set of coordinates
#if VIS == 1
        drawContours(original, contours, maxIdx, CV_RGB(155,155,0), 2);
        drawContours(original, contours2, maxIdx2, CV_RGB(0,155,155), 2);
#endif
#endif

        // low pass filter the coordinate
        newX = LPF_TRUST_RATIO_NEW * newX + (1 - LPF_TRUST_RATIO_NEW) * oldX;
        newY = LPF_TRUST_RATIO_NEW * newY + (1 - LPF_TRUST_RATIO_NEW) * oldY;

        // Relative position to center of the frame
        float dpixX = FRAME_WIDTH / 2.0 - newX;
        float dpixY = FRAME_HEIGHT / 2.0 - newY;
        float ddeg[2];
        ddeg[0] = PIXEL_TO_DEG_X * dpixX;
        ddeg[1] = PIXEL_TO_DEG_Y * dpixY;

        //cout << ddeg[0] << "\t" << ddeg[1] << "\t";

        // Send servo commands to servo server (in delta degrees)
        sendto(sockfd, ddeg, SERVO_SERVER_CMD_PACKET_SIZE, 0,
               (struct sockaddr *)&server, sizeof(server));

#if VIS == 1
        line(original, Point(oldX, oldY), Point(newX, newY), CV_RGB(255,255, 0), 4);
        cv::imshow("CamOrig", original);
        cv::imshow("Cam", imgThresh);
        cv::imshow("Cam2", imgThresh2);
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
