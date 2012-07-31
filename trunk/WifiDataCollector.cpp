#include "WifiDataCollector.h"

#ifdef Q_OS_LINUX
#ifndef Q_OS_ANDROID
#include <stdio.h>
#endif
#endif

#include <QDir>

//#define OLD_SCAN_METHOD

#ifdef Q_OS_ANDROID

	#define IWCONFIG_BINARY QString("%1/iwconfig").arg(QDir::tempPath())
	#define IWLIST_BINARY   QString("%1/iwlist").arg(QDir::tempPath())
	#define IWSCAN_SCRIPT   QString("%1/iwscan.sh").arg(QDir::tempPath())

	#define INTERNAL_IWCONFIG_BINARY ":/data/android/iwconfig"
	#define INTERNAL_IWLIST_BINARY   ":/data/android/iwlist"
	#define INTERNAL_IWSCAN_SCRIPT   ":/data/iwscan.sh"
#else
#ifdef Q_OS_LINUX

	#define IWCONFIG_BINARY "iwconfig"
	#define IWLIST_BINARY   "iwlist"
	#define IWSCAN_SCRIPT   "data/iwscan.sh"

	#define INTERNAL_IWCONFIG_BINARY ""
	#define INTERNAL_IWLIST_BINARY   ""
	#define INTERNAL_IWSCAN_SCRIPT   ""
#endif
#endif

#ifdef Q_OS_WIN
	#define popen  _popen
	#define pclose _pclose
#endif

#ifdef Q_OS_ANDROID
// Just guesses based on observations
#define DBM_MAX  -40
#define DBM_MIN -100
#else
// Just guesses based on observations and the assumption that laptop antenna gains are better than the android's 
#define DBM_MAX  -25
#define DBM_MIN  -100
#endif

#ifdef Q_OS_WIN
#define UNICODE

#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>

#include <stdio.h>
#include <stdlib.h>

// Need to link with Wlanapi.lib and Ole32.lib
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")

#endif


// #define ENABLE_WIRELESS_TOOLS
//
// #ifdef ENABLE_WIRELESS_TOOLS
// /* This method of including wireless tools is copied from iwmulticall.c */
//
// /* We need the library */
// #include "3rdparty/wireless-tools/iwlib.c"
//
// /* Get iwconfig in there. Mandatory. */
// #define main(args...) main_iwconfig(args)
// #define iw_usage(args...) iwconfig_usage(args)
// #define find_command(args...) iwconfig_find_command(args)
// #include "3rdparty/wireless-tools/iwconfig.c"
// #undef find_command
// #undef iw_usage
// #undef main
//
// /* Get iwlist in there. Scanning support is pretty sweet. */
// #define main(args...) main_iwlist(args)
// #define iw_usage(args...) iwlist_usage(args)
// #define find_command(args...) iwlist_find_command(args)
// #include "3rdparty/wireless-tools/iwlist.c"
// #undef find_command
// #undef iw_usage
// #undef main
//
// #endif /* ENABLE_WIRELESS_TOOLS */

QString WifiDataResult::toString() const
{
	QStringList buffer;
	buffer << "WifiDataResult(";
	buffer << QObject::tr("ESSID: \"%1\", ").arg(essid);
	buffer << QObject::tr("MAC: %1, ").arg(mac);
	buffer << QObject::tr("Signal: %1 dBm (%2%)").arg(dbm).arg((int)(value*100));
	buffer << ")";
	return buffer.join("");
}

QDebug operator<<(QDebug dbg, const WifiDataResult &ref)
{
	dbg.nospace() << qPrintable(ref.toString());

	return dbg.nospace();
}

WifiDataCollector::WifiDataCollector()
	: QObject()
	, m_numScans(3)
	, m_scanNum(0)
	, m_continuousMode(true)
	, m_scanProcessStarted(false)
	, m_scanPipe(0)
{
	// Register so it can be passed in a queued connection via signal/slots between threads
	qRegisterMetaType<QList<WifiDataResult> >("QList<WifiDataResult> ");
	
	moveToThread(&m_scanThread);
	connect(&m_scanTimer, SIGNAL(timeout()), this, SLOT(scanWifi()));
	
	updateScanInterval();
	
	// TODO - move this to the main app, then if it returns false, ask the user if they want to continue anyway or exit.
	if(auditIwlistBinary())
	{
		qDebug() << "WifiDataCollector::WifiDataCollector(): before start, currentThreadId:" << QThread::currentThreadId();
		
		// Start the scan thread running
		m_scanThread.start();
		
		// Start a continous scan running (in the scan thread)
		QTimer::singleShot(0, this, SLOT(startScan()));
	}

	/*
	qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
	qRegisterMetaType<QProcess::ProcessState>("QProcess::ProcessState");
	
	connect(&m_scanProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(scanProcError ( QProcess::ProcessError )));
	connect(&m_scanProcess, SIGNAL(finished(int , QProcess::ExitStatus )), this, SLOT(scanProcFinished ( int , QProcess::ExitStatus )));
	connect(&m_scanProcess, SIGNAL(readyReadStandardOutput ()), this, SLOT(scanProcReadyReadStandardOutput ()));
	connect(&m_scanProcess, SIGNAL(stateChanged ( QProcess::ProcessState )), this, SLOT(scanProcStateChanged ( QProcess::ProcessState )));
	connect(&m_scanProcess, SIGNAL(readyRead()), this, SLOT(scanProcReadyRead()));
	*/
}

