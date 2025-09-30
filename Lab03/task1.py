import sys
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QRadioButton, QFileDialog, QColorDialog
)
from PyQt6.QtGui import (
    QPainter, QPen, QColor, QImage, QMouseEvent
)
from PyQt6.QtCore import Qt, QPoint
from PIL import Image
import numpy as np


class DrawingWidget(QWidget):
    def __init__(self, main_window):
        super().__init__()
        self.main_window = main_window
        self.setMouseTracking(True)
        self.drawing = False
        self.last_point = QPoint()
        self.image = QImage(800, 600, QImage.Format.Format_RGB32)
        self.image.fill(Qt.GlobalColor.white)
        self.border_color = QColor(Qt.GlobalColor.black)
        self.pen_width = 5

        self.fill_color = QColor(Qt.GlobalColor.red)
        self.pattern_image = None
        self.pattern_array = None
        self.border_points = []

    def mousePressEvent(self, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            if self.main_window.mode == "draw":
                self.drawing = True
                self.last_point = event.pos()
            elif self.main_window.mode in ("fill_color", "fill_pattern", "border"):
                self.handle_click(event.pos())
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event: QMouseEvent):
        if self.drawing and self.main_window.mode == "draw":
            painter = QPainter(self.image)
            pen = QPen(self.border_color, self.pen_width, Qt.PenStyle.SolidLine, Qt.PenCapStyle.RoundCap)
            painter.setPen(pen)
            painter.drawLine(self.last_point, event.pos())
            self.last_point = event.pos()
            self.update()
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event: QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            self.drawing = False
        super().mouseReleaseEvent(event)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.drawImage(0, 0, self.image)

        if self.border_points:
            pen = QPen(QColor(255, 0, 0), 3)
            painter.setPen(pen)
            for i in range(len(self.border_points) - 1):
                painter.drawLine(self.border_points[i], self.border_points[i + 1])

    def handle_click(self, point: QPoint):
        x, y = point.x(), point.y()
        if not (0 <= x < self.image.width() and 0 <= y < self.image.height()):
            return

        mode = self.main_window.mode
        if mode == "fill_color":
            self.flood_fill_line_by_line(x, y, self.fill_color)
        elif mode == "fill_pattern":
            if self.pattern_array is not None:
                self.flood_fill_with_pattern(x, y, self.pattern_array, (x, y))
        elif mode == "border":
            self.find_and_draw_border(x, y)

    # заливка по линиям
    def flood_fill_line_by_line(self, x, y, fill_color):
        if not (0 <= x < self.image.width() and 0 <= y < self.image.height()):
            return
        
        target_color = QColor(self.image.pixel(x, y))
        if target_color == self.border_color or target_color == fill_color:
            return

        # получаем точки линии
        line = []
        left = x
        while left >= 0:
            c = QColor(self.image.pixel(left, y))
            if c == self.border_color:
                break
            line.append(left)
            left -= 1

        right = x + 1
        while right < self.image.width():
            c = QColor(self.image.pixel(right, y))
            if c == self.border_color:
                break
            line.append(right)
            right += 1

        # заливаем точки
        for px in line:
            self.image.setPixelColor(px, y, fill_color)

        self.update()

        for px in line:
            self.flood_fill_line_by_line(px, y - 1, fill_color)
            self.flood_fill_line_by_line(px, y + 1, fill_color)

    # заливка паттерном
    def flood_fill_with_pattern(self, x, y, pattern, origin):
        ox, oy = origin
        h, w = pattern.shape[:2]

        if not (0 <= x < self.image.width() and 0 <= y < self.image.height()):
            return

        current_color = QColor(self.image.pixel(x, y))
        if current_color == self.border_color:
            return

        # получаем цвет паттерна
        px = (x - ox) % w
        py = (y - oy) % h
        pat_color = pattern[py, px]
        pattern_qcolor = QColor(pat_color[0], pat_color[1], pat_color[2], pat_color[3] if len(pat_color) == 4 else 255)

        if current_color == pattern_qcolor:
            return

        # получаем точки линии
        line = []
        left = x
        while left >= 0:
            c = QColor(self.image.pixel(left, y))
            if c == self.border_color:
                break
            line.append(left)
            left -= 1

        right = x + 1
        while right < self.image.width():
            c = QColor(self.image.pixel(right, y))
            if c == self.border_color:
                break
            line.append(right)
            right += 1

        # заливаем точки паттерном
        for px_line in line:
            px_pat = (px_line - ox) % w
            py_pat = (y - oy) % h
            pat_col = pattern[py_pat, px_pat]
            qc = QColor(pat_col[0], pat_col[1], pat_col[2], pat_col[3] if len(pat_col) == 4 else 255)
            self.image.setPixelColor(px_line, y, qc)

        self.update()

        for px_line in line:
            self.flood_fill_with_pattern(px_line, y - 1, pattern, origin)
            self.flood_fill_with_pattern(px_line, y + 1, pattern, origin)

    # обход и отрисовка границы
    def find_and_draw_border(self, start_x, start_y):
        # ближайшая точка границы справа
        x, y = start_x, start_y
        while 0 <= x < self.image.width():
            if QColor(self.image.pixel(x, y)) == self.border_color:
                break
            x += 1
        else:
            return  
        
        start_point = QPoint(x, y)
        self.border_points = [start_point]

        directions = [
            (1, 0), # вправо
            (1, -1), # вправо-вверх
            (0, -1), # вверх
            (-1, -1), # влево-вверх
            (-1, 0), # влево
            (-1, 1), # влево-вниз
            (0, 1), # вниз
            (1, 1) # вправо-вниз
        ]

        current = start_point
        # обход начинаем вниз
        prev_dir = 6

        while True:
            found = False
            for i in range(8):
                new_dir = (prev_dir - 2 + i) % 8
                dx, dy = directions[new_dir]
                nx, ny = current.x() + dx, current.y() + dy

                if 0 <= nx < self.image.width() and 0 <= ny < self.image.height():
                    if QColor(self.image.pixel(nx, ny)) == self.border_color:
                        next_point = QPoint(nx, ny)

                        if next_point == start_point and len(self.border_points) > 1:
                            self.border_points.append(next_point)
                            self.update()
                            return
                        
                        self.border_points.append(next_point)
                        current = next_point
                        prev_dir = new_dir
                        found = True
                        break
            if not found:
                break

        self.update()


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setGeometry(100, 100, 1000, 700)

        self.mode = "draw"

        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout()
        central_widget.setLayout(layout)

        control_layout = QHBoxLayout()
        layout.addLayout(control_layout)

        self.radio_draw = QRadioButton("Рисование")
        self.radio_draw.setChecked(True)
        self.radio_draw.toggled.connect(lambda: setattr(self, 'mode', 'draw'))
        control_layout.addWidget(self.radio_draw)

        self.radio_fill_color = QRadioButton("Заливка цветом")
        self.radio_fill_color.toggled.connect(lambda: setattr(self, 'mode', 'fill_color'))
        control_layout.addWidget(self.radio_fill_color)

        self.radio_fill_pattern = QRadioButton("Заливка паттерном")
        self.radio_fill_pattern.toggled.connect(lambda: setattr(self, 'mode', 'fill_pattern'))
        control_layout.addWidget(self.radio_fill_pattern)

        self.radio_border = QRadioButton("Граница")
        self.radio_border.toggled.connect(lambda: setattr(self, 'mode', 'border'))
        control_layout.addWidget(self.radio_border)

        self.btn_clear = QPushButton("Очистить")
        self.btn_clear.clicked.connect(self.clear_canvas)
        control_layout.addWidget(self.btn_clear)

        self.btn_color = QPushButton("Цвет заливки")
        self.btn_color.clicked.connect(self.select_fill_color)
        control_layout.addWidget(self.btn_color)

        self.btn_load_pattern = QPushButton("Загрузить паттерн")
        self.btn_load_pattern.clicked.connect(self.load_pattern)
        control_layout.addWidget(self.btn_load_pattern)

        self.drawing_widget = DrawingWidget(self)
        layout.addWidget(self.drawing_widget)

    def clear_canvas(self):
        self.drawing_widget.image.fill(Qt.GlobalColor.white)
        self.drawing_widget.border_points = []
        self.drawing_widget.update()

    def select_fill_color(self):
        color = QColorDialog.getColor()
        if color.isValid():
            self.drawing_widget.fill_color = color

    def load_pattern(self):
        file_name, _ = QFileDialog.getOpenFileName(
            self, "Выберите изображение-паттерн", "",
            "Images (*.png *.jpg *.jpeg *.bmp *.gif)"
        )
        if file_name:
            try:
                pil_img = Image.open(file_name).convert("RGBA")
                self.drawing_widget.pattern_image = pil_img
                self.drawing_widget.pattern_array = np.array(pil_img)
            except Exception as e:
                print(f"Ошибка загрузки паттерна: {e}")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec())