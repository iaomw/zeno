#include "command.h"
#include "graphsmodel.h"
#include "modelrole.h"
#include "modeldata.h"
#include "zassert.h"
#include "apilevelscope.h"
#include "viewparammodel.h"
#include "vparamitem.h"


AddNodeCommand::AddNodeCommand(const QString& id, const NODE_DATA& data, IGraphsModel* pModel, QPersistentModelIndex subgIdx)
    : QUndoCommand()
    , m_id(id)
    , m_model(pModel)
    , m_data(data)
    , m_subgIdx(subgIdx)
{
}

AddNodeCommand::~AddNodeCommand()
{
}

void AddNodeCommand::redo()
{
    m_model->addNode(m_data, m_subgIdx);
}

void AddNodeCommand::undo()
{
    m_model->removeNode(m_id, m_subgIdx);
}


RemoveNodeCommand::RemoveNodeCommand(int row, NODE_DATA data, GraphsModel* pModel, QPersistentModelIndex subgIdx)
    : QUndoCommand()
    , m_data(data)
    , m_model(pModel)
    , m_subgIdx(subgIdx)
    , m_row(row)
{
    m_id = m_data[ROLE_OBJID].toString();

    //all links will be removed when remove node, for caching other type data,
    //we have to clean the data here.
    OUTPUT_SOCKETS outputs = m_data[ROLE_OUTPUTS].value<OUTPUT_SOCKETS>();
    INPUT_SOCKETS inputs = m_data[ROLE_INPUTS].value<INPUT_SOCKETS>();
    for (auto it = outputs.begin(); it != outputs.end(); it++)
    {
        it->second.info.links.clear();
    }
    for (auto it = inputs.begin(); it != inputs.end(); it++)
    {
        it->second.info.links.clear();
    }
    m_data[ROLE_INPUTS] = QVariant::fromValue(inputs);
    m_data[ROLE_OUTPUTS] = QVariant::fromValue(outputs);
}

RemoveNodeCommand::~RemoveNodeCommand()
{
}

void RemoveNodeCommand::redo()
{
    m_model->removeNode(m_id, m_subgIdx);
}

void RemoveNodeCommand::undo()
{
    m_model->addNode(m_data, m_subgIdx);
}


LinkCommand::LinkCommand(bool bAddLink, const EdgeInfo& link, GraphsModel* pModel)
    : QUndoCommand()
    , m_bAdd(bAddLink)
    , m_link(link)
    , m_model(pModel)
{
}

void LinkCommand::redo()
{
    if (m_bAdd)
    {
        m_model->addLink(m_link);
    }
    else
    {
        m_model->removeLink(m_link);
    }
}

void LinkCommand::undo()
{
    if (m_bAdd)
    {
        m_model->removeLink(m_link);
    }
    else
    {
        m_model->addLink(m_link);
    }
}


UpdateBlackboardCommand::UpdateBlackboardCommand(
        const QString& nodeid,
        BLACKBOARD_INFO newInfo,
        BLACKBOARD_INFO oldInfo,
        GraphsModel* pModel,
        QPersistentModelIndex subgIdx) 
    : m_nodeid(nodeid)
    , m_oldInfo(oldInfo)
    , m_newInfo(newInfo)
    , m_pModel(pModel)
    , m_subgIdx(subgIdx)
{
}

void UpdateBlackboardCommand::redo()
{
    m_pModel->updateBlackboard(m_nodeid, m_newInfo, m_subgIdx, false);
}

void UpdateBlackboardCommand::undo()
{
    m_pModel->updateBlackboard(m_nodeid, m_oldInfo, m_subgIdx, false);
}


ImportNodesCommand::ImportNodesCommand(
                const QMap<QString, NODE_DATA>& nodes,
                const QList<EdgeInfo>& links,
                QPointF pos,
                GraphsModel* pModel,
                QPersistentModelIndex subgIdx)
    : m_nodes(nodes)
    , m_links(links)
    , m_model(pModel)
    , m_subgIdx(subgIdx)
    , m_pos(pos)
{
}

void ImportNodesCommand::redo()
{
    m_model->importNodes(m_nodes, m_links, m_pos, m_subgIdx, false);
}

