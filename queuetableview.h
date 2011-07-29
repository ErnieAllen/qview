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

#ifndef QUEUETABLEVIEW_H
#define QUEUETABLEVIEW_H

#include <QTableView>
#include <QSortFilterProxyModel>
#include "model-queue.h"

class QueueTableView : public QTableView
{
    Q_OBJECT
public:
    explicit QueueTableView(QWidget *parent = 0);

    QString                 selectedQueueName(QueueTableModel *, QSortFilterProxyModel *);
    const qmf::Agent&       selectedQueueAgent(QueueTableModel *model, QSortFilterProxyModel *);
    const qmf::DataAddr&    selectedQueueDataAddr(QueueTableModel *model, QSortFilterProxyModel *);

    bool                    hasSelected();

signals:

public slots:

protected:

};

#endif // QUEUETABLEVIEW_H
