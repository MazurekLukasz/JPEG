#include<opencv2/opencv.hpp>
#include<iostream>


using namespace std;
using namespace cv;

const char* _filename = "Images/lena.jpg";

const char* _windowname = "Obraz";
const char* _windowname1 = "Skompresowany obraz";


int main()
{

	// za�adowanie obrazua
	Mat imgOriginal = imread(_filename, IMREAD_UNCHANGED);
	// je�li obraz jest niepoprawny to wy�wietl komunikat
	if (imgOriginal.empty()) {
		cout << "Nie mo�na wczyta� obrazu: " << _filename << endl;
		return -1;
	}


	// Przyk�adowa macierz kwantyzacji
	double dataLuminance[8][8] = {
	{16, 11, 10, 16, 24, 40, 51, 61},
	{12, 12, 14, 19, 26, 58, 60, 55},
	{14, 13, 16, 24, 40, 57, 69, 56},
	{14, 17, 22, 29, 51, 87, 80, 62},
	{18, 22, 37, 56, 68, 109, 103, 77},
	{24, 35, 55, 64, 81, 104, 113, 92},
	{49, 64, 78, 87, 103, 121, 120, 101},
	{72, 92, 95, 98, 112, 100, 103, 99}
	};

	double dataChrominance[8][8] = {
		{17, 18, 24, 27, 99, 99, 99, 99},
		{18, 21, 26, 66, 99, 99, 99, 99},
		{24, 26, 56, 99, 99, 99, 99, 99},
		{47, 66, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99},
		{99, 99, 99, 99, 99, 99, 99, 99}
	};





	int x = 0;
	int y = 0;
	// wy�wietl obraz 
	namedWindow(_windowname, WINDOW_AUTOSIZE);
	moveWindow(_windowname, x, y);
	imshow(_windowname, imgOriginal);

	//---------------------------------------------------------
//---------------------------------------------------------
// zapisz jako zmienne wysoko�� i szeroko�� obrazu
	int height = imgOriginal.size().height;
	int width = imgOriginal.size().width;

	Mat imgFinal = Mat::zeros(height, width, CV_64FC1);
	Mat imgFinal2 = Mat::zeros(height, width, CV_64FC1);

	// Konwersja tablicy 2D (kwantyzacji) na macierzy obrazu
	Mat luminance = Mat(8, 8, CV_64FC1, &dataLuminance);
	Mat chrominance = Mat(8, 8, CV_64FC1, &dataChrominance);



	double t = (double)getTickCount();
// Konwersja koloru
	Mat imgColorConverted;
	cvtColor(imgOriginal, imgColorConverted, COLOR_RGB2YCrCb);


	//podzia� obrazu na 3 kana�y barw
	vector<Mat> planes;
	split(imgColorConverted, planes);


	// Downsampling kana�u chrominancji
	// Przeskalownaie obrazu o 1/4 
	resize(planes[1], planes[1], Size(width / 2, height / 2));
	resize(planes[2], planes[2], Size(width / 2, height / 2));

	// Przeskalowanie do oryginalneych wymiar�w
	resize(planes[1], planes[1], Size(width, height));
	resize(planes[2], planes[2], Size(width, height));


	// Podzia� obrazu na bloki 8x8
	for (int i = 0; i < height; i += 8) {
		for (int j = 0; j < width; j += 8) {
			// Dla ka�dego kana�u
			for (int plane = 0; plane < imgColorConverted.channels(); plane++) {
				// Utw�rz blok
				Mat block = planes[plane](Rect(j, i, 8, 8));
				// Konwersja warto�ci na float
				block.convertTo(block, CV_64FC1);
				// Bit Shift - przesuni�cie bitowe - odj�cie warto�ci 128
				subtract(block, 128.0, block);
				// DCT
				dct(block, block);
				// Kwantyzacja
				if (plane == 0) {
					divide(block, luminance, block);
				}
				else 
				{
					divide(block, chrominance, block);
				}
				// dodanie 128
				add(block, 128.0, block);
				// Konwersja powrotna na -  unsigned int
				block.convertTo(block, CV_8UC1);	
				// Kopiuj blok do obrazu oryginalnego
				block.copyTo(planes[plane](Rect(j, i, 8, 8)));
			}
		}
	}
	// zapisanie wyniku Kompresji w obrazie imgFinal
	merge(planes, imgFinal);

	// wy�wietl czas
	t = ((double)getTickCount() - t) / getTickFrequency();
	std::cout << "Czas kompresji w sekundach: " << t << std::endl;

	// wy�wietlenie obrazu po kompresji
	cvtColor(imgFinal, imgFinal, COLOR_YCrCb2RGB);
	namedWindow(_windowname1, WINDOW_AUTOSIZE);
	moveWindow(_windowname1, x, y);
	imshow(_windowname1, imgFinal);
	// zapiasnie obrazu skompresowanego
	imwrite("Output/Compressed.jpg", imgFinal);
	
	// odwr�cenie koloru
	cvtColor(imgFinal, imgFinal, COLOR_RGB2YCrCb);


	// -----------------------------------------------------
	//--------------------------------------------------
	//---------------------dekompresja------------------
	//---------------------------------------------------------
	t = (double)getTickCount();
	// Podzia� obrazu po kompresji na 3 przestrzenie barw 
	vector<Mat> planes1;
	split(imgFinal, planes1);


	for (int i = 0; i < height; i += 8) {
		for (int j = 0; j < width; j += 8) {
			// Dla ka�dego kana�u barw
			for (int plane = 0; plane < imgFinal.channels(); plane++) {
				// Utw�rz blok 8x8
				Mat block = planes1[plane](Rect(j, i, 8, 8));
				// Konwersja bloku na float
				block.convertTo(block, CV_64FC1);
				// przesuni�cie bitowe
				subtract(block, 128.0, block);
				// DEKWANTYZACJA
				if (plane == 0) {
					multiply(block, luminance, block);
				}
				else {
					multiply(block, chrominance, block);
				}
				// IDCT
				idct(block, block);
				// Dodanie 128 do bloku
				add(block, 128.0, block);
				// Powrotna konwersja do - unsigned int
				block.convertTo(block, CV_8UC1);
				block.copyTo(planes1[plane](Rect(j, i, 8, 8)));
			}
		}
	}
	// zapisanie wyniku w macierzy imgFinal2
	merge(planes1, imgFinal2);
	// zmiana przestrzeni barw
	cvtColor(imgFinal2, imgFinal2, COLOR_YCrCb2RGB);

	// wy�wietl czas
	t = ((double)getTickCount() - t) / getTickFrequency();
	std::cout << "Czas dekompresji w sekundach: " << t << std::endl;


	// wy�wietlenie obrazu
	imshow("Odkompresowany", imgFinal2);
	// zapisanie obrazu
	imwrite("Output/Decompressed.jpg", imgFinal2);
	waitKey(0);
}
