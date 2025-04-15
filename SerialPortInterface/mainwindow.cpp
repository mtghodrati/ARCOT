#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QColor>

static QSerialPort::BaudRate brate[8]={QSerialPort::Baud1200,QSerialPort::Baud2400,QSerialPort::Baud4800,
                                       QSerialPort::Baud9600,QSerialPort::Baud19200,QSerialPort::Baud38400
                                       ,QSerialPort::Baud57600,QSerialPort::Baud115200};

static QSerialPort::DataBits dbits[4]={QSerialPort::Data5,QSerialPort::Data6,QSerialPort::Data7
                                       ,QSerialPort::Data8};

static QSerialPort::StopBits sbits[3]={QSerialPort::OneStop,QSerialPort::OneAndHalfStop,QSerialPort::TwoStop};

static QSerialPort::Parity pbits[3]={QSerialPort::NoParity,QSerialPort::EvenParity,QSerialPort::OddParity};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(port,SIGNAL(readyRead()),this,SLOT(serial_data_received()));
    on_refreshbutton_clicked();
    file = new QFile();
    load_settings();
    qDebug()<<"working path = "<<setting_currpath;
    QString tmpPath = setting_currpath + "/";
    QString tmpFileName = "untitled.txt";
    int i = 1;
    int inddot = tmpFileName.indexOf(".txt");
    while(QFile::exists(tmpPath + tmpFileName))
    {
        tmpFileName = tmpFileName.mid(0,inddot) + "_" + QString::number(i) + ".txt";
        i++;
    }
    path = tmpPath;
    filename = tmpPath + tmpFileName;
    qDebug()<<"tmpPath = " << tmpPath;
    ui->lineEdit_filePath->setText(filename);
    //    QString str = "num=189, num = 156, num= 254, x=1;";
    //    QRegExp regexp("num ?= ?\\d+");
    //    QRegExp numrx("\\d+");
    //    int pos = 0;
    //    QStringList nums;
    //    QString tmp = "";
    //    int tmpind = 0;
    //    while((pos = regexp.indexIn(str,pos)) > -1)
    //    {
    //        tmp = regexp.cap(0);
    //        tmpind = numrx.indexIn(tmp,0);
    //        qDebug()<<numrx.cap(0);
    //        qDebug()<<regexp.cap(1);
    //        pos += regexp.matchedLength();
    //    }
}

MainWindow::~MainWindow()
{
    delete ui;
    if(port->isOpen())
    {
        port->close();
    }
    if(file->isOpen())
    {
        file->close();
    }
}

void MainWindow::on_refreshbutton_clicked()
{
    ui->textBrowser->clear();
    ui->serialComboBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        if((info.portName() == port_name) || (!info.isBusy() && info.isValid()))
        {
            ui->textBrowser->append("Name        : " + info.portName());
            ui->textBrowser->append("Description : " + info.description());
            ui->textBrowser->append("Manufacturer: " + info.manufacturer());
            ui->serialComboBox->addItem(info.portName());
        }
    }
    //    QString str = "a,b,c,";
    //    QStringList strlist = str.split(',');
    //    qDebug()<<"strlist.length = "<<strlist.length();
    //    foreach(QString tmpstr, strlist)
    //    {
    //        qDebug()<<tmpstr;
    //    }
    //    qDebug()<<"end of stringlist"<<strlist.last().isEmpty();
    //    qDebug()<<
}

void MainWindow::serial_data_received()
{
    isConnected=true;
    QByteArray sdata;
    if(port->isReadable())
    {
        QString str;
        while(!port->atEnd())
        {
            //            sdata.append(port->readLine());
            sdata.append(port->readAll());
            process_incoming_messages(sdata);
        }
        backupArray.append(sdata);
        nBytesToBackup += sdata.length();
        if(nBytesToBackup >= sizeLimit)
        {
            saveTempFile();
        }
    }
    else
    {
        qDebug()<<"couldn't read received data.";
    }
}

