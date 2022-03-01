#include "subnetnode.h"
#include "model/graphsmodel.h"
#include "zenoapplication.h"
#include "graphsmanagment.h"
#include <zenoui/util/uihelper.h>


SubInputNode::SubInputNode(const NodeUtilParam& params, QGraphicsItem* parent)
	: ZenoNode(params, parent)
{

}

SubInputNode::~SubInputNode()
{

}

void SubInputNode::onParamEditFinished(PARAM_CONTROL editCtrl, const QString& paramName, const QString& textValue)
{
    //get old name first.
    IGraphsModel* pModel = zenoApp->graphsManagment()->currentModel();
    Q_ASSERT(pModel);
	const QString& nodeid = nodeId();
	QModelIndex subgIdx = this->subGraphIndex();
    const PARAMS_INFO& subInputs = pModel->data2(subgIdx, index(), ROLE_PARAMETERS).value<PARAMS_INFO>();
    const QString& oldName = subInputs["name"].value.toString();
	const QString& name = pModel->name(subgIdx);
	if (oldName == textValue)
		return;

	SOCKET_UPDATE_INFO info;
	info.bInput = true;

	if (paramName == "name")
	{
		info.oldInfo.name = oldName;
		info.newInfo.name = textValue;
		pModel->updateSubnetIO(subgIdx, nodeid, info);
	}
	else if (paramName == "type")
	{
		//todo
	}
	else if (paramName == "defl")
	{
		//todo
	}
}


SubOutputNode::SubOutputNode(const NodeUtilParam& params, QGraphicsItem* parent)
	: ZenoNode(params, parent)
{

}

SubOutputNode::~SubOutputNode()
{

}

void SubOutputNode::onParamEditFinished(PARAM_CONTROL editCtrl, const QString& paramName, const QString& textValue)
{
	//get old name first.
	IGraphsModel* pModel = zenoApp->graphsManagment()->currentModel();
	Q_ASSERT(pModel);
	const QString& nodeid = nodeId();
	const QModelIndex& subgIdx = this->subGraphIndex();
	const PARAMS_INFO& subOutputs = pModel->data2(subgIdx, index(), ROLE_PARAMETERS).value<PARAMS_INFO>();
	const QString& oldName = subOutputs["name"].value.toString();
	const QString& name = pModel->name(subgIdx);
	if (oldName == textValue)
		return;

	SOCKET_UPDATE_INFO info;
	info.bInput = false;
	if (paramName == "name")
	{
		info.oldInfo.name = oldName;
		info.newInfo.name = textValue;
		pModel->updateSubnetIO(subgIdx, nodeid, info);
	}
	else if (paramName == "type")
	{
		//todo
	}
	else if (paramName == "defl")
	{
		//todo
	}
}