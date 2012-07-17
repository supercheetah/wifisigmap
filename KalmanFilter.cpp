
// The KalmanFilter.cpp file 
#include "KalmanFilter.h" 

/// NOTE: Original code and comments all from first post at http://opencv-users.1802565.n2.nabble.com/Kalman-Filter-example-code-td2913451.html, accessed 2012-07-17
// Added modifications by "Reverend Scragglepus" from the same page, post dated "Aug 20, 2009; 1:15am", title "Modifications for those having difficulty.."
// Integrated with Qt by Josiah Bryan 2012-07-17

// Initialize the Kalman filter 
// Check out http://www.edinburghrobotics.com/docs/tutorial_drivers_virtual.html for more detailed info 
// Another great reference is OReilly's Learning OpenCV book on Kalman Filters.  I reference the equations 
// for the kalman filter based on this book 

KalmanFilter::KalmanFilter() 
{ 
	m_timeLastMeasurement = clock(); 
	int dynam_params = 4;	// x,y,dx,dy//,width,height 
	int measure_params = 2; 
	m_pKalmanFilter = cvCreateKalman(dynam_params, measure_params); 
	state = cvCreateMat( dynam_params, 1, CV_32FC1 ); 
	
	// random number generator stuff 
	cvRandInit( &rng, 0, 1, -1, CV_RAND_UNI ); 
	cvRandSetRange( &rng, 0, 1, 0 ); 
	rng.disttype = CV_RAND_NORMAL; 
	cvRand( &rng, state ); 

	process_noise = cvCreateMat( dynam_params, 1, CV_32FC1 ); // (w_k) 
	measurement = cvCreateMat( measure_params, 1, CV_32FC1 ); // two parameters for x,y  (z_k) 
	measurement_noise = cvCreateMat( measure_params, 1, CV_32FC1 ); // two parameters for x,y (v_k) 
	cvZero(measurement); 

	// F matrix data 
	// F is transition matrix.  It relates how the states interact 
	// For single input fixed velocity the new value 
	// depends on the previous value and velocity- hence 1 0 1 0 
	// on top line. New velocity is independent of previous 
	// value, and only depends on previous velocity- hence 0 1 0 1 on second row 
	const float F[] = { 
 		1, 0, 1, 0,// 0, 0,	//x + dx 
 		0, 1, 0, 1,// 0, 0,	//y + dy 
 		0, 0, 1, 0,// 0, 0,	//dx = dx 
 		0, 0, 0, 1,// 0, 0,	//dy = dy 
// 		//0, 0, 0, 0, 1, 0,	//width 
// 		//0, 0, 0, 0, 0, 1,	//height 
		
		// No velocity:
/*		1, 0, 0, 0,// 0, 0,	//x + dx 
		0, 1, 0, 0,// 0, 0,	//y + dy 
		0, 0, 1, 0,// 0, 0,	//dx = dx 
		0, 0, 0, 1,// 0, 0,	//dy = dy 
		//0, 0, 0, 0, 1, 0,	//width 
		//0, 0, 0, 0, 0, 1,	//height*/ 
	}; 
	memcpy( m_pKalmanFilter->transition_matrix->data.fl, F, sizeof(F)); 
	cvSetIdentity( m_pKalmanFilter->measurement_matrix, cvRealScalar(1) ); // (H) 
	cvSetIdentity( m_pKalmanFilter->process_noise_cov, cvRealScalar(1e-5) ); // (Q) 
	cvSetIdentity( m_pKalmanFilter->measurement_noise_cov, cvRealScalar(1e-1) ); // (R) 
	cvSetIdentity( m_pKalmanFilter->error_cov_post, cvRealScalar(1)); 

	// choose random initial state 
	cvRand( &rng, m_pKalmanFilter->state_post ); 

//	InitializeCriticalSection(&mutexPrediction); 
} 

KalmanFilter::~KalmanFilter() 
{ 
	// Destroy state, process_noise, and measurement matricies 
	cvReleaseMat( &state ); 
	cvReleaseMat( &process_noise ); 
	cvReleaseMat( &measurement ); 
	cvReleaseMat( &measurement_noise ); 
	cvReleaseKalman( &m_pKalmanFilter ); 

//	DeleteCriticalSection(&mutexPrediction); 
} 



