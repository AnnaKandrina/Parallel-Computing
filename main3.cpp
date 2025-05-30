#include <fstream>
#include <iostream>
#include <string>
#include "mpi.h"
#include <windows.h>

using namespace std;

// Константные значения
const int EPOCHS = 120;
const char DEAD_CELL = '-';
const char LIVE_CELL = '*';


char** createField(int h, int w) 
{
	// Функция создаёт field с заданными параметрами:
	// h: int - высота поля
	// w: int - ширина поля
	// return: field: char**

	char** field = new char* [h];
	for (int i = 0; i < h; i++) 
	{
		field[i] = new char[w];
		fill_n(field[i], w, DEAD_CELL);
	}
	return field;
}


int countNeighbours(int h, int w, char** field, int x, int y) 
{
	// Фукнция возвращает countNeighbours - количество живых клеток рядом с текущей
	// h: int - высота поля
	// w: int - ширина поля
	// field: char** - поле клеточного автомата
	// x: int - координата текущей клетки по x
	// y: int - координата текущей клетки по y
	// return: countNeighbours: int

	int countNeighbours = 0;
	for (int i = x - 1; i <= x + 1; i++)
	{
		for (int j = y - 1; j <= y + 1; j++)
		{
			if ((i == x && j == y) || !(0 <= i && i < h) || !(0 <= j && j < w))
				continue;
			else if (field[i][j] == LIVE_CELL)
				countNeighbours++;
		}
	}
	return countNeighbours;
}

char** generateNextField(int h, int w, char** field) 
{
	// Фукнция возвращает newField - следующую итерацию состояния клеточного автомата
	// h: int - высота поля
	// w: int - ширина поля
	// field: char** - поле клеточного автомата
	// return: newField: char**

	char** newField = createField(h, w);
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int countOfAliveNeighbours = countNeighbours(h, w, field, i, j);
			if ((countOfAliveNeighbours == 3) || (countOfAliveNeighbours == 2 && field[i][j] == LIVE_CELL))
				newField[i][j] = LIVE_CELL;
			else
				newField[i][j] = DEAD_CELL;
		}
	}
	return newField;
}

