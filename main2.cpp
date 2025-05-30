#define _USE_MATH_DEFINES
#include <iostream>
#include <omp.h>
#include <cmath>  
using namespace std;


struct Point
{
	Point(double x, double y) : x{ x }, y{ y } {}
	double x;
	double y;
};

class Polygon
{
	// Класс, описывающий многоугольник
	int countOfPoints = 3 * pow(10, 8);
	public:
	Polygon(){}

	Point getPoint(int pointIndex) 
	{
		// Функция возвращает newPoint - точку с координатами (x, y)
		// i: int - порядковый индекс точки в многоугольнике
		// return: newPoint: Point

		double r = 120 + pointIndex % 60;
		double x = 500 + r * cos(2 * M_PI * pointIndex / countOfPoints);
		double y = 500 + r * sin(2 * M_PI * pointIndex / countOfPoints);
		Point newPoint = Point{ x, y };
		return newPoint;
	}

	double areaOpenMP(int const N) 
	{
		// Функция возвращает итоговое время totalTime выполнения программы с использованием OpenMP
		// N: int - количество поток OpenMP
		// return: totalTime: double
		
		// Используется формула Гаусса (surveyor's formula)
		// В ней присутсвует константа 0.5, для рассчётов времени работы программы она не важна, поэтому игнорируется
		// Так как основная вычислительная сложность приходится на цикл.
		double start = omp_get_wtime();
		double s = 0;
	#pragma omp parallel  for reduction(+:s) num_threads(N)
		for (int i = 0; i < countOfPoints; ++i) 
		{
			
			Point point_1 = getPoint(i);
			Point point_2 = getPoint((i + 1) % countOfPoints);
			s += point_1.x * point_2.y;
			s -= point_1.y * point_2.x;
		}
		double totalTime = (omp_get_wtime() - start);
		return totalTime;
	}
};


int main()
{
	Polygon pol = Polygon();
	for (int i = 1; i <= 8; ++i) 
		cout << "Count of threads: " << i << " | Total time: " << pol.areaOpenMP(i) * 1000 << " ms" << endl;
	return 0;
}