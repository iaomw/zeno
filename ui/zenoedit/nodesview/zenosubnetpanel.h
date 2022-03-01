#ifndef __ZENO_SUBNET_PANEL_H__
#define __ZENO_SUBNET_PANEL_H__

#include <QtWidgets>

class ZenoSubnetListView;
class ZenoSubnetTreeView;
class IGraphsModel;

class ZenoSubnetPanel : public QWidget
{
	Q_OBJECT
public:
	ZenoSubnetPanel(QWidget* parent = nullptr);
	void initModel(IGraphsModel* pModel);
	QSize sizeHint() const override;
	void setViewWay(bool bListView);

signals:
	void clicked(const QModelIndex& index);

private slots:
	void onNewSubnetBtnClicked();
	void onModelReset();

protected:
	void paintEvent(QPaintEvent* e) override;

private:
	ZenoSubnetListView* m_pListView;
	ZenoSubnetTreeView* m_pTreeView;
	QLabel* m_pTextLbl;
	bool m_bListView;
};

#endif