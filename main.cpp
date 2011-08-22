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

#include "main.h"
#include <iostream>
#include <QSettings>
#include <qmf/DataAddr.h>
#include <qmf/ConsoleEvent.h>
#include <qmf/Query.h>
#include <qpid/messaging/Message.h>


QView::QView(QMainWindow* parent) : QMainWindow(parent)
{
    //
    // Setup some global app vales to be used by the QSettings class
    //
    QCoreApplication::setOrganizationName("Red Hat");
    QCoreApplication::setOrganizationDomain("redhat.com");
    QCoreApplication::setApplicationName("QView");

    //
    // Setup the UI components defined in the explorer_main.ui form
    //
    setupUi(this);


    //
    // Allow some qpid and qmf types to be passed from signals to slots
    //
    qRegisterMetaType<qpid::types::Variant::Map>();
    qRegisterMetaType<qmf::ConsoleEvent>();
    qRegisterMetaType<qmf::Data>();

    //
    // Add UI widgets not defined in explorer_main.ui form
    //
    setupStatusBar();
    createToolBars();

    textBrowser_body->hide();

    //
    // Restore the window size and location
    //
    QSettings settings;
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());

    //
    // Create the object model which stores the list of queried objects.
    //
    headerModel = new HeaderModel(this);
    treeView_objects->setModel(headerModel);
    treeView_objects->header()->setMovable(false);
    treeView_objects->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeView_objects, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerCtxMenu(QPoint)));
    connect(treeView_objects, SIGNAL(expanded(QModelIndex)), headerModel, SLOT(expanded(QModelIndex)));
    connect(treeView_objects, SIGNAL(collapsed(QModelIndex)), headerModel, SLOT(collapsed(QModelIndex)));

    //
    // Create the object-detail model which holds the properties of an object.
    //
    queueModel = new QueueTableModel(this);

    //
    // Create a proxy model to enable sorting by column
    queueProxyModel = new QSortFilterProxyModel(this);
    queueProxyModel->setSourceModel(queueModel);
    queueProxyModel->setFilterKeyColumn(0);

    //
    // Assign the proxy model to the view
    tableView_object->setModel(queueProxyModel);
    tableView_object->horizontalHeader()->setMovable(true);

    // this is the internal object that controls the queue table selection
    itemSelector = tableView_object->selectionModel();
    connect(itemSelector, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(queueSelected()));

    headerPopupMenu = new QMenu;
    headerPopupMenu->addAction(actionDelete);
    headerPopupMenu->addAction(actionCopy_Messages);
    connect(actionDelete, SIGNAL(triggered()), this, SLOT(messageDelete()));

    //
    // Create the thread object that maintains communication with the messaging plane.
    //
    qmf = new QmfThread(this);
    qmf->start();

    //
    // Create the open connection dialog box
    //
    openDialog = new DialogOpen(this);
    connect(openDialog, SIGNAL(dialogOpenAccepted(QString,QString,QString)), qmf, SLOT(connect_url(QString,QString,QString)));

    purgeDialog = new DialogPurge(this);
    connect(purgeDialog, SIGNAL(purgeDialogAccepted(uint)), this, SLOT(queuePurge(uint)));

    copyDialog = new DialogCopy(this);
    connect(copyDialog, SIGNAL(copyDialogAccepted(QString)), this, SLOT(queueCopy(QString)));

    //
    // Linkage for the menu and the Connection Status label.
    //
    connect(qmf, SIGNAL(connectionStatusChanged(QString)), label_connection_status, SLOT(setText(QString)));

    connect(actionOpen_Localhost, SIGNAL(triggered()), qmf, SLOT(connect_localhost()));
    connect(actionClose, SIGNAL(triggered()), qmf, SLOT(disconnect()));
    connect(actionAbout_QView, SIGNAL(triggered()), this, SLOT(showAbout()));
    connect(actionConnection_toolbar, SIGNAL(toggled(bool)), this, SLOT(toggleConnectionToolbar(bool)));
    connect(actionMessage_toolbar, SIGNAL(toggled(bool)), this, SLOT(toggleMessageToolbar(bool)));
    connect(actionQueue_toolbar, SIGNAL(toggled(bool)), this, SLOT(toggleQueueToolbar(bool)));
    connect(connectionToolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(toggleConnectionToolbar(bool)));
    connect(messageToolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(toggleMessageToolbar(bool)));
    connect(queueToolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(toggleQueueToolbar(bool)));

    connect(qmf, SIGNAL(addQueue(qmf::Data,uint)), queueModel, SLOT(addQueue(qmf::Data,uint)));
    connect(qmf, SIGNAL(doneAddingQueues(uint)), this, SLOT(doneAddingQueues(uint)));
    connect(qmf, SIGNAL(gotMessageHeaders(qmf::ConsoleEvent, qpid::types::Variant::Map)), headerModel, SLOT(addHeader(qmf::ConsoleEvent, qpid::types::Variant::Map)));
    connect(qmf, SIGNAL(removedMessage(qmf::ConsoleEvent, qpid::types::Variant::Map)), this, SLOT(messageRemoved(qmf::ConsoleEvent,qpid::types::Variant::Map)));
    connect(actionRefresh, SIGNAL(toggled(bool)), qmf, SLOT(pauseRefreshes(bool)));
    connect(actionRefresh, SIGNAL(toggled(bool)), this, SLOT(on_actionRefresh_toggled(bool)));

    // update the list of messages each time the list of queues are updated
    connect(queueModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(getHeaderIds()));

    // Show/Hide the system queues
    connect(actionShowManagementQueues, SIGNAL(toggled(bool)), queueModel, SLOT(toggleSystemQueues(bool)));
    queueModel->toggleSystemQueues(actionShowManagementQueues->isChecked());

    // Show the last qmf exception
    connect(qmf, SIGNAL(qmfError(QString)), this, SLOT(qmfException(QString)));

    // Show the message body when we click on a NODE_BODY row in the header tree
    connect(treeView_objects, SIGNAL(expanded(QModelIndex)), headerModel, SLOT(selected(QModelIndex)));

    connect(headerModel, SIGNAL(bodySelected(QModelIndex, qmf::ConsoleEvent,qpid::types::Variant::Map)), qmf, SLOT(showBody(QModelIndex,qmf::ConsoleEvent,qpid::types::Variant::Map)));
    connect(qmf, SIGNAL(gotMessageBody(qmf::ConsoleEvent,qpid::types::Variant::Map,QModelIndex)), this, SLOT(gotBody(qmf::ConsoleEvent,qpid::types::Variant::Map,QModelIndex)));
    //connect(headerModel, SIGNAL(summarySelected(QModelIndex)), treeView_objects, SLOT(expand(QModelIndex)));

    //
    // Create linkages to enable and disable main-window components based on the connection status.
    //
    connect(qmf, SIGNAL(isConnected(bool)), actionOpen_Localhost,    SLOT(setDisabled(bool)));
    connect(qmf, SIGNAL(isConnected(bool)), actionOpen,              SLOT(setDisabled(bool)));
    connect(qmf, SIGNAL(isConnected(bool)), actionClose,             SLOT(setEnabled(bool)));
    connect(qmf, SIGNAL(isConnected(bool)), queueModel,              SLOT(connectionChanged(bool)));
    connect(qmf, SIGNAL(isConnected(bool)), menuActions,             SLOT(setEnabled(bool)));
    connect(qmf, SIGNAL(isConnected(bool)), queueToolBar,            SLOT(setEnabled(bool)));
    connect(qmf, SIGNAL(isConnected(bool)), messageToolBar,          SLOT(setEnabled(bool)));
    connect(qmf, SIGNAL(isConnected(bool)), headerModel,             SLOT(clear()));
    connect(qmf, SIGNAL(isConnected(bool)), this,                    SLOT(qmfExceptionClear()));

    // restore the state of all widgets. should be done after all widgets
    // are created and initialized
    restoreState(settings.value("mainWindowState").toByteArray());
}

