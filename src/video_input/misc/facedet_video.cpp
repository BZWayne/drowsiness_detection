#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/face.hpp"
#include <opencv2/core/mat.hpp>
#include <stdio.h>
#include <math.h>

#include <iostream>

using namespace std;
using namespace cv;
using namespace cv::face;

bool isBlinking( Mat frame );
//bool isYawning( Mat frame );
Point middlePoint(Point p1, Point p2);
float blinkingRatio (vector<Point2f> landmarks, int points[]);
float yawningRatio (vector<Point2f> landmarks, int points[]);
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;
Ptr<Facemark> facemark;

int LEFT_EYE_POINTS[6] = {36, 37, 38, 39, 40, 41};
int RIGHT_EYE_POINTS[6] = {42, 43, 44, 45, 46, 47};
int MOUTH[2] = {62, 66};

int main( int argc, const char** argv )
{

    String face_cascade_name = samples::findFile("../../haarcascades/haarcascade_frontalface_alt.xml" );

    String facemark_filename = "../../models/lbfmodel.yaml";
    facemark = createFacemarkLBF();
    facemark -> loadModel(facemark_filename);
    cout << "Loaded facemark LBF model" << endl;

    if( !face_cascade.load( face_cascade_name ) )
    {
        cout << "--(!)Error loading face cascade\n";
        return -1;
    };

    VideoCapture capture("../../sample_videos/driver_day.mp4");
    if ( ! capture.isOpened() )
    {
        cout << "--(!)Error opening video capture\n";
        return -1;
    }

    Mat frame;
    int frame_counter = 0;
    int blink_counter = 0;

    while ( capture.read(frame) )
    {
        if( frame.empty() )
        {
            cout << "--(!) No captured frame -- Break!\n";
            break;
        };

        bool is_blinking = isBlinking( frame );
//      bool is_yawning = isYawning( frame);

        frame_counter++;
        if (is_blinking)
        {
            blink_counter++;
        };
        
//        if (is_yawning)
//        {
//            cout << "Driver is yawning" << endl;
//        };

        if (frame_counter == 20)
        {
            float drowsiness_perc = (float)blink_counter / frame_counter;
            frame_counter = 0;
            blink_counter = 0;
            cout << "Drowsiness percentage: " << (drowsiness_perc) << endl;
            
            if (drowsiness_perc > 0.8)
            {
                cout << "ALERT! The driver is sleepy!" << endl;
            }
        }


        if( waitKey(10) == 27 )
        {
            break; // escape
        }
    }
    return 0;
}

Point middlePoint(Point p1, Point p2) {
    float x = (float)((p1.x + p2.x) / 2);
    float y = (float)((p1.y + p2.y) / 2);
    Point p = Point(x, y);
    return p;
}

float blinkingRatio (vector<Point2f> landmarks, int points[])
{
    Point left = Point(landmarks[points[0]].x, landmarks[points[0]].y);
    Point right = Point(landmarks[points[3]].x, landmarks[points[3]].y);
    Point top = middlePoint(landmarks[points[1]], landmarks[points[2]]);
    Point bottom = middlePoint(landmarks[points[5]], landmarks[points[4]]);

    float eye_width = hypot((left.x - right.x), (left.y - right.y));
    float eye_height = hypot((top.x - bottom.x), (top.y - bottom.y));
    float ratio = eye_width / eye_height;
    
    try {
        float ratio = eye_width / eye_height;
    } catch (exception& e) {
        ratio = 0.0;
    }

    return ratio;
}

float yawningRatio (vector<Point2f> landmarks, int points[])
{
    Point top = landmarks[points[1]];
    Point bottom = landmarks[points[2]];
    float mouth_height = top.y - bottom.y;

    try {
        float mouth_height = top.y - bottom.y;
    } catch (exception& e) {
        mouth_height = 0.0;
    }
    return mouth_height;
}

void isolate( Mat frame, vector<Point2f> landmarks, int points[])
{
    Point region[1][20];

    for (int i = 0; i < 6; i++) {
        region[0][i] = Point(landmarks[points[i]].x, landmarks[points[i]].y);
    }

    Size size = frame.size();
    int height = size.height;
    int width = size.width;

    cv::Mat black_frame = cv::Mat(height, width, CV_8UC1, Scalar::all(0));
    cv::Mat mask = cv::Mat(height, width, CV_8UC1, Scalar::all(255));

    int npt[] = { 6 };
    const Point* ppt[1] = { region[0] };
    cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(0, 0, 0), 0);
    cv::bitwise_not(mask, mask);

    Mat frame_eye;
    frame.copyTo(frame_eye, mask);

    int margin = 5;
    int x_vals[] = {region[0][0].x, region[0][1].x, region[0][2].x, region[0][3].x, region[0][4].x, region[0][5].x};
    int y_vals[] = {region[0][0].y, region[0][1].y, region[0][2].y, region[0][3].y, region[0][4].y, region[0][5].y};
    int min_x = *std::min_element(x_vals, x_vals+6) - margin;
    int max_x = *std::max_element(x_vals, x_vals+6) + margin;
    int min_y = *std::min_element(y_vals, y_vals+6) - margin;
    int max_y = *std::max_element(y_vals, y_vals+6) + margin;

    Mat frame_eye_resized = frame_eye(Range(min_y, max_y), Range(min_x, max_x));
    Point origin = Point(min_x, min_y);

    Size new_size = frame_eye_resized.size();
    int new_height = new_size.height;
    int new_width = new_size.width;
    int center[] = {new_width / 2, new_height / 2};

    imshow("Capture - Face detection", frame);
}

bool isBlinking( Mat frame )
{
    Mat frame_gray;
    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );

    std::vector<Rect> faces;
    face_cascade.detectMultiScale( frame_gray, faces );

    Mat faceROI = frame( faces[0] );

    for ( size_t i = 0; i < faces.size(); i++ )
    {
        rectangle( frame,  Point(faces[i].x, faces[i].y), Size(faces[i].x + faces[i].width, faces[i].y + faces[i].height), Scalar(255,0,0), 2 );

        Mat faceROI_gray = frame_gray( faces[i] );
        faceROI = frame( faces[i] );
    }

    cv::rectangle(frame, faces[0], Scalar(255, 0, 0), 2);
    vector<vector<Point2f> > shapes;

    facemark -> fit(frame, faces, shapes);

    // isolate(frame, shapes[0], LEFT_EYE_POINTS );
    // isolate(frame, shapes[0], RIGHT_EYE_POINTS );
    float blinking_ratio_left = blinkingRatio( shapes[0], LEFT_EYE_POINTS );
    float blinking_ratio_right = blinkingRatio( shapes[0], RIGHT_EYE_POINTS );
    //float yawning_ratio = yawningRatio( shapes[0], MOUTH );

    float avg_blinking_ratio = (blinking_ratio_left + blinking_ratio_right) /2;

    if (avg_blinking_ratio > 5)
    {
        // cout << "BLINKING!" << endl;
        return 1;
    }
    else
    {
        // cout << "not blinking" << endl;
        return 0;
    }
}
    
//bool isYawning( Mat frame )
//    {
//        Mat frame_gray;
//        cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
//        equalizeHist( frame_gray, frame_gray );
//
//        std::vector<Rect> faces;
//        face_cascade.detectMultiScale( frame_gray, faces );
//
//        Mat faceROI = frame( faces[0] );
//
//        for ( size_t i = 0; i < faces.size(); i++ )
//        {
//            rectangle( frame,  Point(faces[i].x, faces[i].y), Size(faces[i].x + faces[i].width, faces[i].y + faces[i].height), Scalar(255,0,0), 2 );
//
//            Mat faceROI_gray = frame_gray( faces[i] );
//            faceROI = frame( faces[i] );
//        }
//
//        cv::rectangle(frame, faces[0], Scalar(255, 0, 0), 2);
//        vector<vector<Point2f> > shapes;
//
//        facemark -> fit(frame, faces, shapes);
//
//        float yawning_ratio = yawningRatio( shapes[0], MOUTH );
//
//        if (yawning_ratio > 8)
//        {
//            return 1;
//        }
//        else
//        {
//            return 0;
//        }
    }

