#include "MainWindow.h"

#include "MapWindow.h"

#include <QApplication>
#include <QDesktopWidget>

MainWindow::MainWindow()
	: QWidget()
	, m_map(0)
{
	m_map = new MapWindow(this);
	m_map->setGeometry(QApplication::desktop()->screenGeometry());

//    m_layoutBase = this;
	setStyleSheet("background-color:rgb(50,50,50); color:#FFFFFF");

}

MainWindow::~MainWindow()
{
}

void MainWindow::resizeEvent(QResizeEvent*)
{
	//qDebug() << "Window Size: "<<width()<<" x "<<height();
	QRect rect = QApplication::desktop()->screenGeometry();
	m_map->setGeometry(rect);
}
