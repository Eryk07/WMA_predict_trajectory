#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/tracking.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <cmath>

#define M_PI 3.14159265358979323846

struct PointDouble { double x; double y; };

using namespace std;
using namespace cv;



int main(int argc, char** argv)
{
	//polskie znaki w konsoli
	setlocale(LC_ALL, "");
	RNG rng(12345);
	
	// Otwarcie filmu
	string path = "in_1.mp4";
	const char * c = path.c_str();
	
	// odczytanie pliku avi
	CvCapture* vid = cvCreateFileCapture(c);

	//schowki na zapasowe wideo
	VideoCapture start(path);
	VideoCapture start2(path);
	VideoCapture start3(path);
	VideoCapture start4(path);

	VideoCapture capture(path);

	// tworzymy okno wyswietlajace obraz

	//cvNamedWindow("Wejscie", 0);
	//cvNamedWindow("Magia", 0);
	cvNamedWindow("Kontur", 0);
	cvNamedWindow("Przewidywanie obliczeniowe", 0);
	cvNamedWindow("Przewidywanie Kalmanem", 0);


	// odczytanie pierwszej klatki - niezbedne do prawidlowego odczytania wlasciwosci pliku
	// przy uzyciu funkcji cvGetCaptureProperty
	cvQueryFrame(vid);

	// odczytujemy z wlasciwosci pliku liczbe klatek na sekunde
	double fps = cvGetCaptureProperty(vid, CV_CAP_PROP_FPS);

	// wyliczamy czas potrzebny do odtwarzania pliku z prawidlowa prêdkoscia
	double odstep_miedzy_klatkami = 500 / fps;

	Mat frame, prev, img, output;
	//prev=0;

	Point box_left(135, 380);
	Point box_right(320, 385);
	int box_width = box_right.x - box_left.x;
	int start_border = 120;

	if (!capture.read(frame)) return -1;
	int countFrames = 0;

	//liczymy klatki w filmie
	while (true)
	{
		if (capture.read(frame) == false) 
			break;

		countFrames ++;		
	}
	cout << "liczba klatek: " << countFrames << endl;
	
	//Pierwsze wyœwietlenie filmu
	capture = start;
	if (!capture.read(frame)) return -1;

	int i = 0;
	while (true)
	{
		// pobranie kolejnej ramki
		if (!capture.read(frame)) return -1;		

		// jezeli nie jest pusta to wyswietlamy
		if (capture.read(frame) != 0){
			//linia kosza
			//line(frame, box_left, box_right, Scalar(0, 0, 255), 2);
			
			//imshow("Wejscie", frame);
			
		}
		else
			break;

		// czekamy przez okreslony czas
		int c = cvWaitKey(odstep_miedzy_klatkami);
		
		prev = frame.clone();

		i++;

		if (i == countFrames / 6)	//przerwanie w po³owie filmu
		{
			cout <<" wystarczy";
			break;
		}
	}
	
	// ================================================================
	// ========================== MAGIA ===============================
	// ================================================================
		
	// Poprzednia pozycja
	Point prev_pos(9999, 9999);

	// Punkt trafienia
	Point hit_point(-1, -1);

	// Wektor punktow do wizualizacji
	vector<Point> vis_points;
	//hierarchia
	vector<Vec4i> hierarchy;
	Mat frameGray;
	cvtColor(prev, prev, CV_BGR2GRAY);

	Point pos(-1, -1);
	
	capture = start2;
	if (!capture.read(frame)) return -1;

	output = frame.clone();
	cvtColor(frame, frameGray, CV_BGR2GRAY);
	prev = frameGray.clone();

	while (true)
	{
		// Sprawdzenie czy jest koniec filmu
		if (!capture.read(frame)) break;
		
		output = frame.clone();
		
		// Skala szarosci
		cvtColor(frame, frameGray, CV_BGR2GRAY);
		// Roznicowanie klatek
		absdiff(frameGray, prev, img);
		// Binaryzacja
		threshold(img, img, 40, 255, THRESH_BINARY);
		// Morfologiczne zamkniêcie
		morphologyEx(img, img, MORPH_CLOSE, Mat());
		
		// Wyznaczenie konturow
		vector<vector<Point> > contours;	
		findContours(img, contours,hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);
		
		// Analiza konturow
		for (int i = 0; i<contours.size(); i++)
		{
			float S = contourArea(contours[i]);
			float L = arcLength(contours[i], true);
			// Wspolczynnik Malinowskiej
			float M = L / (2 * sqrt(M_PI*S)) - 1;
			if (S > 50 && M < 0.5)
			{
				Moments mu = moments(contours[i], false);
				pos = Point(mu.m10 / mu.m00, mu.m01 / mu.m00);
				drawContours(output, contours, i, Scalar(0,0,255), 2, 8);
				break;
			}
		}

		cout << endl << "x: " << pos.x << " y: " << pos.y ;

		// Jesli znaleziono kontur
		if (pos.x > 0)
		{
			// Zapisanie poprzedniej pozycji obiektu
			prev_pos = pos;
			// Dodanie punktu do wizualizacji
			vis_points.push_back(pos);
		}

		// Narysowanie trajektorii
		//punkty
		for (int i = 0; i<vis_points.size(); i++)
			circle(output, vis_points[i], 2, Scalar(0, 100, 255), -1);
		//linie
		for (int i = 0; i<vis_points.size(); i++)
			if(i!=0)
			line(output, vis_points[i], vis_points[i-1], Scalar(0, 255, 0), 3);
		
		// Zapisanie poprzedniej szarej klatki
		prev = frameGray.clone();

		// Wyswietlenie

		//linia kosza
		//line(img, box_left, box_right,Scalar(255, 255, 255), 2);

		//obraz po operacji ró¿nicy, binaryzacji, zamkniêcia
		//imshow("Magia", img);

		//film z konturem
		line(output, box_left, box_right, Scalar(0, 255,0 ), 2);
		imshow("Kontur", output);
		// czekamy przez okreslony czas
		char c = waitKey(50);
		// Jezeli wcisnieto spacje - koniec
		if (c == 27) break;
		}



	//sprawdzamy czy trajektoria przeciê³a liniê obrêczy kosza
	bool in = false;
		
	if (pos.x > box_left.x && pos.x < box_right.x)
		in = true;

	if (in)
		cout << "\n Wesz³o!";
	else
		cout << "\n NIE";

	// ================================================================
	// ========================== PRZEWIDYWANIE =======================
	// ================================================================
	Point max(1080, 720);

	//wyznaczamy maxa, wierzcho³ek paraboli
	for (int i = 0; i < vis_points.size(); i++)
		if (vis_points[i].y < max.y)
			max = vis_points[i];

	capture = start3;
	if (!capture.read(frame)) return -1;

	prev = frame.clone();
	cvtColor(prev, prev, CV_BGR2GRAY);
	Point firstPoint(1, 1);
	pos.x=-1,pos.y=-1;
	vector<Point> past_points;
	int k = 0;
	while (true)
	{
		// Sprawdzenie czy jest koniec filmu
		if (!capture.read(frame)) break;

		output = frame.clone();

		// Skala szarosci
		cvtColor(frame, frameGray, CV_BGR2GRAY);
		// Roznicowanie klatek
		absdiff(frameGray, prev, img);
		// Binaryzacja
		threshold(img, img, 40, 255, THRESH_BINARY);
		// Morfologiczne zamkniêcie
		morphologyEx(img, img, MORPH_CLOSE, Mat());

		// Wyznaczenie konturow
		vector<vector<Point> > contours;
		findContours(img, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);

		// Analiza konturow
		for (int i = 0; i<contours.size(); i++)
		{
			float S = contourArea(contours[i]);
			float L = arcLength(contours[i], true);
			// Wspolczynnik Malinowskiej
			float M = L / (2 * sqrt(M_PI*S)) - 1;
			if (S > 50 && M < 0.5)
			{
				Moments mu = moments(contours[i], false);
				pos = Point(mu.m10 / mu.m00, mu.m01 / mu.m00);
				drawContours(output, contours, i, Scalar(0, 0, 255), 2, 8);
				break;
			}
		}

		// Jesli znaleziono kontur
		if (pos.x > 0)
		{
			// Zapisanie poprzedniej pozycji obiektu
			prev_pos = pos;
			// Dodanie punktu funkcji
			past_points.push_back(pos);
			//zapisanie pierwszego konturu, najbardziej oddalonego na prawo
			if (pos.x > firstPoint.x)
				firstPoint = pos;
			
		}


		//zatrzymanie filmu gdy pi³ka jest w wiercho³ki paraboli
		if (pos == max)
			/*k++;
			if (k==2)*/
			break;

		circle(output, max, 10, (255, 0, 0), -5);

		// Narysowanie trajektorii
		for (int i = 0; i<vis_points.size(); i++)
			circle(output, vis_points[i], 2, Scalar(0, 100, 255), -1);

		for (int i = 0; i<vis_points.size(); i++)
			if (i != 0)
				line(output, vis_points[i], vis_points[i - 1], Scalar(0, 255, 0), 1);

		// Zapisanie poprzedniej klatki, szarej
		prev = frameGray.clone();

		// Wyswietlenie, linia kosza
		line(output, box_left, box_right, Scalar(0, 255, 0), 2);
		imshow("Przewidywanie obliczeniowe", output);

		// czekamy przez okreslony czas
		char c = waitKey(33);
		// Jezeli wcisnieto ESC - koniec
		if (c == 27) break;
	}

	// ========== WYZNACZENIE PARABOLI Z DOTYCHCZASOWYCH PUNKTÓW =============
	// ============ KORZYSTAJ¥C Z W£ASNOŒCI FUNKCJI KWADRATOWEJ ==============

	//wektor przewidywanych punktów (wyznaczonych jako odbicie lustrzane 
	//dotychczasowych, wzglêdem wierzcho³ka paraboli)

	vector<Point> predicted_points;
	int absDiff = 0;
	Point tmp(0, 0);

	//lustrzane odbicie punktów paraboli przed wierzcho³kiem(maxem)
	//punkty mniej wiêcej powy¿ej  granicy rêki (start_border) 
	for (int i = 0; i < past_points.size(); i++) 
		if(past_points[i].y<start_border)
		{
			absDiff = past_points[i].x - max.x;

			tmp.x = past_points[i].x - 2*absDiff;
			tmp.y = past_points[i].y;

			predicted_points.push_back(tmp);
		}

	//wydrukowanie przewidywanych punktów z odbicia lustrzanego
	//for (int i = 0; i < predicted_points.size(); i++)
	//	cout<<"pred x "<<predicted_points[i].x<<" y "<<predicted_points[i].y<<endl;

	//wydrukowanie odbicia lustrzanego

	for (int i = 0; i < predicted_points.size(); i++) 
		circle(output, predicted_points[i], 4, Scalar(0, 0, 0), -2);

	for (int i = 0; i<predicted_points.size(); i++)
		if (i != 0)
			line(output, predicted_points[i], predicted_points[i - 1], Scalar(189, 15, 242), 5);

	imshow("Przewidywanie obliczeniowe", output);

	//przyjmujemy ¿e wiercho³ek jest œrodkiem uk³adu wspoó³rzêdnych
	//i wyznaczamy wspó³czynnik "a" funkcji y=ax^2, uœredniaj¹c go 
	//na podstawie lustrzanych odbiæ dotychczasowej paraboli
	
	double a=0;
	double sum = 0;
	int ile = 0;
	setprecision(3);

	for (int i = predicted_points.size()-1; i > 0 ; i--) {
		double x = double(predicted_points[i].x - max.x);
		double y = double(predicted_points[i].y - max.y);
		cout << x << "  " << y << endl;
		
		if (!(x==0 || y==0 ))
		sum = sum + y / (x * x);
		
		//cout << sum<<endl;

		ile++;
	}

	a = (sum / (predicted_points.size()) );
	cout << endl << a;
	
	//wektor punktów paraboli o wspó³czynniku a
	vector<Point> square_predicted;
	
	double startPoint = double(predicted_points[ile].x);
	tmp.x = predicted_points[ile ].x;
	tmp.y = predicted_points[ile ].y;
	//cout<<"\nmax y: "<<max.y;
	
	//wektor obliczonej paraboli ax^2
	for (double i = startPoint; i > 0; i=i-10)
	{
			tmp.x  = i ;
			tmp.y = (a * ((max.x-i) *(max.x-i))) + max.y;
			square_predicted.push_back(tmp);
			cout << "\n x_kwad: " << tmp.x << " y_kwad: " << tmp.y;
	}
	
	//linia kosza
	line(output, box_left, box_right, Scalar(0, 0, 255), 2);

	//punkty obliczonej paraboli
	for (int i = 0; i<square_predicted.size(); i++)
		circle(output, square_predicted[i], 4, Scalar(102, 204 , 0), -2);
	//linia obliczonej paraboli
	for (int i = 0; i<square_predicted.size(); i++)
		if (i != 0)
			line(output, square_predicted[i], square_predicted[i - 1], Scalar(255, 102, 255), 3);

	//sprawdzenie czy obliczona parabola przecina liniê kosza
	in = false;
	
	for (int i = 0; i<square_predicted.size(); i++)
		if ((square_predicted[i].x > box_left.x && square_predicted[i].x < box_right.x)&&
			(square_predicted[i].y > box_left.y && square_predicted[i].y < box_right.y))
			in = true;

	if (in)
		cout << "\n Wesz³o!";
	else
		cout << "\n NIE";

	waitKey(0);
	imshow("Przewidywanie obliczeniowe", output);
	waitKey(0);

	// ========== PRZEWIDYWANIE RUCHU Z FILTREM KALMANA =============
	//argumenty konstruktora: 4 parametry równania stanu (2polozenia i 2 predkosci)
	//						  2 wielkoœci mierzone: x i y pi³ki
	//						  0 brak macierzy wejœciowej
	KalmanFilter Kalmi(4, 2, 0);
	//inicjalizacja filtru kalmana

	//macierz przejœæ
	Kalmi.transitionMatrix = (Mat_<float>(4, 4) <<  1, 0, 1, 0,     //x + Vx
													0, 1, 0, 1,		//y + Vy
													0, 0, 1, 0,		//Vx
													0, 0, 0, 1);	//Vy

	
	Mat_<float> measurement(2, 1); measurement.setTo(Scalar(0));
	
	//inicjalizacja filtru - pierwsza pozycja pi³ki
	Kalmi.statePre.at<float>(0) = firstPoint.x;
	Kalmi.statePre.at<float>(1) = firstPoint.y;
	//inicjalizacja prêdkoœci - pocz¹tkowo jest równa 0
	Kalmi.statePre.at<float>(2) = 0;
	Kalmi.statePre.at<float>(3) = 0;
	//nastêpne macierze:

	//macierz punktów pomiarowych pi³ki
	setIdentity(Kalmi.measurementMatrix);
	//kowariancja Q
	setIdentity(Kalmi.processNoiseCov, Scalar::all(1e-4));
	//kowariancja R
	setIdentity(Kalmi.measurementNoiseCov, Scalar::all(10));
	// macierz P predykcji P(k)=(I-K(k)*H)*P'(k) )
	setIdentity(Kalmi.errorCovPost, Scalar::all(.1));

	vector<Point> kalmanPoints;

	capture = start4;
	if (!capture.read(frame)) return -1;

	prev = frame.clone();
	cvtColor(prev, prev, CV_BGR2GRAY);
	//vector<Point> past_points;
	//int k = 0;
	pos = firstPoint;
	Mat kalman_output;
	while (true)
	{
		// pierwsze przewidzenie, uaktualnienie pozycji przewidzianej przez Kalmana
		Mat prediction = Kalmi.predict();
		Point predictPt(prediction.at<float>(0), prediction.at<float>(1));

		// Sprawdzenie czy jest koniec filmu
		if (!capture.read(frame)) break;

		kalman_output = frame.clone();

		// Skala szarosci
		cvtColor(frame, frameGray, CV_BGR2GRAY);
		// Roznicowanie klatek
		absdiff(frameGray, prev, img);
		// Binaryzacja
		threshold(img, img, 40, 255, THRESH_BINARY);
		// Morfologiczne zamkniêcie
		morphologyEx(img, img, MORPH_CLOSE, Mat());

		// Wyznaczenie konturow
		vector<vector<Point> > contours;
		findContours(img, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);

		// Analiza konturow
		for (int i = 0; i<contours.size(); i++)
		{
			float S = contourArea(contours[i]);
			float L = arcLength(contours[i], true);
			// Wspolczynnik Malinowskiej
			float M = L / (2 * sqrt(M_PI*S)) - 1;
			if (S > 50 && M < 0.5)
			{
				Moments mu = moments(contours[i], false);
				pos = Point(mu.m10 / mu.m00, mu.m01 / mu.m00);
				drawContours(kalman_output, contours, i, Scalar(0, 0, 255), 2, 8);
				break;
			}
		}
		
		measurement(0) = pos.x;
		measurement(1) = pos.y;

		// Aktualizowanie przewidzianego punktu
		Mat estimated = Kalmi.correct(measurement);

		Point statePt(estimated.at<float>(0), estimated.at<float>(1));
		
		kalmanPoints.push_back(statePt);
		circle(kalman_output,statePt, 4, Scalar(255, 255, 255), 5);

		
		//trajektoria wyznaczona przez filtr kalmana
		for (int i = 0; i < kalmanPoints.size() - 1; i++)
			line(kalman_output, kalmanPoints[i], kalmanPoints[i + 1], Scalar(0, 155, 255), 4);

		circle(kalman_output, max, 10, (255, 0, 0), -5);

		//zatrzymanie filmu gdy pi³ka jest w wiercho³ki paraboli
		//if (pos == max)
		//	/*k++;
		//	if (k==2)*/
		//	break;


		// Narysowanie trajektorii rzeczywistej
		for (int i = 0; i<vis_points.size(); i++)
			circle(kalman_output, vis_points[i], 2, Scalar(0, 100, 255), -1);

		for (int i = 0; i<vis_points.size(); i++)
			if (i != 0)
				line(kalman_output, vis_points[i], vis_points[i - 1], Scalar(0, 255, 0), 1);

		// Zapisanie poprzedniej klatki, szarej
		prev = frameGray.clone();

		// Wyswietlenie, linia kosza
		line(kalman_output, box_left, box_right, Scalar(0, 255, 0), 2);
		imshow("Przewidywanie Kalmanem", kalman_output);

		// czekamy przez okreslony czas
		char c = waitKey(33);
		// Jezeli wcisnieto ESC - koniec
		if (c == 27) break;
	}
	

	//imshow("Przewidywanie", output);
	// Return

	waitKey(0);
	cvDestroyAllWindows();
	cvReleaseCapture(&vid);
	return 0;
}


//============ PRZYDATNE FUNKCJE ============

// Pelne skopiowanie obrazu (np. do wizualizacji)
//Mat output = frame.clone();

// Wartosc bezwzgledna roznicy dwoch obrazow
//absdiff( wejscie1, wejscie2, wyjscie );

// Momenty geometryczne (np. do wyznaczenia srodka konturu)
//moments