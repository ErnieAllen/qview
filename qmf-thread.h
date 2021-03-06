#ifndef _qe_qmf_thread_h
#define _qe_qmf_thread_h
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

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QLineEdit>
#include <QStringList>

#include <qpid/messaging/Connection.h>
#include <qmf/ConsoleSession.h>
#include <qmf/ConsoleEvent.h>
#include "qpid/types/Variant.h"
#include <qmf/Data.h>
#include "model-header.h"
#include <sstream>
#include <deque>

static QModelIndex defaultIndex;
class QmfThread : public QThread {
    Q_OBJECT

public:
    QmfThread(QObject* parent);
    void cancel();
    void getQueueHeaders(const QString&);
    void queueRemoveMessage(const QString&, const qpid::types::Variant::Map&);
    qmf::ConsoleEvent fetchBody(const qpid::types::Variant::Map&);

public slots:
    void connect_localhost();
    void disconnect();
    void connect_url(const QString&, const QString&, const QString&);
    void pauseRefreshes(bool);
    void showBody(const QModelIndex&, const qmf::ConsoleEvent &, const qpid::types::Variant::Map &);


signals:
    void connectionStatusChanged(const QString&);
    void isConnected(bool);
    void addQueue(const qmf::Data&, uint);
    void doneAddingQueues(uint);
    void headerAdded(uint);
    void gotMessageHeaders(const qmf::ConsoleEvent&, const qpid::types::Variant::Map&);
    void gotMessageBody(const qmf::ConsoleEvent&, const qpid::types::Variant::Map&, const QModelIndex&);
    void removedMessage(const qmf::ConsoleEvent&, const qpid::types::Variant::Map&);
    void requestedMessageHeaders(quint32, quint32);
    void doneRequestingHeaders(quint32);

    void qmfError(const QString&);

protected:
    void run();

private:
    struct Command {
        bool connect;
        std::string url;
        std::string conn_options;
        std::string qmf_options;

        Command(bool _c, const std::string& _u, const std::string& _co, const std::string& _qo) :
            connect(_c), url(_u), conn_options(_co), qmf_options(_qo) {}
    };
    typedef std::deque<Command> command_queue_t;

    mutable QMutex lock;
    QWaitCondition cond;
    qpid::messaging::Connection conn;
    qmf::ConsoleSession sess;
    bool cancelled;
    bool connected;
    bool pausedRefreshes;
    command_queue_t command_queue;

    struct Callback {
        uint32_t correlator;
        std::string method;
        qpid::types::Variant::Map args;
        QModelIndex index;

        Callback(std::string _m, const qpid::types::Variant::Map& _a, const QModelIndex& _i) : correlator(0),
            method(_m), args(_a), index(_i) {}
    };

    typedef std::deque<Callback> callback_queue_t;

    callback_queue_t callback_queue;

    void callCallback(const qmf::ConsoleEvent&);
    void emitCallback(const Callback& cb, const qmf::ConsoleEvent& event);
    void addCallback(qmf::Agent, const std::string&,
                                const qpid::types::Variant::Map&,
                                const qmf::DataAddr&,
                                const std::string&,
                                const QModelIndex& = defaultIndex);

    // remember the broker object so we can make qmf calls
    qmf::Data brokerData;
};

#endif