// Display the connection status in the status bar
void QView::setupStatusBar() {
    label_connection_status = new QLabel();
    label_connection_prompt = new QLabel();
    label_connection_prompt->setText("Connection status: ");
    statusBar()->addWidget(label_connection_prompt);
    statusBar()->addWidget(label_connection_status);

    QToolBar* refreshToolbar = new QToolBar(tr("Page refresh"));
    label_exception_prompt = new QLabel(QString(tr("Last Exception: ")));
    label_exception_status = new QLabel();

    refreshToolbar->addWidget(label_exception_prompt);
    refreshToolbar->addWidget(label_exception_status);
    refreshToolbar->addAction(actionRefresh);
    statusBar()->addPermanentWidget(refreshToolbar);
}

// Add a toolbar to the left side of the window
void QView::createToolBars()
{
    connectionToolBar = new QToolBar(tr("Connection actions"));
    addToolBar(Qt::LeftToolBarArea, connectionToolBar);
    connectionToolBar->setObjectName("ConnectionActions");
    connectionToolBar->setIconSize(QSize(32,32));
    connectionToolBar->addAction(actionOpen);
    connectionToolBar->addAction(actionClose);
    connectionToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    queueToolBar = new QToolBar(tr("Queue actions"));
    addToolBar(Qt::LeftToolBarArea, queueToolBar);
    queueToolBar->setObjectName("QueueActions");
    queueToolBar->setIconSize(QSize(32,32));
    //queueToolBar->addAction(actionPause);
    //queueToolBar->addAction(actionResume);
    queueToolBar->setEnabled(false);
    queueToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    messageToolBar = new QToolBar(tr("Message actions"));
    addToolBar(Qt::LeftToolBarArea, messageToolBar);
    messageToolBar->setObjectName("MessageActions");
    messageToolBar->setIconSize(QSize(32,32));
    messageToolBar->addAction(actionPurge);
    messageToolBar->addAction(actionDelete);
    messageToolBar->addAction(actionCopy_Messages);
    //messageToolBar->addAction(actionTrace_a_Message);
    messageToolBar->setEnabled(false);
    messageToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    connect(actionPurge, SIGNAL(triggered()), this, SLOT(showPurge()));
    connect(actionCopy_Messages, SIGNAL(triggered()), this, SLOT(showCopy()));
}