// Set the values for the kalman filter on the first step.  I noticed that when I didn't do this, it didn't track my object 
//void KalmanFilter::predictionBegin(CBlob &blob)
void KalmanFilter::predictionBegin(float x, float y) 
{ 
	// Initialize kalman variables (change in x, change in y,...) 
	// m_pKalmanFilter->state_post is prior state 
//	double dWidth = blob.maxx - blob.minx; 
//	double dHeight = blob.maxy - blob.miny; 

//	m_pKalmanFilter->state_post->data.fl[0] = (float)(blob.minx + (dWidth/2));	//center x 
//	m_pKalmanFilter->state_post->data.fl[1] = (float)(blob.miny + (dHeight/2));	//center y 
	
	m_pKalmanFilter->state_post->data.fl[0] = x; //center x 
	m_pKalmanFilter->state_post->data.fl[1] = y; //center y 
	
	m_pKalmanFilter->state_post->data.fl[2] = (float)0;	 //dx 
	m_pKalmanFilter->state_post->data.fl[3] = (float)0;	 //dy 
	//m_pKalmanFilter->state_post->data.fl[4] = (float)dWidth;	 //width 
	//m_pKalmanFilter->state_post->data.fl[5] = (float)dHeight;	 //height 

} 

//void KalmanFilter::predictionUpdate(CBlob &blob)
void KalmanFilter::predictionUpdate(float x, float y)
{ 
//	EnterCriticalSection(&mutexPrediction); 
	
	m_timeLastMeasurement = clock();
	 
// 	double dWidth = blob.maxx - blob.minx; 
// 	double dHeight = blob.maxy - blob.miny; 
	state->data.fl[0] = x; //center x 
	state->data.fl[1] = y; //center y 
	
//	state->data.fl[0] = (float)(blob.minx + (dWidth/2));	//center x 
//	state->data.fl[1] = (float)(blob.miny + (dHeight/2));	//center y 
	m_bMeasurement = true;
	
//	LeaveCriticalSection(&mutexPrediction); 
} 

//void KalmanFilter::predictionReport(CvPoint &pnt)
void KalmanFilter::predictionReport(float& x, float& y)
{ 
	clock_t timeCurrent = clock(); 

//	EnterCriticalSection(&mutexPrediction); 

	const CvMat* prediction = cvKalmanPredict( m_pKalmanFilter, 0 ); 
	//pnt.x = prediction->data.fl[0]; 
	//pnt.y = prediction->data.fl[1]; 
	
	x = prediction->data.fl[0]; 
	y = prediction->data.fl[1];
	
	//m_dWidth = prediction->data.fl[4]; 
	//m_dHeight = prediction->data.fl[5]; 

	// If we have received the real position recently, then use that position to correct 
	// the kalman filter.  If we haven't received that position, then correct the kalman 
	// filter with the predicted position 
	if (m_bMeasurement) //(timeCurrent - m_timeLastMeasurement < TIME_OUT_LOCATION_UPDATE)	// update with the real position 
	{
		m_bMeasurement = false; 

	}
	// My kalman filter is running much faster than my measurements are coming in.  So this allows the filter to pick up from the last predicted state and continue in between measurements (helps the tracker look smoother) 
	else	// update with the predicted position 
	{ 
		//state->data.fl[0] = pnt.x; 
		//state->data.fl[1] = pnt.y; 
		state->data.fl[0] = x; 
		state->data.fl[1] = y;
	} 

	// generate measurement noise(z_k) 
	cvRandSetRange( &rng, 0, sqrt(m_pKalmanFilter->measurement_noise_cov->data.fl[0]), 0 ); 
	cvRand( &rng, measurement_noise ); 

	// zk = Hk * xk + vk 
	// measurement = measurement_error_matrix * current_state + measurement_noise 
	cvMatMulAdd( m_pKalmanFilter->measurement_matrix, state, measurement_noise, measurement ); 

	// adjust Kalman filter state 
	cvKalmanCorrect( m_pKalmanFilter, measurement ); 

	// generate process noise(w_k) 
	cvRandSetRange( &rng, 0, sqrt(m_pKalmanFilter->process_noise_cov->data.fl[0]), 0 ); 
	cvRand( &rng, process_noise ); 

	// xk = F * xk-1 + B * uk + wk 
	// state = transition_matrix * previous_state + control_matrix * control_input + noise 
	cvMatMulAdd( m_pKalmanFilter->transition_matrix, state, process_noise, state ); 

//	LeaveCriticalSection(&mutexPrediction); 
} 
