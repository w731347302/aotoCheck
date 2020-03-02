#include <opencv2/opencv.hpp>
#include <iostream>
#include <math.h>
#include <stdio.h>

using namespace cv;
using namespace std;
										//定义一些hsv空间色彩范围
Scalar yellow_l = Scalar(24, 43, 46);
Scalar yellow_h = Scalar(35, 255, 255);
Scalar red_l = Scalar(0, 43, 46);
Scalar red_h = Scalar(10, 255, 255);
Scalar red2_l = Scalar(120, 43, 46);
Scalar red2_h = Scalar(180, 255, 255);
Scalar white_l = Scalar(0, 0, 211);
Scalar white_h = Scalar(180, 30, 255);

int table_h[] = {24, 35, 0, 10, 120, 180, 0, 180};
int table_s[] = {43, 255, 43, 255, 43, 255, 0, 30 };
int table_v[] = {46, 255, 46, 255, 46, 255, 211, 255 };


void angle(Mat src)		//纠正倾斜图像
{
	Mat img_hsv;
	Mat img_mask, img_inr, img_ero;

	cvtColor(src, img_hsv, COLOR_BGR2HSV);
	inRange(img_hsv, yellow_l, yellow_h, img_inr);
	imshow("angle", img_inr);
	erode(img_inr, img_ero, Mat(), Point(-1, -1), 1);
	imshow("angle2", img_ero);
	dilate(img_ero, img_mask, Mat(), Point(-1, -1), 12);
	imshow("angle3", img_mask);
	

	vector<vector<Point>> contours;		//【】，【】
	vector<Vec4i> hierarchy;
	findContours(img_mask, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);	//RETR_EXTERNAL:只检测最外围轮廓;RETR_LIST: 检测所有的轮廓

	cout << contours.size() << endl;

	vector<RotatedRect> minRect(contours.size());

	float maxangles = 0;
	for (int i = 0; i < contours.size(); i++)		//遍历轮廓获得最小矩形和倾斜角度
	{
		minRect[i] = minAreaRect(Mat(contours[i]));

		// Mat img_draw = Mat::zeros(img_mask.size(), CV_8UC3);


		if (abs(minRect[i].angle) > abs(maxangles))
		{
			maxangles = minRect[i].angle;
		}
	}
	
	if (maxangles < -45)
	{
		maxangles = 90 + maxangles;
	}

	cout << maxangles << endl;
	int height = src.rows;
	int width = src.cols;
	Point centre(width / 2, height / 2);
	
	Mat M, img_rot;
	M = getRotationMatrix2D(centre, maxangles, 1.0);

	warpAffine(src, img_rot, M, Size(width, height), INTER_CUBIC, BORDER_REPLICATE);

	char s[20];
	sprintf_s(s, "%.2f", maxangles);

	putText(img_rot, "Angle:"+string(s), Point(10, 30), FONT_HERSHEY_SCRIPT_SIMPLEX,0.5, Scalar(0, 0, 255), 2);
	resize(img_rot, img_rot, Size(200, 300));
	imshow("rotated", img_rot);

}


void remove_background(Mat src)  //同色背景去除
{
	int width = src.cols;
	int height = src.rows;
	int nc = src.channels();

	cout << nc << endl;

	int b = src.at<Vec3b>(3, width-1)[0];
	int g = src.at<Vec3b>(3, width-1)[1];
	int r = src.at<Vec3b>(3, width-1)[2];

	cout << b << endl << g << endl << r << endl;

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			if (pow((src.at<Vec3b>(j, i)[0]-b),2) + pow((src.at<Vec3b>(j, i)[1]-g), 2) + pow((src.at<Vec3b>(j, i)[2]-r), 2) < 5000)
			{
				src.at<Vec3b>(j, i)[0] = 0;
				src.at<Vec3b>(j, i)[1] = 0;
				src.at<Vec3b>(j, i)[2] = 0;
			}
		}
	}
	imshow("img_cut", src);
}

void color_divide(Mat src, Scalar c_l, Scalar c_h )//暂时分开的颜色分离函数
{
	Mat img_hsv, img_mask;
	cvtColor(src, img_hsv, COLOR_BGR2HSV);
	inRange(img_hsv, c_l, c_h, img_mask);
}

int color_choose(Mat src)  //颜色选择，获得图片的主体颜色
{
	Mat img_hsv, img_mask, img_bin;
	int ri=0;
	unsigned int max_sum=-100;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;


	cvtColor(src, img_hsv, COLOR_BGR2HSV);

	for(int i = 0;i<8;i+=2)
	{
		cout << table_h[i] << endl << table_s[i] << endl << table_v[i] << endl;
		inRange(img_hsv, Scalar(table_h[i], table_s[i], table_v[i]), Scalar(table_h[i+1], table_s[i+1], table_v[i+1]), img_mask);
		threshold(img_mask, img_bin, 127, 255, THRESH_BINARY);
		findContours(img_mask, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);
		unsigned sum = 0;

		for (int m = 0; m < contours.size(); m++)
		{
			sum += int(contourArea(contours[m]));
		}
		if (sum > max_sum)
		{
			max_sum = sum;
			ri = i;
		}

	}
	return ri;
}

void out_color(int i)	//输出颜色
{
	if (i == 0)
	{
		cout << "yellow" << endl;
	}
	else if (i == 2 or i == 4)
	{
		cout << "red" << endl;
	}
	else if (i == 6)
	{
		cout << "white" << endl;
	}
}

void find(Mat src)  //ROI区域寻找
{
	Mat img_gray, kernel, img_blur, img_hat, img_thresh, img_roi;
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	cvtColor(src, img_gray, COLOR_BGR2GRAY);
	kernel = getStructuringElement(MORPH_RECT, Size(27, 3));
	GaussianBlur(img_gray, img_blur, Size(3, 3), 0);		//高斯滤波
	medianBlur(img_blur, img_blur, 3);			//中值滤波
	morphologyEx(img_blur, img_hat, MORPH_TOPHAT, kernel);	//顶帽	
	dilate(img_hat, img_thresh, Mat(), Point(-1, -1), 14);
	erode(img_thresh, img_thresh, Mat(), Point(-1, -1), 10);
	threshold(img_thresh, img_thresh, 100, 255, THRESH_BINARY);

	findContours(img_thresh, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	cout << contours.size() << endl;
	imshow("img_thresh", img_thresh);

	double max = 0;
	vector<Point> c;
	vector<Rect> boundRect(contours.size());
	Rect rect;

	for (size_t i = 0; i < contours.size(); i++)
	{
		boundRect[i] = boundingRect(contours[i]);

		contourArea(contours[i]);

		if (boundRect[i].height > max)
		{
			max = boundRect[i].height;
			c = contours[i];
		}
	}
	rect = boundingRect(c);
	img_roi = src(rect);
	resize(img_roi, img_roi, Size(200, 300));
	imshow("roi", img_roi);
}




int main()
{
	int ri;
	Mat src = imread("w8.png");
	remove_background(src);
	angle(src);
	ri = color_choose(src);
	out_color(ri);
	find(src);

	waitKey(0);
	return 0;
}