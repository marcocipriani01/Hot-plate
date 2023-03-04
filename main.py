# System
import os
import sys
import time
import pickle
import traceback
# GUI
from settings_dialog import SettingsDialog
from PyQt5 import QtWidgets, uic
from PyQt5.QtCore import QObject, QThread, pyqtSignal
# PyQtGraph
import pyqtgraph as pg
# USB communication
import serial
from serial.tools import list_ports


layout_form = uic.loadUiType("layout.ui")[0]
pg.setConfigOptions(antialias=True)


class Worker(QObject):
    # Attributes
    parent = None
    connected = False
    serial_port = None
    # Signals
    ports_signal = pyqtSignal(list)
    data_signal = pyqtSignal(float, float, float)
    error_signal = pyqtSignal(str)

    def loopWork(self):
        while True:
            if self.connected and self.serial_port is not None:
                try:
                    line = self.serial_port.readline().decode('utf-8').rstrip()
                    self.serial_port.flush()
                    if line != "":
                        if line[0] == 'A':
                            data = line[1:].split(',')
                            self.data_signal.emit(
                                float(data[0]), float(data[1]), float(data[2]))
                        else:
                            print("Serial message: " + line)
                except UnicodeDecodeError as e:
                    pass
                except Exception as e:
                    traceback.print_exc()
                    self.error_signal.emit(e.args[0])
            else:
                self.ports_signal.emit(list_ports.comports())


class ErrorDialog():
    def __init__(self, text):
        self.msg = QtWidgets.QMessageBox()
        self.msg.setIcon(QtWidgets.QMessageBox.Critical)
        self.msg.setText("Error")
        self.msg.setInformativeText(text)
        self.msg.setWindowTitle("Error")
        self.msg.addButton(QtWidgets.QMessageBox.Ok)
        self.msg.exec_()


