#include <iostream>
#include <string>
#include <set>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/opencv.hpp>

#include "SpaceKB.h"

using namespace cv;
using namespace std;

/**
 * Calculation of points of a straight line along rho and theta
 * @param rho -- radial coordinate
 * @param theta -- angular coordinate
 * @param pt1 -- Point 1
 * @param pt2 -- Point 2
 */
void calculatingPoints(double rho, double theta, Point &pt1, Point &pt2)
{
    double a, b, x0, y0;
    a = cos(theta);
    b = sin(theta);

    x0 = a * rho;
    y0 = b * rho;

    pt1.x = cvRound(x0 - 1000 * b);
    pt1.y = cvRound(y0 + 1000 * a);
    pt2.x = cvRound(x0 + 1000 * b);
    pt2.y = cvRound(y0 - 1000 * a);
}

/**
 * Making SpaceKB to find the approximating line through linear regression
 * @param vertical_lines -- vector of points through which the lines pass
 */
void makeSpaceKB(double &result_x, vector <tuple<Point, Point>> vertical_lines)
{
    set< tuple<double, double> > coefficientsKB;  // Коэффициенты b и k прямых x = ky+b для построения пространства Kb

    for (auto & vertical_line : vertical_lines)
    {
        Point pt1, pt2;

        pt1 = get<0>(vertical_line);
        pt2 = get<1>(vertical_line);

        // x = k * y + b
        // Беру k со знаком -, потому что в OpenCV система координат инвертирована относительно оси OY
        double k = - double((pt2.x - pt1.x)) / (pt2.y - pt1.y);
        double b = pt1.x - pt1.y * double(pt2.x - pt1.x) / (pt2.y - pt1.y);

        coefficientsKB.insert(make_tuple(b, k));
    }

    if (coefficientsKB.begin() != coefficientsKB.end())  // если нашлось хотя бы 2 прямые
    {
        double approaching_x = -1;
        double approaching_y = -1;

        SpaceKB spaceKb(coefficientsKB);

        spaceKb.approaching_straight_line(approaching_x, approaching_y);  // вычисление координат точки пересечения прямых

        if (approaching_x != -1 && approaching_y != -1)
        {
            Point pt;
            pt.x = approaching_x;
            pt.y = approaching_y;

            result_x = approaching_x;

            // cout << "Вычисленная точка: (" << approaching_x << " ; " << approaching_y << " )" << endl;
        }
    }
}

/**
 * Select the vertical lines from the all lines
 * @param lines
 * @param delta
 * @return
 */
vector <tuple<Point, Point>> selectionOfVerticalLines(vector<Vec2f> lines, int delta = 300)
{
    vector <tuple<Point, Point>> vertical_lines;  // множество пар точек, через которые проходят вертикальные прямые

    for (auto & line : lines)
    {
        double rho, theta;
        Point pt1, pt2;  // 2 точки, через которые проходит прямая

        rho = line[0];
        theta = line[1];

        calculatingPoints(rho, theta, pt1, pt2);  // вычисление двух точек прямой (pt1, pt2)

        if (abs(pt1.x - pt2.x) < delta)  // если прямая подозрительна на вертикальную
        {
            vertical_lines.emplace_back(pt1, pt2);
        }
    }
    return vertical_lines;
}

/**
 * Draw vertical lines
 * @param src -- Input image
 * @param lines -- vector of pairs of values rho and theta
 * @param delta -- coefficient by which it is determined whether a straight line is vertical
 */
void drawLines(Mat &src, vector <tuple<Point, Point>> lines)
{
    for (auto & line : lines)
    {
        Point pt1, pt2;
        pt1 = get<0>(line);
        pt2 = get<1>(line);

        cv::line(src, pt1, pt2, CV_RGB(255,0,0), 2, CV_AA);  // отрисовка прямой
    }
}

/**
 * Search for vertical straight lines on video using the Hough method
 * @param src -- Input image.
 * @param resize -- image resizing factor.
 * @param delta -- Coefficient by which it is determined that the line is straight.
 *                 The larger it is, the more lines will be selected.
 */
vector<Vec2f> findLinesHough(Mat &src)
{
    Mat srcCopy;
    vector<Vec2f> lines;  // прямые, найденные на изображении

    // Media blur-----------------------
    // int n = 3;
    // medianBlur(src, srcBlurred, n);
    //----------------------------------

    cvtColor(src, srcCopy, COLOR_BGR2GRAY);  // Подготовка изображения для метода Хафа поиска прямых
    Canny(srcCopy, srcCopy, 50, 200, 3);  // Подготовка изображения для метода Хафа поиска прямых

    // HoughLines(srcCopy, lines, 1, CV_PI/180, 150, 0, 0);
    HoughLines(srcCopy, lines, 1, CV_PI / 180, 130, 0, 0);

    return lines;
}

