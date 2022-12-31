#include "zeditparamlayoutdlg.h"
#include "ui_zeditparamlayoutdlg.h"
#include "zassert.h"
#include <zenomodel/include/uihelper.h>
#include "zmapcoreparamdlg.h"
#include <zenomodel/include/uihelper.h>
#include "zenoapplication.h"
#include <zenomodel/include/graphsmanagment.h>
#include <zenomodel/include/iparammodel.h>
#include <zenomodel/include/nodesmgr.h>
#include <zenoui/comctrl/zwidgetfactory.h>
#include <zenomodel/include/globalcontrolmgr.h>
#include "variantptr.h"
#include <zenomodel/include/command.h>


static CONTROL_ITEM_INFO controlList[] = {
    {"Integer",             CONTROL_INT,            "int"},
    {"Float",               CONTROL_FLOAT,          "float"},
    {"String",              CONTROL_STRING,         "string"},
    {"Boolean",             CONTROL_BOOL,           "bool"},
    {"Multiline String",    CONTROL_MULTILINE_STRING, "string"},
    {"read path",           CONTROL_READPATH,       "string"},
    {"write path",          CONTROL_WRITEPATH,      "string"},
    {"Enum",                CONTROL_ENUM,           "string"},
    {"Float Vector 4",      CONTROL_VEC4_FLOAT,     "vec4f"},
    {"Float Vector 3",      CONTROL_VEC3_FLOAT,     "vec3f"},
    {"Float Vector 2",      CONTROL_VEC2_FLOAT,     "vec2f"},
    {"Integer Vector 4",    CONTROL_VEC4_INT,       "vec4i"},
    {"Integer Vector 3",    CONTROL_VEC3_INT,       "vec3i"},
    {"Integer Vector 2",    CONTROL_VEC2_INT,       "vec2i"},
    {"Color",               CONTROL_COLOR,          "color"},
    {"Curve",               CONTROL_CURVE,          "curve"},
    {"SpinBox",             CONTROL_HSPINBOX,       "int"},
    {"Slider",              CONTROL_HSLIDER,        "int"},
    {"SpinBoxSlider",       CONTROL_SPINBOX_SLIDER, "int"},
};

static CONTROL_ITEM_INFO getControl(PARAM_CONTROL ctrl)
{
    for (int i = 0; i < sizeof(controlList) / sizeof(CONTROL_ITEM_INFO); i++)
    {
        if (controlList[i].ctrl == ctrl)
        {
            return controlList[i];
        }
    }
    return CONTROL_ITEM_INFO();
}

static CONTROL_ITEM_INFO getControlByName(const QString& name)
{
    for (int i = 0; i < sizeof(controlList) / sizeof(CONTROL_ITEM_INFO); i++)
    {
        if (controlList[i].name == name)
        {
            return controlList[i];
        }
    }
    return CONTROL_ITEM_INFO();
}


ParamTreeItemDelegate::ParamTreeItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

ParamTreeItemDelegate::~ParamTreeItemDelegate()
{
}

QWidget* ParamTreeItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    bool bEditable = index.data(ROLE_VAPRAM_EDITTABLE).toBool();
    //if (!bEditable)
    //    return nullptr;
    return QStyledItemDelegate::createEditor(parent, option, index);
}



