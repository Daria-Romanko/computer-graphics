import sys
from PIL import Image
from PyQt6.QtWidgets import (QApplication, QMainWindow, QVBoxLayout, 
                             QPushButton, QSlider, QLabel, QFileDialog, QWidget)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QPixmap, QImage

class SimpleHSVEditor(QMainWindow):
    def __init__(self):
        super().__init__()
        self.original_image = None
        self.processed_image = None
        self.setup_ui()
        
    def setup_ui(self):
        self.setGeometry(100, 100, 1000, 800)
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        layout = QVBoxLayout()
        
        self.image_label = QLabel()
        self.image_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.image_label.setMinimumSize(600, 600)
        layout.addWidget(self.image_label)

        self.hue_slider = self.create_slider("Hue:", -360, 360, 0)
        self.saturation_slider = self.create_slider("Saturation:", -100, 100, 0)
        self.brightness_slider = self.create_slider("Brightness:", -100, 100, 0)
        
        layout.addWidget(self.hue_slider['label'])
        layout.addWidget(self.hue_slider['slider'])
        layout.addWidget(self.saturation_slider['label'])
        layout.addWidget(self.saturation_slider['slider'])
        layout.addWidget(self.brightness_slider['label'])
        layout.addWidget(self.brightness_slider['slider'])
        
        buttons_layout = QVBoxLayout()
        
        self.load_btn = QPushButton("Загрузить")
        self.save_btn = QPushButton("Сохранить")
        
        self.load_btn.clicked.connect(self.load_image)
        self.save_btn.clicked.connect(self.save_image)
        
        buttons_layout.addWidget(self.load_btn)
        buttons_layout.addWidget(self.save_btn)
        
        layout.addLayout(buttons_layout)
        central_widget.setLayout(layout)
        
        self.hue_slider['slider'].valueChanged.connect(self.update_display)
        self.saturation_slider['slider'].valueChanged.connect(self.update_display)
        self.brightness_slider['slider'].valueChanged.connect(self.update_display)
    
    def create_slider(self, text, min_val, max_val, default):
        slider_dict = {}
        slider_dict['label'] = QLabel(f"{text} {default}")
        slider_dict['slider'] = QSlider(Qt.Orientation.Horizontal)
        slider_dict['slider'].setRange(min_val, max_val)
        slider_dict['slider'].setValue(default)
        return slider_dict
    
    # H ∈ [0, 360]  
    # S ∈ [0, 1] 
    # V ∈ [0, 1]
    def rgb_to_hsv(self, r, g, b):
        # нормализация
        red = r / 255.0
        green = g / 255.0
        blue= b / 255.0
        
        # находим максимальное и минимальное из всех цветов
        min_val = min(red, green, blue)
        max_val = max(red, green, blue)
        
        # оттенок
        hue = 0.0
        if max_val == min_val:
            hue = 0.0
        elif max_val == red and green >= blue:
            hue = 60 * (green - blue) / (max_val - min_val)
        elif max_val == red and green < blue:
            hue = 60 * (green - blue) / (max_val - min_val) + 360
        elif max_val == green:
            hue = 60 * (blue - red) / (max_val - min_val) + 120
        elif max_val == blue:
            hue = 60 * (red - green) / (max_val - min_val) + 240
        
        # насыщенность
        saturation = 0.0 if max_val == 0 else 1.0 - (min_val / max_val)

        # яркость
        value = max_val
        
        return hue, saturation, value
    
    # H ∈ [0, 360]  
    # S ∈ [0, 100] 
    # V ∈ [0, 100]
    def hsv_to_rgb(self, hue, saturation, value):
        hi = int(hue // 60) % 6

        # значения в процентах
        vmin = ((100 - saturation) * value) / 100
        a = (value - vmin) * (hue % 60) / 60
        vinc = (vmin + a)
        vdec = (value - a)
        
        # переводим в соответствии с распространенным представлением
        vmin = int(vmin * 255 / 100)
        value = int(value * 255 / 100)
        vinc = int(vinc * 255 / 100)
        vdec = int(vdec * 255 / 100)

        if hi == 0: return (value, vinc, vmin)
        elif hi == 1: return (vdec, value, vmin)
        elif hi == 2: return (vmin, value, vinc)
        elif hi == 3: return (vmin, vdec, value)
        elif hi == 4: return (vinc, vmin, value)
        else: return (value, vmin, vdec)
    
    def apply_hsv_adjustments(self):
        if self.original_image is None:
            return None
        
        hue_shift = self.hue_slider['slider'].value()
        sat_scale = self.saturation_slider['slider'].value() / 100.0
        val_scale = self.brightness_slider['slider'].value() / 100.0
        
        img = self.original_image.convert('RGB')
        width, height = img.size
        result = Image.new('RGB', (width, height))
        
        for y in range(height):
            for x in range(width):
                red, green, blue = img.getpixel((x, y))
                h, s, v = self.rgb_to_hsv(red, green, blue)
                
                # изменение значений оттенка, насыщенности и яркости
                h = (h + hue_shift) % 360 # от 0 до 360
                s = min(1,max(0, s * (sat_scale + 1))) * 100 # от 0 до 100
                v = min(1,max(0, v * (val_scale + 1))) * 100 # от 0 до 100
                
                new_r, new_g, new_b = self.hsv_to_rgb(h, s, v)
                result.putpixel((x, y), (new_r, new_g, new_b))
        
        return result
    
    def update_display(self):
        self.hue_slider['label'].setText(f"Hue: {self.hue_slider['slider'].value()}°")
        self.saturation_slider['label'].setText(f"Saturation: {self.saturation_slider['slider'].value()}%")
        self.brightness_slider['label'].setText(f"Brightness: {self.brightness_slider['slider'].value()}%")
        
        if self.original_image is not None:
            self.processed_image = self.apply_hsv_adjustments()
            self.show_image(self.processed_image)
    
    def show_image(self, image):
        qimage = QImage(
            image.tobytes(), 
            image.width, 
            image.height, 
            image.width * 3, 
            QImage.Format.Format_RGB888
        )
        pixmap = QPixmap.fromImage(qimage)
        
        scaled_pixmap = pixmap.scaled(
            self.image_label.width(), 
            self.image_label.height(),
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self.image_label.setPixmap(scaled_pixmap)
    
    def load_image(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Загрузить", "", "Images (*.png *.jpg *.jpeg *.bmp)"
        )
        
        if file_path:
            try:
                self.original_image = Image.open(file_path)
                self.update_display()
            except Exception as e:
                print(f"Error: {e}")
    
    def save_image(self):
        if self.processed_image is None:
            return
        
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Сохранить", "image.jpg", "JPEG (*.jpg);;PNG (*.png)"
        )
        
        if file_path:
            try:
                self.processed_image.save(file_path)
            except Exception as e:
                print(f"Error: {e}")

def main():
    app = QApplication(sys.argv)
    window = SimpleHSVEditor()
    window.show()
    sys.exit(app.exec())

if __name__ == '__main__':
    main()