// process command line arguments
void QView::init(int argc, char *argv[])
{
    QString url;
    QString connectionOptions;
    QString sessionOptions;

    if (argc > 1) {
        url = QString(argv[1]);
        if (argc > 2) {
            connectionOptions = QString(argv[2]);
            if (argc > 3)
                sessionOptions = QString(argv[3]);
        }
    }
    // only connect if we have a url
    if (!url.isEmpty())
        qmf->connect_url(url, connectionOptions, sessionOptions);
}

void QView::headerCtxMenu(const QPoint& pos)
{

    headerPopupMenu->exec(treeView_objects->mapToGlobal(pos));
}

void QView::messageDelete()
{
    // get the name of the current queue
    if (tableView_object->hasSelected()) {
        QString name = tableView_object->selectedQueueName(queueModel, queueProxyModel);

        // get the id of the current message
        QModelIndex index = treeView_objects->currentIndex();
        if (index.isValid()) {
            const qpid::types::Variant::Map& args(headerModel->args(index));

            qmf->queueRemoveMessage(name, args);
        }
    }
}

// SLOT called when we get a response from the messageRemove qmf call
void QView::messageRemoved(const qmf::ConsoleEvent& event, const qpid::types::Variant::Map& callArgs)
{
    Q_UNUSED(event);
    QString name;
    // request the new list of headers
    qpid::types::Variant::Map::const_iterator iter = callArgs.find("name");
    if (iter != callArgs.end()) {
        headerModel->clear();

        name = iter->second.asString().c_str();
        qmf->getQueueHeaders(name);
    }
}


// show the open dialog box
void QView::on_actionOpen_triggered()
{
    if (openDialog) {
        openDialog->show();
    }
}

void QView::on_actionRefresh_toggled(bool checked)
{
    if (checked) {
        actionRefresh->setText(QString(tr("Paused")));
        actionRefresh->setToolTip(QString(tr("Click to resume page updates")));
    } else {
        actionRefresh->setText(QString(tr("Refreshing")));
        actionRefresh->setToolTip(QString(tr("Click to pause page updates")));
    }
}

bool QView::isPaused()
{
    return actionRefresh->isChecked();
}

