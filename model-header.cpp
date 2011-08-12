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

#include "model-header.h"
#include <qmf/SchemaId.h>
#include <qmf/DataAddr.h>
#include <iostream>
#include <QApplication>
#include <QBrush>
#include <QFont>

using std::cout;
using std::endl;

HeaderModel::HeaderModel(QObject* parent) : QAbstractItemModel(parent), nextId(1)
{
    // These properties will be displayed at the top level of the tree
    summaryProperties = QStringList(QString("UserId"));
    summaryProperties << QString("ContentLength");
    summaryProperties << QString("ContentType");
    summaryProperties << QString("MessageId");

    // These properties are displayed on the last line of the header details
    bodyProperties = QStringList(QString("ContentLength"));
    bodyProperties << QString("ContentType");
    bodyProperties << QString("ContentEncoding");

}


void HeaderModel::renumber(IndexList& list)
{
    int sequence = 0;
    for (IndexList::iterator iter = list.begin(); iter != list.end(); iter++)
        (*iter)->row = sequence++;
}


HeaderModel::ObjectIndexPtr
HeaderModel::updateOrInsertNode(IndexList& list, NodeType nodeType, ObjectIndexPtr parent,
                              const std::string& text, const std::string& messageId, const qmf::ConsoleEvent& event,
                              const qpid::types::Variant::Map& map, QModelIndex parentIndex)
{
    ObjectIndexPtr node;
    int rowCount;

    IndexList::iterator iter(list.begin());
    rowCount = 0;

    // always insert a new row
    //rowCount = list.size();
    //iter = list.end();
/*
    while (iter != list.end() && text > (*iter)->text) {
        iter++;
        rowCount++;
    }
*/
    if (nodeType == NODE_SUMMARY) {
        // summary nodes may have the same text, so use the messageId
        while (iter != list.end() && messageId != (*iter)->messageId) {
            iter++;
            rowCount++;
        }
    } else while (iter != list.end() && text != (*iter)->text) {
        // all other nodes may have the same messageId, so use the text
        iter++;
        rowCount++;
    }
/*
    else {
        rowCount = list.size();
        iter = list.end();
    }
*/
    if (iter == list.end() || text != (*iter)->text) {
        //
        // A new data record needs to be inserted in-order in the list.
        //
        beginInsertRows(parentIndex, rowCount, rowCount);
        node.reset(new ObjectIndex());
        node->id = nextId++;
        node->nodeType = nodeType;
        node->text = text;
        node->messageId = messageId;
        node->parent = parent;
        node->event = event;
        node->args = map;
        linkage[node->id] = node;

        if (iter == list.end())
            list.push_back(node);
        else
            list.insert(iter, node);
        renumber(list);
        endInsertRows();
    } else
        node = *iter;

    return node;
}


void HeaderModel::addHeader(const qmf::ConsoleEvent& event, const qpid::types::Variant::Map& callArgs)
{
    // get the messageId that was passed to the qmf call
    std::string messageId;
    qpid::types::Variant::Map::const_iterator iter = callArgs.find("id");
    if (iter != callArgs.end())
        messageId = iter->second.asString();

    // get the arguments returned in the call results
    const qpid::types::Variant::Map& args(event.getArguments());

    // loop throught the retuened arguments and add them to the header tree
    for (iter = args.begin();
       iter != args.end(); iter++) {

       QStringList propList = QStringList();
       QString summary = QString();
       QString bodyProps = QString();
       QString sep(", ");
       QString equ("=");

       bool firstSum = true;
       bool firstBody = true;

       qpid::types::Variant::Map attrs = (iter->second).asMap();
       // each argument in the map is concatenated
       for (qpid::types::Variant::Map::const_iterator iter = attrs.begin();
            iter != attrs.end(); iter++) {

            QString name(iter->first.c_str());
            QString value(iter->second.asString().c_str());
            if (bodyProperties.contains(name)) {
                if (firstBody)
                    firstBody = false;
                else
                    bodyProps += sep;
                bodyProps += name + equ + value;

            } else {
                QString prop = name + equ + value;
                propList << prop;
            }


            if (summaryProperties.contains(name)) {
                if (firstSum)
                     firstSum = false;
                 else
                     summary += sep;
                 summary += name + equ + value;
            }

        }
        ObjectIndexPtr pptr(updateOrInsertNode(summaries, NODE_SUMMARY, ObjectIndexPtr(),
                                             summary.toStdString(), messageId,
                                             event, callArgs, QModelIndex()));

        QStringList::const_iterator iter = propList.constBegin();
        const QStringList::const_iterator end = propList.constEnd();
        for (; iter != end; ++iter) {
            updateOrInsertNode(pptr->children, NODE_DETAIL, pptr,
                  (*iter).toStdString(), messageId,
                  event, callArgs, createIndex(pptr->row, 0, pptr->id));
        }
        // add the message body properties last
        ObjectIndexPtr sptr(updateOrInsertNode(pptr->children, NODE_BODY, pptr,
              bodyProps.toStdString(), messageId,
              event, callArgs, createIndex(pptr->row, 0, pptr->id)));
        // insert a body display node
        if (sptr->children.size() == 0)
            updateOrInsertNode(sptr->children, NODE_BODY_DISPLAY, sptr,
                         "please wait...", messageId,
                         event, callArgs, createIndex(sptr->row, 0, sptr->id));
    }
}

void HeaderModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, summaries.size() - 1);
    summaries.clear();
    linkage.clear();
    endRemoveRows();
}


void HeaderModel::selected(const QModelIndex& index)
{
    //
    // Get the data record linked to the ID.
    //
    quint32 id(index.internalId());
    IndexMap::const_iterator iter(linkage.find(id));
    if (iter == linkage.end())
        return;
    const ObjectIndexPtr ptr(iter->second);

    switch (ptr->nodeType) {
    case NODE_SUMMARY:
        emit summarySelected(index);
        break;
    case NODE_BODY:
        if (ptr->children.front()->text == "please wait...")
            emit bodySelected(index, ptr->event, ptr->args);
        break;
    default:
        break;
    }
}

void HeaderModel::setBodyText(const QModelIndex& index, const QString& body)
{
    quint32 id(index.internalId());
    IndexMap::const_iterator iter(linkage.find(id));
    if (iter == linkage.end())
        return;
    const ObjectIndexPtr ptr(iter->second);
    const ObjectIndexPtr bodyNode(ptr->children.front());


    bodyNode->text = body.toStdString();

    QModelIndex tl = createIndex(bodyNode->row, 0, bodyNode->id);
    emit dataChanged ( tl, tl );

}


int HeaderModel::rowCount(const QModelIndex &parent) const
{
    //
    // If the parent is invalid (top-level), return the number of summaries.
    //
    if (!parent.isValid())
        return (int) summaries.size();

    //
    // Get the data record linked to the ID.
    //
    quint32 id(parent.internalId());
    IndexMap::const_iterator iter(linkage.find(id));
    if (iter == linkage.end())
        return 0;
    const ObjectIndexPtr ptr(iter->second);

    //
    // For parents, return the number of children.
    //
    switch (ptr->nodeType) {
    case NODE_DETAIL:
    case NODE_BODY_DISPLAY:
        return 0;
    case NODE_BODY:
    case NODE_SUMMARY:
        return (int) ptr->children.size();
    }

    //
    // For instance nodes, return 0 because there are no children.
    //
    return 0;
}


int HeaderModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}


QVariant HeaderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole || role == Qt::BackgroundRole || role == Qt::ForegroundRole) {

        //
        // Note that we don't look at the row number in this function.  The index structure
        // is defined such that the internalId is sufficient to identify the data record being
        // interrogated.
        //

        //
        // Get the data record linked to the ID.
        //
        quint32 id(index.internalId());
        IndexMap::const_iterator liter(linkage.find(id));
        if (liter == linkage.end())
            return QVariant();
        const ObjectIndexPtr ptr(liter->second);


        if (role == Qt::DisplayRole)
            return QString(ptr->text.c_str());
        if (ptr->nodeType == NODE_BODY) {
            if (role == Qt::BackgroundRole)
                return QBrush(Qt::yellow);
            if (role == Qt::ForegroundRole)
                return QBrush(Qt::black);
            if (role == Qt::FontRole) {
                QFont bold = QFont(QApplication::font());
                bold.setWeight(QFont::Black);
                return bold;
            }
        }
    }
    return QVariant();

}

const qpid::types::Variant::Map& HeaderModel::args(const QModelIndex& index)
{
    if (index.isValid()) {
        quint32 id(index.internalId());
        IndexMap::const_iterator liter(linkage.find(id));
        if (liter != linkage.end()) {
            const ObjectIndexPtr ptr(liter->second);
            return ptr->args;
        }
    }
    return qpid::types::Variant::Map();
}

QVariant HeaderModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 && role == Qt::DisplayRole && orientation == Qt::Horizontal)
        return QString("Messages");
    return QVariant();
}


QModelIndex HeaderModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    quint32 id(index.internalId());

    //
    // Get the linked record
    //
    IndexMap::const_iterator iter(linkage.find(id));
    if (iter == linkage.end())
        return QModelIndex();
    ObjectIndexPtr ptr(iter->second);

    //
    // Handle the top level node case
    //
    if (ptr->nodeType == NODE_SUMMARY)
        return QModelIndex();

    //
    // Handle the schema and instance level cases
    //
    return createIndex(ptr->parent->row, 0, ptr->parent->id);
}


QModelIndex HeaderModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(column);
    int count;
    IndexList::const_iterator iter;

    if (!parent.isValid()) {
        //
        // Handle the top leven node case
        //
        count = 0;
        iter = summaries.begin();
        while (iter != summaries.end() && count < row) {
            count++;
            iter++;
        }

        if (iter == summaries.end()) {
            return QModelIndex();
        }
        return createIndex(row, 0, (*iter)->id);
    }

    //
    // Get the data record linked to the ID.
    //
    quint32 id(parent.internalId());
    IndexMap::const_iterator link = linkage.find(id);
    if (link == linkage.end())
        return QModelIndex();
    ObjectIndexPtr ptr(link->second);

    //
    // Create an index for the child data record.
    //
    switch (ptr->nodeType) {
    // these don't have children
    case NODE_BODY_DISPLAY:
    case NODE_DETAIL:
        break;
    // these have children
    case NODE_BODY:
    case NODE_SUMMARY:
        count = 0;
        iter = ptr->children.begin();
        while (count < row) {
            iter++;
            count++;
        }

        if (iter == summaries.end())
            return QModelIndex();
        return createIndex(row, 0, (*iter)->id);
    }

    return QModelIndex();
}

