#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

void ReadingCamera() {
    Mat frame;
    VideoCapture *pVCapture = nullptr;
    pVCapture = new VideoCapture(1);

    if(!pVCapture->isOpened()) {
        std::cout << "Could open video stream" << std::endl;
        exit(0);
    }

    while(true) {
        pVCapture->read(frame);
        imshow("Image", frame);
        int key = waitKey(1);
        if(key == 'q') break;
    }

    pVCapture->release();
    delete(pVCapture);
    destroyAllWindows();
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    ReadingCamera();
    return 0;
}