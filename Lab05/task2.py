import sys
import random
from PyQt6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, QHBoxLayout, QWidget, QLabel, QDoubleSpinBox, QSpinBox, QPushButton, QGroupBox, QFrame)
from PyQt6.QtCore import Qt, QPoint
from PyQt6.QtGui import QPainter, QPen, QColor
import math

class MainWidet(QFrame):
    def __init__(self):
        super().__init__()

        self.setMinimumSize(600, 400)
        self.setStyleSheet("background-color: white; border: 1px solid black;")
        self.points = []
    
    def set_points(self, points):
        self.points = points
        self.update()

    def paintEvent(self, a0):
        if not self.points:
            return
        
        painter = QPainter(self)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)

        painter.setPen(QPen(QColor(0,0,255), 2))

        for i in range(1, len(self.points)):
            painter.drawLine(self.points[i-1], self.points[i])


class MidpointDisplacementWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        screen = QApplication.primaryScreen().availableGeometry()
        
        window_width = int(screen.width() * 0.8)
        window_height = int(screen.height() * 0.8)

        self.setFixedSize(window_width, window_height)
        self.move(int(screen.width() * 0.1), int(screen.height() * 0.1))

        self.points = []
        self.history = []
        self.cur_step = -1

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        hbox_layout = QHBoxLayout()
        central_widget.setLayout(hbox_layout)

        control_panel = QGroupBox()
        vbox_layout = QVBoxLayout()
        control_panel.setLayout(vbox_layout)
        hbox_layout.setContentsMargins(10, 10, 10, 10)
        control_panel.setMaximumWidth(300)
        control_panel.setMinimumWidth(200)

        start_h_layout = QHBoxLayout()
        start_h_layout.addWidget(QLabel("Начальная высота:"))
        self.start_h_spin = QSpinBox()
        self.start_h_spin.setRange(0,400)
        self.start_h_spin.setValue(0)
        start_h_layout.addWidget(self.start_h_spin)
        vbox_layout.addLayout(start_h_layout)

        end_h_layout = QHBoxLayout()
        end_h_layout.addWidget(QLabel("Конечная высота:"))
        self.end_h_spin = QSpinBox()
        self.end_h_spin.setRange(0,400)
        self.end_h_spin.setValue(400)
        end_h_layout.addWidget(self.end_h_spin)
        vbox_layout.addLayout(end_h_layout)

        r_layout = QHBoxLayout()
        r_layout.addWidget(QLabel("Шероховатость:"))
        self.r_spin = QDoubleSpinBox()
        self.r_spin.setRange(0.0, 1.0)
        self.r_spin.setValue(0.1)
        r_layout.addWidget(self.r_spin)
        vbox_layout.addLayout(r_layout)

        self.build_button = QPushButton("Построить")
        self.build_button.clicked.connect(self.build)
        vbox_layout.addWidget(self.build_button)

        nav_layout = QHBoxLayout()
        self.prev_button = QPushButton("<-")
        self.prev_button.clicked.connect(self.prev_step)
        self.prev_button.setEnabled(False)
        nav_layout.addWidget(self.prev_button)

        self.next_button = QPushButton("->")
        self.next_button.clicked.connect(self.next_step)
        self.next_button.setEnabled(False)
        nav_layout.addWidget(self.next_button)

        vbox_layout.addLayout(nav_layout)

        self.step_label = QLabel("Шаг: 0")
        vbox_layout.addWidget(self.step_label)

        vbox_layout.addStretch()

        self.main_widget = MainWidet()

        hbox_layout.addWidget(control_panel)
        hbox_layout.addWidget(self.main_widget)

    def build(self):
        self.history = []
        self.cur_step = -1

        w = self.main_widget.width()
        h = self.main_widget.height()

        start_y = h - self.start_h_spin.value()
        end_y = h - self.end_h_spin.value()

        points = [
            QPoint(0, start_y),
            QPoint(w, end_y)
        ]

        self.history.append([QPoint(point.x(), point.y()) for point in points])

        max_iter = 10
        i = 0

        while i < max_iter:
            points = self.perform_displacement_step(points)
            self.history.append([QPoint(point.x(), point.y()) for point in points])
            i += 1
        
        self.cur_step = 0
        self.points = [QPoint(point.x(), point.y()) for point in self.history[0]]
        self.update_display()

    def perform_displacement_step(self, points):
        new_points = []
        r = self.r_spin.value()

        for i in range(len(points) - 1):
            new_points.append(points[i])

            x1, y1 = points[i].x(), points[i].y()
            x2, y2 = points[i+1].x(), points[i+1].y()

            l = math.sqrt(((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)))

            h = (y1 + y2) / 2 + random.uniform(-r * l, r * l)

            h = max(0, min(h, self.main_widget.height()))

            new_points.append(QPoint(int((x1 + x2) / 2),int(h)))
        
        new_points.append(points[-1])
        
        return new_points

    def prev_step(self):
        if self.cur_step > 0:
            self.cur_step -= 1
            self.points = [QPoint(point.x(), point.y()) for point in self.history[self.cur_step]]
            self.update_display()

    def next_step(self):
        if self.cur_step < len(self.history) - 1:
            self.cur_step += 1
            self.points = [QPoint(point.x(), point.y()) for point in self.history[self.cur_step]]
            self.update_display()

    def update_display(self):
        self.main_widget.set_points(self.points)

        self.prev_button.setEnabled(self.cur_step > 0)
        self.next_button.setEnabled(self.cur_step < len(self.history) - 1)

        self.step_label.setText(f"Шаг: {self.cur_step} / {len(self.history) - 1} \n Точек: {len(self.points)}")

def main():
    app = QApplication(sys.argv)
    window = MidpointDisplacementWindow()
    window.show()
    sys.exit(app.exec())
    
if __name__ == "__main__":
    main()