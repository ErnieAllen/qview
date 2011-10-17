#ifndef _qe_object_model_h
#define _qe_object_model_h
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

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QMutex>
#include <QStringList>
#include <qmf/Data.h>
#include <qmf/ConsoleEvent.h>
#include <qpid/types/Variant.h>
#include <sstream>
#include <string>
#include <map>
#include <list>
#include <deque>
#include <boost/shared_ptr.hpp>

Q_DECLARE_METATYPE(qmf::Data);
Q_DECLARE_METATYPE(qmf::ConsoleEvent);
Q_DECLARE_METATYPE(qpid::types::Variant::Map);

class MessageIndex;
typedef boost::shared_ptr<MessageIndex> MessageIndexPtr;
typedef std::map<quint32, MessageIndexPtr> IndexMap;
typedef std::list<MessageIndexPtr> IndexList;

class HeaderModel : public QAbstractItemModel {
    Q_OBJECT

public:
    HeaderModel(QObject* parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex& index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    const qpid::types::Variant::Map& args(const QModelIndex& index);

    const IndexList& getMessageHeaderList();

    typedef enum { NODE_SUMMARY, NODE_DETAIL, NODE_BODY, NODE_BODY_DISPLAY } NodeType;

public slots:
    void addHeader(const qmf::ConsoleEvent&, const qpid::types::Variant::Map&);
    void clear();
    void selected(const QModelIndex&);
    void setBodyText(const QModelIndex&, const QString&);
    void expanded(const QModelIndex&);
    void collapsed(const QModelIndex&);
    void updating(quint32 id, quint32 correlator);
    void expire(quint32 correlator);

signals:
    void bodySelected(const QModelIndex&, const qmf::ConsoleEvent&, const qpid::types::Variant::Map&);
    void summarySelected(const QModelIndex&);

private:
    IndexList summaries;
    IndexMap linkage;
    quint32 nextId;

    void renumber(IndexList&);

    MessageIndexPtr updateOrInsertNode(IndexList& list, NodeType nodeType, MessageIndexPtr parent,
                                  const QMap<QString, QString>& keysValues,
                                  const std::string& messageId,
                                  const qmf::ConsoleEvent& event,
                                  const qpid::types::Variant::Map& map, QModelIndex parentIndex);

    QStringList summaryProperties;
    QStringList bodyProperties;

    //qpid::types::Variant::Map emptyMap;

};


class MessageIndex {
public:
    quint32 id;
    int row;
    HeaderModel::NodeType nodeType;
    MessageIndexPtr parent;
    IndexList children;

    std::string text;       // constructed name=value list
    std::string messageId;  // unique per message
    QMap<QString, QString> nameValues;

    // remember the returned data and the args used to get the data
    qmf::ConsoleEvent event;
    qpid::types::Variant::Map args;
    bool expanded;
    bool changed;
    quint32 correlator; // latest console event correlator used to update this record

};

std::ostream& operator<<(std::ostream& out, const MessageIndex& value);

#endif

