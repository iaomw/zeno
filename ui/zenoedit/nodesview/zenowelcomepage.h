#ifndef __ZENO_WELCOME_PAGE_H__
#define __ZENO_WELCOME_PAGE_H__

#include <QWidget>

namespace Ui
{
	class WelcomePage;
}

class ZenoWelcomePage : public QWidget
{
	Q_OBJECT
public:
	ZenoWelcomePage(QWidget* parent = nullptr);
	void initRecentFiles();

signals:
	void newRequest();
	void openRequest();

 private:
    void deleteItem(QLayout *layout);
private:
	void initSignals();
	Ui::WelcomePage* m_ui;
};


#endif
