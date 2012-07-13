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

	#define INTERNAL_IWCONFIG_BINARY ":/data/android/iwconfig"
	#define INTERNAL_IWLIST_BINARY   ":/data/android/iwlist"

#else
#ifdef Q_OS_LINUX

	#define IWCONFIG_BINARY "iwconfig"
	#define IWLIST_BINARY   "iwlist"

	#define INTERNAL_IWCONFIG_BINARY ""
	#define INTERNAL_IWLIST_BINARY   ""
#endif
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
{
	
	qRegisterMetaType<QList<WifiDataResult> >("QList<WifiDataResult> ");
	
	moveToThread(&m_scanThread);
	connect(&m_scanTimer, SIGNAL(timeout()), this, SLOT(scanWifi()));
	
	// Thru repeated testing on my Android device, .75sec is the aparent *minimum*
	// time necessary to wait between calls to "iwlist" in order for iwlist to 
	// scan again (not generate an error about "transport endpoint" or "no scan results")
	m_scanTimer.setInterval(750);

	// TODO - move this to the main app, then if it returns false, ask the user if they want to continue anyway or exit.
	if(auditIwlistBinary())
	{
		qDebug() << "WifiDataCollector::WifiDataCollector(): before start, currentThreadId:" << QThread::currentThreadId();
		
		// Start the scan thread running
		m_scanThread.start();
		
		// Start a continous scan running (in the scan thread)
		QTimer::singleShot(0, this, SLOT(startScan()));
	}
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
	//qDebug() << "WifiDataCollector::scanWifi(): currentThreadId:"<<QThread::currentThreadId()<<", scanning...";
	
	if(m_scanNum == 0)
		emit scanStarted();
		
	QString buffer;
	QList<WifiDataResult> results;

	QString debugTextFile;

	QString interface;
	
#ifndef Q_OS_ANDROID
	interface = findWlanIf();
	if(interface.isEmpty())
		// scan3.txt is just some sample data I captured for use in development
		debugTextFile = QString("scan3-%1.txt").arg(m_scanNum+1);
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
		qDebug() << "WifiDataCollector::scanWifi(): No scan results";
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
	
			//qDebug() << "WifiDataCollector::scanWifi(): Averaged MAC "<<r.mac<<": dBm: "<<r.dbm<<"dBm, value:"<<r.value;
	
			m_scanResults << r;
		}
		
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
	const double dbmMax  =  -40. + 100.; // these ranges TBD thru testing
	const double dbmMin  = -100. + 100.;

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


	return true;
#endif
}

/*! \brief Read 'iwconfig' and find the first interface with wireless extensions */
QString WifiDataCollector::findWlanIf()
{
	QProcess proc;
//	proc.setProcessChannelMode(QProcess::MergedChannels);
	proc.start(IWCONFIG_BINARY);
	
// 	if(!proc.waitForFinished(10000))
// 	{
// 		qDebug() << "WifiDataCollector::findWlanIf(): Error: Timeout while waiting for" << IWCONFIG_BINARY << "to finish.";
// 		return "";
// 	}

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

QString WifiDataCollector::getIwlistOutput(QString interface)
{
	#ifdef Q_OS_ANDROID
		if(interface.isEmpty())
			interface = "tiwlan0";
	#else
		if(interface.isEmpty())
			interface = findWlanIf();
	#endif


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

	/*
	QStringList commandArgs = QStringList()
		<< "-c" << QString("%1 %2 scan")
			.arg(IWLIST_BINARY)
			.arg(interface);
	*/


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
	
	/*
	if(!proc.waitForFinished(10000))
	{
		qDebug() << "WifiDataCollector::getIwlistOutput(): Error: Timeout while waiting for " IWLIST_BINARY " to finish.";
		QMessageBox::information(0, "Debug", IWLIST_BINARY " timeout");
		return "";
	}
	*/

	QString fileContents = proc.readAllStandardOutput();
	if(!fileContents.contains(" Cell "))
		qDebug() << "WifiDataCollector::getIwlistOutput(): Raw output of" << IWLIST_BINARY << ": "<<fileContents.trimmed();

	//QMessageBox::information(0, "Debug", QString("Raw output: %1").arg(fileContents));

	return fileContents;
}

QString WifiDataCollector::readTextFile(QString fileName)
{
	/// JUST FOR DEVELOPMENT

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
		}
		else
		if(line.contains("Signal level"))
		{
			QRegExp sig("Signal level=(-?\\d+ dBm)", Qt::CaseInsensitive);
			if(sig.indexIn(line) > -1)
			{
				values["signal level"] = sig.cap(1);
				//qDebug() << "WifiDataCollector::parseRawBlock: Debug: Extracted value:"<<values["signal level"]<<"from line:"<<line;
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
				qDebug() << "WifiDataCollector::parseRawBlock: Error: line invalid:"<<line;
				return result; // not valid yet
			}
			values[pair[0].toLower()] = pair[1];
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