ZEditParamLayoutDlg::ZEditParamLayoutDlg(QStandardItemModel* pModel, bool bNodeUI, const QPersistentModelIndex& nodeIdx, IGraphsModel* pGraphsModel, QWidget* parent)
    : QDialog(parent)
    , m_model(nullptr)
    , m_proxyModel(nullptr)
    , m_nodeIdx(nodeIdx)
    , m_pGraphsModel(pGraphsModel)
    , m_bSubgraphNode(false)
{
    m_ui = new Ui::EditParamLayoutDlg;
    m_ui->setupUi(this);

    QStringList lstCtrls = {
        "Tab",
        "Group"
    };
    for (int i = 0; i < sizeof(controlList) / sizeof(CONTROL_ITEM_INFO); i++)
    {
        lstCtrls.append(controlList[i].name);
        m_ui->cbControl->addItem(controlList[i].name);
    }

    m_ui->listConctrl->addItems(lstCtrls);
    m_ui->cbTypes->addItems(UiHelper::getCoreTypeList());

    m_model = qobject_cast<ViewParamModel*>(pModel);
    ZASSERT_EXIT(m_model);

    m_bSubgraphNode = m_pGraphsModel->IsSubGraphNode(m_nodeIdx) && m_model->isNodeModel();

    m_proxyModel = new ViewParamModel(bNodeUI, m_model->nodeIdx(), m_pGraphsModel, this);
    m_proxyModel->clone(m_model);

    m_ui->paramsView->setModel(m_proxyModel);
    m_ui->paramsView->setItemDelegate(new ParamTreeItemDelegate(m_ui->paramsView));

    QItemSelectionModel* selModel = m_ui->paramsView->selectionModel();
    QModelIndex selIdx = selModel->currentIndex();
    const QModelIndex& wtfIdx = m_proxyModel->index(0, 0);
    selModel->setCurrentIndex(wtfIdx, QItemSelectionModel::SelectCurrent);
    m_ui->paramsView->expandAll();

    connect(selModel, SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(onTreeCurrentChanged(const QModelIndex&, const QModelIndex&)));
    connect(m_ui->editName, SIGNAL(editingFinished()), this, SLOT(onNameEditFinished()));
    connect(m_ui->btnAdd, SIGNAL(clicked()), this, SLOT(onBtnAdd()));
    connect(m_ui->btnApply, SIGNAL(clicked()), this, SLOT(onApply()));
    connect(m_ui->btnOk, SIGNAL(clicked()), this, SLOT(onOk()));
    connect(m_ui->btnCancel, SIGNAL(clicked()), this, SLOT(onCancel()));

    connect(m_proxyModel, SIGNAL(editNameChanged(const QModelIndex&, const QString&, const QString&)),
            this, SLOT(onProxyItemNameChanged(const QModelIndex&, const QString&, const QString&)));

    QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), m_ui->paramsView);
    connect(shortcut, SIGNAL(activated()), this, SLOT(onParamTreeDeleted()));

    connect(m_ui->btnChooseCoreParam, SIGNAL(clicked(bool)), this, SLOT(onChooseParamClicked()));
    connect(m_ui->editMin, SIGNAL(editingFinished()), this, SLOT(onMinEditFinished()));
    connect(m_ui->editMax, SIGNAL(editingFinished()), this, SLOT(onMaxEditFinished()));
    connect(m_ui->editStep, SIGNAL(editingFinished()), this, SLOT(onStepEditFinished()));
    connect(m_ui->cbControl, SIGNAL(currentIndexChanged(int)), this, SLOT(onControlItemChanged(int)));
    connect(m_ui->cbTypes, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeItemChanged(int)));

    m_ui->itemsTable->setHorizontalHeaderLabels({ tr("Item Name") });
    connect(m_ui->itemsTable, SIGNAL(cellChanged(int, int)), this, SLOT(onComboTableItemsCellChanged(int, int)));
}

void ZEditParamLayoutDlg::onComboTableItemsCellChanged(int row, int column)
{
    //dump to item.
    QModelIndex layerIdx = m_ui->paramsView->currentIndex();
    if (!layerIdx.isValid() && layerIdx.data(ROLE_VPARAM_TYPE) != VPARAM_PARAM)
        return;

    QStringList lst;
    for (int r = 0; r < m_ui->itemsTable->rowCount(); r++)
    {
        QTableWidgetItem* pItem = m_ui->itemsTable->item(r, 0);
        if (pItem && !pItem->text().isEmpty())
            lst.append(pItem->text());
    }
    if (lst.isEmpty())
        return;

    CONTROL_PROPERTIES properties = layerIdx.data(ROLE_VPARAM_CTRL_PROPERTIES).value<CONTROL_PROPERTIES>();
    properties["items"] = lst;

    proxyModelSetData(layerIdx, properties, ROLE_VPARAM_CTRL_PROPERTIES);
    proxyModelSetData(layerIdx, lst[0], ROLE_PARAM_VALUE);
    if (row == m_ui->itemsTable->rowCount() - 1)
    {
        m_ui->itemsTable->insertRow(m_ui->itemsTable->rowCount());
    }
}

void ZEditParamLayoutDlg::proxyModelSetData(const QModelIndex& index, const QVariant& newValue, int role)
{
    //record this action first.
    const QString& objPath = index.data(ROLE_OBJPATH).toString();
    ViewParamSetDataCommand *pCommand =
        new ViewParamSetDataCommand(m_pGraphsModel, objPath, newValue, ROLE_VPARAM_CTRL_PROPERTIES);
    m_commandSeq.append(pCommand);
    m_proxyModel->setData(index, newValue, role);
}

void ZEditParamLayoutDlg::onParamTreeDeleted()
{
    QModelIndex idx = m_ui->paramsView->currentIndex();
    if (!idx.isValid() || !idx.parent().isValid())
        return;

    QString parentPath = idx.parent().data(ROLE_OBJPATH).toString();
    ViewParamRemoveCommand *pCommand = new ViewParamRemoveCommand(m_pGraphsModel, parentPath, idx.row());
    m_commandSeq.append(pCommand);

    m_proxyModel->removeRow(idx.row(), idx.parent());
}

