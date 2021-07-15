from PySide2.QtGui import *
from PySide2.QtCore import *
from PySide2.QtWidgets import *

from .viewport import ViewportWidget
from .timeline import TimelineWidget
from .editor import NodeEditor

from . import asset_path

class ViewportTimeline(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.viewport = ViewportWidget()
        self.timeline = TimelineWidget()

        self.layout = QVBoxLayout()
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.addWidget(self.viewport)
        self.layout.addWidget(self.timeline)
        self.setLayout(self.layout)

class MainWindow(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        self.setWindowTitle('ZENO Qt Editor')
        self.setGeometry(200, 200, 1200, 1000)

        scrn_size = QDesktopWidget().geometry()
        self_size = self.geometry()
        self.move(
                (scrn_size.width() - self_size.width()) // 2,
                (scrn_size.height() - self_size.height()) // 2)

        self.viewportTimeline = ViewportTimeline()
        self.editor = NodeEditor()

        self.timeline = self.viewportTimeline.timeline
        self.viewport = self.viewportTimeline.viewport
        self.timeline.setEditor(self.editor)

        self.mainsplit = QSplitter(Qt.Vertical)
        self.mainsplit.addWidget(self.viewportTimeline)
        self.mainsplit.addWidget(self.editor)

        self.layout = QVBoxLayout()
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.addWidget(self.mainsplit)
        self.setLayout(self.layout)

        self.setWindowIcon(QIcon(asset_path('logo.ico')))

        self.startTimer(1000 // 60)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Escape:
            self.close()

        super().keyPressEvent(event)

    def timerEvent(self, event):
        self.viewport.on_update()
        self.timeline.on_update()
        super().timerEvent(event)

    def closeEvent(self, event):
        if self.editor.confirm_discard('Exit'):
            event.accept()
        else:
            event.ignore()
