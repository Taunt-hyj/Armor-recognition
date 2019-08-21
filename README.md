# **视觉组考核——装甲板识别**
*识别Robomaster的装甲板的简易程序*
## 算法分析
装甲板识别主要分这几步
> 图像处理—>提取灯柱同时对灯柱进行筛选->灯条匹配—>装甲板的筛选
### 1.图像处理
#### 亮度处理
利用$g(i,j)=\alpha f(i,j)+\beta$其中($\alpha = 1$, $\beta$是亮度增减变量)降低图像的亮度
```C++
Mat Adjust_Bright(Mat src)
{
	Mat dst_BR = Mat::zeros(src.size(), src.type());
	Mat BrightnessLut(1, 256, CV_8UC1);
	for (int i = 0; i < 256; i++) {
		BrightnessLut.at<uchar>(i) = saturate_cast<uchar>(i + _Armor.Image_bright);
	}
	LUT(src, BrightnessLut, dst_BR);
	return dst_BR;
}
```
#### 灰度化处理
考虑到灯柱为红色和蓝色，识别时
1. 对与红色装甲板，(r通道-g通道) * 2
2. 对于蓝色装甲板，b通道-r通道
```C++
if (_Armor.Armor_Color)
	dst = (channels[2] - channels[1]) * 2;
else
    dst = channels[0] - channels[2];
```
#### 均值滤波
考虑到灯柱中心是接近于白色的，在通道相减后进行一下均值滤波
```C++	
blur(dst, dst, Size(1,3)); 
```
#### 图像二值化
阈值化处理得到二值图
```C++
threshold(dst, dst, _Armor.threshold_value, 255, THRESH_BINARY);
```
#### 膨胀
进行膨胀处理让图像中的轮廓更明显
```C++
Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), Point(-1, -1));
dilate(dst, dst, kernel);
```
### 2. 轮廓拟合
#### 寻找轮廓
findContours可以二值图中找到灯柱轮廓
```C++
vector<vector<Point>> Light_Contour; // 发现的轮廓
findContours(dst, Light_Contour, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); // 寻找轮廓
```
#### 筛选轮廓
利用fitEllipse()得到轮廓的旋转矩阵
筛选的条件有
1. 面积
2. 长宽比
3. 角度的倾斜程度
4. 轮廓面积比
```C++
for (int i = 0; i < Light_Contour.size(); i++) {
	// 求轮廓面积
	float Light_Contour_Area = contourArea(Light_Contour[i]);
	// 去除较小轮廓&fitEllipse的限制条件
	if (Light_Contour_Area < _Armor.Light_Area_min || Light_Contour[i].size() <= 5)
		continue;
	// 用椭圆拟合区域得到外接矩形
	RotatedRect Light_Rec = fitEllipse(Light_Contour[i]);
	Light_Rec = adjustRec(Light_Rec);
	if (Light_Rec.angle > _Armor.Light_angle)
		continue;
	// 长宽比和轮廓面积比限制
	if (Light_Rec.size.width / Light_Rec.size.height > _Armor.Light_Aspect_ratio
		|| Light_Contour_Area / Light_Rec.size.area() < _Armor.Light_crown)
		continue;
	// 扩大灯柱的面积
	Light_Rec.size.height *= 1.1;
	Light_Rec.size.width *= 1.1;
	vContour.push_back(Light_Rec);
}
```
### 3. 灯柱匹配
寻找两个相同的灯条
筛选条件有
1. 角度差
2. 长度差比率
3. 宽度差比率
```C++
//判断是否为相同灯条
float Contour_angle = abs(vContour[i].angle - vContour[j].angle); //角度差
if (Contour_angle >= _Armor.Light_Contour_angle)
	continue;
//长度差比率
float Contour_Len1 = abs(vContour[i].size.height - vContour[j].size.height) / max(vContour[i].size.height, vContour[j].size.height);
//宽度差比率
float Contour_Len2 = abs(vContour[i].size.width - vContour[j].size.width) / max(vContour[i].size.width, vContour[j].size.width);
if (Contour_Len1 > _Armor.Light_Contour_Len || Contour_Len2 > _Armor.Light_Contour_Len)
	continue;
```
### 4. 装甲板筛选
筛选条件有
1. 长宽比
2. x差比率
3. y差比率
4. 装甲板角度
```C++
RotatedRect Rect;
Rect.center.x = (vContour[i].center.x + vContour[j].center.x) / 2.; //x坐标
Rect.center.y = (vContour[i].center.y + vContour[j].center.y) / 2.; //y坐标
Rect.angle = (vContour[i].angle + vContour[j].angle) / 2.; //角度
float nh, nw, yDiff, xDiff;
nh = (vContour[i].size.height + vContour[j].size.height) / 2; //高度
// 宽度
nw = sqrt((vContour[i].center.x - vContour[j].center.x) * (vContour[i].center.x - vContour[j].center.x) + (vContour[i].center.y - vContour[j].center.y) * (vContour[i].center.y - vContour[j].center.y));
float ratio = nw / nh; //匹配到的装甲板的长宽比
xDiff = abs(vContour[i].center.x - vContour[j].center.x) / nh; //x差比率
yDiff = abs(vContour[i].center.y - vContour[j].center.y) / nh; //y差比率
Rect = adjustRec(Rect);
if (Rect.angle > _Armor.Armor_angle_min || ratio < _Armor.Armor_ratio_min || ratio > _Armor.Armor_ratio_max || xDiff < _Armor.Armor_xDiff || yDiff > _Armor.Armor_yDiff)
	continue;
Rect.size.height = nh;
Rect.size.width = nw;
vRec.push_back(Rect);
```
### 参数设置
```C++
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
};
```
## 缺陷
1. 由于刚刚接触图像处理的内容，对于图像处理的很多方法并不是很了解，所以在对图像二值化的时候有较大的问题，不能够做到一个动态阈值去动态处理，只能自己去调试。而且二值化的方法也存在一定的问题。
2. 由于素材原因和时间的原因，对装甲板的筛选并没有做到位，会出现很多非装甲板被识别出来，可以在后期做一个对装甲板上的数字的模板匹配。
3. 缺乏运动的目标跟踪机制无法利用之前已经获得的装甲板的位置跟踪下一帧中的装甲板位置来优化算法的时间。
4. 无法计算装甲板的距离，应该根据离装甲板的距离选择不同的处理方法
5. 只能做到简单的装甲板识别，无法进行目标选择，可以根据距离和装甲板的大小选择最优的打击目标
## 参考
[1]. [东南大学的开源代码](https://github.com/SEU-SuperNova-CVRA/Robomaster2018-SEU-OpenSource/tree/master/Armor)
[2]. [CSDN-Raring_Ringtail-RoboMaster视觉教程（4）装甲板识别算法](https://blog.csdn.net/u010750137/article/details/96428059)
[3]. [CSDN-_Daibingh_-RM装甲识别程序分析（一）](https://blog.csdn.net/healingwounds/article/details/78583194)
