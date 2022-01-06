#include "zenomainwindow.h"
#include <comctrl/zenodockwidget.h>
#include "nodesview/znodeseditwidget.h"
#include "panel/zenodatapanel.h"
#include "timeline/ztimeline.h"
#include "tmpwidgets/ztoolbar.h"


ZenoMainWindow::ZenoMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    init();
}

void ZenoMainWindow::init()
{
    initMenu();
    initDocks();
    houdiniStyleLayout();
    //readHoudiniStyleLayout();
}

void ZenoMainWindow::initMenu()
{
    QMenuBar *pMenuBar = new QMenuBar(this);
    if (!pMenuBar)
        return;

    pMenuBar->setMaximumHeight(26);//todo: sizehint

    QMenu *pFile = new QMenu(tr("File"));
    {
        QAction *pAction = new QAction(tr("New"), pFile);
        pAction->setCheckable(false);
        pFile->addAction(pAction);

        pAction = new QAction(tr("Open"), pFile);
        pAction->setCheckable(false);
        pFile->addAction(pAction);

        pAction = new QAction(tr("Save"), pFile);
        pAction->setCheckable(false);
        pFile->addAction(pAction);

        pAction = new QAction(tr("Quit"), pFile);
        pAction->setCheckable(false);
        pFile->addAction(pAction);
    }

    QMenu *pEdit = new QMenu(tr("Edit"));
    {
        QAction *pAction = new QAction(tr("Undo"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Redo"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Cut"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Copy"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);

        pAction = new QAction(tr("Paste"), pEdit);
        pAction->setCheckable(false);
        pEdit->addAction(pAction);
    }

    QMenu *pRender = new QMenu(tr("Render"));

    QMenu *pView = new QMenu(tr("View"));

    QMenu *pWindow = new QMenu(tr("Window"));

    QMenu *pHelp = new QMenu(tr("Help"));

    pMenuBar->addMenu(pFile);
    pMenuBar->addMenu(pEdit);
    pMenuBar->addMenu(pRender);
    pMenuBar->addMenu(pView);
    pMenuBar->addMenu(pWindow);
    pMenuBar->addMenu(pHelp);

    setMenuBar(pMenuBar);
}

void ZenoMainWindow::initDocks()
{
    QWidget *p = takeCentralWidget();
    if (p)
        delete p;

    setDockNestingEnabled(true);

    m_shapeBar = new ZenoDockWidget(this);
    m_shapeBar->setObjectName(QString::fromUtf8("dock_shape"));
    m_shapeBar->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_shapeBar->setWidget(new ZShapeBar);

    m_toolbar = new ZenoDockWidget(this);
    m_toolbar->setObjectName(QString::fromUtf8("dock_toolbar"));
    m_toolbar->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_toolbar->setWidget(new ZToolbar);

    m_view = new ZenoDockWidget("view", this);
    m_view->setObjectName(QString::fromUtf8("dock_view"));
    m_view->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_view->setWidget(new QWidget);

    m_parameter = new ZenoDockWidget("parameter", this);
    m_parameter->setObjectName(QString::fromUtf8("dock_parameter"));
    m_parameter->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_parameter->setWidget(new QWidget);

    m_data = new ZenoDockWidget("data", this);
    m_data->setObjectName(QString::fromUtf8("dock_data"));
    m_data->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_data->setWidget(new ZenoDataPanel);

    m_editor = new ZenoDockWidget("", this);
    m_editor->setObjectName(QString::fromUtf8("dock_editor"));
    m_editor->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    m_editor->setWidget(new ZNodesEditWidget);

    m_timeline = new ZenoDockWidget(this);
    m_timeline->setObjectName(QString::fromUtf8("dock_timeline"));
    m_timeline->setFeatures(QDockWidget::NoDockWidgetFeatures);
    m_timeline->setWidget(new ZTimeline);
}

void ZenoMainWindow::houdiniStyleLayout()
{
    addDockWidget(Qt::TopDockWidgetArea, m_shapeBar);
    splitDockWidget(m_shapeBar, m_toolbar, Qt::Vertical);
    splitDockWidget(m_toolbar, m_timeline, Qt::Vertical);

    splitDockWidget(m_toolbar, m_view, Qt::Horizontal);
    splitDockWidget(m_view, m_editor, Qt::Horizontal);
    splitDockWidget(m_view, m_data, Qt::Vertical);
    splitDockWidget(m_data, m_parameter, Qt::Horizontal);

    writeHoudiniStyleLayout();
}

void ZenoMainWindow::arrangeDocks2()
{
    addDockWidget(Qt::LeftDockWidgetArea, m_toolbar);
    splitDockWidget(m_toolbar, m_view, Qt::Horizontal);
    splitDockWidget(m_view, m_parameter, Qt::Horizontal);

    splitDockWidget(m_view, m_timeline, Qt::Vertical);
    splitDockWidget(m_parameter, m_data, Qt::Vertical);
    splitDockWidget(m_data, m_editor, Qt::Vertical);
    writeSettings2();
}

void ZenoMainWindow::arrangeDocks3()
{
    addDockWidget(Qt::TopDockWidgetArea, m_parameter);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolbar);
    addDockWidget(Qt::BottomDockWidgetArea, m_timeline);
    splitDockWidget(m_toolbar, m_view, Qt::Horizontal);
    splitDockWidget(m_view, m_data, Qt::Vertical);
    splitDockWidget(m_data, m_editor, Qt::Vertical);
    writeHoudiniStyleLayout();
}

void ZenoMainWindow::writeHoudiniStyleLayout()
{
    QSettings settings("Zeno Inc.", "zeno2 ui1");
    settings.beginGroup("mainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();
}

void ZenoMainWindow::writeSettings2()
{
    QSettings settings("Zeno Inc.", "zeno2 ui2");
    settings.beginGroup("mainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();
}

void ZenoMainWindow::readHoudiniStyleLayout()
{
    QSettings settings("Zeno Inc.", "zeno2 ui1");
    settings.beginGroup("mainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();
}

void ZenoMainWindow::readSettings2()
{
    QSettings settings("Zeno Inc.", "zeno2 ui2");
    settings.beginGroup("mainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();
}