void ZEditParamLayoutDlg::onTreeCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    VParamItem* pCurrentItem = static_cast<VParamItem*>(m_proxyModel->itemFromIndex(current));
    if (!pCurrentItem)  return;

    const QString& name = pCurrentItem->data(ROLE_VPARAM_NAME).toString();
    m_ui->editName->setText(name);
    bool bEditable = pCurrentItem->data(ROLE_VAPRAM_EDITTABLE).toBool();
    //m_ui->editName->setEnabled(bEditable);

    //delete old control.
    QLayoutItem* pLayoutItem = m_ui->gridLayout->itemAtPosition(rowValueControl, 1);
    if (pLayoutItem)
    {
        QWidget* pControlWidget = pLayoutItem->widget();
        delete pControlWidget;
    }

    VPARAM_TYPE type = (VPARAM_TYPE)pCurrentItem->data(ROLE_VPARAM_TYPE).toInt();
    if (type == VPARAM_TAB)
    {
        m_ui->cbControl->setEnabled(false);
        m_ui->cbTypes->setEnabled(false);
        m_ui->stackProperties->setCurrentIndex(0);
    }
    else if (type == VPARAM_GROUP)
    {
        m_ui->cbControl->setEnabled(false);
        m_ui->cbTypes->setEnabled(false);
        m_ui->stackProperties->setCurrentIndex(0);
    }
    else if (type == VPARAM_PARAM)
    {
        const QString& paramName = name;
        PARAM_CONTROL ctrl = pCurrentItem->m_info.control;
        const QString& ctrlName = getControl(ctrl).name;
        const QString& dataType = pCurrentItem->m_info.typeDesc;
        QVariant deflVal = pCurrentItem->m_info.value;
        bool bCoreParam = pCurrentItem->data(ROLE_VPARAM_IS_COREPARAM).toBool();
        QVariant controlProperties = pCurrentItem->data(ROLE_VPARAM_CTRL_PROPERTIES);

        {
            BlockSignalScope scope(m_ui->cbControl);
            BlockSignalScope scope2(m_ui->cbTypes);

            m_ui->cbControl->setEnabled(true);
            m_ui->cbControl->clear();
            QStringList items = UiHelper::getControlLists(dataType);
            m_ui->cbControl->addItems(items);
            m_ui->cbControl->setCurrentText(ctrlName);

            m_ui->cbTypes->setCurrentText(dataType);
            m_ui->cbTypes->setEnabled(!bCoreParam || m_pGraphsModel->IsSubGraphNode(m_nodeIdx));
        }

        CallbackCollection cbSets;
        cbSets.cbEditFinished = [=](QVariant newValue) {
            proxyModelSetData(pCurrentItem->index(), newValue, ROLE_PARAM_VALUE);
        };
        if (!deflVal.isValid())
            deflVal = UiHelper::initVariantByControl(ctrl);

        QWidget* valueControl = zenoui::createWidget(deflVal, ctrl, dataType, cbSets, controlProperties);
        if (valueControl)
        {
            valueControl->setEnabled(m_pGraphsModel->IsSubGraphNode(m_nodeIdx));
            m_ui->gridLayout->addWidget(valueControl, rowValueControl, 1);
        }

        const QString& coreName = pCurrentItem->data(ROLE_PARAM_NAME).toString();

        m_ui->editCoreParamName->setText(coreName);
        m_ui->editCoreParamType->setText(dataType);
        m_ui->itemsTable->setRowCount(0);

        if (ctrl == CONTROL_ENUM)
        {
            CONTROL_PROPERTIES pros = pCurrentItem->data(ROLE_VPARAM_CTRL_PROPERTIES).value<CONTROL_PROPERTIES>();
            if (pros.find("items") != pros.end())
            {
                const QStringList& items = pros["items"].toStringList();
                m_ui->itemsTable->setRowCount(items.size() + 1);
                for (int r = 0; r < items.size(); r++)
                {
                    QTableWidgetItem* newItem = new QTableWidgetItem(items[r]);
                    m_ui->itemsTable->setItem(r, 0, newItem);
                }
            }
            else
            {
                m_ui->itemsTable->setRowCount(1);
            }
            m_ui->stackProperties->setCurrentIndex(1);
        }
        else if (ctrl == CONTROL_HSLIDER ||
                 ctrl == CONTROL_HSPINBOX ||
                 ctrl == CONTROL_SPINBOX_SLIDER)
        {
            m_ui->stackProperties->setCurrentIndex(2);
            bool bIntVal = dataType == "int";
            SLIDER_INFO sliderInfo = controlProperties.value<SLIDER_INFO>();

            QVariant step = sliderInfo.step;
            m_ui->editStep->setText(QString::number(bIntVal ? step.toInt() : step.toFloat()));

            QVariant min = sliderInfo.min;
            m_ui->editMin->setText(QString::number(bIntVal ? min.toInt() : min.toFloat()));

            QVariant max = sliderInfo.max;
            m_ui->editMax->setText(QString::number(bIntVal ? max.toInt() : max.toFloat()));
        }
        else
        {
            m_ui->stackProperties->setCurrentIndex(0);
        }
    }
}

