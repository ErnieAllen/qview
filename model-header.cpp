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


MessageIndexPtr
HeaderModel::updateOrInsertNode(IndexList& list, NodeType nodeType, MessageIndexPtr parent,
                              const QMap<QString, QString>& keyValues,
                              const std::string& messageId,
                              const qmf::ConsoleEvent& event,
                              const qpid::types::Variant::Map& map, QModelIndex parentIndex)
{
    MessageIndexPtr node;
    int rowCount;
    bool prevChanged;

    IndexList::iterator iter(list.begin());
    rowCount = 0;

    if (nodeType == NODE_SUMMARY) {
        // Look for the unique messageId in the top level nodes
        while (iter != list.end() && messageId != (*iter)->messageId) {
            iter++;
            rowCount++;
        }
    } else while (iter != list.end() && (keyValues.keys() != (*iter)->nameValues.keys())) {
        // all other nodes may have the same messageId, so look for the keys
        iter++;
        rowCount++;
    }

    if (iter == list.end()) {
        // A new data record needs to be inserted in the list.
        beginInsertRows(parentIndex, rowCount, rowCount);
        node.reset(new MessageIndex());
        node->id = nextId++;
        linkage[node->id] = node;
        node->nodeType = nodeType;
        node->expanded = false;
        node->messageId = messageId;
        node->parent = parent;
        node->event = event;
        node->args = map;
        node->nameValues = keyValues;

        list.push_back(node);
        renumber(list);
        endInsertRows();
    } else {
        node = *iter;

        prevChanged = node->changed;
        if (node->nameValues.values() != keyValues.values()) {
            node->text = "";
            node->changed = true;
        }
        else
            node->changed = false;
        node->nameValues = keyValues;

        QModelIndex tl = createIndex(node->row, 0, node->id);
        // we are updating an existing node
        if ((node->text == "") || (prevChanged != node->changed)) {
            // redraw the row that was just changed
            emit dataChanged ( tl, tl );
        } else if (node->nodeType == NODE_BODY) {
            // if we are updating a BODY node, it *should* already have a child,
            // but just in case...
            if (node->children.size() > 0) {
                if (node->expanded) {
                    // if there is a body node and it is already expanded
                    MessageIndexPtr ptr = node->children.front();
                    // update the body text too
                    emit bodySelected(tl, ptr->event, ptr->args);
                }
            }
        }
    }
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

       QMap<QString, QString> detailMap;
       QMap<QString, QString> summMap;
       summMap[""] = QString(messageId.c_str());
       QMap<QString, QString> bodyMap;

       qpid::types::Variant::Map attrs = (iter->second).asMap();
       // each argument in the map is concatenated
       for (qpid::types::Variant::Map::const_iterator iter = attrs.begin();
            iter != attrs.end(); iter++) {

            QString name(iter->first.c_str());
            QString value(iter->second.asString().c_str());

            switch (iter->second.getType()) {
            case qpid::types::VAR_UINT8:
            case qpid::types::VAR_UINT16:
            case qpid::types::VAR_UINT32:
            case qpid::types::VAR_UINT64:
            case qpid::types::VAR_INT8:
            case qpid::types::VAR_INT16:
            case qpid::types::VAR_INT32:
            case qpid::types::VAR_INT64:
                value = QString::number((qulonglong)iter->second.asUint64());
                break;
            case qpid::types::VAR_FLOAT:
            case qpid::types::VAR_DOUBLE:
                value = QString::number((double)iter->second.asDouble());
                break;

            case qpid::types::VAR_STRING:
            case qpid::types::VAR_BOOL:
            case qpid::types::VAR_UUID:
            case qpid::types::VAR_VOID:
                value = "\"" + QString(iter->second.asString().c_str()) + "\"";
                break;
            case qpid::types::VAR_MAP:
                value = QString("<map/>");
                break;
            case qpid::types::VAR_LIST:
                value = QString("<list/>");
                break;
            }

            // add the property to either the body node or a detail node
            if (bodyProperties.contains(name)) {
                bodyMap[name] = value;
            } else {
                detailMap[name] = value;
            }

            // also add the property to the summary node if appropriate
            if (summaryProperties.contains(name)) {
                summMap[name] = value;
            }

        }
        // add the top level summary node in the tree
        MessageIndexPtr pptr(updateOrInsertNode(this->summaries, NODE_SUMMARY, MessageIndexPtr(),
                                             summMap, messageId,
                                             event, callArgs, QModelIndex()));

        // add all the message properties
        QMap<QString, QString>::const_iterator iter = detailMap.constBegin();
        QMap<QString, QString> prop;
        for (; iter != detailMap.constEnd(); ++iter) {
            prop.clear();
            prop[iter.key()] = iter.value();
            updateOrInsertNode(pptr->children, NODE_DETAIL, pptr,
                  prop, messageId,
                  event, callArgs, createIndex(pptr->row, 0, pptr->id));
        }
        // add the message body properties last
        MessageIndexPtr sptr(updateOrInsertNode(pptr->children, NODE_BODY, pptr,
              bodyMap, messageId,
              event, callArgs, createIndex(pptr->row, 0, pptr->id)));
        // insert a body display node
        prop.clear();
        prop["body"] = "...";
        if (sptr->children.size() == 0)
            updateOrInsertNode(sptr->children, NODE_BODY_DISPLAY, sptr,
                         prop, messageId,
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


void HeaderModel::expanded(const QModelIndex &index)
{
    quint32 id(index.internalId());
    IndexMap::const_iterator iter(linkage.find(id));
    if (iter == linkage.end())
        return;
    const MessageIndexPtr ptr(iter->second);

    ptr->expanded = true;
}

void HeaderModel::collapsed(const QModelIndex &index)
{
    quint32 id(index.internalId());
    IndexMap::const_iterator iter(linkage.find(id));
    if (iter == linkage.end())
        return;
    const MessageIndexPtr ptr(iter->second);

    ptr->expanded = false;
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
    const MessageIndexPtr ptr(iter->second);

    switch (ptr->nodeType) {
    case NODE_SUMMARY:
        emit summarySelected(index);
        break;
    case NODE_BODY:
        if (ptr->children.front()->text == "")
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
    const MessageIndexPtr ptr(iter->second);

    MessageIndexPtr bodyNode(ptr->children.front());

    bodyNode->changed = false;
    if (bodyNode->text != body.toStdString()) {
        bodyNode->text = "";
        bodyNode->nameValues["body"] = body;
        bodyNode->changed = true;
        QModelIndex tl = createIndex(bodyNode->row, 0, bodyNode->id);
        emit dataChanged ( tl, tl );
    }
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
    const MessageIndexPtr ptr(iter->second);

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

// SLOT triggered when a message in the tree is updated
void HeaderModel::updating(quint32 id, quint32 correlator)
{
    QString mId = QString::number(id);

    for (IndexList::iterator iter = summaries.begin(); iter != summaries.end(); iter++) {
        MessageIndexPtr record = *iter;
        if (mId.toStdString() == (*iter)->messageId) {
            (*iter)->correlator = correlator;
            break;
        }
    }
}

// SLOT triggered when all the messages in the tree have been updated
// Remove all messages that did not get updated (because they were consumed)
void HeaderModel::expire(quint32 correlator)
{
    int row = 0;
    for (IndexList::iterator iter = summaries.begin(); iter != summaries.end(); iter++) {
        MessageIndexPtr record = *iter;
        if ((*iter)->correlator != correlator) {
            beginRemoveRows( QModelIndex(), row, row );
            iter = summaries.erase(iter);
            --row;
            endRemoveRows();
        }
        ++row;
    }
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
        const MessageIndexPtr ptr(liter->second);

        if (role == Qt::DisplayRole) {
            if (ptr->text == "") {
                std::string text;
                std::string sep;
                QMap<QString, QString>::const_iterator iter = ptr->nameValues.constBegin();
                for (; iter != ptr->nameValues.constEnd(); ++iter) {
                    text += sep;
                    if (ptr->nodeType != NODE_BODY_DISPLAY) {
                        if (iter.key() != "") {
                            text += iter.key().toStdString();
                            text += "=";
                            sep = ", ";
                        }
                    }
                    text += iter.value().toStdString();
                }
                // none of the message body properties were sent
                if ((ptr->nodeType == NODE_BODY) && (text == ""))
                    text = "Message body";

                ptr->text = text;
            }
            return QString(ptr->text.c_str());
        }
        if (ptr->changed) {
            if (role == Qt::ForegroundRole) {
                return QBrush(Qt::red);
            }
        }
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
            const MessageIndexPtr ptr(liter->second);
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
    MessageIndexPtr ptr(iter->second);

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
    MessageIndexPtr ptr(link->second);

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

std::ostream& operator<<(std::ostream& out, const MessageIndex& header)
{
    out << "   <arguments>\n";

    // get the arguments returned in the call results
    const qpid::types::Variant::Map& args(header.event.getArguments());

    // loop through the retuened arguments and add them to the xml stream
    qpid::types::Variant::Map::const_iterator iter;
    for (iter = args.begin();
       iter != args.end(); iter++) {

       qpid::types::Variant::Map attrs = (iter->second).asMap();
       for (qpid::types::Variant::Map::const_iterator iter = attrs.begin();
            iter != attrs.end(); iter++) {
            out << "    <argument>\n";
            out << "      <name>" << iter->first << "</name>\n";
            out << "      <value>" << iter->second << "</value>\n";
            out << "    </argument>\n";
        }
    }

    out << "   </arguments>\n";
    return out;
}

const IndexList& HeaderModel::getMessageHeaderList()
{
    return this->summaries;
}
