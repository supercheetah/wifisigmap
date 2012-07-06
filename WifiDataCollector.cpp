#include "WifiDataCollector.h"

#ifdef Q_OS_LINUX
	#define IWCONFIG_BINARY "iwconfig"
	#define IWLIST_BINARY   "iwlist"
	
	#define INTERNAL_IWCONFIG_BINARY ""
	#define INTERNAL_IWLIST_BINARY   ""
#else
	#define IWCONFIG_BINARY "/tmp/iwconfig"
	#define IWLIST_BINARY   "/tmp/iwlist"
	
	// TODO - Add these binaries to the .qrc file (heck, *create* the qrc file!)
	#define INTERNAL_IWCONFIG_BINARY ":/data/android/iwconfig"
	#define INTERNAL_IWLIST_BINARY   ":/data/android/iwlist"
#endif

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

QList<WifiDataResult> WifiDataCollector::scanWifi(QString debugTextFile)
{
	QString buffer;
	QList<WifiDataResult> results;
	 
	if(!debugTextFile.isEmpty())
		buffer = readTextFile(debugTextFile);
	else
		buffer = getIwlistOutput();

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
	
	return results;
}

	
double WifiDataCollector::dbmToPercent(int dbm)
{
	// TODO - evaluate and device a better algorithm for changing dbm->%
	double dbmVal = (double)(dbm + 100.); // dBm is a neg #, like -95 dBm, so add 100 to flip it over zero
	const double dbmMax  =  -50. + 100.; // these ranges TBD thru testing
	const double dbmMin  = -120. + 100.;
	
	double val = (dbmVal - dbmMin) / (dbmMax - dbmMin);
	return val;
}

/*! \brief This is for use on the Android platform - it checks for /tmp/iwlist and /tmp/iwconfig,
	and extracts them from :/data if not present (and enables the executable bit.)
	Noop if not on Android.
*/ 
bool WifiDataCollector::auditIwlistBinary()
{
#ifdef Q_OS_LINUX
	return true;
#endif
	
	// Extract iwconfig/iwlist from internal Qt resource storage
	if(!QFile::exists(IWCONFIG_BINARY))
	{
		if(!QFile(INTERNAL_IWCONFIG_BINARY).copy(IWCONFIG_BINARY))
		{
			QString errorMsg = "Error copying " INTERNAL_IWCONFIG_BINARY " to " IWCONFIG_BINARY ", app likely will not function.";
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
			QString errorMsg = "Error copying " INTERNAL_IWLIST_BINARY " to " IWLIST_BINARY ", app likely will not function.";
			qDebug() << "WifiDataCollector::auditIwlistBinary(): " << qPrintable(errorMsg);
			QMessageBox::critical(0, "Setup Error", errorMsg);
			return false;
		}
		else
		{
			qDebug() << "WifiDataCollector::auditIwlistBinary(): Successfully extracted " INTERNAL_IWLIST_BINARY " to " IWLIST_BINARY; 
		}
	}
		
	// Check/set the executable bits on the iwlist/iwconfig binaries
	if(!QFileInfo(IWCONFIG_BINARY).isExecutable())
	{
		//QFile(IWLIST_BINARY).setPermissions(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther))
		system("chmod 755 " IWCONFIG_BINARY);
		qDebug() << "WifiDataCollector::auditIwlistBinary(): Executed chmod 755 " IWCONFIG_BINARY;
	}
		
	if(!QFileInfo(IWLIST_BINARY).isExecutable())
	{
		//QFile(IWLIST_BINARY).setPermissions(QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther))
		system("chmod 755 " IWLIST_BINARY);
		qDebug() << "WifiDataCollector::auditIwlistBinary(): Executed chmod 755 " IWLIST_BINARY;
	}
	
	return true;
}

/*! \brief Read 'iwconfig' and find the first interface with wireless extensions */
QString WifiDataCollector::findWlanIf()
{
	/// JUST FOR DEVELOPMENT
	
	// Load text file
// 	QString fileName = "iwcfg.txt";
// 	QFile file(fileName);
// 	if(!file.open(QIODevice::ReadOnly))
// 	{
// 		QMessageBox::critical(0,QObject::tr("Can't Read File"),QString(QObject::tr("Unable to open %1")).arg(fileName));
// 		return "";
// 	}
// 
// 	QTextStream stream(&file);
// 	QString fileContents = stream.readAll();

	QProcess proc;
	proc.start(IWCONFIG_BINARY);
	if(!proc.waitForFinished(10000))
	{
		qDebug() << "WifiDataCollector::findWlanIf(): Error: Timeout while waiting for " IWCONFIG_BINARY " to finish.";
		return "";
	}
	
	QString fileContents = proc.readAllStandardOutput();
	qDebug() << "WifiDataCollector::findWlanIf(): Raw output of " IWCONFIG_BINARY ": "<<fileContents;
	
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
	
	QProcess proc;
	proc.start(IWLIST_BINARY, QStringList() << interface << "scan");
	if(!proc.waitForFinished(10000))
	{
		qDebug() << "WifiDataCollector::getIwlistOutput(): Error: Timeout while waiting for " IWLIST_BINARY " to finish.";
		return "";
	}
	
	QString fileContents = proc.readAllStandardOutput();
	qDebug() << "WifiDataCollector::getIwlistOutput(): Raw output of " IWLIST_BINARY ": "<<fileContents;
	
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