void ZEditParamLayoutDlg::onBtnAdd()
{
    QModelIndex ctrlIdx = m_ui->listConctrl->currentIndex();
    if (!ctrlIdx.isValid())
        return;

    QModelIndex layerIdx = m_ui->paramsView->currentIndex();
    if (!layerIdx.isValid())
        return;

    QString ctrlName = ctrlIdx.data().toString();
    VPARAM_TYPE type = (VPARAM_TYPE)layerIdx.data(ROLE_VPARAM_TYPE).toInt();
    VParamItem* pItem = static_cast<VParamItem*>(m_proxyModel->itemFromIndex(layerIdx));
    ZASSERT_EXIT(pItem);
    QStringList existNames;
    for (int r = 0; r < pItem->rowCount(); r++)
    {
        QStandardItem* pChildItem = pItem->child(r);
        ZASSERT_EXIT(pChildItem);
        QString _name = pChildItem->data(ROLE_VPARAM_NAME).toString();
        existNames.append(_name);
    }

    if (ctrlName == "Tab")
    {
        if (type != VPARAM_ROOT)
        {
            QMessageBox::information(this, "Error", "create tab needs to place under the root");
            return;
        }
        QString newTabName = UiHelper::getUniqueName(existNames, "Tab");
        VParamItem* pNewItem = new VParamItem(VPARAM_TAB, newTabName);
        pItem->appendRow(pNewItem);

        QString objPath = pItem->data(ROLE_OBJPATH).toString();
        VPARAM_INFO vParam = pNewItem->exportParamInfo();
        ViewParamAddCommand *pCmd = new ViewParamAddCommand(m_pGraphsModel, objPath, vParam);
        m_commandSeq.append(pCmd);
    }
    else if (ctrlName == "Group")
    {
        if (type != VPARAM_TAB)
        {
            QMessageBox::information(this, "Error ", "create group needs to place under the tab");
            return;
        }
        QString newGroup = UiHelper::getUniqueName(existNames, "Group");
        VParamItem* pNewItem = new VParamItem(VPARAM_GROUP, newGroup);
        pItem->appendRow(pNewItem);

        QString objPath = pItem->data(ROLE_OBJPATH).toString();
        VPARAM_INFO vParam = pNewItem->exportParamInfo();
        ViewParamAddCommand *pCmd = new ViewParamAddCommand(m_pGraphsModel, objPath, vParam);
        m_commandSeq.append(pCmd);
    }
    else
    {
        if (type != VPARAM_GROUP)
        {
            QMessageBox::information(this, "Error ", "create control needs to place under the group");
            return;
        }
        CONTROL_ITEM_INFO ctrl = getControlByName(ctrlName);
        QString newItem = UiHelper::getUniqueName(existNames, ctrl.name);
        VParamItem* pNewItem = new VParamItem(VPARAM_PARAM, newItem);
        pNewItem->m_info.control = ctrl.ctrl;
        pNewItem->m_info.typeDesc = ctrl.defaultType;
        pNewItem->m_info.value = UiHelper::initVariantByControl(ctrl.ctrl);
        
        //init properties.
        switch (ctrl.ctrl)
        {
            case CONTROL_SPINBOX_SLIDER:
            case CONTROL_HSPINBOX:
            case CONTROL_HSLIDER:
            {
                CONTROL_PROPERTIES properties;
                properties["step"] = 1;
                properties["min"] = 0;
                properties["max"] = 100;
                pNewItem->setData(properties, ROLE_VPARAM_CTRL_PROPERTIES);
                break;
            }
        }

        pItem->appendRow(pNewItem);

        //record some command about SubInput/SubOutput.


        QString objPath = pItem->data(ROLE_OBJPATH).toString();
        VPARAM_INFO vParam = pNewItem->exportParamInfo();
        ViewParamAddCommand *pCmd = new ViewParamAddCommand(m_pGraphsModel, objPath, vParam);
        m_commandSeq.append(pCmd);
    }
}