/**
 * Draw vertical line
 * @param src -- Input image
 * @param x -- x coordinate of the line
 */
void showVerticalLine(Mat &src, double x)
{
    Point pt1, pt2;

    pt1.x = x;
    pt1.y = 0;
    pt2.x = x;
    pt2.y = src.rows;

    line(src, pt1, pt2, CV_RGB(80, 222, 24), 6, CV_AA);
}

/**
 * Put x coordinate of the point on the image
 * @param src -- Input image
 * @param x -- x coordinate
 */
void showXOnImage(Mat &src, double x)
{
    String text = to_string(x);
    int fontFace = FONT_HERSHEY_SCRIPT_SIMPLEX;
    double fontScale = 2;
    int thickness = 3;
    int baseline=0;
    Size textSize = getTextSize(text, fontFace,
                                fontScale, thickness, &baseline);
    baseline += thickness;

    Point textOrg(15,50);

    putText(src, text, textOrg, fontFace, fontScale,
            Scalar(0, 0, 0), thickness, 8);
}

template <class T>
void bubbleSort(T *values, int size)
{
    for (size_t i = 0; i + 1 < size; i++)
    {
        for (size_t j = 0; j + 1 < size - i; j++)
        {
            if (values[j + 1] < values[j])
            {
                std::swap(values[j], values[j + 1]);
            }
        }
    }
}

/**
 * The median value in the array
 * @param valuesForMedianFilter -- array of values for median filter
 * @param NUMBER_OF_MEDIAN_VALUES -- a number indicating the frequency of the median filter
 * @return -- median value
 */
double medianFilter(double *valuesForMedianFilter, const int NUMBER_OF_MEDIAN_VALUES)
{
    int indexOfResult = (NUMBER_OF_MEDIAN_VALUES - 1) / 2;
    bubbleSort(valuesForMedianFilter, NUMBER_OF_MEDIAN_VALUES);
    return valuesForMedianFilter[indexOfResult];
}

/**
 * Function of finding straight vertical lines
 * @tparam T
 * @param path -- path to the video file. 0 means that the video will be read from the webcam.
 * @param resize -- image resizing factor.
 */
template <class T>
void simpleLineDetection(T path, double resize = 1)
{
    VideoCapture capture(path);
    if (!capture.isOpened())
    {
        cerr<<"Error"<<endl;
        return;
    }

    Mat src;

    int n = 1;  // Счетчик для медианного фильтра
    const int NUMBER_OF_MEDIAN_VALUES = 20;  // Раз во сколько кадров проводим медианный фильтр
    double valuesForMedianFilter[NUMBER_OF_MEDIAN_VALUES - 1];  // Массив значений, который будет сортироваться для медианного фильтра
    double prevResult_x = 0;  // Сохранение предыдущего значения, чтобы выводить на экран

    while (true)
    {
        capture >> src;

        vector<Vec2f> lines = findLinesHough(src);  // нахождение прямых линий

        vector <tuple<Point, Point>> vertical_lines = selectionOfVerticalLines(lines);  // выбор только вертикальных линий

        drawLines(src, vertical_lines);  // отрисовка прямых линий

        double result_x = 0;  // Вычисленная координата x точки схода прямых

        makeSpaceKB(result_x, vertical_lines);  // построение пространства Kb, чтобы найти приближающую прямую через
                                                  // линейную регрессию. Далее обратным отображением находим точку схода прямых

        if (n % NUMBER_OF_MEDIAN_VALUES == 0)  // Если нужно провести медианный фильтр
        {
            prevResult_x = medianFilter(valuesForMedianFilter, NUMBER_OF_MEDIAN_VALUES);
        }
        else
        {
            valuesForMedianFilter[n - 1] = result_x;
        }

        showVerticalLine(src, prevResult_x);

        if (resize != 1)
        {
            cv::resize(src, src, cv::Size(), resize, resize);
        }

        imshow("Result", src);

        if (n % NUMBER_OF_MEDIAN_VALUES == 0)
        {
            n = 1;
        }
        else
        {
            n++;
        }

        int k = waitKey(25);
        if (k == 27)
        {
            break;
        }
    }
}

int main()
{
    const string PATH_test = "../videos/test.AVI";
    const string PATH_test2 = "../videos/test2.mp4";
    const string PATH_road = "../videos/road.mp4";
    const string PATH_road2 = "../videos/road2.mp4";
    const string PATH_road3 = "../videos/road3.mp4";

    simpleLineDetection(PATH_road3, 0.6);
}