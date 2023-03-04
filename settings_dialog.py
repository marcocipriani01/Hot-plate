from PyQt5 import QtCore, QtWidgets, uic

layout_form_settings = uic.loadUiType("settings_dialog.ui")[0]


class SettingsDialog(QtWidgets.QDialog, layout_form_settings):
    save_signal = QtCore.pyqtSignal()
    k_P = 0.0
    k_I = 0.0
    k_D = 0.0
    bang_bang_range = 0.0

    def __init__(self, parent=None):
        QtWidgets.QDialog.__init__(self, parent)
        self.setupUi(self)
        self.ok_button.clicked.connect(self.ok_action)
        self.cancel_button.clicked.connect(self.cancel_action)

    def ok_action(self):
        self.k_P = self.k_P_spinner.value()
        self.k_I = self.k_I_spinner.value()
        self.k_D = self.k_D_spinner.value()
        self.bang_bang_range = self.bang_bang_spinner.value()
        self.save_signal.emit()
        self.close()

    def cancel_action(self):
        self.close()
