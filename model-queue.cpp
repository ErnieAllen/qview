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
    queueColumns.append(Column("autoDelete", "Auto Delete", Qt::AlignLeft, ""));
    queueColumns.append(Column("msgDepth", "Messages", Qt::AlignRight, "N"));
    queueColumns.append(Column("byteDepth", "Bytes", Qt::AlignRight, "N"));
    queueColumns.append(Column("byteTotalEnqueues", "In Bytes", Qt::AlignRight, "N"));
    queueColumns.append(Column("byteTotalDequeues", "Out Bytes", Qt::AlignRight, "N"));
}


bool QueueTableModel::isSystemQueue(const qmf::Data& queue)
{
    const qpid::types::Variant::Map& map(queue.getProperties());
    qpid::types::Variant::Map::const_iterator iter;
    iter = map.find("arguments");
    if (iter != map.end()) {
        const qpid::types::Variant::Map& args(iter->second.asMap());
        iter = args.find("management");
        if (iter == args.end()) {
            return false;
        }
        return true;
    }

    return queue.getProperty("exclusive").asBool();
}

void QueueTableModel::addQueue(const qmf::Data& queue, uint correlator)
{
    if (!queue.isValid())
        return;

    // filter out system queues if required
    if (hideSystemQueues) {
        if (isSystemQueue(queue))
            return;
    }

    // see if the object already exists in the list
    const qpid::types::Variant& name = queue.getProperty("name");

    for (int idx=0; idx<dataList.size(); idx++) {
        qmf::Data existing = dataList.at(idx);
        if (name.isEqualTo(existing.getProperty("name"))) {

            //const qpid::types::Variant::Map& map(queue.getProperties());
            qpid::types::Variant::Map map = qpid::types::Variant::Map(queue.getProperties());
            map["correlator"] = correlator;

            existing.overwriteProperties(map);
            return;
        }
    }

    qmf::Data q = qmf::Data(queue);
    qpid::types::Variant corr =  qpid::types::Variant(correlator);
    q.setProperty("correlator", corr);

    // this is a new queue
    int last = dataList.size();
    beginInsertRows(QModelIndex(), last, last);
    dataList.append(q);
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
    Q_UNUSED(parent);
    return (int) dataList.size();
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

    const qmf::Data& queue = dataList.at(index.row());
    const qpid::types::Variant::Map& attrs(queue.getProperties());
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
    if ((section >=0) && (section < queueColumns.size())) {

        if (role == Qt::TextAlignmentRole) {
            return queueColumns[section].alignment;
        }

        if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
            return queueColumns[section].header;
        }
    }
    return QVariant();
}

const qmf::Data& QueueTableModel::selectedQueue(const QModelIndex& selectedIndex)
{
    if (selectedIndex.isValid()) {
        return dataList.at(selectedIndex.row());
    }
    return qmf::Data();
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
    if (selectedIndex.isValid()) {
        qpid::types::Variant::Map::const_iterator iter;
        const qmf::Data& queue = dataList.at(selectedIndex.row());
        const qpid::types::Variant::Map& attrs(queue.getProperties());

        iter = attrs.find("name");
        if (iter != attrs.end())
            return QString(iter->second.asString().c_str());
    }
    return QString("unknown");
}

QVariant QueueTableModel::selectedQueueDepth(const QModelIndex& selectedIndex)
{
    if (selectedIndex.isValid()) {
        qpid::types::Variant::Map::const_iterator iter;
        const qmf::Data& queue = dataList.at(selectedIndex.row());
        const qpid::types::Variant::Map& attrs(queue.getProperties());

        iter = attrs.find("msgDepth");
        if (iter != attrs.end())
            return QVariant(iter->second.asUint32());
    }
    return QVariant(0);
}

void QueueTableModel::toggleSystemQueues(bool show)
{
    this->hideSystemQueues = !show;
    if (!show) {

        // remove the system queues
        for (int idx=0; idx<dataList.size(); idx++) {
            if (isSystemQueue(dataList.at(idx))) {
                beginRemoveRows( QModelIndex(), idx, idx );
                dataList.removeAt(idx--);
                endRemoveRows();
            }
        }

        // force a refresh of the display
        QModelIndex topLeft = index(0, 0);
        QModelIndex bottomRight = index(dataList.size() - 1, 2);
        emit dataChanged ( topLeft, bottomRight );

    }
}

void QueueTableModel::refresh(uint correlator)
{
    // remove any old queues that were not added/updated with this correlator
    for (int idx=0; idx<dataList.size(); idx++) {
        uint corr = dataList.at(idx).getProperty("correlator").asUint32();
        if (corr != correlator) {
            beginRemoveRows( QModelIndex(), idx, idx );
            dataList.removeAt(idx--);
            endRemoveRows();
        }
    }

    // force a refresh of the display
    QModelIndex topLeft = index(0, 0);
    QModelIndex bottomRight = index(dataList.size() - 1, 2);
    emit dataChanged ( topLeft, bottomRight );
}

std::ostream& operator<<(std::ostream& out, const qmf::Data& queue)
{
    if (queue.isValid()) {
        qpid::types::Variant::Map::const_iterator iter;
        const qpid::types::Variant::Map& attrs(queue.getProperties());

        out << " <properties>\n";
        for (iter = attrs.begin(); iter != attrs.end(); iter++) {
            if (iter->first != "name") {
                out << "  <property>\n";
                out << "   <name>" << iter->first << "</name>\n";
                out << "   <value>" << iter->second << "</value>\n";
                out << "  </property>\n";
            }
        }
        out << " </properties>\n";
    }
    return out;
}
