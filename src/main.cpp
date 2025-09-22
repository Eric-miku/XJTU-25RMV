#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <utility> 
#include <sys/stat.h>
#include <sys/types.h>
using namespace cv;
using namespace std;

// ---------------------- 辅助函数 ----------------------

// 角度归一化到 [0,180)
inline float normalizeAngle(float angle) {
    if (angle < 0) angle += 180.0f;
    if (angle >= 180.0f) angle -= 180.0f;
    return angle;
}

// 获取旋转矩形长边长度
inline float getLongSide(const RotatedRect& r) {
    return max(r.size.width, r.size.height);
}

// 获取旋转矩形长边方向角度
inline float getLongSideAngle(const RotatedRect& r) {
    float angle = r.angle;
    if (r.size.width < r.size.height) angle += 90.0f;
    return normalizeAngle(angle);
}

// 判断是否为长条形矩形
inline bool isLongRect(const RotatedRect& r, float minAspect = 1.5f) {
    float w = r.size.width, h = r.size.height;
    return (max(w,h)/min(w,h) >= minAspect);
}

// 创建文件夹（如果不存在）
void createResultFolder(const std::string& folder = "picture_results") {
    struct stat info;
    if (stat(folder.c_str(), &info) != 0) {
        // 文件夹不存在，创建
        #ifdef _WIN32
            _mkdir(folder.c_str());
        #else
            mkdir(folder.c_str(), 0777);
        #endif
    }
}

// 保存图像并显示
void showAndSave(const std::string& winName, const cv::Mat& img, const std::string& folder = "picture_results") {
    cv::imshow(winName, img);
    std::string filename ="../" + folder + "/" + winName + ".png";
    cv::imwrite(filename, img);
}
// ---------------------- 功能函数 ----------------------

// 1. 读取图像
Mat loadImage(const string& path) {
    Mat img = imread(path);
    if (img.empty()) {
        cerr << "无法加载图像: " << path << endl;
        exit(-1);
    }
    return img;
}

// 2. 颜色空间转换
void colorConversion(const Mat& img) {
    Mat gray, hsv;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    cvtColor(img, hsv, COLOR_BGR2HSV);
    showAndSave("Gray", gray);
    showAndSave("HSV", hsv);
}

// 3. 滤波
void filtering(const Mat& img) {
    Mat blurImg, gaussianImg;
    blur(img, blurImg, Size(10, 10));
    GaussianBlur(img, gaussianImg, Size(9, 9), 0);
    showAndSave("Mean_Blur", blurImg);
    showAndSave("Gaussian_Blur", gaussianImg);
}