class Main(QtWidgets.QMainWindow, layout_form):

    def __init__(self, parent=None):
        QtWidgets.QWidget.__init__(self, parent)
        self.setupUi(self)
        if os.path.isfile("settings.plk"):
            try:
                with open("settings.plk", 'rb') as f:
                    self.parameters = pickle.load(f)
                self.min_temp = self.parameters["min_temp"]
                self.max_temp = self.parameters["max_temp"]
                self.k_P = self.parameters["k_P"]
                self.k_I = self.parameters["k_I"]
                self.k_D = self.parameters["k_D"]
            except:
                traceback.print_exc()
                self.min_temp = 20.0
                self.max_temp = 260.0
                self.k_P = 1.2
                self.k_I = 0.0
                self.k_D = 0.0
        else:
            self.min_temp = 20.0
            self.max_temp = 260.0
            self.k_P = 1.2
            self.k_I = 0.0
            self.k_D = 0.0
        self.target_temp = self.min_temp
        # Serial port
        self.listPorts = list_ports.comports()
        for i in range(len(self.listPorts)):
            self.serial_port_combo.addItem(
                self.listPorts[i].description, self.listPorts[i].name)
        # Plot
        self.graphWidget.setBackground('w')
        self.pen = pg.mkPen(color=(255, 0, 0), width=3)
        self.reflow_profile_pen = pg.mkPen(color=(0, 255, 0), width=2)
        self.graphWidget.setTitle(
            "Heatplate temperature", color="k", size="13pt")
        label_style = {'color': 'k', 'font-size': '13px'}
        self.graphWidget.setLabel('left', 'Temperature (°C)', **label_style)
        self.graphWidget.setLabel('bottom', 'Time (s)', **label_style)
        self.graphWidget.showGrid(x=True, y=True)
        self.graphWidget.setYRange(
            self.min_temp - 10.0, self.max_temp + 10.0, padding=0)
        self.time = []
        self.temperature = []
        self.reflow_overlay_enable = False
        self.reflow_overlay = []
        self.reflow_overlay_time = []
        self.graphWidget.plot(self.time, self.temperature, pen=self.pen)
        # UI listeners
        self.connect_button.clicked.connect(self.connect_disconnect)
        self.clear_plot_button.clicked.connect(self.clear_plot)
        self.settings_button.clicked.connect(self.open_settings)
        self.set_target_button.clicked.connect(self.changeTemp)
        self.turn_off_button.clicked.connect(self.turn_off_heatplate)
        self.overlay_button.clicked.connect(self.show_reflow_overlay)
        self.serial_port_combo.activated.connect(self.select_device)
        # Settings dialog
        self.settings_dialog = SettingsDialog(self)
        self.settings_dialog.apply_signal.connect(self.update_settings)
        # Worker thread
        self.worker = Worker()
        self.worker.parent = self
        self.executionThread = QThread()
        self.worker.moveToThread(self.executionThread)
        self.executionThread.started.connect(self.worker.loopWork)
        self.executionThread.start()
        self.worker.ports_signal.connect(self.update_ports_list)
        self.worker.data_signal.connect(self.display_data)
        self.worker.error_signal.connect(self.on_error)

    def show_reflow_overlay(self):
        self.reflow_overlay_enable = not self.reflow_overlay_enable
        if self.reflow_overlay_enable:
            self.overlay_button.setText("Hide reflow profile overlay")
            self.reflow_overlay = [25, 100, 150, 183, 230, 235, 230, 183, 150]
            self.reflow_overlay_time = [0, 30, 120, 150, 195, 210, 225, 240, 250]
            for i in range(len(self.reflow_overlay)):
                self.reflow_overlay_time[i] = self.reflow_overlay_time[i] + \
                    (time.time() - self.initTime)
            self.graphWidget.plot(self.reflow_overlay_time, self.reflow_overlay,
                                  pen=self.reflow_profile_pen, symbol='+', symbolSize=15)
        else:
            self.overlay_button.setText("Show reflow profile overlay")
            self.reflow_overlay.clear()
            self.reflow_overlay_time.clear()

    def send_settings(self):
        try:
            self.worker.serial_port.write(("$T" + str(int(self.k_P * 1000)) + "," +
                                           str(int(self.k_I * 1000)) + "," +
                                           str(int(self.k_D * 1000)) + "%").encode('utf-8'))
        except Exception as e:
            traceback.print_exc()
            ErrorDialog(e.args[0])

    def turn_off_heatplate(self):
        self.target_temp_spinner.setValue(self.min_temp)
        self.changeTemp()

    def clear_plot(self):
        self.initTime = time.time()
        self.time.clear()
        self.temperature.clear()
        self.reflow_overlay_enable = False
        self.overlay_button.setText("Show reflow profile overlay")
        self.reflow_overlay.clear()
        self.reflow_overlay_time.clear()
        self.graphWidget.clear()

    def select_device(self):
        self.connect_button.setDisabled(False)

    def open_settings(self):
        self.settings_dialog.exec_()

    def on_error(self, string):
        ErrorDialog(string)
        self.disconnect()

    def display_data(self, temp, rate_of_change, duty):
        self.temperature_display.display(temp)
        self.time.append(time.time() - self.initTime)
        self.temperature.append(temp)
        self.graphWidget.plot(self.time, self.temperature,
                              pen=self.pen, clear=True)
        self.rate_of_change_display.display(rate_of_change)
        self.duty_display.display(duty)
        if self.reflow_overlay_enable:
            self.graphWidget.plot(self.reflow_overlay_time, self.reflow_overlay,
                                  pen=self.reflow_profile_pen, symbol='+', symbolSize=15)

    def changeTemp(self):
        self.worker.serial_port.reset_input_buffer()
        self.worker.serial_port.reset_output_buffer()
        self.send_settings()
        self.target_temp = self.target_temp_spinner.value()
        try:
            self.worker.serial_port.write(
                ("$S" + str(int(self.target_temp * 100)) + "%").encode('utf-8'))
        except Exception as e:
            traceback.print_exc()
            ErrorDialog(e.args[0])

    def update_settings(self):
        self.min_temp = self.settings_dialog.settings["min_temp"]
        self.max_temp = self.settings_dialog.settings["max_temp"]
        self.k_P = self.settings_dialog.settings["k_P"]
        self.k_I = self.settings_dialog.settings["k_I"]
        self.k_D = self.settings_dialog.settings["k_D"]
        self.target_temp_spinner.setMinimum(self.min_temp)
        self.target_temp_spinner.setMaximum(self.max_temp)
        try:
            self.send_settings()
            self.statusBar().showMessage("Settings updated successfully")
        except Exception as e:
            traceback.print_exc()
            ErrorDialog(
                "Eror: " + e.args[0] + ". Check if you are connected to the heatplate.")
            self.statusBar().showMessage("Settings not updated")

    def disconnect(self):
        try:
            self.worker.serial_port.write(
                ("$S" + str(int(self.min_temp * 100)) + "%").encode('utf-8'))
            time.sleep(0.3)
        except:
            pass
        self.worker.connected = False
        self.worker.serial_port.close()
        self.worker.serial_port = None
        self.statusBar().showMessage("Controller disconnected")
        self.connect_button.setText("Connect")
        self.target_temp_spinner.setDisabled(True)
        self.set_target_button.setDisabled(True)
        self.turn_off_button.setDisabled(True)
        self.settings_button.setDisabled(True)

    def connect_disconnect(self):
        if self.connect_button.isChecked() == True:
            if self.serial_port_combo.currentText() != 'None':
                portIndex = self.serial_port_combo.currentIndex()
                try:
                    self.worker.serial_port = serial.Serial(
                        self.listPorts[portIndex].device, baudrate=115200, timeout=10, write_timeout=10)
                    time.sleep(0.5)
                    self.worker.connected = True
                    self.clear_plot()
                    self.statusBar().showMessage("Controller connected")
                    self.connect_button.setText("Disconnect")
                    self.target_temp_spinner.setDisabled(False)
                    self.set_target_button.setDisabled(False)
                    self.turn_off_button.setDisabled(False)
                    self.settings_button.setDisabled(False)
                    self.send_settings()
                except Exception as erro:
                    traceback.print_exc()
                    self.connect_button.setChecked(False)
                    errorMessage = erro.args[0]
                    ErrorDialog(errorMessage)
        else:
            self.disconnect()

    def update_ports_list(self, newListPorts):
        if self.listPorts != newListPorts:
            self.listPorts = newListPorts
            self.serial_port_combo.clear()
            for i in range(len(self.listPorts)):
                self.serial_port_combo.addItem(self.listPorts[i].description)


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    Main().show()
    app.exec_()