WifiDataCollector::~WifiDataCollector()
{
	//m_scanProcess.close();
	if(m_scanPipe)
	{
		pclose(m_scanPipe);
		m_scanPipe = 0;
	}

}

void WifiDataCollector::scanProcError ( QProcess::ProcessError error )
	{ qDebug() << "WifiDataCollector::scanProcError(): "<<error; }
void WifiDataCollector::scanProcFinished ( int exitCode, QProcess::ExitStatus exitStatus )
	{ qDebug() << "WifiDataCollector::scanProcFinished(): "<<exitCode<<", "<<exitStatus; }
void WifiDataCollector::scanProcReadyReadStandardOutput ()
	{ qDebug() << "WifiDataCollector::scanProcReadyReadStandardOutput()"; }
void WifiDataCollector::scanProcStateChanged ( QProcess::ProcessState newState )
	{ qDebug() << "WifiDataCollector::scanProcStateChanged(): "<<newState; }
void WifiDataCollector::scanProcReadyRead ()
	{ qDebug() << "WifiDataCollector::scanProcReadyRead()"; }



void WifiDataCollector::setWlanDevice(QString dev)
{
	m_wlanDevice = dev;
	updateScanInterval();
}

void WifiDataCollector::setDataTextfile(QString file)
{
	m_dataTextfile = file;
	updateScanInterval();
}

void WifiDataCollector::updateScanInterval()
{
	// Thru repeated testing on my Android device, .75sec is the aparent *minimum*
	// time necessary to wait between calls to "iwlist" in order for iwlist to
	// scan again (not generate an error about "transport endpoint" or "no scan results")
	#ifdef Q_OS_ANDROID
	m_scanTimer.setInterval(750);

	#else
	if(!m_dataTextfile.isEmpty() || findWlanIf().isEmpty())
		// We are running on a machine WITHOUT wireless, so we are going to use a dummy data source,
		// therefore add artificial delay between scans
		m_scanTimer.setInterval(750);
	else
		// iwlist will delay long enough between scans, we don't need to add more delay here on non-Android systems
		//m_scanTimer.setInterval(10);

		m_scanTimer.setInterval(750);
	#endif
}

class WifiResultAverage
{
public:
	WifiResultAverage(WifiDataResult r)
		: result(r)
	{
		ap       = r.mac;
		valueSum = r.value;
		dbmSum   = r.dbm;
		count    = 1;
	};

	void add(double v, int d)
	{
		valueSum += v;
		dbmSum   += d;
		count    ++;
	}

	void add(WifiDataResult r) {
		add(r.value, r.dbm); // we only store the first result
	}

	double avgValue() {
		return valueSum/(double)count;
	}

	double avgDbm() {
		return ((double)dbmSum)/((double)count);
	}

	QString ap;
	double dbmSum;
	double valueSum;
	int count;
	WifiDataResult result;
};

void WifiDataCollector::startScan(int numScans, bool continous)
{
	qDebug() << "WifiDataCollector::startScan(): currentThreadId:"<<QThread::currentThreadId()<<", numScans:"<<numScans<<", continous:"<<continous;
	m_scanNum = 0;
	m_numScans = numScans;
	m_continuousMode = continous;
	
	m_scanTimer.start();
}

void WifiDataCollector::stopScan()
{
	qDebug() << "WifiDataCollector::stopScan(): Stopping scan timer";
	m_scanTimer.stop();
}

QList<WifiDataResult> WifiDataCollector::scanResults()
{
	QMutexLocker lock(&m_resultsMutex);
	return m_scanResults;
}

