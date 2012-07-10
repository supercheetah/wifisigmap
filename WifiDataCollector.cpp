#include "WifiDataCollector.h"

#include <QDir>

//#define OLD_SCAN_METHOD

#ifdef Q_OS_ANDROID

    #define IWCONFIG_BINARY QString("%1/iwconfig").arg(QDir::tempPath());
    #define IWLIST_BINARY   QString("%1/iwlist").arg(QDir::tempPath());
    #define IWSCAN_SCRIPT   QString("%1/iwscan").arg(QDir::tempPath());

    #define INTERNAL_IWCONFIG_BINARY ":/data/android/iwconfig"
    #define INTERNAL_IWLIST_BINARY   ":/data/android/iwlist"
    #define INTERNAL_IWSCAN_SCRIPT   ":/data/android/iwscan.sh"
#else
#ifdef Q_OS_LINUX

	#define IWCONFIG_BINARY "iwconfig"
	#define IWLIST_BINARY   "iwlist"
	#define IWSCAN_SCRIPT   "iwscan.sh"
	
	#define INTERNAL_IWCONFIG_BINARY ""
	#define INTERNAL_IWLIST_BINARY   ""
	#define INTERNAL_IWSCAN_SCRIPT   ""
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
{
	// TODO - move this to the main app, then if it returns false, ask the user if they want to continue anyway or exit.
	auditIwlistBinary();
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


QList<WifiDataResult> WifiDataCollector::scanWifi(QString debugTextFile)
{
	QString buffer;
	QList<WifiDataResult> results;
	 
	if(!debugTextFile.isEmpty())
		buffer = readTextFile(debugTextFile);
#ifdef OLD_SCAN_METHOD
	else
	{
		buffer = getIwlistOutput();
	}
#else
	else
	{
		// The new method of scanning is very fast,
		// so try to make sure we get *something*
		int counter = 0;
		while(counter ++ < 10 && 
		     !buffer.contains(" Cell "))
		      buffer = getIwlistOutput();
	}
#endif
	
	qDebug() << "WifiDataCollector::scanWifi(): Raw buffer: "<<buffer;

	//QMessageBox::information(0, "Debug", QString("Raw buffer: %1").arg(buffer));

#ifdef OLD_SCAN_METHOD

	if(buffer.isEmpty() || buffer.contains("No scan results"))
	{
		qDebug() << "WifiDataCollector::scanWifi(): No scan results";
		return results; // return empty list
	}
		
	QStringList rawBlocks = buffer.split(" Cell ");
	
	foreach(QString block, rawBlocks)
	{
		if(block.toLower().contains("scan completed"))
			continue;
			
		WifiDataResult result = parseRawBlock(block);
		if(!result.valid)
		{
			qDebug() << "WifiDataCollector::scanWifi(): Error parsing raw block: "<<block;
			continue;
		}
		
		qDebug() << "WifiDataCollector::scanWifi(): Parsed result: "<<result;
		results << result;
	}
#else
	QStringList scanBuffers = buffer.split("# Scan");
	qDebug() << "WifiDataCollector::scanWifi(): New Scan Method: Found "<<scanBuffers<<" scan results";
	
	int idx = 0;
	foreach(QString scanBuffer, scanBuffers)
	{
		idx ++;
		if(scanBuffer.isEmpty() || scanBuffer.contains("No scan results"))
		{
			//qDebug() << "WifiDataCollector::scanWifi(): No scan results";
			//return results; // return empty list
			qDebug() << "WifiDataCollector::scanWifi(): No scan results in buffer "<<idx;
			continue;
		}
			
		QStringList rawBlocks = scanBuffer.split(" Cell ");
		
		int blockIdx = 0;
		foreach(QString block, rawBlocks)
		{
			blockIdx ++;
			if(block.toLower().contains("scan completed"))
			{
				qDebug() << "WifiDataCollector::scanWifi(): No scan results in buffer "<<idx<<", block "<<blockIdx<<" (scan completed)";
				continue;
			}
				
			WifiDataResult result = parseRawBlock(block);
			if(!result.valid)
			{
				qDebug() << "WifiDataCollector::scanWifi(): Error parsing raw block "<<blockIdx<<": "<<block;
				continue;
			}
			
			qDebug() << "WifiDataCollector::scanWifi(): New scan method, parsed result for block"<<blockIdx<<": "<<result; //<<", raw:"<<block;
			results << result;
		}
	}
	
	// The "new" scanning method just scans X number of times, possibly duplicating APs
	// So here we have to merge (average) the signal readings of any duplicate APs
	
	QHash<QString, WifiResultAverage*> resultMap;
	
	foreach(WifiDataResult r, results)
	{
		qDebug() << "WifiDataCollector::scanWifi(): Merging results, current: "<<r.mac<<", dbm:"<<r.dbm<<", val:"<<r.value;
		if(!resultMap.contains(r.mac))
			resultMap.insert(r.mac, new WifiResultAverage(r));
		else
			resultMap.value(r.mac)->add(r);
	}
	
	results.clear();
	foreach(WifiResultAverage *avg, resultMap.values())
	{
		WifiDataResult r = avg->result;
		
		// Update the value & dbm of this AP
		r.value = avg->avgValue();
		r.dbm   = (int)avg->avgDbm();
		
		// Since MapGraphicsScene *only* stores r.rawData, update the rawData with "fake" readings
		r.rawData["signal level"] = QString("%1 dBm").arg((int)r.dbm);
		
		qDebug() << "WifiDataCollector::scanWifi(): Averaged MAC "<<r.mac<<": dBm: "<<r.dbm<<"dBm, value:"<<r.value;
		
		results << r;
	}
	
#endif
	
	
	return results;
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

/*! \brief This is for use on the Android platform - it checks for /tmp/iwlist and /tmp/iwconfig,
	and extracts them from :/data if not present (and enables the executable bit.)
	Noop if not on Android.
*/ 
bool WifiDataCollector::auditIwlistBinary()
{
#ifndef Q_OS_ANDROID
	return true;
#endif
	
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
			qDebug() << "WifiDataCollector::auditIwlistBinary(): Successfully extracted " INTERNAL_IWCONFIG_BINARY " to " IWCONFIG_BINARY; 
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
			qDebug() << "WifiDataCollector::auditIwlistBinary(): Successfully extracted " INTERNAL_IWLIST_BINARY " to " IWLIST_BINARY; 
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
			qDebug() << "WifiDataCollector::auditIwlistBinary(): Successfully extracted " INTERNAL_IWSCAN_SCRIPT " to " IWSCAN_SCRIPT; 
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
}

/*! \brief Read 'iwconfig' and find the first interface with wireless extensions */
QString WifiDataCollector::findWlanIf()
{
	QProcess proc;
	proc.start(IWCONFIG_BINARY);
	if(!proc.waitForFinished(10000))
	{
		qDebug() << "WifiDataCollector::findWlanIf(): Error: Timeout while waiting for" << IWCONFIG_BINARY << "to finish.";
		return "";
	}
	
	QString fileContents = proc.readAllStandardOutput();
	qDebug() << "WifiDataCollector::findWlanIf(): Raw output of" << IWCONFIG_BINARY << ": "<<fileContents;
	
	// Parse output (iwconfig is nice - it only prints wireless interfaces actually found to stdout)
	QStringList tokens = fileContents.split(" ");
	if(fileContents.isEmpty() || tokens.isEmpty())
	{
		qDebug() << "WifiDataCollector::findWlanIf(): Error: No wireless interfaces found on this system.";
		return "";
	}
	
	QString wlanIf = tokens.takeFirst();
	qDebug() << "WifiDataCollector::findWlanIf(): Success, found interface: "<<wlanIf;
	return wlanIf;
}

QString WifiDataCollector::getIwlistOutput(QString interface)
{
	if(interface.isEmpty())
		interface = findWlanIf();
	
	#ifdef Q_OS_ANDROID
		if(interface.isEmpty())
			interface = "tiwlan0";
	#endif
	
	#ifdef OLD_SCAN_METHOD
	
		#ifdef Q_OS_ANDROID
			// Must scan as root first to force the wifi card to re-scan the area
			// HOWEVER, this doesn't always *print* all the access points that it finds.
			// So, we scan here, ignore output, then scan again (as normal user), which
			// *does* print out all the results, but doesn't force a re-scan (normal
			// users use the cached results in the kernel)
			system(qPrintable(QString("su -c '%1 scan'; sleep 1").arg(IWLIST_BINARY)));
		#endif
		
		QStringList scanArgs = QStringList() << interface << "scan";
	
		//QMessageBox::information(0, "Debug", QString("Scan args: %1").arg(scanArgs.join(" ")));
	
		QProcess proc;
		proc.start(IWLIST_BINARY, scanArgs);
	
	#else
	
		int numScanAttempts = 3; // TODO: Configurable?
		
		QStringList scanArgs = QStringList() << interface << QString::number(numScanAttempts);
		
		//QMessageBox::information(0, "Debug", QString("Scan args: %1").arg(scanArgs.join(" ")));
	
		QProcess proc;
		proc.start(IWSCAN_SCRIPT, scanArgs);
	#endif

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
	qDebug() << "WifiDataCollector::getIwlistOutput(): Raw output of" << IWLIST_BINARY << ": "<<fileContents;
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
		QMessageBox::critical(0,QObject::tr("Can't Read File"),QString(QObject::tr("Unable to open %1")).arg(fileName));
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
		{
			QStringList pair = line.split(QRegExp("[:=]"));
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
	valid = true;
	
	// Parse values in 'values' and populate result
	essid = values["essid"].replace("\"","");
	mac   = values["address"];
	dbm   = values["signal level"].replace(" dBm","").toInt();
	chan  = values["frequency"].replace(" GHz","").toDouble();
	value = WifiDataCollector::dbmToPercent(dbm);
}