void MainWindow::on_connect_pushButton_clicked()
{
    if(!isConnected)
    {
        while(!isSaveFileDetermined)
        {
            on_pushButton_clicked();
        }
        port->setPortName(ui->serialComboBox->currentText());
        port->setBaudRate(brate[ui->comboBox_baudrate->currentIndex()]);
        port->setDataBits(dbits[ui->comboBox_databits->currentIndex()]);
        port->setStopBits(sbits[ui->comboBox_stopbits->currentIndex()]);
        port->setParity(pbits[ui->comboBox_paritybits->currentIndex()]);
        if(!port->isOpen())
        {
            if(port->open(QIODevice::ReadWrite))
            {
                isConnected = true;
                ui->tabWidget->setCurrentIndex(1);
                ui->connect_pushButton->setText("Disconnect");
                ui->checkBox->setEnabled(false);
                ui->lineEdit_pattern->setEnabled(false);
                if(hasStartPattern && ui->lineEdit_pattern->text().isEmpty())
                {
                    spattern = ui->lineEdit_pattern->text();
                    isSaving = false;
                }
                else
                {
                    hasStartPattern = false;
                    isSaving = true;
                }
                ui->textEdit->clear();
            }
            else
            {
                QMessageBox::critical(this,"COM port error...","System can not open the specified port ("+
                                      ui->serialComboBox->currentText()+").");
            }
        }
    }
    else
    {
        port->close();
        isSaveFileDetermined = false;
        QDir tempDir(tempFolderPath);
        tempDir.removeRecursively();
        currentTempFileName.clear();
        tempFilePaths.clear();
        backupArray.clear();
        ui->connect_pushButton->setText("Connect");
        isConnected = false;
        isSaving = false;
        file->close();
        ui->checkBox->setEnabled(true);
        ui->lineEdit_pattern->setEnabled(true);
        qDebug()<<"filename = "<<filename;
        int ind;
        int ind_start = filename.lastIndexOf("/")+1;
        QString tmpPath = filename.left(ind_start);
        QString tmpfilename = filename.mid(ind_start);
        qDebug()<<"ind_start = "<<ind_start;
        int ind_end = tmpfilename.lastIndexOf(".");
        qDebug()<<"ind_end = "<<ind_end;
        ind = tmpfilename.lastIndexOf('_');
        qDebug()<<"ind_underline = "<<ind<<", (i<ind_end && i>ind_start) = "<<(ind<ind_end & ind>ind_start);
        if(ind<ind_end && ind>-1)
        {
            ind_end = ind;
            qDebug()<<"ind_end updated to i"<<ind;
        }
        QString filename_log = tmpPath+"/" + "log_" + tmpfilename;
        qDebug()<<"log file name: "<<filename_log;
        QFile logFile(filename_log);
        int indlog = 1;
        if(logFile.exists())
        {
            filename_log = tmpPath + "/" + "log" + QString::number(indlog) + "_" + tmpfilename;
            indlog++;
            logFile.setFileName(filename_log);
        }
        if(logFile.open(QIODevice::WriteOnly|QIODevice::Text))
        {
            logFile.write(QString("This file is the log file of the following file:\n" + filename+"\n---------\n").toLatin1());
            logFile.write(ui->textEdit->toPlainText().toLatin1());
            logFile.close();
        }
        else {
            QMessageBox::critical(this,"Log File Save Error...","Could not save the log file. Please copy the "
                                                                "Serial port monitor content before starting a new connection. Otherwise the content"
                                                                "will be lost permanently.");
        }
        qDebug()<<"final ind_end = "<<ind_end;
        ind = 1;
        QString fname = tmpfilename.mid(0,ind_end);// filename.mid(ind_start, ind_end - ind_start);
        qDebug()<<"fname = "<<fname;
        qDebug()<<"tmpPath = "<<tmpPath;
        QString tmp = tmpPath + fname + "_" + QString::number(ind) + ".txt";
        file->setFileName(tmp);
        while(file->exists())
        {
            ind++;
            tmp = tmpPath + fname + "_" + QString::number(ind) + ".txt";
            file->setFileName(tmp);
        }
        filename = tmp;
    }
    if(isConnected)
    {
        recordingDate = QDate::currentDate();
        recordingTime = QTime::currentTime();
        file->setFileName(filename);
        if(file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
        {
            isFileOpen = true;
            QString tmpStr = "Date=" + recordingDate.toString("yyyy.MM.dd") + "; Time=" +
                    recordingTime.toString("hh:mm:ss") + "; Rat=" + ui->lineEdit_Rat->text() +
                    "; Task=" + ui->lineEdit_Task->text() + ";\n";
            file->write(tmpStr.toLatin1());
        }
        //        setting_currpath = QDir::currentPath();
        setting_pattern = (ui->checkBox->isChecked()) ? ui->lineEdit_pattern->text():"";
        setting_numpattern = ui->lineEdit_countindicator->text();
        save_settings();
    }
}

void MainWindow::on_pushButton_clicked()
{
//     tmpPath;
    QString pname;
    int slash1,slash2;
    slash1=0;
    slash2=0;
    QFileDialog fileDialog;
    QString tmpPath = fileDialog.getSaveFileName(this, "Select Program", setting_currpath, "Text (*.txt)");
    if(!tmpPath.isEmpty())
    {
        isSaveFileDetermined = true;
    }
    ui->lineEdit_filePath->setText(tmpPath);
//    qDebug()<<"file path is: "<<tmpPath;
    path = tmpPath.mid(0,tmpPath.lastIndexOf('/')+1);
    QString fname = tmpPath.mid(tmpPath.lastIndexOf('/')+1,tmpPath.lastIndexOf(".")-1-tmpPath.lastIndexOf('/'));
    tempFolderPath = path + "backupFolder " + fname;
    QDir tempDir = QDir(tempFolderPath);
    int i = 1;
    while(tempDir.exists())
    {
        tempFolderPath = path + "backupFolder" + QString::number(i) + " " + fname;
        tempDir.setPath(tempFolderPath);
        i++;
    }
    tempDir.mkdir(tempFolderPath);
    currentTempFileName = "TMP1.txt";
    tempFilePaths.append(tempFolderPath+"/TMP1.txt");
    qDebug()<<"backup file name: "<<tempFolderPath+"/"+currentTempFileName;
    backupFile = new QFile(tempFolderPath+"/"+currentTempFileName);
    setting_currpath = path;
    //    QDir::setCurrent(path);
    qDebug()<<"path changed to: "<<QDir::currentPath();
    //    setting_currpath = path;
    filename = tmpPath;
    file->setFileName(tmpPath);
    if(!tmpPath.isEmpty()&&(!tmpPath.isNull()))
    {
        //        qDebug()<<"Reading File.";
        while((slash2!=-1))
        {
            slash1=tmpPath.indexOf("/",slash1+1);
            slash2=tmpPath.indexOf("/",slash1+1);
        }
        //        programName = path.mid(slash1+1);
        //        update_program_name();
        //        read_file(path);
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    ui->textEdit->clear();
}

void MainWindow::on_pushButton_2_clicked()
{
    if(isConnected)
    {
        if(port->isOpen())
        {
            port->write(ui->lineEdit->text().toLatin1());
            ui->textEdit->setTextColor(Qt::blue);
            ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy/MM/dd, hh:mm:ss - ") + ui->lineEdit->text());
            ui->textEdit->setTextColor(Qt::black);
        }
    }
}

void MainWindow::on_checkBox_toggled(bool checked)
{
    hasStartPattern = checked;
    ui->lineEdit_pattern->setEnabled(checked);
    isSaving = !checked;
}

void MainWindow::process_incoming_messages(QString str)
{
    bool readyToPrint = false;
    QStringList stlist;
    str.remove("\r");
    str.remove("\n");
    if(isHalfLine)
    {
        if(!str.contains(setting_EOLstring))
        {
            halfLine.append(str);
            if(halfLine.contains(setting_EOLstring))
            {
                process_incoming_messages(halfLine);
            }
        }
        else
        {
            stlist = str.split(setting_EOLstring);
            halfLine.append(stlist.at(0));
            ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy/MM/dd, hh:mm:ss - ") + halfLine);

            isHalfLine = false;
            stlist.removeFirst();
            miniprocess(stlist);
        }
    }
    else
    {
        stlist = str.split(setting_EOLstring);
        miniprocess(stlist);
    }
    //    str = QString::fromStdString(sdata.toStdString());
    //    if(str.contains(setting_EOLstring))
    //    {
    //        QStringList strlist = str.split(setting_EOLstring);
    //        if(strlist.length()>1)
    //        {
    //            for(int i=0;i<strlist.length()-1;i++)
    //            {
    //                if(i==0 && isHalfLine)
    //                {
    //                    ui->textEdit->append(halfLine + )
    //                }
    //                ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy/MM/dd, hh:mm:ss - ") + strlist.at(i));
    //            }
    //            if(!strlist.last().isEmpty())
    //            {
    //                isHalfLine = true;
    //                halfLine = strlist.last();
    //            }
    //        }
    //        else
    //        {

    //        }
    //        if(str.indexOf(setting_EOLstring)<str.length()-2)
    //        {
    //            int nlind = str.indexOf(setting_EOLstring);

    //        }
    //        else
    //        {
    //            str.prepend(halfLine);
    //            isHalfLine = false;
    //            halfLine.clear();
    //            readyToPrint = true;
    //        }
    //    }
    //    else
    //    {
    //        halfLine.append(str);
    //        readyToPrint = false;
    //        isHalfLine = true;
    //    }
    //    if(str.contains("num="))
    //    {
    //        int num = str.mid(str.indexOf("num=")+4).toInt();
    //        ui->label_num->setText(QString::number(num));

    //    }
    //    ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy/MM/dd, hh:mm:ss - ") + str);

}

void MainWindow::process_incoming_messages(QByteArray bytearr)
{
    process_incoming_messages(QString::fromStdString(bytearr.toStdString()));
    if(file->isOpen())
    {
        if(isSaving)
        {
            file->write(bytearr);
        }
        else if(spattern.compare(QString::fromStdString(bytearr.toStdString())) == 0)
        {
            file->write(bytearr);
            isSaving = true;
        }
    }
}

void MainWindow::miniprocess(QStringList sl)
{
    for(int i=0;i<sl.length()-1;i++)
    {
        ui->textEdit->append(QDateTime::currentDateTime().toString("yyyy/MM/dd, hh:mm:ss - ") + sl.at(i));
        number_display(sl.at(i));
    }
    if(sl.last().isEmpty())
    {
        isHalfLine = false;
        halfLine.clear();
    }
    else
    {
        isHalfLine = true;
        halfLine = sl.last();
    }
}

void MainWindow::number_display(QString str)
{
    QRegExp regexp(pattern);
    QRegExp numrx("\\d+");
    int pos = 0;
    QStringList nums;
    QString tmp = "";
    int tmpind = 0;
    while((pos = regexp.indexIn(str,pos)) > -1)
    {
        tmp = regexp.cap(0);
        tmpind = numrx.indexIn(tmp,0);
        nums<<numrx.cap(0);
        //        qDebug()<<regexp.cap(1);
        pos += regexp.matchedLength();
    }

    if(str.contains("num="))
    {
        //        int num = str.mid(str.indexOf("num=")+4).toInt();
        //        ui->label_num->setText(QString::number(num));
        ui->label_num->setText(nums.last());
    }
}

void MainWindow::load_settings()
{
    QFile settingsFile("settings.txt");
    QString str;
    if(settingsFile.open(QIODevice::ReadOnly|QIODevice::Text))
    {
        qDebug()<<"opened settings file.";
        str = QString::fromStdString(settingsFile.readLine().toStdString());
        qDebug()<<str.remove("\n");
        if(str.endsWith("/"))
        {
            str = str.mid(0,str.length()-1);
        }
        if(QDir(str).exists())
        {
            //            QDir::setCurrent(str);
            setting_currpath = str;
        }
        str = QString::fromStdString(settingsFile.readLine().toStdString());
        qDebug()<<str.remove("\n");
        setting_pattern = str;
        if(str.isEmpty())
        {
            ui->lineEdit_pattern->setEnabled(false);
        }
        else {

            ui->lineEdit_pattern->setText(str);
        }
        str = QString::fromStdString(settingsFile.readLine().toStdString());
        qDebug()<<str;
        ui->lineEdit_countindicator->setText(str);
        setting_numpattern = str;
        //        str = QString::fromStdString(settingsFile.readLine().toStdString());
        //        ui->serialComboBox->setCurrentText(str)
        settingsFile.close();
    }
    else
    {
        qDebug()<<"could not open settings file: "<<settingsFile.errorString();
    }
    if(setting_currpath.isEmpty())
    {
        qDebug()<<"could not open settings file.";
        setting_currpath = QDir::currentPath();
    }
}

void MainWindow::save_settings()
{
    QFile settingsFile("settings.txt");
    QString str = setting_currpath + "\n";
    qDebug()<<"setting: "<<str;
    if(settingsFile.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        settingsFile.write(str.toLatin1());
        str = setting_pattern + "\n";
        qDebug()<<"setting: "<<str;
        settingsFile.write(str.toLatin1());
        str = setting_numpattern + "\n";
        qDebug()<<"setting: "<<str;
        settingsFile.write(setting_numpattern.toLatin1());
        settingsFile.close();
    }
}

void MainWindow::saveTempFile()
{
    if(backupFile->open(QIODevice::Text|QIODevice::WriteOnly))
    {
        QString tmpStr = "Date=" + recordingDate.toString("yyyy.MM.dd") + "; Time=" +
                recordingTime.toString("hh:mm:ss") + "; Rat=" + ui->lineEdit_Rat->text() +
                "; Task=" + ui->lineEdit_Task->text() + "; backup temp file number " +
                QString::number(tempFilePaths.length()) +" to " + file->fileName() + ";\n";
        backupArray.prepend(tmpStr.toLatin1());
        backupFile->write(backupArray);
        backupFile->close();
        backupArray.clear();
        currentTempFileName = "TMP" + QString::number(tempFilePaths.length()+1) +".txt";
        tempFilePaths.append(tempFolderPath + "/" + currentTempFileName);
        backupFile->setFileName(tempFolderPath+"/"+currentTempFileName);
        nBytesToBackup = 0;
    }
}

void MainWindow::on_pushButton_4_clicked()
{
    if(ui->lineEdit_pattern->text().contains("\\d+"))
    {
        pattern = ui->lineEdit_pattern->text();
    }
    else {

        pattern = ui->lineEdit_pattern->text()+"\\d+";
    }
}
