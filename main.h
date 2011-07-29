#ifndef _qe_main_h
#define _qe_main_h
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

#include <QtGui>
#include "ui_qview_main.h"
#include "dialogopen.h"
#include "dialogabout.h"
#include "dialogpurge.h"
#include "qmf-thread.h"
#include "model-header.h"
#include "model-queue.h"

class QView : public QMainWindow, private Ui::MainWindow {
    Q_OBJECT

public:
    QView(QMainWindow* parent = 0);
    ~QView();

    void init(int argc, char *argv[]);
    bool isPaused();

    QLabel *label_connection_prompt;
    QLabel *label_connection_status;

public slots:
    void queueSelected();
    void showAbout();
    void showPurge();
    void toggleConnectionToolbar(bool);
    void toggleQueueToolbar(bool);
    void toggleMessageToolbar(bool);
    void queuesAdded();
    void showBody(const QModelIndex&, const qmf::ConsoleEvent&, const qpid::types::Variant::Map&);
    void headerCtxMenu(const QPoint&);
    void messageDelete();
    void queuePurge(uint);
    void getHeaderIds();
    void messageRemoved(const qmf::ConsoleEvent& event, const qpid::types::Variant::Map& callArgs);

private:
    typedef enum { REFRESH_NORMAL, REFRESH_PAUSED, REFRESH_STOPPED } RefreshState;

    QmfThread* qmf;

    HeaderModel* headerModel;
    QueueTableModel* queueModel;
    QSortFilterProxyModel* queueProxyModel;
    QItemSelectionModel* itemSelector;

    DialogOpen* openDialog;
    DialogPurge* purgeDialog;

    void createToolBars();
    void setupStatusBar();

    QToolBar *connectionToolBar;
    QToolBar *queueToolBar;
    QToolBar *messageToolBar;
    QToolButton *refreshButton;
    QMenu *headerPopupMenu;


private slots:
    void on_lineEdit_queue_filter_textChanged(QString );
    void on_actionOpen_triggered();
    void on_actionRefresh_toggled(bool);

};

#endif
