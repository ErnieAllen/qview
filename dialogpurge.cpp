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

#include "dialogpurge.h"
#include "ui_dialogpurge.h"

DialogPurge::DialogPurge(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogPurge)
{
    ui->setupUi(this);
    ui->label_error->hide();
}

DialogPurge::~DialogPurge()
{
    delete ui;
}

void DialogPurge::setQueueName(const QString& name)
{
    ui->label_queuename->setText(name);
}

void DialogPurge::accept()
{
    uint count = 0;

    ui->label_error->hide();
    if (ui->radioButton_all->isChecked())
        count = 0;
    else if (ui->radioButton_top->isChecked())
        count = 1;
    else {

        QString str = ui->lineEdit->text();
        if (str.isEmpty()) {
            ui->label_error->show();
            return;
        }
        bool ok;
        count = str.toUInt(&ok, 10);
        if (!ok) {
            ui->label_error->show();
            return;
        }
    }
    emit purgeDialogAccepted(count);
    hide();;
}
