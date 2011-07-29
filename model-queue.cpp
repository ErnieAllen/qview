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

#include "model-queue.h"
#include <iostream>

using std::cout;
using std::endl;

QueueTableModel::QueueTableModel(QObject* parent) : QAbstractTableModel(parent)
{
    // Initialize the default columns
    queueColumns.append(Column("name", "Name", Qt::AlignLeft, ""));
    queueColumns.append(Column("durable", "Durable", Qt::AlignLeft, ""));
    queueColumns.append(Column("autoDelete", "Auto Delete", Qt::AlignLeft, ""));
    queueColumns.append(Column("exclusive", "Exclusive", Qt::AlignLeft, ""));
    queueColumns.append(Column("msgDepth", "Messages", Qt::AlignRight, "N"));
    queueColumns.append(Column("byteDepth", "Bytes", Qt::AlignRight, "N"));
    queueColumns.append(Column("byteTotalEnqueues", "In Bytes", Qt::AlignRight, "N"));
}


void QueueTableModel::addObject(const qmf::Data& object)
{
    if (!object.isValid())
        return;

    // see if the object already exists in the list
    const qpid::types::Variant& name = object.getProperty("name");

    for (int idx=0; idx<dataList.size(); idx++) {
        qmf::Data existing = dataList.at(idx);
        if (name.isEqualTo(existing.getProperty("name"))) {
            const qpid::types::Variant::Map& map(object.getProperties());
            existing.overwriteProperties(map);

            // force a refresh of the changed row
            QModelIndex topLeft = index(0, 0);
            QModelIndex bottomRight = index(dataList.size() - 1, 2);
            emit dataChanged ( topLeft, bottomRight );

            return;
        }
    }

    // this is a new queue
    beginInsertRows(QModelIndex(), 0, 0);
    dataList.append(object);
    endInsertRows();
}


void QueueTableModel::connectionChanged(bool isConnected)
{
    if (!isConnected)
        clear();
}

void QueueTableModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, dataList.count() - 1);
    dataList.clear();
    endRemoveRows();
}


int QueueTableModel::rowCount(const QModelIndex &parent) const
{
    //
    // If the parent is invalid (top-level), return the number of attributes.
    //
    if (!parent.isValid())
        return (int) dataList.size();

    //
    // This is not a tree so there are not child rows.
    //
    return 0;
}


int QueueTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return queueColumns.size();
}


QVariant QueueTableModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid())
        return QVariant();

    if (role == Qt::TextAlignmentRole) {
        return queueColumns[index.column()].alignment;
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    const qmf::Data& object = dataList.at(index.row());
    const qpid::types::Variant::Map& attrs(object.getProperties());
    qpid::types::Variant::Map::const_iterator iter;

    iter = attrs.find(queueColumns[index.column()].name);
    if (iter == attrs.end()) {
        return QString("--");
    }

    //return QString(iter->second.asString().c_str());

    switch (iter->second.getType()) {
    default:
        if (queueColumns[index.column()].format == "B")
            return fmtBytes(iter->second);
        else if (queueColumns[index.column()].format == "N")
            return QVariant((qulonglong)iter->second.asUint64());
        // else display as string
    case qpid::types::VAR_STRING:
    case qpid::types::VAR_BOOL:
    case qpid::types::VAR_UUID:
    case qpid::types::VAR_VOID:
        return QString(iter->second.asString().c_str());
    case qpid::types::VAR_MAP:
        return QString("<map>");
    case qpid::types::VAR_LIST:
        return QString("<list>");
    }
    return QVariant();
}

QString QueueTableModel::fmtBytes(const qpid::types::Variant& v) const
{
    const char *sizes = " KMGTPY";
    uint k = 1024;
    uint which_size = 0;

    qlonglong b = v.asUint64();
    while (b >= k) {
        b /= k;
        ++which_size;
    }
    if (which_size < strlen(sizes)) {
        QChar c = QChar(sizes[which_size]);
        return QString("%1").arg(b) + QString(&c, 1);
    }
    return QString().setNum(b);
}

QVariant QueueTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section < queueColumns.size()) {

        if (role == Qt::TextAlignmentRole) {
            return queueColumns[section].alignment;
        }

        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            return queueColumns[section].header;
        }
    }
    return QVariant();
}

const qmf::Agent& QueueTableModel::selectedQueueAgent(const QModelIndex& selectedIndex)
{
    return dataList.at(selectedIndex.row()).getAgent();
}

const qmf::DataAddr& QueueTableModel::selectedQueueDataAddr(const QModelIndex& selectedIndex)
{
    return dataList.at(selectedIndex.row()).getAddr();
}

QString QueueTableModel::selectedQueueName(const QModelIndex& selectedIndex)
{
    qpid::types::Variant::Map::const_iterator iter;
    const qmf::Data& object = dataList.at(selectedIndex.row());
    const qpid::types::Variant::Map& attrs(object.getProperties());

    iter = attrs.find("name");
    if (iter == attrs.end())
        return QString("unknown");
    else
        return QString(iter->second.asString().c_str());

}