int main()
{
	int size, rank;
	int h, w;
	int x, y;
	int shift;
	
	// Инициализация MPI
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	size = 4; // Значение mpi-процессов
	printf("rank: %d \t size: %d \n", rank, size); // Смотрим сколько и какие процессы в работе
	double Time_work = MPI_Wtime();


	if (rank == 0)
	{
		// Считываем данные входного файла и раскидываем их по переменным
		ifstream input_data("input.txt");
		input_data >> h >> w;

		shift = h % size;
		char** field = createField(h, w);

		
		while (input_data >> x >> y)
			field[x][y] = LIVE_CELL;

		input_data.close();

		MPI_Status status;
		char* send_bufer = new char[w];
		char* recv_bufer = new char[w];

		for (int k = 1; k < size; k++) 
		{
			int* loc_size = new int[2]{h / size + 2, w};
			MPI_Send(loc_size, 2, MPI_INT, k, 1, MPI_COMM_WORLD);

			for (int i = 1; i < h / size + 1; i++) 
			{
				for (int j = 0; j < w; j++)
				{
					recv_bufer[j] = field[shift + (h / size * k) + i - 1][j];
				}

				MPI_Send(recv_bufer, w, MPI_CHAR, k, 2, MPI_COMM_WORLD);
			}
		}

		int loc_height = h / size + h % size + 1;
		char** loc_word = createField(loc_height, w);

		for (int i = 0; i < loc_height; i++)
		{
			for (int j = 0; j < w; j++)
			{
				loc_word[i][j] = field[i][j];
			}
		}
		double Time_work_iter = MPI_Wtime();

		for (int gen = 1; gen <= EPOCHS; gen++) 
		{
			for (int j = 0; j < w; j++)
			{
				send_bufer[j] = loc_word[loc_height - 2][j];
			}

			MPI_Send(send_bufer, w, MPI_CHAR, rank + 1, 1, MPI_COMM_WORLD);
			MPI_Recv(recv_bufer, w, MPI_CHAR, rank + 1, 1, MPI_COMM_WORLD, &status);
			for (int j = 0; j < w; j++)
			{
				loc_word[loc_height - 1][j] = recv_bufer[j];
			}

			char** new_world = generateNextField(loc_height, w, loc_word);
			loc_word = new_world;
		}

		for (int i = 0; i < loc_height; i++)
		{
			for (int j = 0; j < w; j++)
			{
				field[i][j] = loc_word[i][j];
			}
		}

		for (int k = 1; k < size; k++) 
		{
			for (int i = 1; i < h / size + 1; i++) 
			{
				MPI_Recv(recv_bufer, w, MPI_CHAR, k, 1, MPI_COMM_WORLD, &status);
				for (int j = 0; j < w; j++)
				{
					field[shift + (h / size * k) + i - 1][j] = recv_bufer[j];
				}
			}
		}


		ofstream out("output.txt", ofstream::out | ofstream::trunc);
		if (out.is_open())
		{
			out << h << " " << w << endl;
			for (int i = 0; i < h; i++)
				for (int j = 0; j < w; j++)
					if (field[i][j] == LIVE_CELL)
						out << i << " " << j << endl;
		}
		out.close();
	}
	else
	{
		int count;
		MPI_Status status;

		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_INT, &count);
		int* sizeOfField = new int[count];
		MPI_Recv(sizeOfField, count, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		h = sizeOfField[0];
		w = sizeOfField[1];

		char* send_bufer = new char[w];
		char* recv_bufer = new char[w];

		char** field = createField(h, w);
		for (int i = 1; i < h - 1; i++) 
		{
			MPI_Recv(recv_bufer, w, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &status);
			for (int j = 0; j < w; j++)
			{
				field[i][j] = recv_bufer[j];
			}
		}

		for (int gen = 1; gen <= EPOCHS; gen++) 
		{
			if (rank != size - 1) {
				for (int j = 0; j < w; j++)
				{
					send_bufer[j] = field[h - 2][j];
				}

				MPI_Sendrecv(send_bufer, w, MPI_CHAR, rank + 1, 1, recv_bufer, w, MPI_CHAR, rank - 1, 1, MPI_COMM_WORLD, &status);
				for (int j = 0; j < w; j++)
				{
					field[0][j] = recv_bufer[j];
				}

				for (int j = 0; j < w; j++)
				{
					send_bufer[j] = field[1][j];
				}
				MPI_Sendrecv(send_bufer, w, MPI_CHAR, rank - 1, 1, recv_bufer, w, MPI_CHAR, rank + 1, 1, MPI_COMM_WORLD, &status);
				for (int j = 0; j < w; j++)
				{
					field[h - 1][j] = recv_bufer[j];
				}
			}
			else
			{
				MPI_Recv(recv_bufer, w, MPI_CHAR, rank - 1, 1, MPI_COMM_WORLD, &status);
				for (int j = 0; j < w; j++)
				{
					field[0][j] = recv_bufer[j];
				}
				for (int j = 0; j < w; j++)
				{
					send_bufer[j] = field[1][j];
				}
				MPI_Send(send_bufer, w, MPI_CHAR, rank - 1, 1, MPI_COMM_WORLD);
			}
			
			char** new_world = generateNextField(h, w, field);
			field = new_world;
		}
		for (int i = 1; i < h - 1; i++) 
		{
			for (int j = 0; j < w; j++)
			{
				send_bufer[j] = field[i][j];
			}
			MPI_Send(send_bufer, w, MPI_CHAR, 0, 1, MPI_COMM_WORLD);
		}
	}

	Time_work = MPI_Wtime() - Time_work;
	if (rank == 0) 
	{
		cout << "Total time = " << Time_work << endl;
	}
	MPI_Finalize();
	return 0;
}