// 4. 红色区域提取 & 轮廓分析
void redRegion(const Mat& img) {
    Mat hsv;
    cvtColor(img, hsv, COLOR_BGR2HSV);

    Mat mask1, mask2, redMask;
    inRange(hsv, Scalar(0, 100, 100), Scalar(10, 255, 255), mask1);
    inRange(hsv, Scalar(160, 100, 100), Scalar(179, 255, 255), mask2);
    redMask = mask1 | mask2;

    vector<vector<Point>> contours;
    findContours(redMask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    Mat contourImg = img.clone();
    double totalArea = 0.0; // 用于累加总面积

    for (size_t i = 0; i < contours.size(); i++) {
        Rect box = boundingRect(contours[i]);
        drawContours(contourImg, contours, (int)i, Scalar(0, 255, 0), 2);
        rectangle(contourImg, box, Scalar(255, 0, 0), 2);

        double area = contourArea(contours[i]);
        totalArea += area; // 累加
        cout << "轮廓 " << i << " 面积: " << area << endl;
    }

    cout << "所有轮廓总面积: " << totalArea << endl; // 输出总面积

    showAndSave("Red_Mask", redMask);
    showAndSave("Contours_&_Boxes", contourImg);
}

// 5. 图像形态学处理
void morphology(const Mat& img) {
    Mat gray, binary, dilated, eroded, flood;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    threshold(gray, binary, 128, 255, THRESH_BINARY);
    dilate(binary, dilated, Mat(), Point(-1, -1), 2);
    erode(dilated, eroded, Mat(), Point(-1, -1), 2);

    flood = eroded.clone();
    floodFill(flood, Point(0, 0), Scalar(255));

    showAndSave("Binary", binary);
    showAndSave("Dilated", dilated);
    showAndSave("Eroded", eroded);
    showAndSave("FloodFill", flood);
}

// 6. 绘制
void drawing(const Mat& img) {
    Mat drawImg = img.clone();
    circle(drawImg, Point(100, 100), 50, Scalar(0, 0, 255), 3);
    rectangle(drawImg, Point(200, 200), Point(300, 300), Scalar(0, 255, 0), 3);
    putText(drawImg, "Hello OpenCV", Point(50, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 0), 2);
    showAndSave("Drawing", drawImg);
}

// 7. 旋转
void rotateImage(const Mat& img) {
    Point2f center(img.cols/2.0F, img.rows/2.0F);
    Mat rot = getRotationMatrix2D(center, 35, 1.0);
    Mat rotated;
    warpAffine(img, rotated, rot, img.size());
    showAndSave("Rotated", rotated);
}

// 8. 裁剪
void cropImage(const Mat& img) {
    Rect roi(0, 0, img.cols/2, img.rows/2);
    Mat cropped = img(roi);
    showAndSave("Cropped", cropped);
}

// 9. 装甲板检测
vector<RotatedRect> detectArmor(const Mat& img, bool useHSV = false) {
    vector<RotatedRect> armors;
    Mat mask;

    if (useHSV) {
        Mat hsv;
        cvtColor(img, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(100, 150, 150), Scalar(140, 255, 255), mask);
        showAndSave("HSV", hsv);
    } else {
        Mat gray;
        cvtColor(img, gray, COLOR_BGR2GRAY);
        threshold(gray, mask, 222, 255, THRESH_BINARY);
    }

    Mat morph;
    Mat kernel = getStructuringElement(MORPH_RECT, Size(9, 9));
    morphologyEx(mask, morph, MORPH_OPEN, kernel);
    showAndSave("morph_open", morph);

    vector<vector<Point>> contours;
    findContours(morph, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    for (auto& c : contours) {
        RotatedRect rbox = minAreaRect(c);
        double area = contourArea(c);
        double aspect = rbox.size.width / rbox.size.height;
        if (area > 50 && aspect > 0.5 && aspect < 10.0)
            armors.push_back(rbox);
    }
    return armors;
}

// 10. 绘制旋转矩形
void drawArmor(Mat& img, const vector<RotatedRect>& armors, const Scalar& color = Scalar(0,0,255), int thickness=2) {
    for (auto& rbox : armors) {
        Point2f vertices[4];
        rbox.points(vertices);
        for (int i = 0; i < 4; i++)
            line(img, vertices[i], vertices[(i+1)%4], color, thickness);
    }
}

// 11. 平行矩形检测
vector<pair<int,int>> findParallelPairs(const vector<RotatedRect>& armors, 
                                        float thresholdAngle = 10.0f,
                                        float minScale = 1.0f,
                                        float maxScale = 3.0f,
                                        float minCenterAngle = 10.0f) {
    vector<pair<int,int>> parallelPairs;
    int n = (int)armors.size();

    for (int i=0; i<n; i++) {
        float angle1 = getLongSideAngle(armors[i]);
        float long1 = getLongSide(armors[i]);
        if (!isLongRect(armors[i])) continue;

        Point2f c1 = armors[i].center;

        for (int j=i+1; j<n; j++) {
            float angle2 = getLongSideAngle(armors[j]);
            float long2 = getLongSide(armors[j]);
            if (!isLongRect(armors[j])) continue;

            float diff = abs(angle1 - angle2);
            if (diff > 90.0f) diff = 180.0f - diff;

            Point2f c2 = armors[j].center;
            float dist = norm(c1 - c2);
            float avgLength = (long1 + long2)/2.0f;

            float dx = c2.x - c1.x;
            float dy = c2.y - c1.y;
            float centerLineAngle = normalizeAngle(atan2(dy,dx)*180.0f/CV_PI);

            float angleDiff1 = abs(centerLineAngle - angle1);
            float angleDiff2 = abs(centerLineAngle - angle2);
            if (angleDiff1 > 90.0f) angleDiff1 = 180.0f - angleDiff1;
            if (angleDiff2 > 90.0f) angleDiff2 = 180.0f - angleDiff2;

            if (diff < thresholdAngle &&
                dist >= minScale * avgLength &&
                dist <= maxScale * avgLength &&
                angleDiff1 > minCenterAngle &&
                angleDiff2 > minCenterAngle) {
                parallelPairs.emplace_back(i,j);
            }
        }
    }
    return parallelPairs;
}

// 12. 绘制平行矩形对
void drawParallelPairs(Mat& img, const vector<RotatedRect>& armors, const vector<pair<int,int>>& pairs) {
    for (auto& p : pairs) {
        drawArmor(img, {armors[p.first]}, Scalar(255,0,0), 5);
        drawArmor(img, {armors[p.second]}, Scalar(0,255,0), 5);
    }
}

// ---------------------- 主函数 ----------------------
int main() {
    createResultFolder("picture_results");

    Mat img = loadImage("../resources/test_image.png");

    imshow("Original", img); 
    colorConversion(img);
    filtering(img); 
    redRegion(img); 
    morphology(img); 
    drawing(img); 
    rotateImage(img); 
    cropImage(img); 

    Mat img2 = loadImage("../resources/test_image_2.png");

    vector<RotatedRect> armors = detectArmor(img2);

    Mat result = img2.clone();
    drawArmor(result, armors);
    showAndSave("Detected_Armor", result);

    vector<pair<int,int>> parallelPairs = findParallelPairs(armors, 10.0f);

    cout << "平行矩形对数量: " << parallelPairs.size() << endl;
    for (auto& p : parallelPairs)
        cout << "矩形索引对: " << p.first << " 和 " << p.second << endl;

    drawParallelPairs(result, armors, parallelPairs);
    showAndSave("Detected_Armor_with_Parallel_Pairs", result);
    //展示时长
    waitKey(1000);
    return 0;
}

