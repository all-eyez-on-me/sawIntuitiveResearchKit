/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  Author(s):  Anton Deguet
  Created on: 2013-08-24

  (C) Copyright 2013-2017 Johns Hopkins University (JHU), All Rights Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/


// system include
#include <iostream>

// Qt include
#include <QString>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollBar>
#include <QCloseEvent>
#include <QCoreApplication>

// cisst
#include <cisstMultiTask/mtsInterfaceRequired.h>
#include <cisstParameterTypes/prmPositionJointGet.h>

#include <sawIntuitiveResearchKit/mtsIntuitiveResearchKitArmQtWidget.h>


CMN_IMPLEMENT_SERVICES_DERIVED_ONEARG(mtsIntuitiveResearchKitArmQtWidget, mtsComponent, std::string);

mtsIntuitiveResearchKitArmQtWidget::mtsIntuitiveResearchKitArmQtWidget(const std::string & componentName, double periodInSeconds):
    mtsComponent(componentName),
    TimerPeriodInMilliseconds(periodInSeconds * 1000),
    DirectControl(false),
    LogEnabled(false)
{
    QMMessage = new mtsMessageQtWidget();

    // Setup CISST Interface
    InterfaceRequired = AddInterfaceRequired("Manipulator");
    if (InterfaceRequired) {
        QMMessage->SetInterfaceRequired(InterfaceRequired);
        InterfaceRequired->AddFunction("GetPositionCartesian", Arm.GetPositionCartesian);
        InterfaceRequired->AddFunction("GetRobotControlState", Arm.GetRobotControlState);
        InterfaceRequired->AddFunction("SetRobotControlState", Arm.SetRobotControlState);
        InterfaceRequired->AddFunction("GetPeriodStatistics", Arm.GetPeriodStatistics);
    }
}

void mtsIntuitiveResearchKitArmQtWidget::Configure(const std::string &filename)
{
    CMN_LOG_CLASS_INIT_VERBOSE << "Configure: " << filename << std::endl;
}

void mtsIntuitiveResearchKitArmQtWidget::Startup(void)
{
    setupUi();
    startTimer(TimerPeriodInMilliseconds); // ms
    if (!LogEnabled) {
        QMMessage->hide();
    }
    if (!parent()) {
        show();
    }
}

void mtsIntuitiveResearchKitArmQtWidget::Cleanup(void)
{
    this->hide();
}

void mtsIntuitiveResearchKitArmQtWidget::closeEvent(QCloseEvent * event)
{
    int answer = QMessageBox::warning(this, tr("mtsIntuitiveResearchKitArmQtWidget"),
                                      tr("Do you really want to quit this application?"),
                                      QMessageBox::No | QMessageBox::Yes);
    if (answer == QMessageBox::Yes) {
        event->accept();
        QCoreApplication::exit();
    } else {
        event->ignore();
    }
}

void mtsIntuitiveResearchKitArmQtWidget::timerEvent(QTimerEvent * CMN_UNUSED(event))
{
    // make sure we should update the display
    if (this->isHidden()) {
        return;
    }

    mtsExecutionResult executionResult;
    executionResult = Arm.GetPositionCartesian(Position);
    if (!executionResult) {
        CMN_LOG_CLASS_RUN_ERROR << "Manipulator.GetPositionCartesian failed, \""
                                << executionResult << "\"" << std::endl;
    }
    QFRPositionWidget->SetValue(Position.Position());

    std::string state;
    Arm.GetRobotControlState(state);
    QLEState->setText(state.c_str());

    Arm.GetPeriodStatistics(IntervalStatistics);
    QMIntervalStatistics->SetValue(IntervalStatistics);

    // for derived classes
    this->timerEventDerived();
}

void mtsIntuitiveResearchKitArmQtWidget::SlotLogEnabled(void)
{
    LogEnabled = QPBLog->isChecked();
    if (LogEnabled) {
        QMMessage->show();
    } else {
        QMMessage->hide();
    }
}

void mtsIntuitiveResearchKitArmQtWidget::SlotEnableDirectControl(bool toggle)
{
    DirectControl = toggle;
    QPBHome->setEnabled(toggle);
}

void mtsIntuitiveResearchKitArmQtWidget::SlotHome(void)
{
    Arm.SetRobotControlState(std::string("Home"));
}

void mtsIntuitiveResearchKitArmQtWidget::setupUi(void)
{
    MainLayout = new QVBoxLayout;

    QHBoxLayout * topLayout = new QHBoxLayout;
    MainLayout->addLayout(topLayout);

    // 3D position
    QFRPositionWidget = new vctQtWidgetFrameDoubleRead(vctQtWidgetRotationDoubleRead::OPENGL_WIDGET);
    topLayout->addWidget(QFRPositionWidget, 0, 0);

    // timing
    QVBoxLayout * timingLayout = new QVBoxLayout();
    QMIntervalStatistics = new mtsQtWidgetIntervalStatistics();
    timingLayout->addWidget(QMIntervalStatistics);
    timingLayout->addStretch();
    topLayout->addLayout(timingLayout);

    // state
    QHBoxLayout * stateLayout = new QHBoxLayout;
    MainLayout->addLayout(stateLayout);

    // messages on/off
    QPBLog = new QPushButton("Messages");
    QPBLog->setCheckable(true);
    stateLayout->addWidget(QPBLog);
    QCBEnableDirectControl = new QCheckBox("Direct control");
    stateLayout->addWidget(QCBEnableDirectControl);
    QPBHome = new QPushButton("Home");
    stateLayout->addWidget(QPBHome);
    QLabel * stateLabel = new QLabel("State:");
    stateLayout->addWidget(stateLabel);
    QLEState = new QLineEdit("");
    QLEState->setReadOnly(true);
    stateLayout->addWidget(QLEState);

    // for derived classes
    this->setupUiDerived();

    // messages
    QMMessage->setupUi();
    MainLayout->addWidget(QMMessage);

    setLayout(MainLayout);
    setWindowTitle("Manipulator");
    resize(sizeHint());

    connect(QPBLog, SIGNAL(clicked()),
            this, SLOT(SlotLogEnabled()));
    connect(QCBEnableDirectControl, SIGNAL(toggled(bool)),
            this, SLOT(SlotEnableDirectControl(bool)));
    connect(QPBHome, SIGNAL(clicked()),
            this, SLOT(SlotHome()));

    // set initial values
    QCBEnableDirectControl->setChecked(DirectControl);
    SlotEnableDirectControl(DirectControl);
}
