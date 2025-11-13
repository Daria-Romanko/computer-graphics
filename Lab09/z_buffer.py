import numpy as np
import math

class ZBuffer:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.buffer = np.full((height, width), np.inf)  # Инициализируем +бесконечностью
        self.color_buffer = np.zeros((height, width, 3), dtype=np.uint8)
    
    def clear(self):
        self.buffer.fill(np.inf)  # Сбрасываем в +бесконечность
        self.color_buffer.fill(0)
    
    def update(self, x, y, z, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            # Для перспективной проекции: чем БЛИЖЕ объект, тем МЕНЬШЕ значение z
            if z < self.buffer[y, x]:  # Меняем сравнение на <
                self.buffer[y, x] = z
                self.color_buffer[y, x] = color
    
    def draw_triangle(self, points_2d, depths, color, use_perspective=False):
        """Корректная реализация z-буфера для треугольников"""
        if len(points_2d) != 3:
            return
        
        # Находим ограничивающий прямоугольник
        min_x = max(0, int(min(p[0] for p in points_2d)))
        max_x = min(self.width - 1, int(max(p[0] for p in points_2d)))
        min_y = max(0, int(min(p[1] for p in points_2d)))
        max_y = min(self.height - 1, int(max(p[1] for p in points_2d)))
        
        if min_x >= max_x or min_y >= max_y:
            return
        
        # Преобразуем точки в numpy массивы
        p0 = np.array([points_2d[0][0], points_2d[0][1]])
        p1 = np.array([points_2d[1][0], points_2d[1][1]])
        p2 = np.array([points_2d[2][0], points_2d[2][1]])
        
        # Вычисляем площадь треугольника для проверки ориентации
        area = (p1[0] - p0[0]) * (p2[1] - p0[1]) - (p2[0] - p0[0]) * (p1[1] - p0[1])
        if abs(area) < 1e-10:
            return
        
        # Предварительные вычисления для барицентрических координат
        v0 = p1 - p0
        v1 = p2 - p0
        d00 = np.dot(v0, v0)
        d01 = np.dot(v0, v1)
        d11 = np.dot(v1, v1)
        denom = d00 * d11 - d01 * d01
        
        if abs(denom) < 1e-10:
            return
        
        inv_denom = 1.0 / denom
        
        # Обрабатываем каждый пиксель в bounding box
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                point = np.array([x, y])
                v2 = point - p0
                
                d20 = np.dot(v2, v0)
                d21 = np.dot(v2, v1)
                
                v = (d11 * d20 - d01 * d21) * inv_denom
                w = (d00 * d21 - d01 * d20) * inv_denom
                u = 1.0 - v - w
                
                # Проверяем, находится ли точка внутри треугольника
                if u >= -0.001 and v >= -0.001 and w >= -0.001:
                    if use_perspective:
                        # Для перспективы: корректная интерполяция с использованием 1/z
                        z0, z1, z2 = depths
                        
                        # Интерполируем 1/z
                        inv_z = u * (1.0/z0) + v * (1.0/z1) + w * (1.0/z2)
                        if abs(inv_z) < 1e-10:
                            continue
                        z_value = 1.0 / inv_z
                    else:
                        # Для аксонометрии: линейная интерполяция z
                        z_value = u * depths[0] + v * depths[1] + w * depths[2]
                    
                    self.update(x, y, z_value, color)