// A queue row was just highlighted
void QView::queueSelected() {
    // enable the buttons and menus
    menuActions->setEnabled(true);
    queueToolBar->setEnabled(true);
    messageToolBar->setEnabled(true);

    // clear out the header data from the tree view
    headerModel->clear();

    // call the broker to get the list of headers for the selected queue
    getHeaderIds();
}

// a batch of queues were just added
void QView::doneAddingQueues(uint correlator)
{
    // The first time queues are added, there is no selection in queue table, so select one
    if (!itemSelector->hasSelection()) {
        tableView_object->selectRow(0);
        // and adjust the size of the name column
        tableView_object->resizeColumnToContents(0);
    }

/*
    // If the selected queue has messages, enable the export action
    QVariant depth = tableView_object->selectedQueueDepth(queueModel, queueProxyModel);
    if (depth.toInt() > 0)
        actionCopy_Messages->setEnabled(true);
    else
        actionCopy_Messages->setEnabled(false);
*/

    // Force a refresh of the queue display to make sure we see the changes
    queueModel->refresh(correlator);
}

// SLOT: called when a queue is highlighted (mouse or keyboard)
// Asks qmf to get the message headers for the selected queue
void QView::getHeaderIds()
{
    if (tableView_object->hasSelected()) {
        QString name = tableView_object->selectedQueueName(queueModel, queueProxyModel);
        qmf->getQueueHeaders(name);
    }
}

// The text in the filter edit box was changed
void QView::on_lineEdit_queue_filter_textChanged(QString filter)
{
    queueProxyModel->setFilterRegExp(QRegExp(filter, Qt::CaseInsensitive,
                                        QRegExp::FixedString));
}

// SLOT: Show the about dialog box
void QView::showAbout() {
    DialogAbout* about = new DialogAbout(this);
    about->show();
}

// SLOT: Show the purge dialog box
void QView::showPurge()
{
    QString name = tableView_object->selectedQueueName(queueModel, queueProxyModel);
    if (!name.isEmpty()) {
        purgeDialog->setQueueName(name);
        purgeDialog->show();
    }
}

// SLOT: Show the copy dialog box
void QView::showCopy()
{
    QString name = tableView_object->selectedQueueName(queueModel, queueProxyModel);
    if (!name.isEmpty()) {
        //copyDialog->setQueueName(name);
        copyDialog->show();
    }
}

// SLOT: Show the current message body
void QView::gotBody(const qmf::ConsoleEvent &event, const qpid::types::Variant::Map &args, const QModelIndex& index)
{
    QString body;
    std::string contentType;
    qpid::types::Variant::Map::const_iterator iter = args.find("ContentType");
    if (iter != args.end())
        contentType = iter->second.asString();

    const qpid::types::Variant::Map& results(event.getArguments());
    iter = results.find("body");
    if (iter != results.end()) {
        body = decodeBody(iter->second, contentType);
        headerModel->setBodyText(index, body);
    }
}

QString QView::decodeBody(const qpid::types::Variant& var, const std::string& contentType)
{
    QString body;
    if (contentType == "amqp/map") {

        qpid::messaging::Message message;
        message.setContent(var.asString());
        message.setContentType(contentType);

        qpid::types::Variant::Map bodyMap;
        qpid::messaging::decode(message, bodyMap);

        std::stringstream bodyStream;
        bodyStream << bodyMap;
        body = QString(bodyStream.str().c_str());
    } else if (contentType == "amqp/list") {
        body = QString("TODO: decode the list");
    } else {
        body = QString(var.asString().c_str());
    }
    return body;
}

// SLOT: The purge dialog was accepted. Send the request and update the display
void QView::queuePurge(uint count)
{
    if (tableView_object->hasSelected()) {
        qmf::Agent agent = tableView_object->selectedQueueAgent(queueModel, queueProxyModel);
        const qmf::DataAddr& dataAddr = tableView_object->selectedQueueDataAddr(queueModel, queueProxyModel);

        qpid::types::Variant::Map map;
        map["request"] = count;
        // syncronous call
        agent.callMethod("purge", map, dataAddr);

        // reget the queue's data
        // clear out the header data from the tree view
        headerModel->clear();
        getHeaderIds();
    }
}

