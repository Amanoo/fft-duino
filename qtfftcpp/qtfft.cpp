#include "qtfft.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QChartView>
#include <QLineSeries>
#include <QComboBox>
#include <QSerialPortInfo>
#include <QSerialPort>
#include <cmath>
#include <QPushButton>
#include <QDebug>
#include <QLogValueAxis>
#include <QValueAxis>
#include <QDebug>

void QtFFT::fetchAvailablePorts()
{
    ports = QSerialPortInfo::availablePorts();
    portSelect->clear();

    for(auto port : ports) {
        portSelect->addItem(port.portName());
    }

}

void QtFFT::connectToPort()
{
    if(port->isOpen()) {
        port->close();
        connectButton->setText("Connect");
        return;
    }

    if(portSelect->currentIndex() < 0 || portSelect->currentIndex() >= ports.length())
        return;

    port->setPort(ports[portSelect->currentIndex()]);
    if(port->open(QIODevice::ReadWrite)) {
        connectButton->setText("Disconnect");

        QDataStream stream(port);
        port->write("hoi",3);
    }
}

void QtFFT::readData()
{

    QDataStream stream(port);
    stream.setByteOrder(QDataStream::BigEndian);

    QDataStream out(&buffer, QIODevice::WriteOnly);
    // Sync with reading so quint64 reading and writing are equal byte wise
    out.setByteOrder(QDataStream::LittleEndian);

    while(size_t(port->bytesAvailable()) > sizeof(quint64)) {
        quint64 v;
        stream >> v;

        switch(state) {
        case Idle: {
            switch(v) {
            case 0xFFFFFFFFFFFFFFFF:
                qDebug() << "State changed to setup";
                state = Setup;
                break;
            case 0x000000000000FFFF:
                qDebug() << "State changed to Data";
                state = Data;
                break;
            }
        } break;
        case Setup:
        case Data: {
            if(v == 0x00000000FFFFFFFF) {
                //qDebug() << "0x00000000FFFFFFFF";
                if(state == Setup){
                    parseSetup();
                }
                if(state == Data) {
                    parseData();
                }
                state = Idle;
            } else {
                out << v;
            }
        } break;
        }
    }
}

void QtFFT::parseData()
{
    QDataStream stream(buffer);

    double d;
    stream >> d;

    for(auto it = points.begin();!stream.atEnd() && it != points.end();it++) {
        stream >> d;

        it->setY(d);
        maxY=qMax(maxY,d);
    }

    line->replace(points);
    chartView->chart()->axisY()->setMax(maxY);
}

void QtFFT::parseSetup()
{
    QDataStream stream(buffer);

    double samplingFrequency;
    double sampleSize;

    stream >> samplingFrequency;
    stream >> sampleSize;

    points.resize(sampleSize/2-1);
    qreal abscissa = samplingFrequency/sampleSize;
    for(auto it = points.begin(); it != points.end(); it++) {
        it->setX(abscissa);
        abscissa += samplingFrequency/sampleSize;
    }
    chartView->chart()->axisX()->setMin(samplingFrequency/sampleSize);
    chartView->chart()->axisX()->setMax(samplingFrequency/2);

}

QtFFT::QtFFT(QWidget *parent)
    : QWidget(parent)
{
    // Create global vertical layout
    QVBoxLayout *l = new QVBoxLayout(this);

    // Create top bar horizontal layout
    QHBoxLayout *select = new QHBoxLayout();
    l->addLayout(select);

    // Create the selector
    portSelect = new QComboBox();

    // Create the actual serial port
    port = new QSerialPort(this);
    // Connect the "data available" signal to our readData function so no polling
    connect(port, &QIODevice::readyRead, this, &QtFFT::readData);

    // Initial fetch of available ports
    fetchAvailablePorts();

    // Create a manual refresh button for when the device was not attached before startting
    auto refreshButton = new QPushButton("Refresh");
    // Set the size policy to fixed so the button takes up the least amount of space
    refreshButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // Connect the press to the fetch function (code reuse)
    connect(refreshButton, &QPushButton::clicked, this, &QtFFT::fetchAvailablePorts);

    // Create a connect button
    connectButton = new QPushButton("Connect");
    // Same size policy
    connectButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // Connect the press to the connect function
    connect(connectButton, &QPushButton::clicked, this, &QtFFT::connectToPort);

    // Add all widgets in order, [refresh] [ selector       ] [connect]
    select->addWidget(refreshButton);
    select->addWidget(portSelect);
    select->addWidget(connectButton);

    // Add the chart view
    l->addWidget(chartView = new QtCharts::QChartView());

    // Initialize the points with some stuff
    for(int I = 1; I <= 360; I++) {
        points << QPointF(I, std::sin(I/360.0*2*M_PI));
    }

    // Construct a line series
    line = new QtCharts::QLineSeries();
    // Put the points in the line series
    line->replace(points);
    // Put a label on it
    line->setName("Sine");
    auto vAxis = new QtCharts::QValueAxis();
    auto hAxis = new QtCharts::QLogValueAxis();
    // Construct a chart for it
    auto chart = new QtCharts::QChart();
    chart->setTitle("FFT");
    // Add the series to it
    chart->addSeries(line);
    chart->addAxis(hAxis, Qt::AlignBottom);
    chart->addAxis(vAxis, Qt::AlignLeft);
    line->attachAxis(hAxis);
    line->attachAxis(vAxis);
    // Connect the chart to the view
    chartView->setChart(chart);
}

QtFFT::~QtFFT()
{
}

