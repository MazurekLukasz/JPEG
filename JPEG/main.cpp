#include<opencv2/opencv.hpp>
#include<iostream>


using namespace std;
using namespace cv;

const char* _filename = "Images/lena.jpg";

const char* _windowname = "Obraz";
const char* _windowname1 = "Skompresowany obraz";


int main()
{

	// za³adowanie obrazua
	Mat imgOriginal = imread(_filename, IMREAD_UNCHANGED);
	// jeœli obraz jest niepoprawny to wyœwietl komunikat
	if (imgOriginal.empty()) {
		cout << "Nie mo¿na wczytaæ obrazu: " << _filename << endl;
		return -1;
	}


	// Przyk³adowa macierz kwantyzacji
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
	// wyœwietl obraz 
	namedWindow(_windowname, WINDOW_AUTOSIZE);
	moveWindow(_windowname, x, y);
	imshow(_windowname, imgOriginal);

	//---------------------------------------------------------
//---------------------------------------------------------
// zapisz jako zmienne wysokoœæ i szerokoœæ obrazu
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


	//podzia³ obrazu na 3 kana³y barw
	vector<Mat> planes;
	split(imgColorConverted, planes);


	// Downsampling kana³u chrominancji
	// Przeskalownaie obrazu o 1/4 
	resize(planes[1], planes[1], Size(width / 2, height / 2));
	resize(planes[2], planes[2], Size(width / 2, height / 2));

	// Przeskalowanie do oryginalneych wymiarów
	resize(planes[1], planes[1], Size(width, height));
	resize(planes[2], planes[2], Size(width, height));


	// Podzia³ obrazu na bloki 8x8
	for (int i = 0; i < height; i += 8) {
		for (int j = 0; j < width; j += 8) {
			// Dla ka¿dego kana³u
			for (int plane = 0; plane < imgColorConverted.channels(); plane++) {
				// Utwórz blok
				Mat block = planes[plane](Rect(j, i, 8, 8));
				// Konwersja wartoœci na float
				block.convertTo(block, CV_64FC1);
				// Bit Shift - przesuniêcie bitowe - odjêcie wartoœci 128
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

	// wyœwietl czas
	t = ((double)getTickCount() - t) / getTickFrequency();
	std::cout << "Czas kompresji w sekundach: " << t << std::endl;

	// wyœwietlenie obrazu po kompresji
	cvtColor(imgFinal, imgFinal, COLOR_YCrCb2RGB);
	namedWindow(_windowname1, WINDOW_AUTOSIZE);
	moveWindow(_windowname1, x, y);
	imshow(_windowname1, imgFinal);
	// zapiasnie obrazu skompresowanego
	imwrite("Output/Compressed.jpg", imgFinal);
	
	// odwrócenie koloru
	cvtColor(imgFinal, imgFinal, COLOR_RGB2YCrCb);


	// -----------------------------------------------------
	//--------------------------------------------------
	//---------------------dekompresja------------------
	//---------------------------------------------------------
	t = (double)getTickCount();
	// Podzia³ obrazu po kompresji na 3 przestrzenie barw 
	vector<Mat> planes1;
	split(imgFinal, planes1);


	for (int i = 0; i < height; i += 8) {
		for (int j = 0; j < width; j += 8) {
			// Dla ka¿dego kana³u barw
			for (int plane = 0; plane < imgFinal.channels(); plane++) {
				// Utwórz blok 8x8
				Mat block = planes1[plane](Rect(j, i, 8, 8));
				// Konwersja bloku na float
				block.convertTo(block, CV_64FC1);
				// przesuniêcie bitowe
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

	// wyœwietl czas
	t = ((double)getTickCount() - t) / getTickFrequency();
	std::cout << "Czas dekompresji w sekundach: " << t << std::endl;


	// wyœwietlenie obrazu
	imshow("Odkompresowany", imgFinal2);
	// zapisanie obrazu
	imwrite("Output/Decompressed.jpg", imgFinal2);
	waitKey(0);
}