void ZEditParamLayoutDlg::recordSubInputCommands(bool bSubInput, VParamItem* pItem)
{
    if (!m_bSubgraphNode)
        return;

    const QString& subgName = m_nodeIdx.data(ROLE_OBJNAME).toString();
    const QModelIndex& subgIdx = m_pGraphsModel->index(subgName);

    const QString& vName = pItem->m_info.name;
    QString coreName;
    if (pItem->data(ROLE_VPARAM_IS_COREPARAM).toBool())
        coreName = pItem->data(ROLE_PARAM_NAME).toString();
    const QString& typeDesc = pItem->m_info.typeDesc;
    const QVariant& deflVal = pItem->m_info.value;
    const PARAM_CONTROL ctrl = (PARAM_CONTROL)pItem->data(ROLE_PARAM_CTRL).toInt();

    //new added param.
    const QVariant& defl = pItem->data(ROLE_PARAM_VALUE);

    QPointF pos(0, 0);
    //todo: node arrangement.

    //QString subIO_ident = NodesMgr::createNewNode(m_pGraphsModel, subgIdx, bSubInput ? "SubInput" : "SubOutput", pos);
    NODE_DATA newNodeData = NodesMgr::newNodeData(m_pGraphsModel, bSubInput ? "SubInput" : "SubOutput", pos);
    QString subIO_ident = newNodeData[ROLE_OBJID].toString();
    AddNodeCommand *pNewNodeCmd = new AddNodeCommand(subIO_ident, newNodeData, m_pGraphsModel, subgIdx);
    m_commandSeq.append(pNewNodeCmd);

    QString m_namePath, m_typePath, m_deflPath; //todo: fill the target path for SubInput/SubOutput.


    ViewParamSetDataCommand* pNameCmd = new ViewParamSetDataCommand(m_pGraphsModel, m_namePath, vName, ROLE_PARAM_VALUE);
    ViewParamSetDataCommand* pTypeCmd = new ViewParamSetDataCommand(m_pGraphsModel, m_typePath, typeDesc, ROLE_PARAM_TYPE);
    ViewParamSetDataCommand* pDeflTypeCmd = new ViewParamSetDataCommand(m_pGraphsModel, m_deflPath, typeDesc, ROLE_PARAM_TYPE);
    ViewParamSetDataCommand* pDeflValueCmd = new ViewParamSetDataCommand(m_pGraphsModel, m_deflPath, defl, ROLE_PARAM_VALUE);

    //have to bind new param idx on subgraph node.
    IParamModel* _subgnode_paramModel =
        QVariantPtr<IParamModel>::asPtr(m_nodeIdx.data(bSubInput ? ROLE_INPUT_MODEL : ROLE_OUTPUT_MODEL));
    ZASSERT_EXIT(_subgnode_paramModel);
    const QModelIndex& newParamFromItem = _subgnode_paramModel->index(vName);

    const QString& itemObjPath = pItem->data(ROLE_OBJPATH).toString();
    const QString& targetPath = newParamFromItem.data(ROLE_OBJPATH).toString();

    MapParamIndexCommand *pMappingCmd = new MapParamIndexCommand(m_pGraphsModel, itemObjPath, targetPath);


    updateSubgParamControl(m_pGraphsModel, subgName, bSubInput, vName, ctrl);
}



void ZEditParamLayoutDlg::onProxyItemNameChanged(const QModelIndex& itemIdx, const QString& oldPath, const QString& newName)
{
    ViewParamSetDataCommand *pCmd = new ViewParamSetDataCommand(m_pGraphsModel, oldPath, newName, ROLE_VPARAM_NAME);
    m_commandSeq.append(pCmd);
}

void ZEditParamLayoutDlg::onNameEditFinished()
{
    const QModelIndex& currIdx = m_ui->paramsView->currentIndex();
    if (!currIdx.isValid())
        return;

    QStandardItem* pItem = m_proxyModel->itemFromIndex(currIdx);
    QString oldPath = pItem->data(ROLE_OBJPATH).toString();
    QString newName = m_ui->editName->text();
    pItem->setData(newName, ROLE_VPARAM_NAME);
    onProxyItemNameChanged(pItem->index(), oldPath, newName);
}

void ZEditParamLayoutDlg::onLabelEditFinished()
{

}

void ZEditParamLayoutDlg::onHintEditFinished()
{

}

void ZEditParamLayoutDlg::onMinEditFinished()
{
    QModelIndex layerIdx = m_ui->paramsView->currentIndex();
    if (!layerIdx.isValid() && layerIdx.data(ROLE_VPARAM_TYPE) != VPARAM_PARAM)
        return;

    CONTROL_PROPERTIES properties = layerIdx.data(ROLE_VPARAM_CTRL_PROPERTIES).value<CONTROL_PROPERTIES>();
    int from = m_ui->editMin->text().toInt();
    properties["min"] = from;
    proxyModelSetData(layerIdx, properties, ROLE_VPARAM_CTRL_PROPERTIES);
}

