#include <QtGui>
#include "SurfaceInterpolate.h"

class MainWindow : public QLabel {
	public:
		MainWindow();
};

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	MainWindow mw;
	mw.show();
	mw.adjustSize();
	return app.exec();
}

using namespace SurfaceInterpolate;

MainWindow::MainWindow()
{
	//QRectF rect(0,0,4001,3420);
	//QRectF rect(0,0,854, 374);
	QRectF rect(0,0,1353,765);
	//QRectF(89.856,539.641 1044.24x661.359)
	double w = rect.width(), h = rect.height();

	
	
	
	// Setup our list of points - these can be in any order, any arrangement.
	
// 	QList<qPointValue> points = QList<qPointValue>()
// 		<< qPointValue(QPointF(0,0), 		0.0)
// // 		<< qPointValue(QPointF(10,80), 		0.5)
// // 		<< qPointValue(QPointF(80,10), 		0.75)
// // 		<< qPointValue(QPointF(122,254), 	0.33)
// 		<< qPointValue(QPointF(w,0), 		0.0)
// 		<< qPointValue(QPointF(w/3*1,h/3*2),	1.0)
// 		<< qPointValue(QPointF(w/2,h/2),	1.0)
// 		<< qPointValue(QPointF(w/3*2,h/3*1),	1.0)
// 		<< qPointValue(QPointF(w,h), 		0.0)
// 		<< qPointValue(QPointF(0,h), 		0.0);

// 	QList<qPointValue> points = QList<qPointValue>()
// // 		<< qPointValue(QPointF(0,0), 		0.0)
// // 
// // 		<< qPointValue(QPointF(w*.5,h*.25),	1.0)
// // 		<< qPointValue(QPointF(w*.5,h*.75),	1.0)
// // 
// // 		<< qPointValue(QPointF(w*.5,h*.5),	0.5)
// // 
// // 		<< qPointValue(QPointF(w*.25,h*.5),	1.0)
// // 		<< qPointValue(QPointF(w*.75,h*.5),	1.0)
// // 
// // 		<< qPointValue(QPointF(w,w),		0.0);
// 		<< qPointValue(QPointF(0,0), 		0.0)
// 
// // 		<< qPointValue(QPointF(w*.2,h*.2),	1.0)
// // 		<< qPointValue(QPointF(w*.5,h*.2),	1.0)
// // 		<< qPointValue(QPointF(w*.8,h*.2),	1.0)
// 
// 		<< qPointValue(QPointF(w*.2,h*.8),	1.0)
// 		<< qPointValue(QPointF(w*.5,h*.8),	1.0)
// 		<< qPointValue(QPointF(w*.8,h*.8),	1.0)
// 
// 		<< qPointValue(QPointF(w*.8,h*.5),	1.0)
// 		<< qPointValue(QPointF(w*.8,h*.2),	1.0)
// 		
// 		//<< qPointValue(QPointF(w*.75,h),	1.0)
// 
// 		<< qPointValue(QPointF(w,w),		0.0)
// 		;

  

  	QList<qPointValue> points;// = QList<qPointValue>()
//   	points << qPointValue( QPointF(116.317, 2180.41), 0.6);
//         points << qPointValue( QPointF(408.87, 2939.64), 0.533333);
//         points << qPointValue( QPointF(71.2619, 3276.22), 0.52);
//         points << qPointValue( QPointF(326.109, 2682.47), 0.506667);
//         points << qPointValue( QPointF(246.38, 2017.56), 0.613333);
//         points << qPointValue( QPointF(210.603, 1790.57), 0.68);
//         points << qPointValue( QPointF(69.7606, 55.8084), 0.626667);
//         points << qPointValue( QPointF(1505.94, 59.3849), 0.68);
//         points << qPointValue( QPointF(2607.61, 53.2941), 0.693333);
//         points << qPointValue( QPointF(3934.88, 49.4874), 0.666667);
//         points << qPointValue( QPointF(3929.81, 1338.7), 0.693333);
//         points << qPointValue( QPointF(3942.07, 1899.13), 0.666667);
//         points << qPointValue( QPointF(2005.55, 1972.79), 0.906667);
//         points << qPointValue( QPointF(3811.29, 3186.99), 0.6);
// 		<< qPointValue(QPointF(0,0), 		0.0)
// 		<< qPointValue(QPointF(w,h),		0.0);

/*	
	points << qPointValue( QPointF(409.028, 2934.03), 0.286667);
        points << qPointValue( QPointF(263.632, 2944.56), 0.284444);
        points << qPointValue( QPointF(63.2585, 2967.1), 0.32);
        points << qPointValue( QPointF(65.64, 3271.95), 0.25);
        points << qPointValue( QPointF(168.789, 3272.29), 0.315556);
        points << qPointValue( QPointF(230.707, 3278.98), 0.306667);
        points << qPointValue( QPointF(58.8397, 3080.36), 0.213333);
        points << qPointValue( QPointF(196.52, 3077.11), 0.34);
        points << qPointValue( QPointF(302.373, 3041.57), 0.4);
        points << qPointValue( QPointF(264.275, 3032.89), 0.413333);
        points << qPointValue( QPointF(234.196, 2817.1), 0.28);
        points << qPointValue( QPointF(233.266, 2763.37), 0.426667);
        points << qPointValue( QPointF(226.289, 2715), 0.323333);
        points << qPointValue( QPointF(272.57, 2715.46), 0.435556);
        points << qPointValue( QPointF(315.827, 2697.09), 0.453333);
        points << qPointValue( QPointF(366.295, 2691.51), 0.36);
        points << qPointValue( QPointF(400.482, 2730.58), 0.326667);
        points << qPointValue( QPointF(485.137, 2724.07), 0.326667);
        points << qPointValue( QPointF(484.954, 2814.81), 0.27);
        points << qPointValue( QPointF(498.843, 2924.77), 0.42);
        points << qPointValue( QPointF(390.625, 2824.07), 0.364444);
        points << qPointValue( QPointF(306.134, 2627.31), 0.382222);
        points << qPointValue( QPointF(235.532, 2600.12), 0.408889);
        points << qPointValue( QPointF(307.827, 2570.06), 0.431111);
        points << qPointValue( QPointF(303.641, 2521.78), 0.462222);
        points << qPointValue( QPointF(344.108, 2555.27), 0.408889);
        points << qPointValue( QPointF(388.482, 2553.04), 0.513333);
        points << qPointValue( QPointF(394.621, 2580.95), 0.396667);
        points << qPointValue( QPointF(394.9, 2622.25), 0.417778);
        points << qPointValue( QPointF(487.835, 2466.52), 0.493333);
        points << qPointValue( QPointF(485.602, 2622.25), 0.391111);
        points << qPointValue( QPointF(490.067, 2542.15), 0.457778);
        points << qPointValue( QPointF(234.295, 2541.47), 0.408889);
        points << qPointValue( QPointF(231.883, 2494.05), 0.422222);
        points << qPointValue( QPointF(309.446, 2463.11), 0.422222);
        points << qPointValue( QPointF(235.902, 2433.77), 0.391111);
        points << qPointValue( QPointF(237.51, 2370.68), 0.39);
        points << qPointValue( QPointF(313.063, 2392.78), 0.363333);
        points << qPointValue( QPointF(161.555, 2335.71), 0.3);
        points << qPointValue( QPointF(101.675, 2316.82), 0.36);
        points << qPointValue( QPointF(128.199, 2250.92), 0.391111);
        points << qPointValue( QPointF(161.153, 2123.52), 0.377778);
        points << qPointValue( QPointF(69.9267, 2165.72), 0.326667);
        points << qPointValue( QPointF(195.312, 2208.72), 0.34);
        points << qPointValue( QPointF(198.527, 2312), 0.38);
        points << qPointValue( QPointF(268.454, 2286.68), 0.38);
        points << qPointValue( QPointF(304.623, 2189.83), 0.408889);
        points << qPointValue( QPointF(240.323, 2153.26), 0.368889);
        points << qPointValue( QPointF(282.52, 2110.66), 0.376667);
        points << qPointValue( QPointF(227.865, 2079.72), 0.373333);
        points << qPointValue( QPointF(308.642, 2050.38), 0.366667);
        points << qPointValue( QPointF(264.034, 2015.82), 0.393333);
        points << qPointValue( QPointF(184.06, 2040.73), 0.386667);
        points << qPointValue( QPointF(178.835, 1967.99), 0.346667);
        points << qPointValue( QPointF(179.237, 1884), 0.36);
        points << qPointValue( QPointF(172.807, 1846.23), 0.38);
        points << qPointValue( QPointF(116.946, 1854.26), 0.34);
        points << qPointValue( QPointF(111.32, 1929.82), 0.373333);
        points << qPointValue( QPointF(120.161, 1998.94), 0.391111);
        points << qPointValue( QPointF(73.9455, 2012.6), 0.376667);
        points << qPointValue( QPointF(63.4966, 2054.4), 0.38);
        points << qPointValue( QPointF(18.8882, 2049.17), 0.126667);
        points << qPointValue( QPointF(17.2807, 2092.17), 0.2);
        points << qPointValue( QPointF(70.3286, 1808.85), 0.435556);
        points << qPointValue( QPointF(137.04, 1799.21), 0.43);
        points << qPointValue( QPointF(75.1511, 1912.94), 0.42);
        points << qPointValue( QPointF(231.481, 1956.74), 0.41);
        points << qPointValue( QPointF(218.621, 1877.57), 0.431111);
        points << qPointValue( QPointF(217.416, 1802.02), 0.39);
        points << qPointValue( QPointF(216.21, 1703.56), 0.4);
        points << qPointValue( QPointF(73.1417, 1634.84), 0.382222);
        points << qPointValue( QPointF(67.9173, 1415.81), 0.373333);
        points << qPointValue( QPointF(63.0948, 1267.92), 0.373333);
        points << qPointValue( QPointF(107, 892), 0.368889);
        points << qPointValue( QPointF(116, 458), 0.364444);
        points << qPointValue( QPointF(127, 69), 0.32);
        points << qPointValue( QPointF(456, 73), 0.37);
        points << qPointValue( QPointF(668, 55), 0.37);
        points << qPointValue( QPointF(841.531, 58.8349), 0.312);
        points << qPointValue( QPointF(1225.73, 55.2582), 0.356667);
        points << qPointValue( QPointF(1500.62, 50.2347), 0.377778);
        points << qPointValue( QPointF(1617, 62.5143), 0.293333);
        points << qPointValue( QPointF(1572.9, 220.754), 0.333333);
        points << qPointValue( QPointF(1579.04, 361.132), 0.36);
        points << qPointValue( QPointF(1566.21, 594.165), 0.376667);
        points << qPointValue( QPointF(1615.88, 662.819), 0.393333);
        points << qPointValue( QPointF(1663.88, 597.514), 0.37);
        points << qPointValue( QPointF(1668.91, 485.323), 0.363333);
        points << qPointValue( QPointF(1662.21, 347.457), 0.36);
        points << qPointValue( QPointF(1650.77, 244.755), 0.346667);
        points << qPointValue( QPointF(1732.26, 253.406), 0.313333);
        points << qPointValue( QPointF(1918.41, 239.452), 0.33);
        points << qPointValue( QPointF(1922.87, 329.595), 0.35);
        points << qPointValue( QPointF(1941.85, 208.474), 0.306667);
        points << qPointValue( QPointF(1966.13, 149.03), 0.323333);
        points << qPointValue( QPointF(2005.2, 59.4444), 0.286667);
        points << qPointValue( QPointF(2094.79, 128.378), 0.346667);
        points << qPointValue( QPointF(2297.12, 138.424), 0.38);
        points << qPointValue( QPointF(2311.35, 51.351), 0.376667);
        points << qPointValue( QPointF(2397.31, 60.0025), 0.313333);
        points << qPointValue( QPointF(2496.66, 52.4673), 0.303333);
        points << qPointValue( QPointF(2570.62, 50.2347), 0.352);
        points << qPointValue( QPointF(2614.72, 4.46531), 0.234667);
        points << qPointValue( QPointF(2643.18, 51.351), 0.352);
        points << qPointValue( QPointF(2787.75, 61.677), 0.341333);
        points << qPointValue( QPointF(2870.35, 51.9092), 0.342222);
        points << qPointValue( QPointF(3031.94, 48.002), 0.386667);
        points << qPointValue( QPointF(3036.69, 106.051), 0.393333);
        points << qPointValue( QPointF(3093.9, 65.8633), 0.41);
        points << qPointValue( QPointF(3144.69, 117.214), 0.346667);
        points << qPointValue( QPointF(3317.16, 51.9092), 0.356667);
        points << qPointValue( QPointF(3474.84, 49.3974), 0.313333);
        points << qPointValue( QPointF(3558.29, 60.2816), 0.306667);
        points << qPointValue( QPointF(3553.55, 87.3525), 0.312);
        points << qPointValue( QPointF(3720.16, 43.8158), 0.31);
        points << qPointValue( QPointF(3876.72, 52.1883), 0.373333);
        points << qPointValue( QPointF(3939.24, 61.677), 0.311111);
        points << qPointValue( QPointF(3912.17, 101.586), 0.373333);
        points << qPointValue( QPointF(3945.38, 481.974), 0.37);
        points << qPointValue( QPointF(3947.61, 656.4), 0.417778);
        points << qPointValue( QPointF(3993.38, 651.935), 0.173333);
        points << qPointValue( QPointF(3984.73, 694.913), 0.173333);
        points << qPointValue( QPointF(3934, 1036), 0.346667);
        points << qPointValue( QPointF(3939.81, 1417.25), 0.36);
        points << qPointValue( QPointF(3927.66, 1576.97), 0.368889);
        points << qPointValue( QPointF(3931.71, 1733.22), 0.33);
        points << qPointValue( QPointF(3930.56, 1873.84), 0.34);
        points << qPointValue( QPointF(3568.87, 1925.93), 0.336667);
        points << qPointValue( QPointF(3180.56, 1917.25), 0.4);
        points << qPointValue( QPointF(2790.51, 1954.28), 0.413333);
        points << qPointValue( QPointF(2398.73, 1950.23), 0.433333);
        points << qPointValue( QPointF(2112.27, 1951.39), 0.45);
        points << qPointValue( QPointF(1979.85, 1968.22), 0.45);
        points << qPointValue( QPointF(1614.72, 1967.53), 0.422222);
        points << qPointValue( QPointF(1221.68, 1961.25), 0.408889);
        points << qPointValue( QPointF(833.291, 1964.04), 0.436667);
        points << qPointValue( QPointF(526.999, 1982.41), 0.42);
        points << qPointValue( QPointF(445.368, 1966.13), 0.4);
        points << qPointValue( QPointF(330.479, 1979.15), 0.4);
        points << qPointValue( QPointF(270.244, 1971.01), 0.39);
        points << qPointValue( QPointF(274.43, 1889.38), 0.4);
        points << qPointValue( QPointF(512.5, 1639.17), 0.36);
        points << qPointValue( QPointF(832.5, 1641.67), 0.41);
        points << qPointValue( QPointF(1230.83, 1629.17), 0.413333);
        points << qPointValue( QPointF(1494.17, 1640.83), 0.426667);
        points << qPointValue( QPointF(1611.53, 1649.04), 0.386667);
        points << qPointValue( QPointF(2042, 1636), 0.396667);
        points << qPointValue( QPointF(2396, 1633), 0.403333);
        points << qPointValue( QPointF(2794, 1635), 0.36);
        points << qPointValue( QPointF(3147.18, 1657.5), 0.343333);
        points << qPointValue( QPointF(3548.4, 1444.8), 0.377778);
        points << qPointValue( QPointF(3594, 1599.6), 0.336667);
        points << qPointValue( QPointF(3753.6, 1290), 0.36);
        points << qPointValue( QPointF(3544.56, 1358.02), 0.36);
        points << qPointValue( QPointF(3167.44, 1362.85), 0.373333);
        points << qPointValue( QPointF(2779.22, 1359.47), 0.383333);
        points << qPointValue( QPointF(2392.94, 1369.12), 0.396667);
        points << qPointValue( QPointF(2007.62, 1358.99), 0.4);
        points << qPointValue( QPointF(1612.17, 1359.47), 0.386667);
        points << qPointValue( QPointF(1228.3, 1360.92), 0.406667);
        points << qPointValue( QPointF(834.298, 1354.17), 0.408889);
        points << qPointValue( QPointF(736.883, 1288.1), 0.337778);
        points << qPointValue( QPointF(547.357, 1371.05), 0.435556);
        points << qPointValue( QPointF(465.229, 1337.92), 0.38);
        points << qPointValue( QPointF(363.922, 1298.85), 0.376667);
        points << qPointValue( QPointF(427.832, 904.224), 0.346667);
        points << qPointValue( QPointF(451.554, 703.844), 0.36);
        points << qPointValue( QPointF(831.663, 725.891), 0.37);
        points << qPointValue( QPointF(766.8, 339.6), 0.34);
        points << qPointValue( QPointF(1203.6, 757.2), 0.366667);
        points << qPointValue( QPointF(1615.88, 728.868), 0.333333);
        points << qPointValue( QPointF(2004.27, 726.542), 0.36);
        points << qPointValue( QPointF(2392.94, 733.796), 0.376667);
        points << qPointValue( QPointF(2775.46, 730.324), 0.364444);
        points << qPointValue( QPointF(3167.82, 729.167), 0.37);
        points << qPointValue( QPointF(846.354, 2953.32), 0.302222);
        points << qPointValue( QPointF(747.01, 2953.8), 0.243333);
        points << qPointValue( QPointF(604.745, 2959.59), 0.306667);
        points << qPointValue( QPointF(618.731, 3083.53), 0.28);
        points << qPointValue( QPointF(610.05, 3163.58), 0.24);
        points << qPointValue( QPointF(607.639, 3248.46), 0.266667);
        points << qPointValue( QPointF(666.782, 3267.26), 0.2);
        points << qPointValue( QPointF(727.063, 3270.95), 0.288889);
        points << qPointValue( QPointF(809.113, 3267.26), 0.293333);
        points << qPointValue( QPointF(861.023, 3269.61), 0.253333);
        points << qPointValue( QPointF(932.356, 3269.27), 0.284444);
        points << qPointValue( QPointF(810.788, 3316.49), 0.153333);
        points << qPointValue( QPointF(916.281, 3306.45), 0.186667);
        points << qPointValue( QPointF(857.004, 3330.22), 0.14);
        points << qPointValue( QPointF(926.997, 3164.45), 0.32);
        points << qPointValue( QPointF(929.342, 3080.39), 0.28);
        points << qPointValue( QPointF(989.958, 3051.25), 0.303333);
        points << qPointValue( QPointF(1098.47, 3038.53), 0.313333);
        points << qPointValue( QPointF(1197.93, 2997.34), 0.32);
        points << qPointValue( QPointF(1105.16, 2953.8), 0.3);
        points << qPointValue( QPointF(969.864, 2962.51), 0.326667);
        points << qPointValue( QPointF(892.838, 2951.12), 0.333333);
        points << qPointValue( QPointF(743.473, 3052.26), 0.333333);
        points << qPointValue( QPointF(745.483, 3153.06), 0.28);
        points << qPointValue( QPointF(865.644, 2876.64), 0.333333);
        points << qPointValue( QPointF(981.385, 2889.66), 0.313333);
        points << qPointValue( QPointF(1033.83, 2897.2), 0.313333);
        points << qPointValue( QPointF(1048.57, 2839.93), 0.284444);
        points << qPointValue( QPointF(1114.88, 2841.94), 0.326667);
        points << qPointValue( QPointF(1116.55, 2904.23), 0.326667);
        points << qPointValue( QPointF(1211.33, 2910.93), 0.368889);
        points << qPointValue( QPointF(1234.1, 2849.98), 0.383333);
        points << qPointValue( QPointF(1251.18, 2953.46), 0.333333);
        points << qPointValue( QPointF(1243.48, 3005.04), 0.336);
        points << qPointValue( QPointF(1251.18, 3069.67), 0.330667);
        points << qPointValue( QPointF(1253.86, 3228.08), 0.382222);
        points << qPointValue( QPointF(1233.76, 3275.64), 0.346667);
        points << qPointValue( QPointF(1321.84, 3220.71), 0.276667);
        points << qPointValue( QPointF(1472.21, 3218.37), 0.325333);
        points << qPointValue( QPointF(1638.66, 3214.35), 0.293333);
        points << qPointValue( QPointF(1760.89, 3265.59), 0.306667);
        points << qPointValue( QPointF(1826.87, 2998.68), 0.39);
        points << qPointValue( QPointF(1771, 2845), 0.26);
        points << qPointValue( QPointF(1763, 2616), 0.404444);
        points << qPointValue( QPointF(1769, 2412), 0.377778);
        points << qPointValue( QPointF(1794, 2232), 0.416667);
        points << qPointValue( QPointF(1825.19, 2169.8), 0.466667);
        points << qPointValue( QPointF(1726.87, 2078.91), 0.403333);
        points << qPointValue( QPointF(1616.75, 2087.35), 0.453333);
        points << qPointValue( QPointF(1279.31, 2080.05), 0.443333);
        points << qPointValue( QPointF(1219.36, 2026.13), 0.443333);
        points << qPointValue( QPointF(1041.2, 2026.8), 0.462222);
        points << qPointValue( QPointF(1032.49, 1969.53), 0.484444);
        points << qPointValue( QPointF(1037.18, 2397.87), 0.43);
        points << qPointValue( QPointF(1278.98, 2402.89), 0.453333);
        points << qPointValue( QPointF(1276.3, 2556.61), 0.36);
        points << qPointValue( QPointF(1279.31, 2669.14), 0.373333);
        points << qPointValue( QPointF(1281.32, 2748.84), 0.317333);
        points << qPointValue( QPointF(1286.68, 2818.17), 0.262222);
        points << qPointValue( QPointF(1286.01, 2882.13), 0.42);
        points << qPointValue( QPointF(1370, 2871.41), 0.35);
        points << qPointValue( QPointF(1596.96, 2826.72), 0.356667);
        points << qPointValue( QPointF(1606.71, 2655.2), 0.317333);
        points << qPointValue( QPointF(1562, 2417), 0.453333);
        points << qPointValue( QPointF(1458, 2217), 0.44);
        points << qPointValue( QPointF(1892.04, 2077.71), 0.408889);
        points << qPointValue( QPointF(1876.77, 2203.49), 0.423333);
        points << qPointValue( QPointF(1933.83, 2202.29), 0.391111);
        points << qPointValue( QPointF(1996.53, 2201.08), 0.413333);
        points << qPointValue( QPointF(2062.44, 2171.75), 0.48);
        points << qPointValue( QPointF(2131.96, 2170.14), 0.406667);
        points << qPointValue( QPointF(2056.01, 2091.37), 0.38);
        points << qPointValue( QPointF(1989.7, 2042.74), 0.366667);
        points << qPointValue( QPointF(1923.79, 2282.26), 0.343333);
        points << qPointValue( QPointF(1929.41, 2395.99), 0.34);
        points << qPointValue( QPointF(1935.83, 2633.33), 0.426667);
        points << qPointValue( QPointF(1914.17, 2839.17), 0.333333);
        points << qPointValue( QPointF(1955, 2987.5), 0.32);
        points << qPointValue( QPointF(1973.33, 3156.67), 0.314667);
        points << qPointValue( QPointF(2054.17, 3240.83), 0.258667);
        points << qPointValue( QPointF(2132.76, 3217.43), 0.284444);
        points << qPointValue( QPointF(2182.2, 3163.58), 0.346667);
        points << qPointValue( QPointF(2187.42, 3092.05), 0.315556);
        points << qPointValue( QPointF(2188.62, 3008.86), 0.303333);
        points << qPointValue( QPointF(2191.44, 2911.2), 0.326667);
        points << qPointValue( QPointF(2191.2, 2632.8), 0.368889);
        points << qPointValue( QPointF(2191.2, 2391.6), 0.313333);
        points << qPointValue( QPointF(2196.18, 2263.31), 0.306667);
        points << qPointValue( QPointF(2249.42, 2176.5), 0.366667);
        points << qPointValue( QPointF(2298.03, 2067.13), 0.396667);
        points << qPointValue( QPointF(2409.14, 2032.41), 0.4);
        points << qPointValue( QPointF(2391.2, 2159.72), 0.36);
        points << qPointValue( QPointF(2460.65, 2205.44), 0.34);
        points << qPointValue( QPointF(2539.35, 2164.93), 0.33);
        points << qPointValue( QPointF(2612.85, 2098.38), 0.36);
        points << qPointValue( QPointF(2458.91, 2276.62), 0.213333);
        points << qPointValue( QPointF(2455.44, 2424.19), 0.3);
        points << qPointValue( QPointF(2445.6, 2608.8), 0.355556);
        points << qPointValue( QPointF(2450.23, 2806.13), 0.333333);
        points << qPointValue( QPointF(2451.97, 3072.92), 0.276667);
        points << qPointValue( QPointF(2396.99, 3256.37), 0.34);
        points << qPointValue( QPointF(2581.6, 3228.59), 0.288889);
        points << qPointValue( QPointF(2676.5, 3248.84), 0.286667);
        points << qPointValue( QPointF(2653, 3032), 0.32);
        points << qPointValue( QPointF(2650, 2877), 0.346667);
        points << qPointValue( QPointF(2639, 2705), 0.28);
        points << qPointValue( QPointF(2631.37, 2647.57), 0.32);
        points << qPointValue( QPointF(2743.06, 2630.21), 0.28);
        points << qPointValue( QPointF(2743.06, 2482.06), 0.32);
        points << qPointValue( QPointF(2721.06, 2332.18), 0.346667);
        points << qPointValue( QPointF(2717.59, 2248.84), 0.366667);
        points << qPointValue( QPointF(2788.77, 2090.86), 0.346667);
        points << qPointValue( QPointF(2805.75, 2238.62), 0.34);
        points << qPointValue( QPointF(2965.37, 2181.23), 0.382222);
        points << qPointValue( QPointF(3091.72, 2185.09), 0.34);
        points << qPointValue( QPointF(3158.76, 2026.91), 0.404444);
        points << qPointValue( QPointF(3197.34, 2204.38), 0.36);
        points << qPointValue( QPointF(3187.21, 2341.82), 0.311111);
        points << qPointValue( QPointF(3207.95, 2640.82), 0.311111);
        points << qPointValue( QPointF(3205.54, 2822.14), 0.293333);
        points << qPointValue( QPointF(3187.69, 2920.52), 0.288889);
        points << qPointValue( QPointF(3193.48, 3084.01), 0.302222);
        points << qPointValue( QPointF(3165.51, 3257.14), 0.337778);
        points << qPointValue( QPointF(3230.76, 3218.37), 0.146667);
        points << qPointValue( QPointF(3260.23, 3227.08), 0.28);
        points << qPointValue( QPointF(3329.79, 3276.3), 0.302222);
        points << qPointValue( QPointF(3489.29, 3282.7), 0.236667);
        points << qPointValue( QPointF(3562.17, 3270.29), 0.275556);
        points << qPointValue( QPointF(3653.07, 3268.47), 0.36);
        points << qPointValue( QPointF(3659.09, 3200.55), 0.306667);
        points << qPointValue( QPointF(3813.82, 3258.02), 0.32);
        points << qPointValue( QPointF(3914.29, 3252.39), 0.2);
        points << qPointValue( QPointF(3914.29, 3134.64), 0.333333);
        points << qPointValue( QPointF(3838.73, 3016.89), 0.306667);
        points << qPointValue( QPointF(3662.31, 3028.55), 0.288889);
        points << qPointValue( QPointF(3567.87, 2936.52), 0.324444);
        points << qPointValue( QPointF(3652.26, 2803.1), 0.313333);
        points << qPointValue( QPointF(3678.39, 2705.44), 0.275556);
        points << qPointValue( QPointF(3938, 2609.39), 0.28);
        points << qPointValue( QPointF(3881.33, 2628.28), 0.328889);
        points << qPointValue( QPointF(3591.98, 2644.35), 0.353333);
        points << qPointValue( QPointF(3579.93, 2485.61), 0.303333);
        points << qPointValue( QPointF(3588.77, 2230.02), 0.342222);
        points << qPointValue( QPointF(3564.65, 2045.56), 0.271111);
        points << qPointValue( QPointF(3933.18, 2040.33), 0.343333);
        points << qPointValue( QPointF(3937.19, 2310.39), 0.271111);
        points << qPointValue( QPointF(3743.09, 2214.35), 0.355556);
        points << qPointValue( QPointF(3525.67, 2162.9), 0.333333);
        points << qPointValue( QPointF(3421.99, 2166.52), 0.328889);
        points << qPointValue( QPointF(3329.56, 2195.86), 0.346667);
        points << qPointValue( QPointF(3266.06, 2219.97), 0.373333);
        points << qPointValue( QPointF(3354.87, 2056.41), 0.343333);
        points << qPointValue( QPointF(985.806, 2453.46), 0.466667);
        points << qPointValue( QPointF(982.189, 2506.91), 0.431111);
        points << qPointValue( QPointF(980.179, 2618.63), 0.391111);
        points << qPointValue( QPointF(976.964, 2742.41), 0.404444);
        points << qPointValue( QPointF(979.376, 2806.71), 0.364444);
        points << qPointValue( QPointF(875.289, 2580.86), 0.44);
        points << qPointValue( QPointF(884.131, 2494.05), 0.457778);
        points << qPointValue( QPointF(761.96, 2488.43), 0.426667);
        points << qPointValue( QPointF(625.723, 2492.85), 0.488889);
        points << qPointValue( QPointF(547.759, 2499.68), 0.433333);
        points << qPointValue( QPointF(539.32, 2390.37), 0.43);
        points << qPointValue( QPointF(539.32, 2264.58), 0.413333);
        points << qPointValue( QPointF(534.095, 2152.05), 0.426667);
        points << qPointValue( QPointF(536.506, 2054.8), 0.422222);
        points << qPointValue( QPointF(757, 2129), 0.476667);
        points << qPointValue( QPointF(750, 2292), 0.444444);
        points << qPointValue( QPointF(905, 2269), 0.466667);
        points << qPointValue( QPointF(729.861, 2863.19), 0.366667);
        points << qPointValue( QPointF(661.111, 2880.56), 0.346667);
        points << qPointValue( QPointF(656.25, 2910.42), 0.337778);
        points << qPointValue( QPointF(706.635, 2891.84), 0.293333);
        points << qPointValue( QPointF(591.095, 2885.48), 0.353333);
        points << qPointValue( QPointF(538.181, 2887.49), 0.453333);
        points << qPointValue( QPointF(537.176, 2965.86), 0.306667);
        points << qPointValue( QPointF(540.19, 3005.37), 0.364444);
        points << qPointValue( QPointF(534.162, 3053.6), 0.32);
        points << qPointValue( QPointF(497.993, 3051.25), 0.355556);
        points << qPointValue( QPointF(448.428, 3055.27), 0.324444);
        points << qPointValue( QPointF(397.524, 3055.61), 0.328889);
        points << qPointValue( QPointF(355.327, 3051.25), 0.355556);
        points << qPointValue( QPointF(329.874, 3007.38), 0.296667);
        points << qPointValue( QPointF(279.975, 2985.61), 0.368889);
        points << qPointValue( QPointF(324.516, 2940.07), 0.342222);
        points << qPointValue( QPointF(358.341, 2893.18), 0.351111);
        points << qPointValue( QPointF(325.856, 2828.88), 0.306667);
        points << qPointValue( QPointF(267.249, 2873.76), 0.355556);
        points << qPointValue( QPointF(306.097, 2750.85), 0.4);
        points << qPointValue( QPointF(441.061, 2672.49), 0.48);
        points << qPointValue( QPointF(532.153, 2677.84), 0.44);
        points << qPointValue( QPointF(537.846, 2612.2), 0.52);
        points << qPointValue( QPointF(542.2, 2542.54), 0.52);
        points << qPointValue( QPointF(539.186, 2750.85), 0.413333);
        points << qPointValue( QPointF(540.86, 2805.1), 0.373333);
        points << qPointValue( QPointF(235.098, 3052.26), 0.311111);
        points << qPointValue( QPointF(190.892, 3126.27), 0.284444);
        points << qPointValue( QPointF(154.723, 3172.82), 0.23);
        points << qPointValue( QPointF(67.9843, 3151.05), 0.208889);
        points << qPointValue( QPointF(116.879, 3212.68), 0.3);
        points << qPointValue( QPointF(221.033, 3182.53), 0.236667);
        points << qPointValue( QPointF(126.257, 3085.41), 0.303333);
        points << qPointValue( QPointF(132.62, 3020.78), 0.355556);
        points << qPointValue( QPointF(57.6024, 3019.44), 0.29);
        points << qPointValue( QPointF(75.352, 2896.87), 0.36);
        points << qPointValue( QPointF(128.601, 2899.21), 0.306667);
        points << qPointValue( QPointF(211.655, 2954.13), 0.373333);
        points << qPointValue( QPointF(219.358, 2864.38), 0.386667);
        points << qPointValue( QPointF(135.634, 2835.92), 0.377778);
        points << qPointValue( QPointF(68.989, 2833.57), 0.406667);
        points << qPointValue( QPointF(69.9937, 2766.93), 0.302222);
        points << qPointValue( QPointF(79.7057, 2631.63), 0.426667);
        points << qPointValue( QPointF(80.0406, 2662.44), 0.395556);
        points << qPointValue( QPointF(129.605, 2733.77), 0.406667);
        points << qPointValue( QPointF(185.868, 2688.23), 0.328889);
        points << qPointValue( QPointF(181.515, 2623.93), 0.313333);
        points << qPointValue( QPointF(70.6635, 2564.98), 0.4);
        points << qPointValue( QPointF(124.917, 2515.08), 0.4);
        points << qPointValue( QPointF(150.034, 2559.29), 0.364444);
        points << qPointValue( QPointF(413.194, 2096.64), 0.37);
        points << qPointValue( QPointF(419.56, 2382.52), 0.426667);
*/
//         points << qPointValue( QPointF(75,75), 1.0);
// //         points << qPointValue( QPointF(648.362, 283.993), 0.410667);
// //         points << qPointValue( QPointF(129.726, 243.538), 0.264);
// //         points << qPointValue( QPointF(294.657, 197.242), 0.26);
// //         points << qPointValue( QPointF(498.73, 283.324), 0.326667);
//         points << qPointValue( QPointF(25,25), 0.5);

       points << qPointValue( QPointF(618.207, 191.9), 0.146667);
        points << qPointValue( QPointF(606.206, 147.299), 0);
        points << qPointValue( QPointF(620.319, 140.289), 0);
        points << qPointValue( QPointF(662.378, 132.064), 0);
        points << qPointValue( QPointF(727.286, 120.842), 0);
        points << qPointValue( QPointF(781.013, 111.439), 0);
        points << qPointValue( QPointF(814.605, 173.209), 0.0266667);
        points << qPointValue( QPointF(816.081, 191.636), 0);
        points << qPointValue( QPointF(817.941, 230.94), 0);
        points << qPointValue( QPointF(812.592, 260.709), 0);
        points << qPointValue( QPointF(748.776, 277.965), 0.12);
        points << qPointValue( QPointF(838.64, 286.338), 0.08);
        points << qPointValue( QPointF(838.902, 362.479), 0);
        points << qPointValue( QPointF(740.147, 397.303), 0.0933333);
        points << qPointValue( QPointF(687.206, 401.018), 0.24);
        points << qPointValue( QPointF(634.232, 416.038), 0.266667);
        points << qPointValue( QPointF(598.355, 419.091), 0.346667);
        points << qPointValue( QPointF(581.419, 373.271), 0.253333);
        points << qPointValue( QPointF(625.026, 337.805), 0.28);
        points << qPointValue( QPointF(678.837, 318.822), 0.2);
        points << qPointValue( QPointF(666.995, 276.466), 0.226667);
        points << qPointValue( QPointF(633.941, 290.71), 0.2);
        points << qPointValue( QPointF(627.545, 242.452), 0.146667);
        points << qPointValue( QPointF(664.449, 231.712), 0.28);
        points << qPointValue( QPointF(578.706, 262.608), 0.146667);
        points << qPointValue( QPointF(567.557, 285.191), 0.133333);
        points << qPointValue( QPointF(547.931, 229.505), 0.0933333);
        points << qPointValue( QPointF(539.234, 196.858), 0);
        points << qPointValue( QPointF(534.874, 171.712), 0);
        points << qPointValue( QPointF(563.212, 152.439), 0);
        points << qPointValue( QPointF(588.915, 150.383), 0);
	
	Interpolator ip;
	
	QList<qPointValue> originalInputs = points;

	// Set bounds to be at minimum the bounds of the background
	points << qPointValue( QPointF(0,0), ip.interpolateValue(QPointF(0,0), originalInputs ) );
	QPointF br = QPointF((double)w, (double)h);
	points << qPointValue( br, ip.interpolateValue(br, originalInputs) );
	
// 	srand(time(NULL));

// 	for(int i=0; i<100; i++)
// 	{
// 		points << qPointValue(QPointF((double)rand()/(double)RAND_MAX*w, (double)rand()/(double)RAND_MAX*h), (double)rand()/(double)RAND_MAX);
// 		qDebug() << "points << qPointValue("<<points.last().point<<", "<<points.last().value<<");";
// 	}
	
// 	points << qPointValue( QPointF(77.8459, 259.802), 0.980021);
// 	points << qPointValue( QPointF(220.788, 23.7931), 0.445183);

// 	points << qPointValue( QPointF(136.454, 214.625), 0.684312);
// 	points << qPointValue( QPointF(2.11327, 169.876), 0.669569);
// 	points << qPointValue( QPointF(150.927, 244.475), 0.25839);
// 	points << qPointValue( QPointF(289.744, 107.131), 0.764456);
// 	points << qPointValue( QPointF(8.46319, 294.867), 0.996068);
//  



// points << qPointValue( QPointF(145.152, 881.28), 0.8);
// points << qPointValue( QPointF(89.856, 603.072), 0.183333);
// points << qPointValue( QPointF(361.152, 1007.42), 0.216667);
// points << qPointValue( QPointF(1134.1, 936.776), 0.1);
// points << qPointValue( QPointF(884.8, 789.019), 0.0166667);
// points << qPointValue( QPointF(832.851, 660.204), 0.183333);
// points << qPointValue( QPointF(944.251, 539.641), 0.05);
// points << qPointValue( QPointF(793.522, 979.344), 0.0833333);
// points << qPointValue( QPointF(746.287, 1065.78), 0.266667);
// points << qPointValue( QPointF(777.358, 1191.72), 0.1);
// points << qPointValue( QPointF(1057, 1201), 0.0166667);
// points << qPointValue( QPointF(975, 1043), 0.1);
// 
// 





	// Fuzzing, above, produced these points, which aren't rendred properly:
// 	points << qPointValue(QPointF(234.93, 118.315),  0.840188);
// 	qDebug() << "Added point: "<<points.last().point<<", val:"<<points.last().value;
// 	points << qPointValue(QPointF(59.2654, 273.494), 0.79844);
// 	qDebug() << "Added point: "<<points.last().point<<", val:"<<points.last().value;
	

	//exit(-1);


//  	#define val(x) ( ((double)x) / 6. )
//  	QList<qPointValue> points = QList<qPointValue>()
//  		<< qPointValue(QPointF(0,0),	val(0))
//  		
// 		<< qPointValue(QPointF(w/4*1,h/4*1),	val(1))
// 		<< qPointValue(QPointF(w/4*2,h/4*1),	val(2))
// 		<< qPointValue(QPointF(w/4*3,h/4*1),	val(4))
// 		<< qPointValue(QPointF(w/4*4,h/4*1),	val(1))
// 		
// 		<< qPointValue(QPointF(w/4*1,h/4*2),	val(6))
// 		<< qPointValue(QPointF(w/4*2,h/4*2),	val(3))
// 		<< qPointValue(QPointF(w/4*3,h/4*2),	val(5))
// 		<< qPointValue(QPointF(w/4*4,h/4*2),	val(2))
// 		
// 		<< qPointValue(QPointF(w/4*1,h/4*3),	val(4))
// 		<< qPointValue(QPointF(w/4*2,h/4*3),	val(2))
// 		<< qPointValue(QPointF(w/4*3,h/4*3),	val(1))
// 		<< qPointValue(QPointF(w/4*4,h/4*3),	val(5))
// 		
// 		<< qPointValue(QPointF(w/4*1,h/4*4),	val(5))
// 		<< qPointValue(QPointF(w/4*2,h/4*4),	val(4))
// 		<< qPointValue(QPointF(w/4*3,h/4*4),	val(2))
// 		<< qPointValue(QPointF(w/4*4,h/4*4),	val(3))
// 		
// 		;
// 
// 	#undef val
	
	
	QImage img = ip.renderPoints(points, QSize(750,750));

	QString objCode = ip.generate3dSurface(points, QSize(750,750));

	printf("# SurfaceInterpolate::generate3dSurface():\n%s\n", qPrintable(objCode));
	
	//exit(-1);
	/*
	
	// Just some debugging:
	
	QImage img(4,4, QImage::Format_ARGB32_Premultiplied);
	
	QList<QList<qPointValue> > quads;
	
	QList<qPointValue> points;
	
// 	points = QList<qPointValue>()
// 		<< qPointValue(QPointF(0,0), 					0.0)
// 		<< qPointValue(QPointF(img.width()/2,0),			0.5)
// 		<< qPointValue(QPointF(img.width()/2,img.height()), 		0.0)
// 		<< qPointValue(QPointF(0,img.height()), 			0.5);
// 	quads << points;
// 	
// 	points = QList<qPointValue>()
// 		<< qPointValue(QPointF(img.width()/2,0), 		0.5)
// 		<< qPointValue(QPointF(img.width(),0),			0.0)
// 		<< qPointValue(QPointF(img.width(),img.height()), 	0.5)
// 		<< qPointValue(QPointF(img.width()/2,img.height()), 	0.0);
// 	quads << points;


	points = QList<qPointValue>()
		<< qPointValue(QPointF(0,0), 				0.0)
		<< qPointValue(QPointF(img.width()-1,0),		0.0)
		<< qPointValue(QPointF(img.width()-1,img.height()-1), 	1.0)
		<< qPointValue(QPointF(0,img.height()-1), 		0.0);
	quads << points;
	
	
	//quads << (points);
	foreach(QList<qPointValue> corners, quads)
	{
		//for(int y=0; y<img.height(); y++)
		for(int y=corners[0].point.y(); y<=corners[2].point.y(); y++)
		{
			QRgb* scanline = (QRgb*)img.scanLine(y);
			for(int x=corners[0].point.x(); x<=corners[2].point.x(); x++)
			{
				double value = quadInterpolate(corners, (double)x, (double)y);
				qDebug() << "pnt:"<<x<<","<<y<<", val:"<<value;
				QColor color = colorForValue(value);
				scanline[x] = color.rgba();
			}
		}
	}
	*/
//	img = img.scaled(rect.size().toSize());
	img.save("interpolate.jpg");

	setPixmap(QPixmap::fromImage(img));//.scaled(200,200)));
	
	//exit(-1);
}