void ImportNodesCommand::undo()
{
    for (QString id : m_nodes.keys())
    {
        m_model->removeNode(id, m_subgIdx, false);
    }
}


ModelDataCommand::ModelDataCommand(IGraphsModel* pModel, const QModelIndex& idx, const QVariant& oldData, const QVariant& newData, int role)
    : m_model(pModel)
    , m_oldData(oldData)
    , m_newData(newData)
    , m_role(role)
    , m_index(idx)
{
    m_objPath = idx.data(ROLE_OBJPATH).toString();
}

void ModelDataCommand::ensureIdxValid()
{
    if (!m_index.isValid())
    {
        m_index = m_model->indexFromPath(m_objPath);    //restore the index because the index may be invalid after new/delete item.
    }
}

void ModelDataCommand::redo()
{
    ensureIdxValid();
    //todo: setpos case.
    ApiLevelScope scope(m_model);
    QAbstractItemModel* pModel = const_cast<QAbstractItemModel*>(m_index.model());
    if (pModel)
        pModel->setData(m_index, m_newData, m_role);
}

void ModelDataCommand::undo()
{
    ensureIdxValid();
    ApiLevelScope scope(m_model);
    QAbstractItemModel* pModel = const_cast<QAbstractItemModel*>(m_index.model());
    if (pModel)
        pModel->setData(m_index, m_oldData, m_role);
}


UpdateSubgDescCommand::UpdateSubgDescCommand(IGraphsModel *pModel, const QString &subgraphName, const NODE_DESC newDesc)
    : m_model(pModel)
    , m_subgName(subgraphName)
    , m_newDesc(newDesc)
{
    m_model->getDescriptor(m_subgName, m_oldDesc);
}

void UpdateSubgDescCommand::redo()
{
    m_model->updateSubgDesc(m_subgName, m_newDesc);
}

void UpdateSubgDescCommand::undo()
{
    m_model->updateSubgDesc(m_subgName, m_oldDesc);
}




static QStandardItem* getParentPath(IGraphsModel* pGraphsModel, const QString& parentPath)
{
    ZASSERT_EXIT(pGraphsModel, nullptr);
    QModelIndex parent = pGraphsModel->indexFromPath(parentPath);
    if (parent.isValid())
    {
        const QAbstractItemModel* model = parent.model();
        ZASSERT_EXIT(model, nullptr);
        ViewParamModel* pModel = qobject_cast<ViewParamModel*>(const_cast<QAbstractItemModel*>(model));
        QStandardItem* parentItem = pModel->itemFromIndex(parent);
        return parentItem;
    }
    return nullptr;
}


///////////////////////////////////////////////////////////////////////
ViewParamAddCommand::ViewParamAddCommand(IGraphsModel* pModel, const QString& parentObjPath, const VPARAM_INFO& newItem)
    : QUndoCommand(COMMAND_VIEWADD)
    , m_model(pModel)
    , m_parentPath(parentObjPath)
    , m_itemData(newItem)
    , m_rowInserted(0)
{
    QStandardItem* parentItem = getParentPath(m_model, m_parentPath);
    ZASSERT_EXIT(parentItem);
    m_rowInserted = parentItem->rowCount();
}

void ViewParamAddCommand::redo()
{
    QStandardItem* parentItem = getParentPath(m_model, m_parentPath);
    ZASSERT_EXIT(parentItem);
    VParamItem* newItem = ViewParamModel::importParam(m_itemData);
    parentItem->appendRow(newItem);
}

void ViewParamAddCommand::undo()
{
    QStandardItem* parentItem = getParentPath(m_model, m_parentPath);
    ZASSERT_EXIT(parentItem);
    parentItem->removeRow(m_rowInserted);
}


/////////////////////////////////////////////////////////////////////
ViewParamRemoveCommand::ViewParamRemoveCommand(IGraphsModel* pModel, const QString& parentObjPath, int row)
    : QUndoCommand(COMMAND_VIEWREMOVE)
    , m_model(pModel)
    , m_parentPath(parentObjPath)
    , m_row(row)
{
    QStandardItem* parentItem = getParentPath(m_model, m_parentPath);
    VParamItem* pItem = static_cast<VParamItem*>(parentItem->child(m_row));
    m_deleteItem = pItem->exportParamInfo();
}