/*QList<WifiDataResult> */
void WifiDataCollector::scanWifi()
// QString debugTextFile)
{
#ifdef Q_OS_WIN
	// The following code is copied verbatim from http://stackoverflow.com/a/3193613
	// Will rework to populate m_scanResults soon, for now, just getting the code into place.

	// Declare and initialize variables.

	HANDLE hClient = NULL;
	DWORD dwMaxClient = 2;      //
	DWORD dwCurVersion = 0;
	DWORD dwResult = 0;
	DWORD dwRetVal = 0;
	int iRet = 0;

	WCHAR GuidString[39] = {0};

	unsigned int i, j, k;

	/* variables used for WlanEnumInterfaces  */

	PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
	PWLAN_INTERFACE_INFO pIfInfo = NULL;

	PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;
	PWLAN_AVAILABLE_NETWORK pBssEntry = NULL;

	dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
	if (dwResult != ERROR_SUCCESS) {
		wprintf(L"WlanOpenHandle failed with error: %u\n", dwResult);
		return 1;
		// You can use FormatMessage here to find out why the function failed
	}

	dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
	if (dwResult != ERROR_SUCCESS) {
		wprintf(L"WlanEnumInterfaces failed with error: %u\n", dwResult);
		return 1;
		// You can use FormatMessage here to find out why the function failed
	} else {
		wprintf(L"Num Entries: %lu\n", pIfList->dwNumberOfItems);
		wprintf(L"Current Index: %lu\n", pIfList->dwIndex);
		for (i = 0; i < (int) pIfList->dwNumberOfItems; i++) {
			pIfInfo = (WLAN_INTERFACE_INFO *) &pIfList->InterfaceInfo[i];
			wprintf(L"  Interface Index[%u]:\t %lu\n", i, i);
			iRet = StringFromGUID2(pIfInfo->InterfaceGuid, (LPOLESTR) &GuidString,
				sizeof(GuidString)/sizeof(*GuidString));
			// For c rather than C++ source code, the above line needs to be
			// iRet = StringFromGUID2(&pIfInfo->InterfaceGuid, (LPOLESTR) &GuidString,
			//     sizeof(GuidString)/sizeof(*GuidString));
			if (iRet == 0)
				wprintf(L"StringFromGUID2 failed\n");
			else {
				wprintf(L"  InterfaceGUID[%d]: %ws\n",i, GuidString);
			}
			wprintf(L"  Interface Description[%d]: %ws", i,
				pIfInfo->strInterfaceDescription);
			wprintf(L"\n");
			wprintf(L"  Interface State[%d]:\t ", i);
			switch (pIfInfo->isState) {
			case wlan_interface_state_not_ready:
				wprintf(L"Not ready\n");
				break;
			case wlan_interface_state_connected:
				wprintf(L"Connected\n");
				break;
			case wlan_interface_state_ad_hoc_network_formed:
				wprintf(L"First node in a ad hoc network\n");
				break;
			case wlan_interface_state_disconnecting:
				wprintf(L"Disconnecting\n");
				break;
			case wlan_interface_state_disconnected:
				wprintf(L"Not connected\n");
				break;
			case wlan_interface_state_associating:
				wprintf(L"Attempting to associate with a network\n");
				break;
			case wlan_interface_state_discovering:
				wprintf(L"Auto configuration is discovering settings for the network\n");
				break;
			case wlan_interface_state_authenticating:
				wprintf(L"In process of authenticating\n");
				break;
			default:
				wprintf(L"Unknown state %ld\n", pIfInfo->isState);
				break;
			}
			wprintf(L"\n");

			dwResult = WlanGetAvailableNetworkList(hClient,
							 &pIfInfo->InterfaceGuid,
							 0,
							 NULL,
							 &pBssList);

			if (dwResult != ERROR_SUCCESS) {
				wprintf(L"WlanGetAvailableNetworkList failed with error: %u\n",
					dwResult);
				dwRetVal = 1;
				// You can use FormatMessage to find out why the function failed
			} else {
				wprintf(L"WLAN_AVAILABLE_NETWORK_LIST for this interface\n");

				wprintf(L"  Num Entries: %lu\n\n", pBssList->dwNumberOfItems);

				for (j = 0; j < pBssList->dwNumberOfItems; j++) {
					pBssEntry = (WLAN_AVAILABLE_NETWORK *) & pBssList->Network[j];

					wprintf(L"  Profile Name[%u]:  %ws\n", j, pBssEntry->strProfileName);

					wprintf(L"  SSID[%u]:\t\t ", j);
					if (pBssEntry->dot11Ssid.uSSIDLength == 0)
						wprintf(L"\n");
					else {
						for (k = 0; k < pBssEntry->dot11Ssid.uSSIDLength; k++) {
							wprintf(L"%c", (int) pBssEntry->dot11Ssid.ucSSID[k]);
						}
						wprintf(L"\n");
					}

					wprintf(L"  BSS Network type[%u]:\t ", j);
					switch (pBssEntry->dot11BssType) {
					case dot11_BSS_type_infrastructure   :
						wprintf(L"Infrastructure (%u)\n", pBssEntry->dot11BssType);
						break;
					case dot11_BSS_type_independent:
						wprintf(L"Infrastructure (%u)\n", pBssEntry->dot11BssType);
						break;
					default:
						wprintf(L"Other (%lu)\n", pBssEntry->dot11BssType);
						break;
					}

					wprintf(L"  Number of BSSIDs[%u]:\t %u\n", j, pBssEntry->uNumberOfBssids);

					wprintf(L"  Connectable[%u]:\t ", j);
					if (pBssEntry->bNetworkConnectable)
						wprintf(L"Yes\n");
					else
						wprintf(L"No\n");

					wprintf(L"  Signal Quality[%u]:\t %u\n", j, pBssEntry->wlanSignalQuality);

					wprintf(L"  Security Enabled[%u]:\t ", j);
					if (pBssEntry->bSecurityEnabled)
						wprintf(L"Yes\n");
					else
						wprintf(L"No\n");

					wprintf(L"  Default AuthAlgorithm[%u]: ", j);
					switch (pBssEntry->dot11DefaultAuthAlgorithm) {
					case DOT11_AUTH_ALGO_80211_OPEN:
						wprintf(L"802.11 Open (%u)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					case DOT11_AUTH_ALGO_80211_SHARED_KEY:
						wprintf(L"802.11 Shared (%u)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					case DOT11_AUTH_ALGO_WPA:
						wprintf(L"WPA (%u)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					case DOT11_AUTH_ALGO_WPA_PSK:
						wprintf(L"WPA-PSK (%u)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					case DOT11_AUTH_ALGO_WPA_NONE:
						wprintf(L"WPA-None (%u)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					case DOT11_AUTH_ALGO_RSNA:
						wprintf(L"RSNA (%u)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					case DOT11_AUTH_ALGO_RSNA_PSK:
						wprintf(L"RSNA with PSK(%u)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					default:
						wprintf(L"Other (%lu)\n", pBssEntry->dot11DefaultAuthAlgorithm);
						break;
					}

					wprintf(L"  Default CipherAlgorithm[%u]: ", j);
					switch (pBssEntry->dot11DefaultCipherAlgorithm) {
					case DOT11_CIPHER_ALGO_NONE:
						wprintf(L"None (0x%x)\n", pBssEntry->dot11DefaultCipherAlgorithm);
						break;
					case DOT11_CIPHER_ALGO_WEP40:
						wprintf(L"WEP-40 (0x%x)\n", pBssEntry->dot11DefaultCipherAlgorithm);
						break;
					case DOT11_CIPHER_ALGO_TKIP:
						wprintf(L"TKIP (0x%x)\n", pBssEntry->dot11DefaultCipherAlgorithm);
						break;
					case DOT11_CIPHER_ALGO_CCMP:
						wprintf(L"CCMP (0x%x)\n", pBssEntry->dot11DefaultCipherAlgorithm);
						break;
					case DOT11_CIPHER_ALGO_WEP104:
						wprintf(L"WEP-104 (0x%x)\n", pBssEntry->dot11DefaultCipherAlgorithm);
						break;
					case DOT11_CIPHER_ALGO_WEP:
						wprintf(L"WEP (0x%x)\n", pBssEntry->dot11DefaultCipherAlgorithm);
						break;
					default:
						wprintf(L"Other (0x%x)\n", pBssEntry->dot11DefaultCipherAlgorithm);
						break;
					}

					wprintf(L"\n");
				}
			}
		}

	}
	if (pBssList != NULL) {
		WlanFreeMemory(pBssList);
		pBssList = NULL;
	}

	if (pIfList != NULL) {
		WlanFreeMemory(pIfList);
		pIfList = NULL;
	}

	return dwRetVal;
#endif


	//qDebug() << "WifiDataCollector::scanWifi(): currentThreadId:"<<QThread::currentThreadId()<<", scanning...";
	
	if(m_scanNum == 0)
		emit scanStarted();
		
	QString buffer;
	QList<WifiDataResult> results;

	QString debugTextFile;

	QString interface;
	
#ifndef Q_OS_ANDROID
	if(!m_dataTextfile.isEmpty())
		debugTextFile = m_dataTextfile;
	else
	{
		interface = m_wlanDevice;

		if(interface.isEmpty())
			interface = findWlanIf();
		
		if(interface.isEmpty())
			// scan3.txt is just some sample data I captured for use in development
			//debugTextFile = QString("scan3-%1.txt").arg(m_scanNum+1);
			debugTextFile = QString("scan3.txt");
	}
#endif

	// Read input from debug file or the actual wifi card
	if(!debugTextFile.isEmpty())
		buffer = readTextFile(debugTextFile);
	else
		buffer = getIwlistOutput();
	
	//qDebug() << "WifiDataCollector::scanWifi(): Raw buffer: "<<buffer;

	//QMessageBox::information(0, "Debug", QString("Raw buffer: %1").arg(buffer));

	// Parse the resulting input buffer
	if(buffer.isEmpty() || buffer.contains("No scan results") || buffer.contains("Operation not supported"))
	{
		//qDebug() << "WifiDataCollector::scanWifi(): No scan results";
		//return results; // return empty list
	}
	else
	{
		QStringList rawBlocks = buffer.split(" Cell ");
	
		foreach(QString block, rawBlocks)
		{
			if(block.toLower().contains("scan completed"))
				continue;
	
			WifiDataResult result = parseRawBlock(block);
			if(!result.valid || result.mac.isEmpty())
			{
				qDebug() << "WifiDataCollector::scanWifi(): Error parsing raw block: "<<block;
				continue;
			}
	
			//qDebug() << "WifiDataCollector::scanWifi(): Parsed result: "<<result;
			results << result;
		}
	}

	m_resultBuffer << results;
	
	if(!m_continuousMode)
		emit scanProgress(m_scanNum / ((double)m_numScans));
	
	m_scanNum ++;
	
	if(m_continuousMode || 
	   m_scanNum >= m_numScans)
	{
		// The "new" scanning method just scans X number of times, possibly duplicating APs
		// So here we have to merge (average) the signal readings of any duplicate APs
	
		QHash<QString, WifiResultAverage*> resultMap;
	
		foreach(QList<WifiDataResult> results, m_resultBuffer)
		{
			foreach(WifiDataResult r, results)
			{
				//qDebug() << "WifiDataCollector::scanWifi(): Merging results, current: "<<r.mac<<", dbm:"<<r.dbm<<", val:"<<r.value;
				if(!resultMap.contains(r.mac))
					resultMap.insert(r.mac, new WifiResultAverage(r));
				else
					resultMap.value(r.mac)->add(r);
			}
		}
		
		// If running continuous scan, we just shift off the first (earliest) scan
		// since the next scan will just be appended to the end. This maintains a 
		// "running average" of the scan results, with the window being at most 
		// m_numScans samples long
		if(m_continuousMode)
		{
			while(m_resultBuffer.size() >= m_numScans)
			      m_resultBuffer.takeFirst();
		}
		else
		{
			// If not in continous, clear the buffer for the next scan
			m_resultBuffer.clear();
		}
	
		// This lock will hold till the end of the function
		QMutexLocker lock(&m_resultsMutex);
		
		// Even in continous mode, we always write a fresh list to the output buffer
		m_scanResults.clear();
		
		foreach(WifiResultAverage *avg, resultMap.values())
		{
			WifiDataResult r = avg->result;
	
			// Update the value & dbm of this AP
			r.value = avg->avgValue();
			r.dbm   = (int)avg->avgDbm();
	
			// Since MapGraphicsScene *only* stores r.rawData, update the rawData with "fake" readings
			r.rawData["signal level"] = QString("%1 dBm").arg((int)r.dbm);
	
			//qDebug() << "WifiDataCollector::scanWifi(): Averaged "<<r.essid<<", MAC "<<r.mac<<": dBm: "<<r.dbm<<"dBm, value:"<<r.value;
	
			m_scanResults << r;
		}
		//qDebug() << "WifiDataCollector::scanWifi(): Found"<<m_scanResults.size()<<" APs nearby";
		
		if(m_scanNum >= m_numScans)
			m_scanNum = 0;
		
		emit scanFinished(m_scanResults);
		
		if(!m_continuousMode)
			m_scanTimer.stop();
	}
}


double WifiDataCollector::dbmToPercent(int dbm)
{
	// TODO - evaluate and device a better algorithm for changing dbm->%
	double dbmVal = (double)(dbm + 100.); // dBm is a neg #, like -95 dBm, so add 100 to flip it over zero
	const double dbmMax  = DBM_MAX + 100.; // these ranges TBD thru testing
	const double dbmMin  = DBM_MIN + 100.;

	double val = (dbmVal - dbmMin) / (dbmMax - dbmMin);
	return val;
}

/*! \brief This is for use on the Android platform - it checks for $TMP/iwlist and $TMP/iwconfig,
	and extracts them from :/data if not present (and enables the executable bit.)
	Noop if not on Android. ($TMP = QDir::tempPath()) 
*/
bool WifiDataCollector::auditIwlistBinary()
{
#ifndef Q_OS_ANDROID
	return true;
#else
	// Extract ARM versions of iwconfig/iwlist and our iwscan.sh from internal Qt resource storage
	if(!QFile::exists(IWCONFIG_BINARY))
	{
		if(!QFile(INTERNAL_IWCONFIG_BINARY).copy(IWCONFIG_BINARY))
		{
			QString errorMsg = QString("Error copying " INTERNAL_IWLIST_BINARY " to %1, app likely will not function.").arg(IWCONFIG_BINARY);
			qDebug() << "WifiDataCollector::auditIwlistBinary(): " << qPrintable(errorMsg);
			QMessageBox::critical(0, "Setup Error", errorMsg);
			return false;
		}
		else
		{
			qDebug() << "WifiDataCollector::auditIwlistBinary(): Successfully extracted " << INTERNAL_IWCONFIG_BINARY << " to " << IWCONFIG_BINARY;
		}
	}

	if(!QFile::exists(IWLIST_BINARY))
	{
		if(!QFile(INTERNAL_IWLIST_BINARY).copy(IWLIST_BINARY))
		{
			QString errorMsg = QString("Error copying " INTERNAL_IWLIST_BINARY " to %1, app likely will not function.").arg(IWLIST_BINARY);
			qDebug() << "WifiDataCollector::auditIwlistBinary(): " << qPrintable(errorMsg);
			QMessageBox::critical(0, "Setup Error", errorMsg);
			return false;
		}
		else
		{
			qDebug() << "WifiDataCollector::auditIwlistBinary(): Successfully extracted " << INTERNAL_IWLIST_BINARY << " to " << IWLIST_BINARY;
		}
	}

	if(!QFile::exists(IWSCAN_SCRIPT))
	{
		if(!QFile(INTERNAL_IWSCAN_SCRIPT).copy(IWSCAN_SCRIPT))
		{
			QString errorMsg = QString("Error copying " INTERNAL_IWSCAN_SCRIPT " to %1, app likely will not function.").arg(IWSCAN_SCRIPT);
			qDebug() << "WifiDataCollector::auditIwlistBinary(): " << qPrintable(errorMsg);
			QMessageBox::critical(0, "Setup Error", errorMsg);
			return false;
		}
		else
		{
			qDebug() << "WifiDataCollector::auditIwlistBinary(): Successfully extracted " << INTERNAL_IWSCAN_SCRIPT << " to " << IWSCAN_SCRIPT;
		}
	}

	// Check/set the executable bits on the iwlist/iwconfig binaries
	if(!QFileInfo(IWCONFIG_BINARY).isExecutable())
	{
		//QFile(IWLIST_BINARY).setPermissions(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther))
		system(qPrintable(QString("chmod 755 %1").arg(IWCONFIG_BINARY)));
		qDebug() << "WifiDataCollector::auditIwlistBinary(): Executed chmod 755" << IWCONFIG_BINARY;
	}

	if(!QFileInfo(IWLIST_BINARY).isExecutable())
	{
		//QFile(IWLIST_BINARY).setPermissions(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther))
		system(qPrintable(QString("chmod 755 %1").arg(IWLIST_BINARY)));
		qDebug() << "WifiDataCollector::auditIwlistBinary(): Executed chmod 755" << IWLIST_BINARY;
	}

	if(!QFileInfo(IWSCAN_SCRIPT).isExecutable())
	{
		//QFile(IWLIST_BINARY).setPermissions(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther))
		system(qPrintable(QString("chmod 755 %1").arg(IWSCAN_SCRIPT)));
		qDebug() << "WifiDataCollector::auditIwlistBinary(): Executed chmod 755" << IWSCAN_SCRIPT;
	}


	return true;
#endif
}

/*! \brief Read 'iwconfig' and find the first interface with wireless extensions */
QString WifiDataCollector::findWlanIf()
{
#ifdef Q_OS_WIN
	return "";
#endif

	QProcess proc;
	proc.start(IWCONFIG_BINARY);

	if (!proc.waitForStarted())
		QMessageBox::information(0, "Debug", "start err:" + proc.errorString());
	

	if (!proc.waitForFinished(-1))
		QMessageBox::information(0, "Debug", "finish err:" + proc.errorString());
	
	QString fileContents = proc.readAllStandardOutput();
	//qDebug() << "WifiDataCollector::findWlanIf(): Raw output of" << IWCONFIG_BINARY << ": "<<fileContents;

	// Parse output (iwconfig is nice - it only prints wireless interfaces actually found to stdout)
	QStringList tokens = fileContents.split(" ");
	if(fileContents.isEmpty() || tokens.isEmpty())
	{
		//qDebug() << "WifiDataCollector::findWlanIf(): Error: No wireless interfaces found on this system.";
		return "";
	}

	QString wlanIf = tokens.takeFirst();
	//qDebug() << "WifiDataCollector::findWlanIf(): Success, found interface: "<<wlanIf;
	return wlanIf;
}

/*! \brief Read 'iwconfig' and find all wireless interface with wireless extensions */
QStringList WifiDataCollector::findWlanInterfaces()
{
	QStringList list;
#ifdef Q_OS_WIN
	return list;
#endif

	QProcess proc;
	proc.start(IWCONFIG_BINARY);
	
	if (!proc.waitForStarted())
	{
		QMessageBox::information(0, "Debug", "start err:" + proc.errorString());
	}


	if (!proc.waitForFinished(-1))
	{
		QMessageBox::information(0, "Debug", "finish err:" + proc.errorString());
	}
	
	QString fileContents = proc.readAllStandardOutput();
	
	// Parse output (iwconfig is nice - it only prints wireless interfaces actually found to stdout)
	QStringList tokens = fileContents.split("\n\n");
	if(fileContents.isEmpty() || tokens.isEmpty())
	{
		return list;
	}

	foreach(QString token, tokens)
	{
		QStringList subtokens = token.split(" ");
		if(subtokens.isEmpty())
			continue;
			
		QString wlanIf = subtokens.takeFirst();
		if(!wlanIf.isEmpty())
			list << wlanIf;
	}

	return list;
}

QString WifiDataCollector::getIwlistOutput(QString interface)
{
	if(!m_scanProcessStarted) //m_scanProcess.state() == QProcess::NotRunning)
	{
		#ifdef Q_OS_ANDROID
			if(interface.isEmpty())
				interface = "tiwlan0";
		#else
			if(interface.isEmpty())
				interface = findWlanIf();
		#endif

		m_scanProcessStarted = true;
		QString cmd;

		#ifdef Q_OS_ANDROID
			cmd = QString("su -c 'IWSCAN=%1 IF=%2 SLEEP=0.8 %3 2>&1'")
		#else
			cmd = QString(       "IWSCAN=%1 IF=%2 SLEEP=0.2 %3" )
		#endif
			.arg(IWLIST_BINARY)
			.arg(interface)
			.arg(IWSCAN_SCRIPT);

		#ifdef Q_OS_LINUX
		#ifndef Q_OS_ANDROID
		if(getuid() != 0)
			// kdesu doesn't work correctly for our needs (or rather, I don't know how to make it work)
			// - We never get any output from the subcommand (the contents of cmd) (sudo gives us the output we need)
			// - The kdesu process (and subcommand) stay active even after pclose() (just using sudo doesn't stay active after pclose)
			//cmd = "kdesu /bin/bash -c '" + cmd.replace("'", "\\'") + "'";
			
			cmd = "sudo " + cmd;
		#endif
		#endif

		qDebug() << "WifiDataCollector::getIwlistOutput(): Starting scan loop: "<<cmd;

		m_scanPipe = popen(qPrintable(cmd), "r");
		if(!m_scanPipe)
		{
			qDebug() << "WifiDataCollector::getIwlistOutput(): Error opening pipe for cmd: "<<cmd;
			m_scanProcessStarted = false;
			return "";
		}
	}

	if(m_scanProcessStarted)
	{
		QString fileContents;
		char buffer[128];

		bool done = false;
		while(!done)
		{
			if(feof(m_scanPipe))
			{
				m_scanProcessStarted = false;
				pclose(m_scanPipe);
				m_scanPipe = 0;
				qDebug() << "WifiDataCollector::getIwlistOutput(): EOF on scan pipe, reopen on next scan call";
				return "";
			}

			if(fgets(buffer, 128, m_scanPipe) != NULL)
				fileContents += buffer;

			//qDebug() << "WifiDataCollector::getIwlistOutput(): [loop] current fileContents:"<<fileContents.trimmed();
			
			if(fileContents.contains("--EOS--"))
			{
				fileContents = fileContents.replace("--EOS--","");
				done = true;
			}
		}

		if(!fileContents.contains(" Cell "))
			qDebug() << "WifiDataCollector::getIwlistOutput(): Bad output from" << IWLIST_BINARY << ": "<<fileContents.trimmed();

		//qDebug() << "WifiDataCollector::getIwlistOutput(): Raw pipe read:"<<fileContents.trimmed();

		return fileContents;
	}

	// scan process not started by now, just return an empty string and let the next level of code handle it
	return ""; 
	
/*

	#ifdef Q_OS_ANDROID
		// Must scan as root first to force the wifi card to re-scan the area
		// HOWEVER, this doesn't always *print* all the access points that it finds.
		// So, we scan here, ignore output, then scan again (as normal user), which
		// *does* print out all the results, but doesn't force a re-scan (normal
		// users use the cached results in the kernel)
		system(qPrintable(QString("su -c '%1 scan'; sleep 1").arg(IWLIST_BINARY)));
	#else
	#ifdef Q_OS_LINUX
		if(getuid() != 0)
		{
			int ret = system(qPrintable(QString("sudo %1 scan 2>/dev/null >/dev/null").arg(IWLIST_BINARY)));
			(void)ret; // ignore warnings about unused ret val of system()
		}
	#endif
	#endif

	QStringList scanArgs = QStringList() << interface << "scan";

	//QMessageBox::information(0, "Debug", QString("Scan args: %1").arg(scanArgs.join(" ")));

	QProcess proc;

	//qDebug() << "WifiDataCollector::getIwlistOutput(): su commandArgs: "<<commandArgs;

	proc.setProcessChannelMode(QProcess::MergedChannels);
	//proc.start("su", commandArgs);
	proc.start(IWLIST_BINARY, scanArgs);
	
	if (!proc.waitForStarted())
	{
		qDebug() << "********************** Scanner start failed! " << proc.errorString();
		QMessageBox::information(0, "Debug", "start err:" + proc.errorString());
	}


	if (!proc.waitForFinished(-1))
	{
		qDebug() << "********************** Scanner unable to finish! " << proc.errorString();
		QMessageBox::information(0, "Debug", "finish err:" + proc.errorString());
	}
	
	QString fileContents = proc.readAllStandardOutput();
	if(!fileContents.contains(" Cell "))
		qDebug() << "WifiDataCollector::getIwlistOutput(): Raw output of" << IWLIST_BINARY << ": "<<fileContents.trimmed();

	//QMessageBox::information(0, "Debug", QString("Raw output: %1").arg(fileContents));

	return fileContents;
*/
}

QString WifiDataCollector::readTextFile(QString fileName)
{
	// Load text file
	QFile file(fileName);
	if(!file.open(QIODevice::ReadOnly))
	{
		qDebug() << "WifiDataCollector::readTextFile(): Unable to read:" <<fileName;
		
		// Can't show message box since readTextFile() since we're being called from inside the thread
		//QMessageBox::critical(0,QObject::tr("Can't Read File"),QString(QObject::tr("Unable to open %1")).arg(fileName));
		return "";
	}

	QTextStream stream(&file);
	QString fileContents = stream.readAll();

	return fileContents;
}

WifiDataResult WifiDataCollector::parseRawBlock(QString buffer)
{
/*01 - Address: 00:1A:70:59:5B:6F
                    ESSID:"PHC"
                    Protocol:IEEE 802.11 BG
                    Mode:Managed
                    Frequency:2.412 GHz
                    Signal level=-86 dBm
                    Encryption key:off
                    Bit Rates:1 Mb/s; 2 Mb/s; 5.5 Mb/s; 11 Mb/s; 6 Mb/s
                              9 Mb/s; 12 Mb/s; 18 Mb/s; 24 Mb/s; 36 Mb/s
                              48 Mb/s; 54 Mb/s
                    Extra:Bcn int = 100 ms
                    IE: Unknown: 0003504843
                    IE: Unknown: 010882848B962430486C
                    IE: Unknown: 030101
                    IE: Unknown: 050400010000
                    IE: Unknown: 2A0104
                    IE: Unknown: 2F0104
                    IE: Unknown: 32040C121860
                    IE: Unknown: DD090010180204F4000000
          */

	//qDebug() << "WifiDataCollector::parseRawBlock(): "<<buffer;
	
	buffer = buffer.replace(QRegExp("^\\s*\\d+ - Address:"),"Address:");
	QStringList lines = buffer.split("\n");

	WifiDataResult result;

	QStringHash values;
	while(!lines.isEmpty())
	{
		QString line = lines.takeFirst();

		line = line.replace(QRegExp("[\\r\\n]"),"");
		line = line.replace(QRegExp("(^\\s+|\\s+$)"),"");

		if(line.isEmpty())
			continue;

		if(line.startsWith("Bit Rates"))
		{
			continue;
			// Code below is buggy (last line gets dropped) - remove for now until I have time to debug
			/*
			line = line.replace("Bit Rates:","");

			QStringList bitRates;
			bitRates.append(line);

			line = "";
			while(!line.contains(":") && !lines.isEmpty())
			{
				line = lines.takeFirst();
				if(line.contains(":"))
					break;

				line = line.replace(QRegExp("(^\\s+|\\s+$)"),"");
				bitRates.append(line);
			}

			if(line.contains(":"))
				lines.prepend(line);

			values["bit rates"] = bitRates.join("; ");
			*/
		}
		else
		if(line.startsWith("IE: Unknown:"))
		{
			continue;
		}
		else
		if(line.startsWith("Address"))
		{
			line = line.replace("Address: ","");
			values["address"] = line;
			//qDebug() << "WifiDataCollector::parseRawBlock: [parse:mac] "<<line;
		}
		else
		if(line.contains("Signal level"))
		{
			QRegExp sigDbm("Signal level=(-?\\d+ dBm)", Qt::CaseInsensitive);
			QRegExp sigLvl("Signal level=(\\d+)/(\\d+)", Qt::CaseInsensitive);
			if(sigDbm.indexIn(line) > -1)
			{
				values["signal level"] = sigDbm.cap(1);
				//qDebug() << "WifiDataCollector::parseRawBlock: Debug: Extracted value:"<<values["signal level"]<<"from line:"<<line;
				//qDebug() << "WifiDataCollector::parseRawBlock: [parse:sig:dBm] "<<values["signal level"];
			}
			else
			if(sigLvl.indexIn(line) > -1)
			{
				// Create a fake dBm value based on our MIN/MAX assumptions defined at the top of this file
				int val0 = sigLvl.cap(1).toInt();
				int val1 = sigLvl.cap(2).toInt();
				double value = (double)val0 / (double)val1;
				double range = DBM_MAX - DBM_MIN;
				int    dBm   = (int)(DBM_MIN + range * value);
				values["signal level"] = QString("%1 dBm").arg(dBm);

				//qDebug() << "WifiDataCollector::parseRawBlock: [parse:sig:fakeDbm] line:"<<line<<", val0:"<<val0<<", val1:"<<val1<<", value:"<<value<<", range:"<<range<<", dNm:"<<dBm;
			}
			else
			{
				qDebug() << "WifiDataCollector::parseRawBlock: Error: Unable to extract signal level from line:"<<line;
				return result; // not valid yet
			}
		}
		else
		{
			QStringList pair = line.split(QRegExp(":"));
			if(pair.length() < 2)
			{
				//qDebug() << "WifiDataCollector::parseRawBlock: Error: line invalid:"<<line;
				//return result; // not valid yet
				continue;
			}
			QString key = pair.takeFirst().toLower().trimmed();
			QString value = pair.join(":").trimmed();
			values[key] = value;

			//qDebug() << "WifiDataCollector::parseRawBlock: [parse:generic] "<<key<<" = "<<value;
		}
	}

	//qDebug() << "WifiDataCollector::parseRawBlock: raw values: "<<values;

	result.loadRawData(values);

	return result;
}

void WifiDataResult::loadRawData(QStringHash values)
{
	rawData = values;

	// Parse values in 'values' and populate result
	essid = values["essid"].replace("\"","");
	mac   = values["address"];
	dbm   = values["signal level"].replace(" dBm","").toInt();
	chan  = values["frequency"].replace(" GHz","").toDouble();
	value = WifiDataCollector::dbmToPercent(dbm);

	if(!mac.isEmpty())
		valid = true;
}