// SLOT: The copy dialog was accepted. Copy the message headers/body
void QView::queueCopy(const QString& file)
{
    QString xml = exportQueue();
    if (file.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(xml);
    } else {
        QFile f(file);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(xml.toStdString().c_str());
            f.close();
        }
    }
}

QString QView::exportQueue()
{
    std::stringstream buff;
    const qmf::Data& queue(tableView_object->selectedQueue(queueModel, queueProxyModel));
    qpid::types::Variant::Map::const_iterator iter;
    const qpid::types::Variant::Map& attrs(queue.getProperties());

    // treat the queue name property as an attribute in the xml file
    buff << "<queue";
    iter = attrs.find("name");
    if (iter != attrs.end()) {
        buff << " name=\"" << iter->second.asString() << "\"";
    }
    buff << ">\n";

    // export all the other properties
    buff << queue;

    // export all the message properties
    QString messages = exportAllMessages();
    buff << messages.toStdString();

    buff << "</queue>\n";

    return QString(buff.str().c_str());
}

QString QView::exportAllMessages()
{
    std::stringstream buff;
    QString body;
    std::string contentType;

    const IndexList& messages(headerModel->getMessageHeaderList());
    buff << " <messages>\n";

    IndexList::const_iterator iter;
    iter = messages.begin();
    while (iter != messages.end()) {

        buff << "  <message sequence=\"" << (**iter).messageId << "\">\n";

        // add all the message header attributes
        buff << **iter;

        // add the body
        qpid::types::Variant::Map::const_iterator i;
        const qpid::types::Variant::Map& m1((**iter).event.getArguments());
        i = m1.begin();
        const qpid::types::Variant::Map& m(i->second.asMap());
        i = m.find("ContentType");
        if (i != m.end())
            contentType = i->second.asString();
        body = exportMessageBody(**iter, contentType);
        buff << "   <body>";
        buff << body.toStdString();
        buff << "</body>\n";

        buff << "  </message>\n";
        iter++;
    }

    buff << " </messages>\n";
    return QString(buff.str().c_str());
}

QString QView::exportMessageBody(const MessageIndex& header, const std::string& contentType)
{
    qmf::ConsoleEvent bodyEvent = qmf->fetchBody(header.args);
    if (bodyEvent.getType() == qmf::CONSOLE_METHOD_RESPONSE) {
        const qpid::types::Variant::Map& results(bodyEvent.getArguments());
        qpid::types::Variant::Map::const_iterator iter = results.find("body");
        if (iter != results.end()) {
            return decodeBody(iter->second, contentType);
        }
    }
    return QString();
}

// SLOT: Show/Hide the Connection toolbar
void QView::toggleConnectionToolbar(bool on) {
    if (on) {
        connectionToolBar->show();
    } else {
        connectionToolBar->hide();
    }
    actionConnection_toolbar->setChecked(on);
}

// SLOT: Show/Hide the Message toolbar
void QView::toggleMessageToolbar(bool on) {
    if (on) {
        messageToolBar->show();
    } else {
        messageToolBar->hide();
    }
    actionMessage_toolbar->setChecked(on);
}

// SLOT: Show/Hide the Queue toolbar
void QView::toggleQueueToolbar(bool on) {
    if (on) {
        queueToolBar->show();
    } else {
        queueToolBar->hide();
    }
    actionQueue_toolbar->setChecked(on);
}

// SLOT: Show the qmf exception that just happened
void QView::qmfException(const QString &exception)
{
    label_exception_status->setText(exception);
    label_exception_status->setStyleSheet("color: red; font-weight:bold; padding-right:1em;");
}

// SLOT: Reset the qmf exception status
void QView::qmfExceptionClear()
{
    label_exception_status->setText(tr("None"));
    label_exception_status->setStyleSheet("color: green; padding-right:1em;");
}

QView::~QView()
{
    // save the window size and location
    QSettings settings;
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("mainWindowState", saveState());

    qmf->cancel();
    qmf->wait();
    delete qmf;
    if (openDialog)
        delete openDialog;
    if (purgeDialog)
        delete purgeDialog;
    if (copyDialog)
        delete copyDialog;
    delete headerPopupMenu;
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QMainWindow *window = new QMainWindow;
    QView qe(window);

    qe.show();
    qe.init(argc, argv);

    return app.exec();
}

