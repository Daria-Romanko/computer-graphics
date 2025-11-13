import numpy as np
import math
from common import Point3D

class Camera:
    def __init__(self, position=Point3D(0, 0, -5), target=Point3D(0, 0, 0), up_vector=Point3D(0, 1, 0), 
                 fov=60, aspect_ratio=1.0, near_plane=0.1, far_plane=100.0):
        self.position = position
        self.target = target
        self.up_vector = up_vector
        self.fov = fov
        self.aspect_ratio = aspect_ratio
        self.near_plane = near_plane
        self.far_plane = far_plane
        self.view_matrix = None
        self.projection_matrix = None

        # Вычисляем начальные углы на основе позиции камеры
        self._update_angles_from_position()
        
        self.update_matrices()
    
    def _update_angles_from_position(self):
        """Обновляет углы angle_x и angle_y на основе текущей позиции камеры"""
        # Вектор от цели к камере
        direction = self.position - self.target
        distance = direction.length()
        
        if distance > 0:
            # Нормализуем вектор
            direction = direction.normalize()
            
            # Вычисляем углы в сферических координатах
            self.angle_x = math.acos(direction.y)  # угол от оси Y
            self.angle_y = math.atan2(direction.z, direction.x)  # угол в плоскости XZ
            
            # Ограничиваем угол angle_x, чтобы камера не переворачивалась
            self.angle_x = max(0.1, min(math.pi - 0.1, self.angle_x))
        else:
            # Значения по умолчанию, если камера в цели
            self.angle_x = math.radians(30)
            self.angle_y = math.radians(45)
    
    def update_matrices(self):
        """Обновляет матрицы вида и проекции"""
        self.view_matrix = self._calculate_view_matrix()
        self.projection_matrix = self._calculate_projection_matrix()
    
    def _calculate_view_matrix(self):
        """Вычисляет матрицу вида по алгоритму look-at"""
        # Вектор направления (forward)
        f = (self.target - self.position).normalize()
        
        # Вектор вправо (right) - обратите внимание на порядок: cross(up, f)
        s = self.up_vector.normalize().cross(f).normalize()
        
        # Вектор вверх (up)
        u = f.cross(s).normalize()
        
        # Матрица вида согласно алгоритму
        view_matrix = np.array([
            [s.x, s.y, s.z, -s.dot(self.position)],
            [u.x, u.y, u.z, -u.dot(self.position)],
            [-f.x, -f.y, -f.z, f.dot(self.position)],
            [0, 0, 0, 1]
        ])
        
        return view_matrix
    
    def _calculate_projection_matrix(self):
        """Перспективная матрица проекции"""
        f = 1.0 / math.tan(math.radians(self.fov) / 2)
        n, f_far = self.near_plane, self.far_plane
        a = self.aspect_ratio

        return np.array([
            [f / a, 0, 0, 0],
            [0, f, 0, 0],
            [0, 0, -(f_far + n) / (f_far - n), -(2 * f_far * n) / (f_far - n)],
            [0, 0, -1, 0]
        ])
    
    def rotate_around_target(self, dx, dy, distance=None):
        """Вращает камеру вокруг цели с использованием матричного подхода"""
        if distance is None:
            distance = (self.position - self.target).length()

        self.angle_x += dx
        self.angle_y += dy

        # Ограничим угол, чтобы не переворачивалась
        self.angle_x = max(0.1, min(math.pi - 0.1, self.angle_x))

        # Вычисляем новую позицию камеры сферическими координатами
        x = distance * math.sin(self.angle_x) * math.cos(self.angle_y)
        y = distance * math.cos(self.angle_x)
        z = distance * math.sin(self.angle_x) * math.sin(self.angle_y)

        self.position = Point3D(self.target.x + x, self.target.y + y, self.target.z + z)
        self.update_matrices()
    
    def move_forward(self, dist):
        """Движение вперед/назад"""
        direction = (self.target - self.position).normalize()
        self.position += direction * dist
        self.target += direction * dist
        self._update_angles_from_position()
        self.update_matrices()

    def move_vertical(self, dist):
        """Движение вверх/вниз"""
        up = self.up_vector.normalize()
        self.position += up * dist
        self.target += up * dist
        self._update_angles_from_position()
        self.update_matrices()

    def strafe(self, dist):
        """Движение влево/вправо"""
        forward = (self.target - self.position).normalize()
        right = self.up_vector.normalize().cross(forward).normalize()
        self.position += right * dist
        self.target += right * dist
        self._update_angles_from_position()
        self.update_matrices()
    
    def get_view_projection_matrix(self):
        """Возвращает комбинированную матрицу вида-проекции"""
        return np.dot(self.projection_matrix, self.view_matrix)
    
    def set_position(self, position):
        """Устанавливает позицию камеры"""
        self.position = position
        self._update_angles_from_position()
        self.update_matrices()
    
    def set_target(self, target):
        """Устанавливает цель камеры"""
        self.target = target
        self._update_angles_from_position()
        self.update_matrices()
    
    def set_fov(self, fov):
        """Устанавливает поле зрения"""
        self.fov = fov
        self.update_matrices()
    
    def set_aspect_ratio(self, aspect_ratio):
        """Устанавливает соотношение сторон"""
        self.aspect_ratio = aspect_ratio
        self.update_matrices()