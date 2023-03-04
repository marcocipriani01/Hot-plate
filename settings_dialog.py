import pickle
import os.path
import traceback
from PyQt5 import QtCore, QtWidgets, uic

layout_form_settings = uic.loadUiType("settings_dialog.ui")[0]


class SettingsDialog(QtWidgets.QDialog, layout_form_settings):
    apply_signal = QtCore.pyqtSignal()

    def __init__(self, parent=None):
        QtWidgets.QDialog.__init__(self, parent)
        self.setupUi(self)
        if os.path.isfile("settings.plk"):
            try:
                with open("settings.plk", 'rb') as f:
                    self.settings = pickle.load(f)
                self.min_spinner.setValue(self.settings["min_temp"])
                self.max_spinner.setValue(self.settings["max_temp"])
                self.k_P_spinner.setValue(self.settings["k_P"])
                self.k_I_spinner.setValue(self.settings["k_I"])
                self.k_D_spinner.setValue(self.settings["k_D"])
            except:
                traceback.print_exc()
                QtWidgets.QErrorMessage().showMessage('Error opening file!')
        else:
            self.saveInfo()
        self.ok_button.clicked.connect(self.ok_action)
        self.cancel_button.clicked.connect(self.cancel_action)
        self.min_spinner.valueChanged.connect(self.apply_limits)
        self.max_spinner.valueChanged.connect(self.apply_limits)

    def apply_limits(self):
        self.min_spinner.setMaximum(self.max_spinner.value())
        self.max_spinner.setMinimum(self.min_spinner.value())

    def ok_action(self):
        self.saveInfo()
        self.apply_signal.emit()
        self.close()

    def cancel_action(self):
        self.close()

    def saveInfo(self):
        self.settings = {
            "min_temp": self.min_spinner.value(),
            "max_temp": self.max_spinner.value(),
            "k_P": self.k_P_spinner.value(),
            "k_I": self.k_I_spinner.value(),
            "k_D": self.k_D_spinner.value(),
        }
        try:
            with open("settings.plk", 'wb') as f:
                pickle.dump(self.settings, f)
        except:
            traceback.print_exc()
            QtWidgets.QErrorMessage().showMessage('Error saving file!')