void ZEditParamLayoutDlg::onMaxEditFinished()
{
    QModelIndex layerIdx = m_ui->paramsView->currentIndex();
    if (!layerIdx.isValid() && layerIdx.data(ROLE_VPARAM_TYPE) != VPARAM_PARAM)
        return;

    CONTROL_PROPERTIES properties = layerIdx.data(ROLE_VPARAM_CTRL_PROPERTIES).value<CONTROL_PROPERTIES>();
    int to = m_ui->editMax->text().toInt();
    properties["max"] = to;
    proxyModelSetData(layerIdx, properties, ROLE_VPARAM_CTRL_PROPERTIES);
}

void ZEditParamLayoutDlg::onControlItemChanged(int idx)
{
    const QString& controlName = m_ui->cbControl->itemText(idx);
    PARAM_CONTROL ctrl = UiHelper::getControlByDesc(controlName);
    QModelIndex layerIdx = m_ui->paramsView->currentIndex();
    if (!layerIdx.isValid() && layerIdx.data(ROLE_VPARAM_TYPE) != VPARAM_PARAM)
        return;

    proxyModelSetData(layerIdx, ctrl, ROLE_PARAM_CTRL);
}

void ZEditParamLayoutDlg::onTypeItemChanged(int idx)
{
    const QString& dataType = m_ui->cbTypes->itemText(idx);
    QModelIndex layerIdx = m_ui->paramsView->currentIndex();
    if (!layerIdx.isValid() && layerIdx.data(ROLE_VPARAM_TYPE) != VPARAM_PARAM)
        return;

    VParamItem* pItem = static_cast<VParamItem*>(m_proxyModel->itemFromIndex(layerIdx));
    pItem->m_info.typeDesc = dataType;
    pItem->m_info.control = UiHelper::getControlByType(dataType);
    pItem->m_info.value = UiHelper::initVariantByControl(pItem->m_info.control);

    QLayoutItem* pLayoutItem = m_ui->gridLayout->itemAtPosition(rowValueControl, 1);
    if (pLayoutItem)
    {
        QWidget* pControlWidget = pLayoutItem->widget();
        delete pControlWidget;
    }

    //update control list
    QStringList items = UiHelper::getControlLists(dataType);
    m_ui->cbControl->clear();
    m_ui->cbControl->addItems(items);
}

void ZEditParamLayoutDlg::onStepEditFinished()
{
    QModelIndex layerIdx = m_ui->paramsView->currentIndex();
    if (!layerIdx.isValid() && layerIdx.data(ROLE_VPARAM_TYPE) != VPARAM_PARAM)
        return;

    CONTROL_PROPERTIES properties = layerIdx.data(ROLE_VPARAM_CTRL_PROPERTIES).value<CONTROL_PROPERTIES>();
    int step = m_ui->editStep->text().toInt();
    properties["step"] = step;

    const QString &objPath = layerIdx.data(ROLE_OBJPATH).toString();
    ViewParamSetDataCommand *pCommand = new ViewParamSetDataCommand(m_pGraphsModel, objPath, properties, ROLE_VPARAM_CTRL_PROPERTIES);
    m_commandSeq.append(pCommand);

    m_proxyModel->setData(layerIdx, properties, ROLE_VPARAM_CTRL_PROPERTIES);
}

void ZEditParamLayoutDlg::onChooseParamClicked()
{
    ZMapCoreparamDlg dlg(m_nodeIdx);
    if (QDialog::Accepted == dlg.exec())
    {
        QModelIndex coreIdx = dlg.coreIndex();

        QModelIndex viewparamIdx = m_ui->paramsView->currentIndex();
        QStandardItem* pItem = m_proxyModel->itemFromIndex(viewparamIdx);
        VParamItem* pViewItem = static_cast<VParamItem*>(pItem);

        if (coreIdx.isValid())
        {
            pViewItem->mapCoreParam(coreIdx);

            //update control info.
            const QString& newName = coreIdx.data(ROLE_PARAM_NAME).toString();
            const QString& typeDesc = coreIdx.data(ROLE_PARAM_TYPE).toString();

            m_ui->editCoreParamName->setText(newName);
            m_ui->editCoreParamType->setText(typeDesc);

            PARAM_CONTROL newCtrl = UiHelper::getControlByType(typeDesc);
            pViewItem->setData(newCtrl, ROLE_PARAM_CTRL);
        }
        else
        {
            pViewItem->m_index = QModelIndex();
            pViewItem->setData(CONTROL_NONE, ROLE_PARAM_CTRL);
            m_ui->editCoreParamName->setText("");
            m_ui->editCoreParamType->setText("");
        }
    }
}

