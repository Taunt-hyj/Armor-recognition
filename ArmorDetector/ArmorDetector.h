#pragma once
#include <iostream>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/opencv.hpp>
#include <vector>
using namespace cv;
using namespace std;
// 装甲板颜色
#define RED 1
#define BLUE 0

struct ArmorParam {
    float Image_bright; // 图像降低的亮度
    int threshold_value; // threshold阈值

    float Light_Area_min; // 灯柱的最小值
    float Light_Aspect_ratio; // 灯柱的长宽比限制
    float Light_crown; // 灯柱的轮廓面积比限制

    float Light_angle; // 灯柱的倾斜角度
    float Light_Contour_angle; // 灯柱角度差
    float Light_Contour_Len; // 灯柱长度差比率

    float Armor_ratio_max; // 装甲板的长宽比max
    float Armor_ratio_min; // 装甲板的长宽比min
    float Armor_xDiff; // 装甲板x差比率
    float Armor_yDiff; // 装甲板y差比率
    float Armor_angle_min; // 装甲板角度min
    bool Armor_Color; // 装甲板颜色
    ArmorParam()
    {
        Image_bright = -100;
        threshold_value = 25;
        Light_Area_min = 20;
        Light_Aspect_ratio = 0.7;
        Light_crown = 0.5;
        Light_angle = 5;
        Light_Contour_angle = 4.2;
        Light_Contour_Len = 0.25;
        Armor_ratio_max = 5.0;
        Armor_ratio_min = 1.0;
        Armor_xDiff = 0.5;
        Armor_yDiff = 2.0;
        Armor_angle_min = 5;
        Armor_Color = BLUE;
    }
} _Armor;

Mat Adjust_Bright(Mat); // 调节图像亮度
Mat Threshold_Demo(Mat); // 二值化操作
RotatedRect& adjustRec(RotatedRect& rec); // 调整rec
bool Found_Contour(Mat, vector<RotatedRect>&); // 寻找轮廓
bool Identify_board(Mat, vector<RotatedRect>&, vector<RotatedRect>&); // 识别装甲板
void drawBox(vector<RotatedRect>&, Mat&, Mat&); // 画出轮廓
void drawBox1(vector<RotatedRect>&, Mat&, Mat&); //画出灯柱