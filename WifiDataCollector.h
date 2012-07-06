#ifndef WifiDataCollector_H
#define WifiDataCollector_H

#include <QtGui>

typedef QHash<QString,QString> QStringHash;

class WifiDataResult
{	
public:
	WifiDataResult()
		: essid("")
		, mac("")
		, dbm(-999)
		, chan(0.0)
		, value(0.0)
		, valid(false)
	{}
	
	QString essid;
	QString mac;
	int dbm;
	double chan;
	double value;
	QStringHash rawData;
	bool valid;
	
	QString toString() const;
	void loadRawData(QStringHash rawData);
};

QDebug operator<<(QDebug dbg, const WifiDataResult &result);

class WifiDataCollector
{
public:
	WifiDataCollector();
	
	QList<WifiDataResult> scanWifi(QString debugTextFile="");
	
	bool auditIwlistBinary();
	QString findWlanIf();

	static double dbmToPercent(int dbm);

protected:
	QString getIwlistOutput(QString interface = "");
	
	WifiDataResult parseRawBlock(QString buffer);
	
	QString readTextFile(QString file);
	
	
};

#endif
