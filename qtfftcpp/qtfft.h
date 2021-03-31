#ifndef QTFFT_H
#define QTFFT_H

#include <QWidget>

namespace QtCharts {
class QChartView;
class QLineSeries;
}

class QSerialPortInfo;
class QSerialPort;
class QComboBox;
class QPushButton;
class QDataStream;

class QtFFT : public QWidget
{
    Q_OBJECT

    enum State {
        Idle,
        Setup,
        Data,
    };

    State state = Idle;

    QtCharts::QChartView *chartView;
    QtCharts::QLineSeries *line;
    QComboBox *portSelect;
    QList<QSerialPortInfo> ports;
    QSerialPort *port;
    QPushButton *connectButton;
    QByteArray buffer;
    QVector<QPointF> points;
    double maxY;

    void fetchAvailablePorts();
    void connectToPort();
    void readData();

    void parseData();
    void parseSetup();

public:
    QtFFT(QWidget *parent = nullptr);
    ~QtFFT();

};
#endif // QTFFT_H
