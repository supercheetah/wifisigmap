#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtGui>
#include <QDialog>

namespace Ui {
    class OptionsDialog;
}

class MapGraphicsScene;
class OptionsDialog : public QDialog 
{
	Q_OBJECT
	
public:
	OptionsDialog(MapGraphicsScene *scene, QWidget *parent = 0);
	~OptionsDialog();
	
protected:
	void changeEvent(QEvent *e);
	void showEvent(QShowEvent *);
	
private slots:
	void applySettings();
	void wifiDeviceIdxChanged(int);
	void renderModeChanged(int);
	void selectAllAp();
	void clearAllAp();
	
private:
	Ui::OptionsDialog *ui;
	MapGraphicsScene *m_scene;
	QString m_filename;
	QHash<QString,QCheckBox*> m_apCheckboxes;
	QStringList m_deviceList;
};

#endif // OPTIONSDIALOG_H
