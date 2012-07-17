#ifndef KalmanFilter_H
#define KalmanFilter_H

/// NOTE: Original code and comments all from first post at http://opencv-users.1802565.n2.nabble.com/Kalman-Filter-example-code-td2913451.html, accessed 2012-07-17
// Added modifications by "Reverend Scragglepus" from the same page, post dated "Aug 20, 2009; 1:15am", title "Modifications for those having difficulty.."
// Integrated with Qt by Josiah Bryan 2012-07-17

#include <opencv/cv.h>  // you added opencv to your includes search path, right? 
#include <time.h> // for clock_t 
//#include <windows.h>  // for CRITICAL_SECTION, if you use windows AND you want locking. 

const int SMALL_WINDOW_WIDTH = 320; 
const int SMALL_WINDOW_HEIGHT = 240; 

class KalmanFilter
{ 
public: 
	KalmanFilter(); 
	virtual ~KalmanFilter(); 

// 	virtual void predictionBegin(CBlob &blob); 
// 	virtual void predictionUpdate(CBlob &blob); 
// 	
	virtual void predictionBegin(float x, float y);
	virtual void predictionUpdate(float x, float y);
	
	//virtual void predictionReport(CvPoint &pnt);
	virtual void predictionReport(float& x, float& y);


private: 
	CvKalman *m_pKalmanFilter; 
	CvRandState rng; 
	CvMat* state; 
	CvMat* process_noise; 
	CvMat* measurement; 
	CvMat* measurement_noise; 
	bool m_bMeasurement; 
	clock_t m_timeLastMeasurement; 
//	CRITICAL_SECTION mutexPrediction; 
}; 


#endif
