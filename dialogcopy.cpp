/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "dialogcopy.h"
#include "ui_dialogcopy.h"
#include <QFileDialog>

DialogCopy::DialogCopy(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogCopy)
{
    ui->setupUi(this);
    connect(ui->radioButtonCopyFile, SIGNAL(toggled(bool)), this, SLOT(copyToFileToggled(bool)));
    connect(ui->pushButtonBrowse, SIGNAL(clicked()), this, SLOT(browse()));
}

DialogCopy::~DialogCopy()
{
    delete ui;
}

void DialogCopy::showEvent(QShowEvent *e)
{
    ui->label_error->hide();
    QWidget::showEvent(e);
}

void DialogCopy::accept()
{
    ui->label_error->hide();
    if (ui->radioButtonCopyFile->isChecked())
    {
        QString str = ui->lineEditCopyFileName->text();
        if (str.isEmpty()) {
            ui->label_error->show();
            return;
        }
        QFile f(ui->lineEditCopyFileName->text());
        if (!f.exists()) {
            if (f.open(QIODevice::WriteOnly)) {
                f.close();
            } else {
                ui->label_error->show();
                return;
            }
        }
    }
    emit copyDialogAccepted(ui->lineEditCopyFileName->text());
    hide();;
}

void DialogCopy::copyToFileToggled(bool checked)
{
    ui->lineEditCopyFileName->setEnabled(checked);
    ui->pushButtonBrowse->setEnabled(checked);
}

void DialogCopy::browse()
 {
     QString file = QFileDialog::getSaveFileName ( this,
                        tr("Save to file"), QDir::currentPath());

     if (!file.isEmpty()) {
        ui->lineEditCopyFileName->setText(file);
        accept();
     }
 }
