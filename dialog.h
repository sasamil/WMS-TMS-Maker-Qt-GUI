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

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QProcess>
#include <QMessageBox>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

private slots:
    void rightMessage();
    void wrongMessage();
    void on_finish(int);

    void on_pushExecute_clicked();
    void on_pushBreak_clicked();
    void on_pushLoadUpdates_clicked();
    void on_pushClearUpdates_clicked();
    void on_pushOpen_clicked();
    void on_pushSave_clicked();
    void on_pushDefault_clicked();
    void on_groupUBox_toggled(bool);

private:
    Ui::Dialog *ui;
    QProcess* pTilemaker;

    bool fileExists(const QString&);
    
    bool validateResolution();
    bool validateBBOX();
    bool validateUrl();
    bool validateLayer();
    bool validateSRS();
    bool validateAndSaveUpdates();
    QString validateUpdateRow(int, double, double, double, double, double, double);
};

#endif // DIALOG_H