void ZEditParamLayoutDlg::updateSubgParamControl(
            IGraphsModel* pGraphsModel,
            const QString& subgName,
            bool bSubInput,
            const QString& coreParam,
            PARAM_CONTROL ctrl)
{
    //update control for every subgraph node, which the name is `subgName`, for the param named `paramName`.
    PARAM_CLASS cls = bSubInput ? PARAM_INPUT : PARAM_OUTPUT;
    GlobalControlMgr::instance().onParamUpdated(subgName, cls, coreParam, ctrl);

    QModelIndexList subgNodes = pGraphsModel->findSubgraphNode(subgName);
    for (auto idx : subgNodes)
    {
        for (auto role : {ROLE_CUSTOMUI_NODE, ROLE_CUSTOMUI_PANEL})
        {
            ViewParamModel *paramsModel = QVariantPtr<ViewParamModel>::asPtr(idx.data(role));
            QModelIndex viewIdx = paramsModel->indexFromName(cls, coreParam);
            m_pGraphsModel->ModelSetData(viewIdx, ctrl, ROLE_PARAM_CTRL);
        }
    }
}

void ZEditParamLayoutDlg::applySubgraphNode()
{
    if (m_pGraphsModel->IsSubGraphNode(m_nodeIdx) && m_model->isNodeModel())
    {
        //sync to core param model first, and then the coreparam model will notify the view param model to update.
        QStandardItem* _root = m_proxyModel->invisibleRootItem();
        ZASSERT_EXIT(_root && _root->rowCount() == 1);

        QStandardItem* pRoot = _root->child(0);
        ZASSERT_EXIT(pRoot && pRoot->rowCount() == 1);

        const QString& subgName = m_nodeIdx.data(ROLE_OBJNAME).toString();
        const QModelIndex& subgIdx = m_pGraphsModel->index(subgName);

        QStringList newInputsKeys, newOutputsKeys;

        QStandardItem* pTab = pRoot->child(0);
        for (int i = 0; i < pTab->rowCount(); i++)
        {
            QStandardItem* pGroup = pTab->child(i);
            ZASSERT_EXIT(pGroup);
            const QString& groupName = pGroup->text();

            if (groupName != "In Sockets" && groupName != "Out Sockets")
            {
                continue;
            }

            bool bSubInput = groupName == "In Sockets";

            QSet<QString> deleteParams;
            if (groupName == "In Sockets")
            {
                INPUT_SOCKETS inputs = m_nodeIdx.data(ROLE_INPUTS).value<INPUT_SOCKETS>();
                for (QString coreName : inputs.keys())
                    deleteParams.insert(coreName);
            }
            else
            {
                OUTPUT_SOCKETS outputs = m_nodeIdx.data(ROLE_OUTPUTS).value<OUTPUT_SOCKETS>();
                for (QString coreName : outputs.keys())
                    deleteParams.insert(coreName);
            }

            for (int r = 0; r < pGroup->rowCount(); r++)
            {
                VParamItem* pItem = static_cast<VParamItem*>(pGroup->child(r));
                const QString& vName = pItem->m_info.name;
                QString coreName;
                if (pItem->data(ROLE_VPARAM_IS_COREPARAM).toBool())
                    coreName = pItem->data(ROLE_PARAM_NAME).toString();
                const QString& typeDesc = pItem->m_info.typeDesc;
                const QVariant& deflVal = pItem->m_info.value;
                const PARAM_CONTROL ctrl = (PARAM_CONTROL)pItem->data(ROLE_PARAM_CTRL).toInt();

                if (coreName.isEmpty())
                {
                    //new added param.
                    const QVariant& defl = pItem->data(ROLE_PARAM_VALUE);

                    QPointF pos(0, 0);
                    //todo: node arrangement.

                    QString subIO_ident = NodesMgr::createNewNode(m_pGraphsModel, subgIdx, bSubInput ? "SubInput" : "SubOutput", pos);
                    const QModelIndex& nodeIdx = m_pGraphsModel->index(subIO_ident, subgIdx);
                    IParamModel* paramModel = QVariantPtr<IParamModel>::asPtr(nodeIdx.data(ROLE_PARAM_MODEL));
                    const QModelIndex& idxName = paramModel->index("name");
                    const QModelIndex& idxType = paramModel->index("type");
                    const QModelIndex& idxDefl = paramModel->index("defl");

                    m_pGraphsModel->ModelSetData(idxName, vName, ROLE_PARAM_VALUE);
                    m_pGraphsModel->ModelSetData(idxType, typeDesc, ROLE_PARAM_VALUE);
                    m_pGraphsModel->ModelSetData(idxDefl, typeDesc, ROLE_PARAM_TYPE);
                    m_pGraphsModel->ModelSetData(idxDefl, defl, ROLE_PARAM_VALUE);

                    //have to bind new param idx on subgraph node.
                    IParamModel *_subgnode_paramModel = QVariantPtr<IParamModel>::asPtr(m_nodeIdx.data(bSubInput ? ROLE_INPUT_MODEL : ROLE_OUTPUT_MODEL));
                    pItem->mapCoreParam(_subgnode_paramModel->index(vName));

                    updateSubgParamControl(m_pGraphsModel, subgName, bSubInput, vName, ctrl);
                }
                else if (coreName != "SRC" && coreName != "DST")
                {
                    // rename
                    bool bNameChanged = coreName != vName;

                    const QModelIndex& subInOutput = UiHelper::findSubInOutputIdx(m_pGraphsModel, bSubInput, coreName, subgIdx);
                    IParamModel* paramModel = QVariantPtr<IParamModel>::asPtr(subInOutput.data(ROLE_PARAM_MODEL));
                    const QModelIndex& nameIdx = paramModel->index("name");
                    const QModelIndex& typeIdx = paramModel->index("type");
                    const QModelIndex& deflIdx = paramModel->index("defl");

                    //update the value on "name" in SubInput/SubOutput.
                    m_pGraphsModel->ModelSetData(nameIdx, vName, ROLE_PARAM_VALUE);
                    //update type and value if necessaily.
                    m_pGraphsModel->ModelSetData(typeIdx, typeDesc, ROLE_PARAM_VALUE);
                    m_pGraphsModel->ModelSetData(deflIdx, deflVal, ROLE_PARAM_VALUE);
                    // set control and then will sync to all view param.
                    m_pGraphsModel->ModelSetData(deflIdx, ctrl, ROLE_PARAM_CTRL);

                    updateSubgParamControl(m_pGraphsModel, subgName, bSubInput, vName, ctrl);
                }

                deleteParams.remove(coreName);

                if (bSubInput)
                    newInputsKeys.push_back(vName);     //vName is core name.
                else
                    newOutputsKeys.push_back(vName);
            }

            //remove core param
            QStringList removeList;
            for (QString delCoreParam : deleteParams)
            {
                const QModelIndex& subInOutput = UiHelper::findSubInOutputIdx(m_pGraphsModel, bSubInput, delCoreParam, subgIdx);
                removeList.append(subInOutput.data(ROLE_OBJID).toString());
            }
            for (QString ident : removeList)
            {
                //currently, the param about subgraph is construct or deconstruct by SubInput/SubOutput node.
                //so, we have to remove or add the SubInput/SubOutput node to affect the params.
                m_pGraphsModel->removeNode(ident, subgIdx, true);
            }
        }

        //first, we have to rearrange the order of the params on this subnet, by arranging it's descriptor.
        NODE_DESC descSubg;
        m_pGraphsModel->getDescriptor(subgName, descSubg);

        INPUT_SOCKETS newInputs;
        for (QString sockName : newInputsKeys)
            newInputs.insert(sockName, descSubg.inputs[sockName]);
        descSubg.inputs = newInputs;

        OUTPUT_SOCKETS newOutputs;
        for (QString sockName : newOutputsKeys)
            newOutputs.insert(sockName, descSubg.outputs[sockName]);
        descSubg.outputs = newOutputs;

        m_pGraphsModel->undoRedo_updateSubgDesc(subgName, descSubg);

        //arrange the specific order for view model.
        m_model->arrangeOrder(newInputsKeys, newOutputsKeys);
        //second, update all other subgraph nodes
    }
}

void ZEditParamLayoutDlg::onApply()
{
    m_pGraphsModel->beginTransaction("edit custom param for node");
    zeno::scope_exit scope([=]() { m_pGraphsModel->endTransaction(); });

    //bool bSubgName = m_pGraphsModel->IsSubGraphNode(m_nodeIdx) && m_model->isNodeModel();
    //for (int i = 0; i < m_commandSeq.size(); i++)
    //{
    //    m_commandSeq[i]->redo();
    //}
    if (m_bSubgraphNode)
    {
        applySubgraphNode();
    }
    else
    {
        bool bNodeUI = m_model->isNodeModel();
        VPARAM_INFO root = m_proxyModel->exportParams();
        m_pGraphsModel->ModelSetData(m_nodeIdx, QVariant::fromValue(root), bNodeUI ? ROLE_CUSTOMUI_NODE_IO : ROLE_CUSTOMUI_PANEL_IO);
    }
}

void ZEditParamLayoutDlg::onOk()
{
    accept();
    onApply();
}

void ZEditParamLayoutDlg::onCancel()
{
    reject();
}
