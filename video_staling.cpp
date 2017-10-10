//g++ video_staling.cpp `pkg-config --cflags --libs opencv`
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


int  main(int argc, const char *argv[])
{
    cv::Mat frame;
    // 可从摄像头输入视频流或直接播放视频文件
    //cv::VideoCapture capture(0); //捕获摄像头
    cv::VideoCapture capture(argv[1]); //视频文件
//	std::string videoStreamAddress = "/root/f0.ts"; //管道
    //cv::VideoCapture capture("rtsp://192.168.1.10");
//	capture.open(videoStreamAddress);
	capture.isOpened();
    double fps;
    char string[10];  // 用于存放帧率的字符串
    cv::namedWindow("Camera FPS");
    double t = 0;


    while (1)
    {
        t = (double)cv::getTickCount();
        if (cv::waitKey(50) == 30){ break; }
        if (capture.isOpened())
        {
            capture >> frame;
            // getTickcount函数：返回从操作系统启动到当前所经过的毫秒数
            // getTickFrequency函数：返回每秒的计时周期数
            // t为该处代码执行所耗的时间,单位为秒,fps为其倒数
            t = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
            fps = 1.0 / t;
	
			printf("----fps:%f\n",capture.get(CV_CAP_PROP_FPS)); //获取视频帧率
		
            sprintf(string, "%.2f", fps);      // 帧率保留两位小数
            std::string fpsString("FPS:");
            fpsString += string;                    // 在"FPS:"后加入帧率数值字符串
            // 将帧率信息写在输出帧上
            putText(frame, // 图像矩阵
                    fpsString,                  // string型文字内容
                    cv::Point(5, 20),           // 文字坐标，以左下角为原点
                    cv::FONT_HERSHEY_SIMPLEX,   // 字体类型
                    0.5, // 字体大小
                    cv::Scalar(0, 0, 0));       // 字体颜色


            cv::imshow("Camera FPS", frame);
        }
        else
        {
            std::cout << "No Camera Input!" << std::endl;
            break;
        }
    }
}
