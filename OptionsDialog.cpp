#include "OptionsDialog.h"
#include "ui_OptionsDialog.h"
#include "MapGraphicsScene.h"


bool OptionsDialog_sort_byEssid(MapApInfo *a, MapApInfo *b)
{
	if(!a || !b) return false;

	return a->essid.toLower() < b->essid.toLower();

}

OptionsDialog::OptionsDialog(MapGraphicsScene *ms, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::OptionsDialog)
	, m_scene(ms)
{
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(applySettings()));
	
	// Setup wifi devices list
	QString dev = ms->currentDevice();
	QStringList devices = WifiDataCollector::findWlanInterfaces();
	
	m_deviceList = devices;
	
	// Disable on Android for now ONLY because the Qt filebrowser on Android is, well, crap.
	// When we get a decent filebrowser for Android, reenable this option on Android
	#ifndef Q_OS_ANDROID
	devices << "(Read from file...)";
	#endif
	
	ui->wifiDevice->addItems(devices);
	
	int curIdx = devices.indexOf(dev);
	if(curIdx < 0)
	{
		#ifndef Q_OS_ANDROID
		// assume its a file
		ui->currentDeviceLabel->setText(dev);
		m_filename = dev;
		curIdx = devices.size() - 1;
		#else
		curIdx = 0;
		#endif
	}
	
	ui->wifiDevice->setCurrentIndex(curIdx);
	
	#ifndef Q_OS_ANDROID
	// TODO confirm signal name
	connect(ui->wifiDevice, SIGNAL(activated(int)), this, SLOT(wifiDeviceIdxChanged(int)));
	#endif
	
	
	ui->autoLocateAPs->setChecked(ms->autoGuessApLocations());
	ui->guessMyLocation->setChecked(ms->showMyLocation());
	
	MapRenderOptions renderOpts = ms->renderOpts();
	ui->showReadingMarkers->setChecked(renderOpts.showReadingMarkers);
	
	QStringList renderOptions = QStringList()
		<< "(None)"
		<< "Radial Lines"
		<< "Circles"
		<< "Rectangle";
	
	MapGraphicsScene::RenderMode rm = ms->renderMode();
	
	ui->renderType->addItems(renderOptions);
	ui->renderType->setCurrentIndex((int)rm);
	
	if(rm == MapGraphicsScene::RenderRadial)
	{
		ui->renderRadialOpts->setVisible(true);
		ui->renderCircleOpts->setVisible(false);
	}
	else
	if(rm == MapGraphicsScene::RenderCircles)
	{
		ui->renderRadialOpts->setVisible(false);
		ui->renderCircleOpts->setVisible(true);
	}
	else
	{
		ui->renderRadialOpts->setVisible(false);
		ui->renderCircleOpts->setVisible(false);
	}
	
	connect(ui->renderType, SIGNAL(currentIndexChanged(int)), this, SLOT(renderModeChanged(int)));
	
	ui->multipleCircles->setChecked(renderOpts.multipleCircles);
	ui->fillCircles->setChecked(renderOpts.fillCircles);
	
	ui->circleSteps->setValue(renderOpts.radialCircleSteps);
	ui->levelSteps->setValue(renderOpts.radialLevelSteps);
	ui->angelDiff->setValue(renderOpts.radialAngleDiff);
	ui->levelDiff->setValue(renderOpts.radialLevelDiff);
	ui->lineWeight->setValue(renderOpts.radialLineWeight);
	
	
	QList<MapApInfo*> apList = ms->apInfo();
	
	QVBoxLayout *vbox = new QVBoxLayout(ui->apListBox);

	QPushButton *btn;
	QHBoxLayout *hbox = new QHBoxLayout();
	btn = new QPushButton("Check All");
	connect(btn, SIGNAL(clicked()), this, SLOT(selectAllAp()));
	hbox->addWidget(btn);

	btn = new QPushButton("Clear All");
	connect(btn, SIGNAL(clicked()), this, SLOT(clearAllAp()));
	hbox->addWidget(btn);

	vbox->addLayout(hbox);
	
	qSort(apList.begin(), apList.end(), OptionsDialog_sort_byEssid);

	foreach(MapApInfo *info, apList)
	{
		QCheckBox *cb = new QCheckBox(QString("%1 (%2)").arg(info->essid).arg(info->mac));
		
		cb->setChecked(info->renderOnMap);

		QPalette p = cb->palette();
		p.setColor(QPalette::WindowText, m_scene->baseColorForAp(info->mac).darker());
		cb->setPalette(p);
		
		m_apCheckboxes[info->mac] = cb;
		
		vbox->addWidget(cb);
	}

	setWindowTitle("Options");
	setWindowIcon(QPixmap(":/data/images/icon.png"));
}

