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

#include "queuetableview.h"

QueueTableView::QueueTableView(QWidget *parent) :
    QTableView(parent)
{
}

bool QueueTableView::hasSelected()
{
    QModelIndex index = currentIndex();
    return index.row() >= 0;
}

const qmf::Data& QueueTableView::selectedQueue(QueueTableModel *model, QSortFilterProxyModel *proxy)
{
    QModelIndex index = currentIndex();
    QModelIndex sindex = proxy->mapToSource(index);
    return model->selectedQueue(sindex);
}

const qmf::Agent& QueueTableView::selectedQueueAgent(QueueTableModel *model, QSortFilterProxyModel *proxy)
{
    QModelIndex index = currentIndex();
    QModelIndex sindex = proxy->mapToSource(index);
    return model->selectedQueueAgent(sindex);
}

QString QueueTableView::selectedQueueName(QueueTableModel *model, QSortFilterProxyModel *proxy)
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        QModelIndex sindex = proxy->mapToSource(index);
        return model->selectedQueueName(sindex);
    }
    return QString();
}

const qmf::DataAddr& QueueTableView::selectedQueueDataAddr(QueueTableModel *model, QSortFilterProxyModel *proxy)
{
    QModelIndex index = currentIndex();
    QModelIndex sindex = proxy->mapToSource(index);
    return model->selectedQueueDataAddr(sindex);
}

QVariant QueueTableView::selectedQueueDepth(QueueTableModel *model, QSortFilterProxyModel *proxy)
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        QModelIndex sindex = proxy->mapToSource(index);
        return model->selectedQueueDepth(sindex);
    }
    return QVariant();
}
