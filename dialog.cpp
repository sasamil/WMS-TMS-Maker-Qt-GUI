/***************************************************************************
 *   Copyright (C) 2018 by Саша Миленковић                                 *
 *   sasa.milenkovic.xyz@gmail.com                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *   ( http://www.gnu.org/licenses/gpl-3.0.en.html )                       *
 *									   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
#include <QDir>
#include <QTextStream>
#include <QFileDialog>

#include <QDebug>

#include "dialog.h"
#include "ui_dialog.h"

const int TILESIZE = 512;
double dleft=-180.0, dbottom=-90.0, dright=180.0, dtop=90.0, dhres=.0, dlres=100000.0;

//----------------------------------------------
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    pTilemaker = new QProcess(this);
    pTilemaker->setProcessChannelMode(QProcess::MergedChannels);

    // connect signals
    connect(pTilemaker, SIGNAL(readyReadStandardOutput()),this, SLOT(rightMessage()) );
    connect(pTilemaker, SIGNAL(readyReadStandardError()), this, SLOT(wrongMessage()) );
    connect(pTilemaker, SIGNAL(finished(int)), this, SLOT(on_finish(int)));
}

//----------------------------------------------
Dialog::~Dialog()
{
    if(pTilemaker)
    {
        delete pTilemaker;
        pTilemaker = NULL;
    }

    delete ui;
}

//----------------------------------------------
void Dialog::on_pushExecute_clicked()
{
    QStringList args;

    if(!validateUrl())
    {
        ui->editUrl->setFocus(Qt::ActiveWindowFocusReason);
        return;
    }

    args << "--url" << ui->editUrl->text().simplified();

    if(!validateLayer())
    {
        ui->editLayer->setFocus(Qt::ActiveWindowFocusReason);
        return;
    }

    args << "--layer" << ui->editLayer->text().simplified();

    if(!validateBBOX())
    {
        ui->editBBOX->setFocus(Qt::ActiveWindowFocusReason);
        return;
    }

    args << "--bbox" << ui->editBBOX->text().simplified();

    if(!validateResolution())
    {
        ui->editRes->setFocus(Qt::ActiveWindowFocusReason);
        return;
    }

    args << "--res" << ui->editRes->text().simplified();

    if(!validateSRS())
    {
        ui->editRes->setFocus(Qt::ActiveWindowFocusReason);
        return;
    }

    QString ssrs = ui->editSRS->text().simplified();
    if(!ssrs.isEmpty())
        args << "--crs" << ssrs;

    if(ui->checkVerbose->isChecked())
        args << "--verbose";

    if(ui->checkNoOpt->isChecked())
        args << "--no-opt";

    if(ui->checkSkipdirs->isChecked())
        args << "--skipdirs";

    if(ui->radioBlack->isChecked())
        args << "--background" << "black";

    if(ui->radioModerate->isChecked())
        args << "--excmode" << "1";
    else if(ui->radioStrict->isChecked())
        args << "--excmode" << "0";

    if(ui->spinQuality->value() != 90)
        args << "--quality" << QString::number(ui->spinQuality->value());

    if(ui->spinThreads->value() != 1)
        args << "--threads" << QString::number(ui->spinThreads->value());

    if(ui->groupUBox->isChecked())
    {
        if(!validateAndSaveUpdates())
        {
            if(fileExists("temp.txt"))
            {
                QFile file("temp.txt");
                file.remove();
            }

            return;
        }

        args << "--file" << "temp.txt";
    }

    ui->pushExecute->setEnabled(false);
    ui->pushBreak->setEnabled(true);
    ui->pushBreak->setFocus();
    ui->textProcessOutput->clear();
    ui->textProcessOutput->setStyleSheet("");

    QString command = QDir::currentPath() + QDir::separator() + "tilemaker_wms";

    pTilemaker->start(command, args);
}

//----------------------------------------------
// show right message
void Dialog::rightMessage()
{
    QByteArray strdata = pTilemaker->readAllStandardOutput();
    ui->textProcessOutput->setTextColor(Qt::black);
    ui->textProcessOutput->append(strdata);
}

//----------------------------------------------
// show wrong message
void Dialog::wrongMessage()
{
    QByteArray strdata = pTilemaker->readAllStandardError();
    ui->textProcessOutput->setTextColor(Qt::red);
    ui->textProcessOutput->append(strdata);
}

//----------------------------------------------
// on finish slot
void Dialog::on_finish(int exitcode)
{
    Q_UNUSED(exitcode)

    ui->pushBreak->setEnabled(false);
    ui->pushExecute->setEnabled(true);
    ui->pushExecute->setFocus();

    ui->textProcessOutput->setStyleSheet("background-image: url(:/icons/glonass_f.png);");

    if(fileExists("temp.txt"))
    {
        QFile file("temp.txt");
        file.remove();
    }
}

//----------------------------------------------
// on finish slot
void Dialog::on_pushBreak_clicked()
{
    pTilemaker->kill();
    ui->pushBreak->setEnabled(false);
    ui->pushExecute->setEnabled(true);
    ui->pushExecute->setFocus();
}



//==============================================
// set of private manual functions

//----------------------------------------------
bool Dialog::fileExists(const QString& path)
{
    QFileInfo check_file(path);
    return check_file.exists() && check_file.isFile();
}

//----------------------------------------------
bool Dialog::validateUrl()
{
    QString surl = ui->editUrl->text().simplified();
    if(surl.isEmpty())
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching cannot start if the URL of WMS server is not set.\n\nEnter the URL, please.");
        return false;
    }

    if(surl.contains(" "))
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because of invalid URL (two strings instead of just one).\n\nCorrect the data, please.");
        return false;
    }

    return true;
}

//----------------------------------------------
bool Dialog::validateLayer()
{
    QString slayer = ui->editLayer->text().simplified();
    if(slayer.isEmpty())
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching cannot start if the WMS layer is not set.\n\nEnter the layer, please.");
        return false;
    }

    if(slayer.contains(" "))
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because of invalid 'layer' (two strings instead of just one).\n\nCorrect the data, please.");
        return false;
    }

    return true;
}

//----------------------------------------------
bool Dialog::validateResolution()
{
    bool bretval;

    QString sres = ui->editRes->text().simplified();
    if(sres.isEmpty())
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching cannot start if the resolution is not set.\n\nEnter the resolution, please.");
        return false;
    }

    dhres = sres.toDouble(&bretval);
    if(!bretval)
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because\nthe resolution is not valid.\n\nCorrect the data, please.");

    double hspan = dright - dleft;
    double wspan = dtop - dbottom;
    double minspan = hspan < wspan ? hspan : wspan;

    dlres = minspan * 2.0 / TILESIZE;

    return bretval;
}

//----------------------------------------------
bool Dialog::validateBBOX()
{
    QString sbbox = ui->editBBOX->text().simplified();
    if(sbbox.isEmpty())
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching cannot start if the bounding box (BBOX) is not set.\n\nEnter the BBOX, please.");
        return false;
    }

    QStringList strlist;
    strlist = sbbox.split(",");
    if(strlist.size()!=4)
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The bounding box (BBOX) is not valid.\nIt should have 4 parameters (left, bottom, right, top), but it is not so?!\n\nCorrect the data, please.");
        return false;
    }

    bool bretval;

    dleft = strlist[0].trimmed().toDouble(&bretval);
    if(!bretval)
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because\nthe BBOX.left is not valid.\n\nCorrect the data, please.");
        return false;
    }

    dbottom = strlist[1].trimmed().toDouble(&bretval);
    if(!bretval)
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because\nthe BBOX.bottom is not valid.\n\nCorrect the data, please.");
        return false;
    }

    dright = strlist[2].trimmed().toDouble(&bretval);
    if(!bretval)
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because\nthe BBOX.right is not valid.\n\nCorrect the data, please.");
        return false;
    }

    dtop = strlist[3].trimmed().toDouble(&bretval);
    if(!bretval)
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because\nthe BBOX.top is not valid.\n\nCorrect the data, please.");
        return false;
    }

    if(dleft >= dright)
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because\nBBOX.left should be less than BBOX.right, but it is not so?!\n\nCorrect the data, please.");
        return false;
    }

    if(dbottom >= dtop)
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because\nBBOX.bottom should be less than BBOX.top, but it is not so?!\n\nCorrect the data, please.");
        return false;
    }

    return true;
}

//----------------------------------------------
bool Dialog::validateSRS()
{
    QString ssrs = ui->editSRS->text().simplified();
    if(ssrs.contains(" "))
    {
        QMessageBox::warning(this, "Irregular Input Data",
                             "The caching will not start because of invalid SRS (two strings instead of just one).\n\nCorrect the data, please.");
        return false;
    }

    return true;
}

//----------------------------------------------------------
QString Dialog::validateUpdateRow(int row,
                                  double left, double bottom, double right, double top,
                                  double minres, double maxres)
{
    bool bOK;
    QString srow = QString::number(row);
    QString sleft, sbottom, sright, stop, shres, slres, sfitres;
    double  dleft, dbottom, dright, dtop, dhres, dlres, dfitres;

    QTableWidgetItem* itemleft = ui->tableUpdates->item(row, 0);
    QTableWidgetItem* itembottom = ui->tableUpdates->item(row, 1);
    QTableWidgetItem* itemright = ui->tableUpdates->item(row, 2);
    QTableWidgetItem* itemtop = ui->tableUpdates->item(row, 3);

    QTableWidgetItem* itemhres = ui->tableUpdates->item(row, 4);
    QTableWidgetItem* itemlres = ui->tableUpdates->item(row, 5);
    QTableWidgetItem* itemfitres = ui->tableUpdates->item(row, 6);

    bool bEmptyRegion = (!itemleft || itemleft->text().simplified().isEmpty()) &&
                        (!itembottom || itembottom->text().simplified().isEmpty()) &&
                        (!itemright || itemright->text().simplified().isEmpty()) &&
                        (!itemtop || itemtop->text().simplified().isEmpty());

    bool bEmptyRow =bEmptyRegion &&
                    (!itemhres || itemhres->text().simplified().isEmpty()) &&
                    (!itemlres || itemlres->text().simplified().isEmpty()) &&
                    (!itemfitres || itemfitres->text().simplified().isEmpty());

    if(bEmptyRow)
        return "empty";
    else if(bEmptyRegion)
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->selectRow(row);
        return "Error at UBOX " + srow + ": Not all the mandatory values (left,bottom,right,top) have been set?!";
    }

    // tho following is for - no bEmptyRow and no bEmptyRegion

    //------ left ------------
    if(!itemleft || itemleft->text().simplified().isEmpty())
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,0);
        return "Error at UBOX " + srow + ":  there is no 'left' parameter?!";
    }
    else
    {
        sleft = itemleft->text().simplified();
        dleft = sleft.toDouble(&bOK);
        if(!bOK)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,0);
            return "Error at UBOX " + srow + ":  the 'left' parameter is not valid?!";
        }

        sleft = QString::number(dleft,'g',8); // thus, sleft may be a bit more formatted
    }

    //------ bottom ------------
    if(!itembottom || itembottom->text().simplified().isEmpty())
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,1);
        return "Error at UBOX " + srow + ":  there is no 'bottom' parameter?!";
    }
    else
    {
        sbottom = itembottom->text().simplified();
        dbottom = sbottom.toDouble(&bOK);
        if(!bOK)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,1);
            return "Error at UBOX " + srow + ":  the 'bottom' parameter is not valid?!";
        }

        sbottom = QString::number(dbottom,'g',8); // thus, sbottom may be a bit more formatted
    }

    //------ right ------------
    if(!itemright || itemright->text().simplified().isEmpty())
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,2);
        return "Error at UBOX " + srow + ":  there is no 'right' parameter?!";
    }
    else
    {
        sright = itemright->text().simplified();
        dright = sright.toDouble(&bOK);
        if(!bOK)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,2);
            return "Error at UBOX " + srow + ":  the 'right' parameter is not valid?!";
        }

        sright = QString::number(dright,'g',8); // thus, sright may be a bit more formatted
    }

    //------ top ------------
    if(!itemtop || itemtop->text().simplified().isEmpty())
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,3);
        return "Error at UBOX " + srow + ":  there is no 'top' parameter?!";
    }
    else
    {
        stop = itemtop->text().simplified();
        dtop = stop.toDouble(&bOK);
        if(!bOK)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,3);
            return "Error at UBOX " + srow + ":  the 'top' parameter is not valid?!";
        }

        stop = QString::number(dtop,'g',8); // thus, stop may be a bit more formatted
    }

    //------ logical inconsistencies ------------
    if(dleft >= dright)
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,2);
        return "Error at UBOX " + srow + ":  the 'left' parameter is bigger than the 'right' parameter?!";
    }
    if(dbottom >= dtop)
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,3);
        return "Error at UBOX " + srow + ":  the 'bottom' parameter is bigger than the 'top' parameter?!";
    }

    if(dleft < left)
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,0);
        return "Error at UBOX " + srow + ":  the 'left' parameter is less than the BBOX.left parameter?!";
    }
    if(dbottom < bottom)
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,1);
        return "Error at UBOX " + srow + ":  the 'bottom' parameter is less than the BBOX.bottom parameter?!";
    }
    if(dright > right)
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,2);
        return "Error at UBOX " + srow + ":  the 'right' parameter is bigger than the BBOX.right parameter?!";
    }
    if(dtop > top)
    {
        ui->tableUpdates->setFocus();
        ui->tableUpdates->setCurrentCell(row,3);
        return "Error at UBOX " + srow + ":  the 'top' parameter is bigger than the BBOX.top parameter?!";
    }

    // resolutions

    bool bhres = itemhres && !itemhres->text().simplified().isEmpty();
    if(bhres)
    {
        shres = itemhres->text().simplified();
        dhres = shres.toDouble(&bOK);
        if(!bOK)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,4);
            return "Error at UBOX " + srow + ":  the 'high resolution' parameter is not valid?!";
        }

        if(dhres < minres)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,4);
            return "Error at UBOX " + srow + ":  the 'high resolution' parameter is better than general resolution?!";
        }

        if(dhres > maxres)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,4);
            return "Error at UBOX " + srow + ":  the 'high resolution' parameter is too low?!";
        }

        shres = QString::number(dhres); // thus, shres may be a bit more formatted
    }
    else
        shres = "";

    bool blres = itemlres && !itemlres->text().simplified().isEmpty();
    if(blres)
    {
        if(!bhres)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,4);
            return "Error at UBOX " + srow + ":  there is 'low resolution' but there is no 'high resolution'?!";
        }

        slres = itemlres->text().simplified();
        dlres = slres.toDouble(&bOK);
        if(!bOK)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,5);
            return "Error at UBOX " + srow + ":  the 'low resolution' parameter is not valid?!";
        }

        if(dlres < dhres)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,5);
            return "Error at UBOX " + srow + ":  the 'low resolution' is better than 'the high' resolution?!";
        }

        if(dlres > maxres)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,5);
            return "Error at UBOX " + srow + ":  the 'low resolution' parameter is too low?!";
        }

        slres = QString::number(dlres,'g',8); // thus, slres may be a bit more formatted
    }
    else
        slres = "";

    bool bfitres = itemfitres && !itemfitres->text().simplified().isEmpty();
    if(bfitres)
    {
        if(!blres)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,5);
            return "Error at UBOX " + srow + ":  there is 'fitting resolution' but there is no 'low resolution'?!";
        }

        sfitres = itemfitres->text().simplified();
        dfitres = sfitres.toDouble(&bOK);
        if(!bOK)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,6);
            return "Error at UBOX " + srow + ":  the 'fitting resolution' parameter is not valid?!";
        }

        if(dfitres < dhres || dfitres > dlres)
        {
            ui->tableUpdates->setFocus();
            ui->tableUpdates->setCurrentCell(row,6);
            return "Error at UBOX " + srow + ":  the 'fitting resolution' parameter is not between 'high resolution' and 'low resolution'?!";
        }

        sfitres = QString::number(dfitres,'g',8); // thus, sleft may be a bit more formatted
    }
    else
        sfitres = "";

    QString sretval = sleft + "," + sbottom + "," + sright + "," + stop;

    if(!shres.isEmpty())
        sretval += "," + shres;

    if(!slres.isEmpty())
        sretval += "," + slres;

    if(!sfitres.isEmpty())
        sretval += "," + sfitres;

    return sretval;
}

//----------------------------------------------
bool Dialog::validateAndSaveUpdates()
{
    QFile outfile("temp.txt");
    if (outfile.open(QIODevice::WriteOnly))
    {
        QTextStream out(&outfile);

        for(int row = 0; row<ui->tableUpdates->rowCount(); row++)
        {
            QString const strrow = validateUpdateRow(row, dleft, dbottom, dright, dtop, dhres, dlres);

            if(strrow == "empty")
                continue;

            if(strrow.startsWith("Error"))
            {
                QMessageBox::warning(this, "Irregular Input Data", strrow);
                return false;
            }

            out << strrow << endl;
        }

        outfile.close();
    }
    else // it should never happen
    {
        QMessageBox::warning(this, "Error", "Cannnot open the the temporary updates-file for writing.");
        return false;
    }

    return true;
}

//----------------------------------------------
void Dialog::on_pushLoadUpdates_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Load Update Regions From a File"),
                                                    QDir::currentPath(),
                                                    tr("Txt file (*.txt);;All files (*.*)"));
    if(filename != QString::null)
    {
        on_pushClearUpdates_clicked(); // necessary !

        QFile inputfile(filename);
        if (inputfile.open(QIODevice::ReadOnly))
        {
           QTextStream in(&inputfile);
           int uboxnumber = 0;
           while (!in.atEnd())
           {
              QString line = in.readLine();
              if(line.simplified().isEmpty())
                  continue;

              QStringList strlist = line.split(",");
              if(strlist.size()<4 || strlist.size()>7)
              {
                  QString msg = "Irregular number of parameters of ubox No. " + QString::number(uboxnumber+1) + ".\n\nData will not be loaded.";
                  QMessageBox::warning(this, "Irregular Input Data", msg);
                  return;
              }

              QTableWidgetItem* itemleft = new QTableWidgetItem(strlist[0].simplified());
              ui->tableUpdates->setItem(uboxnumber, 0, itemleft);

              QTableWidgetItem* itembottom = new QTableWidgetItem(strlist[1].simplified());
              ui->tableUpdates->setItem(uboxnumber, 1, itembottom);

              QTableWidgetItem* itemright = new QTableWidgetItem(strlist[2].simplified());
              ui->tableUpdates->setItem(uboxnumber, 2, itemright);

              QTableWidgetItem* itemtop = new QTableWidgetItem(strlist[3].simplified());
              ui->tableUpdates->setItem(uboxnumber, 3, itemtop);


              if(strlist.size() > 4)
              {
                  QTableWidgetItem* itemhres = new QTableWidgetItem(strlist[4].simplified());
                  ui->tableUpdates->setItem(uboxnumber, 4, itemhres);
              }


              if(strlist.size() > 5)
              {
                  QTableWidgetItem* itemlres = new QTableWidgetItem(strlist[5].simplified());
                  ui->tableUpdates->setItem(uboxnumber, 5, itemlres);
              }

              if(strlist.size() > 6)
              {
                  QTableWidgetItem* itemfitres = new QTableWidgetItem(strlist[6].simplified());
                  ui->tableUpdates->setItem(uboxnumber, 6, itemfitres);
              }

              ++uboxnumber;
           }
           inputfile.close();
        }
        else // it should never happen
            QMessageBox::warning(this, "Error",
                                 "Cannnot open the file for reading.");
    }
}

//----------------------------------------------
void Dialog::on_pushClearUpdates_clicked()
{
    // tricky solution
    ui->tableUpdates->setRowCount(0);
    ui->tableUpdates->setRowCount(20);
}

//----------------------------------------------
void Dialog::on_pushOpen_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    QDir::currentPath(),
                                                    tr("Tilemaker Input Parameters (*.tip);;All files (*.*)"));
    if(filename != QString::null)
    {
        bool bOK;

        QFile inputfile(filename);
        if (inputfile.open(QIODevice::ReadOnly))
        {
           QTextStream in(&inputfile);

           // edits
           QString line = in.readLine();
           if(!line.startsWith("url:")) goto error_report;
           ui->editUrl->setText(line.right(line.length()-4));

           line = in.readLine();
           if(QString::null==line || !line.startsWith("layer:")) goto error_report;
           ui->editLayer->setText(line.right(line.length()-6));

           line = in.readLine();
           if(QString::null==line || !line.startsWith("bbox:")) goto error_report;
           ui->editBBOX->setText(line.right(line.length()-5));

           line = in.readLine();
           if(QString::null==line || !line.startsWith("res:")) goto error_report;
           ui->editRes->setText(line.right(line.length()-4));

           line = in.readLine();
           if(QString::null==line || !line.startsWith("srs:")) goto error_report;
           ui->editSRS->setText(line.right(line.length()-4));

           // spins
           line = in.readLine();
           if(QString::null==line || !line.startsWith("threads:")) goto error_report;
           int ithreads = line.right(line.length()-8).simplified().toInt(&bOK);
           if(!bOK) goto error_report;
           ui->spinThreads->setValue(ithreads);

           line = in.readLine();
           if(QString::null==line || !line.startsWith("quality:")) goto error_report;
           int iquality = line.right(line.length()-8).simplified().toInt(&bOK);
           if(!bOK) goto error_report;
           ui->spinQuality->setValue(iquality);

           // checks
           line = in.readLine();
           if(QString::null==line || !line.startsWith("noopt:")) goto error_report;
           int inoopt = line.right(line.length()-6).simplified().toInt(&bOK);
           if(!bOK) goto error_report;
           ui->checkNoOpt->setChecked(inoopt != 0);

           line = in.readLine();
           if(QString::null==line || !line.startsWith("skipdirs:")) goto error_report;
           int iskipdirs = line.right(line.length()-9).simplified().toInt(&bOK);
           if(!bOK) goto error_report;
           ui->checkSkipdirs->setChecked(iskipdirs != 0);

           line = in.readLine();
           if(QString::null==line || !line.startsWith("verbose:")) goto error_report;
           int iverbose = line.right(line.length()-8).simplified().toInt(&bOK);
           if(!bOK) goto error_report;
           ui->checkVerbose->setChecked(iverbose != 0);

           line = in.readLine();
           if(QString::null==line || !line.startsWith("background:")) goto error_report;
           QString sbgr = line.right(line.length()-11).simplified();
           if("white" == sbgr)
               ui->radioWhite->setChecked(true);
           else if("black" == sbgr)
               ui->radioBlack->setChecked(true);
           else if("transparent" == sbgr)
               ui->radioTransparent->setChecked(true);
           else
               goto error_report;

           line = in.readLine();
           if(QString::null==line || !line.startsWith("exceptions:")) goto error_report;
           QString sexc = line.right(line.length()-11).simplified();
           if("tolerant" == sexc)
               ui->radioTolerant->setChecked(true);
           else if("moderate" == sexc)
               ui->radioModerate->setChecked(true);
           else if("strict" == sexc)
               ui->radioStrict->setChecked(true);
           else
               goto error_report;

           line = in.readLine();
           if(QString::null==line || !line.startsWith("format:")) goto error_report;
           QString sfmt = line.right(line.length()-7).simplified();
           if("jpeg" == sfmt)
               ui->radioJpeg->setChecked(true);
           else if("png" == sfmt)
               ui->radioPng->setChecked(true);
           else if("gif" == sfmt)
               ui->radioGIF->setChecked(true);
           else
               goto error_report;

           // updates
           line = in.readLine();
           if(QString::null==line || !line.startsWith("updates:")) goto error_report;
           int iupdates = line.right(line.length()-8).simplified().toInt(&bOK);
           if(!bOK) goto error_report;
           ui->groupUBox->setChecked(iupdates != 0);

           on_pushClearUpdates_clicked(); // clear the existing updates on the update-table

           int uboxnumber = 0;
           while (!in.atEnd())
           {
              line = in.readLine();
              if(line.simplified().isEmpty())
                  continue;

              QStringList strlist = line.split(",");
              if(strlist.size()<4 || strlist.size()>7) goto error_report;

              QString str = strlist[0].simplified();
              if(!str.isEmpty())
              {
                  QTableWidgetItem* itemleft = new QTableWidgetItem(str);
                  ui->tableUpdates->setItem(uboxnumber, 0, itemleft);
              }

              str = strlist[1].simplified();
              if(!str.isEmpty())
              {
                  QTableWidgetItem* itembottom = new QTableWidgetItem(str);
                  ui->tableUpdates->setItem(uboxnumber, 1, itembottom);
              }

              str = strlist[2].simplified();
              if(!str.isEmpty())
              {
                  QTableWidgetItem* itemright = new QTableWidgetItem(str);
                  ui->tableUpdates->setItem(uboxnumber, 2, itemright);
              }

              str = strlist[3].simplified();
              if(!str.isEmpty())
              {
                  QTableWidgetItem* itemtop = new QTableWidgetItem(str);
                  ui->tableUpdates->setItem(uboxnumber, 3, itemtop);
              }


              if(strlist.size() > 4)
              {
                  str = strlist[4].simplified();
                  if(!str.isEmpty())
                  {
                      QTableWidgetItem* itemhres = new QTableWidgetItem(str);
                      ui->tableUpdates->setItem(uboxnumber, 4, itemhres);
                  }
              }


              if(strlist.size() > 5)
              {
                  str = strlist[5].simplified();
                  if(!str.isEmpty())
                  {
                      QTableWidgetItem* itemlres = new QTableWidgetItem(str);
                      ui->tableUpdates->setItem(uboxnumber, 5, itemlres);
                  }
              }

              if(strlist.size() > 6)
              {
                  str = strlist[6].simplified();
                  if(!str.isEmpty())
                  {
                      QTableWidgetItem* itemfitres = new QTableWidgetItem(str);
                      ui->tableUpdates->setItem(uboxnumber, 6, itemfitres);
                  }
              }

              ++uboxnumber;
           }

           inputfile.close();

           QString msg = "File " + filename + " has been succesfully opened!";
           QMessageBox::information(this, "Success", msg);
        }
        else // it should never happen
            QMessageBox::warning(this, "Error",
                                 "Cannnot open the file for reading.");
    }

    return;

error_report:
    QString msg = "File " + filename + " is not a regular *.tip file!?\n\nDefault values will be set.";
    QMessageBox::warning(this, "Error Reading File", msg);
    on_pushDefault_clicked();
}

//----------------------------------------------
void Dialog::on_pushSave_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save Input Parameters As ..."),
                                                    QDir::currentPath(),
                                                    tr("Tilemaker Input Parameters (*.tip);;All files (*.*)"));
    if(filename != QString::null)
    {
        if(!filename.endsWith(".tip"))
            filename += ".tip";

        //qDebug() << filename;

        QFile outfile(filename);
        if (outfile.open(QIODevice::WriteOnly))
        {
           QTextStream out(&outfile);

           out << "url:" << ui->editUrl->text() << endl;
           out << "layer:" << ui->editLayer->text() << endl;
           out << "bbox:" << ui->editBBOX->text() << endl;
           out << "res:" << ui->editRes->text() << endl;
           out << "srs:" << ui->editSRS->text() << endl;

           out << "threads:" << ui->spinThreads->value() << endl;
           out << "quality:" << ui->spinQuality->value() << endl;

           out << "noopt:" << ui->checkNoOpt->isChecked() << endl;
           out << "skipdirs:" << ui->checkSkipdirs->isChecked() << endl;
           out << "verbose:" << ui->checkVerbose->isChecked() << endl;

           out << "background:" << (ui->radioWhite->isChecked() ? "white" : ui->radioBlack->isChecked() ? "black" : "transparent") << endl;
           out << "exceptions:" << (ui->radioTolerant->isChecked() ? "tolerant" : ui->radioModerate->isChecked() ? "moderate" : "strict") << endl;
           out << "format:" << (ui->radioJpeg->isChecked() ? "jpeg" : ui->radioPng->isChecked() ? "png" : "gif") << endl;

           out << "updates:" << ui->groupUBox->isChecked() << endl;
           for(int row = 0; row<ui->tableUpdates->rowCount(); row++)
           {
               QTableWidgetItem* itemleft = ui->tableUpdates->item(row, 0);
               QTableWidgetItem* itembottom = ui->tableUpdates->item(row, 1);
               QTableWidgetItem* itemright = ui->tableUpdates->item(row, 2);
               QTableWidgetItem* itemtop = ui->tableUpdates->item(row, 3);

               QTableWidgetItem* itemhres = ui->tableUpdates->item(row, 4);
               QTableWidgetItem* itemlres = ui->tableUpdates->item(row, 5);
               QTableWidgetItem* itemfitres = ui->tableUpdates->item(row, 6);

               out << (itemleft ? itemleft->text() : "") << ",";
               out << (itembottom ? itembottom->text() : "") << ",";
               out << (itemright ? itemright->text() : "") << ",";
               out << (itemtop ? itemtop->text() : "") << ",";
               out << (itemhres ? itemhres->text() : "") << ",";
               out << (itemlres ? itemlres->text() : "") << ",";
               out << (itemfitres ? itemfitres->text() : "") << endl;
           }

           QString msg = "File " + filename + " has been succesfully saved!";
           QMessageBox::information(this, "Success", msg);
        }
        else // it should never happen
            QMessageBox::warning(this, "Error",
                                 "Cannnot open the file for writing.");
    }
}

//----------------------------------------------
void Dialog::on_pushDefault_clicked()
{
    ui->editUrl->setText("");
    ui->editLayer->setText("");
    ui->editBBOX->setText("");
    ui->editRes->setText("");
    ui->editSRS->setText("EPSG:3857");

    on_pushClearUpdates_clicked();
    ui->groupUBox->setChecked(false);

    ui->spinThreads->setValue(1);
    ui->spinQuality->setValue(90);

    ui->checkNoOpt->setChecked(false);
    ui->checkSkipdirs->setChecked(false);
    ui->checkVerbose->setChecked(true);

    ui->radioWhite->setChecked(true);
    ui->radioTolerant->setChecked(true);
    ui->radioJpeg->setChecked(true);

    // disputable !?
    ui->textProcessOutput->setText("");

    QMessageBox::information(this, "Info", "The default parameters have been set.");
}

//----------------------------------------------
void Dialog::on_groupUBox_toggled(bool arg1)
{
    ui->checkSkipdirs->setEnabled(arg1);
}
