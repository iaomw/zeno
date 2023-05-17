#ifndef __ZREC_FRAME_SELECT_DIG_H__
#define __ZREC_FRAME_SELECT_DIG_H__

#include <QtWidgets>

namespace Ui
{
    class RecFrameSelectDlg;
}

class ZRecFrameSelectDlg : public QDialog
{
    Q_OBJECT

    enum RunState
    {
        NO_RUN,
        RUN_INCOMPLETE,
        RUN_COMPLELTE,
        RUNNING,
    };

public:
    ZRecFrameSelectDlg(QWidget* parent = nullptr);
    QPair<int, int> recordFrameRange(bool& runBeforeRun) const;

private slots:
    void onRunNow();
    void onRecordNow();
    void onCancelRecord();
    void onRecFromEdited();
    void onRecToEdited();

private:
    Ui::RecFrameSelectDlg* m_ui;
    bool validateFrame();

    int m_recStartF;
    int m_recEndF;
    bool m_bRunBeforeRecord;
    RunState m_state;
};


#endif