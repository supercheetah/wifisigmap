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

class WifiDataCollector : public QObject 
{
	Q_OBJECT
public:
	WifiDataCollector();
	
	QList<WifiDataResult> scanResults();

	bool auditIwlistBinary();
	
	static QString findWlanIf();
	static QStringList findWlanInterfaces();

	// If empty (default), uses first wlan device found
	void setWlanDevice(QString dev="") {  m_wlanDevice = dev; }
	
	// Set data textfile to load data from a file instead of read it live (mainly for debugging/testing) 
	void setDataTextfile(QString file) { m_dataTextfile = file; }
	QString dataTextfile() { return m_dataTextfile; }

	// Converts dBm in the range of [-100,-40] to a range of [0,1]
	// dBm range used subject to change if needed in later revisions.
	static double dbmToPercent(int dbm);

public slots:
	void startScan(int numScans=3, bool continuous=true);
	void stopScan();
	
signals:
	void scanStarted();
	void scanProgress(double progress);
	void scanFinished(QList<WifiDataResult>);
	
protected slots:
	void scanWifi();

protected:
	QThread m_scanThread;
	
	int m_numScans;
	int m_scanNum; // 0-m_numScans
	bool m_continuousMode;
	QTimer m_scanTimer;
	
	QMutex m_resultsMutex;
	
	QList<WifiDataResult> m_scanResults;
	QList<QList<WifiDataResult> > m_resultBuffer;
	
	QString getIwlistOutput(QString interface = "");
	
	WifiDataResult parseRawBlock(QString buffer);
	
	QString readTextFile(QString file);
	
	QString m_dataTextfile;

	QString m_wlanDevice;
};

#endif
