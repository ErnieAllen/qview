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

#ifndef DIALOGCOPY_H
#define DIALOGCOPY_H

#include <QDialog>

namespace Ui {
    class DialogCopy;
}

class DialogCopy : public QDialog
{
    Q_OBJECT

public:
    explicit DialogCopy(QWidget *parent = 0);
    ~DialogCopy();

public slots:
    void accept();
    void browse();
    void copyToFileToggled(bool);

signals:
    void copyDialogAccepted(const QString&);

private:
    Ui::DialogCopy *ui;
    void showEvent(QShowEvent *);
};

#endif // DIALOGCOPY_H
