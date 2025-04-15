#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QDateTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_refreshbutton_clicked();

    void serial_data_received();

    void on_connect_pushButton_clicked();

    void on_pushButton_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_checkBox_toggled(bool checked);

    void on_pushButton_4_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *port = new QSerialPort();
    QString filename = "untitled.txt";
    QString path = "C:\\";
    QString port_name = "";
    QString halfLine = "";
    QString pattern = "num ?= ?\\d+";
    QString setting_currpath = "";
    QString setting_pattern = "";
    QString setting_numpattern = "num ?= ?\\d+";
    QString setting_EOLstring = "$";
    QString currentTempFileName = "";
    QString tempFolderPath = "";
    QStringList tempFilePaths;
    qint32 baudrate = 0;
    bool isConnected = false;
    bool isFileOpen = false;
    bool hasStartPattern = false;
    bool isSaving = true;
    bool isHalfLine = false;
    bool isSaveFileDetermined = false;
    QString spattern = "";
    QFile *file;
    QFile *backupFile;
    QByteArray backupArray;
    int nBytesToBackup = 0;
    int sizeLimit = 1e4;
    QDate recordingDate;
    QTime recordingTime;

    void process_incoming_messages(QString str);
    void process_incoming_messages(QByteArray bytearr);
    void miniprocess(QStringList sl);
    void number_display(QString str);
    void load_settings();
    void save_settings();
    void saveTempFile();
};

#endif // MAINWINDOW_H