void OptionsDialog::clearAllAp()
{
	foreach(QCheckBox *cb, m_apCheckboxes)
		cb->setChecked(false);
}

void OptionsDialog::selectAllAp()
{
	foreach(QCheckBox *cb, m_apCheckboxes)
		cb->setChecked(true);
}

void OptionsDialog::renderModeChanged(int mode)
{
	MapGraphicsScene::RenderMode rm = (MapGraphicsScene::RenderMode)mode;
	
	if(rm == MapGraphicsScene::RenderRadial)
	{
		ui->renderRadialOpts->setVisible(true);
		ui->renderCircleOpts->setVisible(false);
	}
	else
	if(rm == MapGraphicsScene::RenderCircles)
	{
		ui->renderRadialOpts->setVisible(false);
		ui->renderCircleOpts->setVisible(true);
	}
	else
	{
		ui->renderRadialOpts->setVisible(false);
		ui->renderCircleOpts->setVisible(false);
	}
}

void OptionsDialog::applySettings()
{
	bool oldFlag = m_scene->pauseRenderUpdates(true);
	
	foreach(QString mac, m_apCheckboxes.keys())
		m_scene->setRenderAp(mac, m_apCheckboxes[mac]->isChecked());

	//qDebug() << "OptionsDialog::applySettings()";
	int curIdx = ui->wifiDevice->currentIndex();
	QStringList devices = WifiDataCollector::findWlanInterfaces();
	if(curIdx > devices.size() - 1)
		m_scene->setDevice(m_filename);
	else
		m_scene->setDevice(devices[curIdx]);
	
	m_scene->setAutoGuessApLocations(ui->autoLocateAPs->isChecked());
	m_scene->setShowMyLocation(ui->guessMyLocation->isChecked());
	
	MapRenderOptions renderOpts = m_scene->renderOpts();
	renderOpts.showReadingMarkers = ui->showReadingMarkers->isChecked();
	
	m_scene->setRenderMode((MapGraphicsScene::RenderMode)ui->renderType->currentIndex());
	
	renderOpts.multipleCircles	= ui->multipleCircles->isChecked();
	renderOpts.fillCircles		= ui->fillCircles->isChecked();
	
	renderOpts.radialCircleSteps	= ui->circleSteps->value();
	renderOpts.radialLevelSteps	= ui->levelSteps->value();
	renderOpts.radialAngleDiff	= ui->angelDiff->value();
	renderOpts.radialLevelDiff	= ui->levelDiff->value();
	renderOpts.radialLineWeight	= ui->lineWeight->value();
	
	m_scene->setRenderOpts(renderOpts);

	m_scene->pauseRenderUpdates(oldFlag);
	
}

void OptionsDialog::wifiDeviceIdxChanged(int idx)
{
	if(idx < m_deviceList.size())
		return;
	
	QString curFile = m_filename;
	if(curFile.trimmed().isEmpty())
		curFile = QSettings("wifisigmap").value("last-dev-file","").toString();

	QString fileName = QFileDialog::getOpenFileName(this, tr("Open Results"), curFile, tr("Text File (*.txt);;Any File (*.*)"));
	if(fileName != "")
	{
		QSettings("wifisigmap").setValue("last-dev-file",fileName);
		
		ui->currentDeviceLabel->setText(fileName);
		m_filename = fileName;
	}
}

OptionsDialog::~OptionsDialog()
{
	delete ui;
}
	
void OptionsDialog::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type()) 
	{
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		default:
			break;
	}
}