void ViewParamRemoveCommand::redo()
{
    QStandardItem* parentItem = getParentPath(m_model, m_parentPath);
    ZASSERT_EXIT(parentItem);
    parentItem->removeRow(m_row);
}

void ViewParamRemoveCommand::undo()
{
    QStandardItem* parentItem = getParentPath(m_model, m_parentPath);
    VParamItem *newItem = ViewParamModel::importParam(m_deleteItem);
    parentItem->appendRow(newItem);
}


/////////////////////////////////////////////////////////////////////////////////
ViewParamSetDataCommand::ViewParamSetDataCommand(IGraphsModel *pModel, const QString &vitemPath, const QVariant &newValue, int role)
    : QUndoCommand(COMMAND_VIEWSETDATA)
    , m_model(pModel)
    , m_vitemPath(vitemPath)
    , m_newValue(newValue)
    , m_role(role)
{
    QModelIndex itemIdx = m_model->indexFromPath(m_vitemPath);
    m_oldValue = itemIdx.data(m_role);
}

void ViewParamSetDataCommand::redo()
{
    QModelIndex itemIdx = m_model->indexFromPath(m_vitemPath);
    if (itemIdx.isValid())
    {
        const QAbstractItemModel* model = itemIdx.model();
        ZASSERT_EXIT(model);
        ViewParamModel* pModel = qobject_cast<ViewParamModel*>(const_cast<QAbstractItemModel*>(model));
        pModel->setData(itemIdx, m_newValue, m_role);
    }
}

void ViewParamSetDataCommand::undo()
{
    QModelIndex itemIdx = m_model->indexFromPath(m_vitemPath);
    if (itemIdx.isValid())
    {
        const QAbstractItemModel *model = itemIdx.model();
        ZASSERT_EXIT(model);
        ViewParamModel *pModel = qobject_cast<ViewParamModel *>(const_cast<QAbstractItemModel *>(model));
        pModel->setData(itemIdx, m_oldValue, m_role);
    }
}


ViewParamMoveCommand::ViewParamMoveCommand(IGraphsModel* pModel, const QString& srcParentPath, int srcRow, const QString& dstParentPath, int dstRow)
    : QUndoCommand(COMMAND_VIEWMOVE)
    , m_model(pModel)
    , m_srcParent(srcParentPath)
    , m_srcRow(srcRow)
    , m_dstParent(dstParentPath)
    , m_dstRow(dstRow)
{
}

void ViewParamMoveCommand::redo()
{

}

void ViewParamMoveCommand::undo()
{

}


MapParamIndexCommand::MapParamIndexCommand(IGraphsModel* pModel, const QString& sourceObj, const QString& dstObj)
    : QUndoCommand(COMMAND_MAPPING)
    , m_model(pModel)
    , m_sourceObj(sourceObj)
    , m_dstObj(dstObj)
{
    QModelIndex paramIdx = m_model->indexFromPath(m_sourceObj);
    QModelIndex oldIdx = paramIdx.data(ROLE_PARAM_COREIDX).toModelIndex();
    if (oldIdx.isValid())
        m_oldMappingObj = oldIdx.data(ROLE_OBJPATH).toString();
}

void MapParamIndexCommand::redo()
{
    QModelIndex paramIdx = m_model->indexFromPath(m_sourceObj);
    QAbstractItemModel* model = const_cast<QAbstractItemModel*>(paramIdx.model());
    if (model)
    {
        QModelIndex targetIdx = m_model->indexFromPath(m_dstObj);
        model->setData(paramIdx, targetIdx, ROLE_PARAM_COREIDX);
    }
}

void MapParamIndexCommand::undo()
{
    QModelIndex paramIdx = m_model->indexFromPath(m_sourceObj);
    QAbstractItemModel *model = const_cast<QAbstractItemModel *>(paramIdx.model());
    if (model)
    {
        QModelIndex oldIdx = m_model->indexFromPath(m_oldMappingObj);
        model->setData(paramIdx, oldIdx, ROLE_PARAM_COREIDX);
    }
}
