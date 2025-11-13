import numpy as np
import math
import pygame
from common import Point3D
from z_buffer import ZBuffer

class Lighting:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        
        self.light_position = Point3D(5, 5, -5)  # позиция источника света
        self.light_color = (255, 255, 255)       # цвет света (белый) (255, 20, 147)(розовый)
        self.ambient_intensity = 0.2             # фоновое освещение
        self.diffuse_intensity = 0.8             # диффузное освещение
        
        self.use_lighting = False
        self.shading_mode = "gouraud"  # "gouraud"
        
        self.z_buffer = ZBuffer(width, height)

    def set_light_position(self, position):
        self.light_position = position
        
    def set_lighting_enabled(self, enabled):
        self.use_lighting = enabled

    def set_shading_mode(self, mode):
        self.shading_mode = mode

    def calculate_vertex_color_lambert(self, vertex, normal, base_color, camera_position=None):
        to_light = (self.light_position - vertex).normalize()
        normal = normal.normalize()
        
        # косинус угла между нормалью и направлением к свету
        cos_angle = max(0, normal.dot(to_light))
        
        base_color_array = np.array(base_color)
        light_color_array = np.array(self.light_color)
        
        # диффузная составляющая
        diffuse = base_color_array * light_color_array / 255.0 * self.diffuse_intensity * cos_angle
        
        # фоновая составляющая
        ambient = base_color_array * self.ambient_intensity
        
        final_color = np.minimum(255, (ambient + diffuse).astype(int))
        
        return tuple(final_color)
    
    def calculate_vertex_normals(self, polyhedron):
        """
        Вычисляет нормали для каждой вершины многогранника
        """
        vertex_normals = {}
        vertex_faces = {}
        
        for face in polyhedron.faces:
            for vertex in face.points:
                vertex_key = (vertex.x, vertex.y, vertex.z)
                if vertex_key not in vertex_normals:
                    vertex_normals[vertex_key] = Point3D(0, 0, 0)
                    vertex_faces[vertex_key] = 0
        
        for face in polyhedron.faces:
            face_normal = face.get_normal()
            for vertex in face.points:
                vertex_key = (vertex.x, vertex.y, vertex.z)
                vertex_normals[vertex_key] = vertex_normals[vertex_key] + face_normal
                vertex_faces[vertex_key] += 1
        
        for vertex_key in vertex_normals:
            if vertex_faces[vertex_key] > 0:
                vertex_normals[vertex_key] = vertex_normals[vertex_key].normalize()
        
        return vertex_normals

    def apply_gouraud_shading(self, polyhedron, camera_position):
        """
        Применяет шейдинг Гуро к многограннику 
        Вычисляет цвет в каждой вершине по модели Ламберта
        """
        # 1. Вычисляем нормали вершин
        vertex_normals = self.calculate_vertex_normals(polyhedron)
        
        # 2. Вычисляем цвет в каждой вершине по модели Ламберта
        for face in polyhedron.faces:
            face.vertex_colors = []
            for vertex in face.points:
                vertex_key = (vertex.x, vertex.y, vertex.z)
                normal = vertex_normals.get(vertex_key, Point3D(0, 0, 1))
                
                vertex_color = self.calculate_vertex_color_lambert(
                    vertex, normal, face.color, camera_position
                )
                face.vertex_colors.append(vertex_color)

    def apply_phong_shading(self, vertex, normal, base_color, camera_position):
        """
        ШЕЙДИНГ ФОНГА
        """
        return base_color
    
    def barycentric_coords(self, x, y, points_2d):
        if len(points_2d) != 3:
            return (0, 0, 0)
        
        p1, p2, p3 = points_2d
        denom = (p2[1] - p3[1]) * (p1[0] - p3[0]) + (p3[0] - p2[0]) * (p1[1] - p3[1])
        if abs(denom) < 1e-10:
            return (0, 0, 0)
        
        w1 = ((p2[1] - p3[1]) * (x - p3[0]) + (p3[0] - p2[0]) * (y - p3[1])) / denom
        w2 = ((p3[1] - p1[1]) * (x - p3[0]) + (p1[0] - p3[0]) * (y - p3[1])) / denom
        w3 = 1 - w1 - w2
        
        return (w1, w2, w3)

    def interpolate_color(self, w1, w2, w3, vertex_colors):
        if len(vertex_colors) != 3:
            return (255, 255, 255)
        
        # Билинейная интерполяция цвета
        r = int(w1 * vertex_colors[0][0] + w2 * vertex_colors[1][0] + w3 * vertex_colors[2][0])
        g = int(w1 * vertex_colors[0][1] + w2 * vertex_colors[1][1] + w3 * vertex_colors[2][1])
        b = int(w1 * vertex_colors[0][2] + w2 * vertex_colors[1][2] + w3 * vertex_colors[2][2])
        
        # Ограничиваем значения цвета
        r = max(0, min(255, r))
        g = max(0, min(255, g))
        b = max(0, min(255, b))
        
        return (r, g, b)

    def draw_triangle_with_lighting(self, screen, points_2d, depths, vertex_colors, use_zbuffer=False, face_color=None):
        """
        метод отрисовки треугольника с поддержкой освещения и шейдинга Гуро
        """
        if len(points_2d) != 3:
            return
        
        # Находим ограничивающий прямоугольник
        min_x = max(0, int(min(p[0] for p in points_2d)))
        max_x = min(self.width - 1, int(max(p[0] for p in points_2d)))
        min_y = max(0, int(min(p[1] for p in points_2d)))
        max_y = min(self.height - 1, int(max(p[1] for p in points_2d)))
        
        # Рисуем пиксели с интерполяцией цвета
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                w1, w2, w3 = self.barycentric_coords(x, y, points_2d)
                
                # Проверяем, находится ли точка внутри треугольника
                if w1 >= 0 and w2 >= 0 and w3 >= 0:
                    if use_zbuffer:
                        # Интерполируем глубину
                        z = w1 * depths[0] + w2 * depths[1] + w3 * depths[2]
                        
                        # Проверяем Z-буфер
                        if 0 <= x < self.width and 0 <= y < self.height:
                            if z < self.z_buffer.buffer[y, x]:
                                # Обновляем Z-буфер
                                self.z_buffer.buffer[y, x] = z
                                
                                # Определяем цвет (шейдинг Гуро)
                                if vertex_colors:
                                    color = self.interpolate_color(w1, w2, w3, vertex_colors)
                                else:
                                    color = face_color or (255, 255, 255)
                                
                                # Обновляем цвет в буфере
                                self.z_buffer.color_buffer[y, x] = color
                    else:
                        # Без Z-буфера
                        if vertex_colors:
                            color = self.interpolate_color(w1, w2, w3, vertex_colors)
                        else:
                            color = face_color or (255, 255, 255)
                        
                        if 0 <= x < self.width and 0 <= y < self.height:
                            screen.set_at((x, y), color)

    def draw_with_z_buffer_gouraud(self, screen, faces, project_3d_to_2d, camera_position):
        """
        Отрисовка с использованием Z-буфера и шейдинга Гуро
        """
        self.z_buffer.clear()
        screen.fill((0, 0, 0))
        
        for face in faces:
            points_2d = []
            depths = []
            
            # Проецируем точки и вычисляем глубины
            for point in face.points:
                projected = project_3d_to_2d(point)
                points_2d.append(projected)
                
                # Для глубины используем расстояние от камеры
                depth = (point - camera_position).length()
                depths.append(depth)
            
            # Отрисовываем грань с шейдингом Гуро
            if len(points_2d) >= 3:
                if len(points_2d) == 3:
                    # Для треугольников - напрямую
                    self.draw_triangle_with_lighting(screen, points_2d, depths, face.vertex_colors, use_zbuffer=True)
                else:
                    # Разбиваем многоугольник на треугольники
                    for i in range(1, len(points_2d) - 1):
                        tri_points = [points_2d[0], points_2d[i], points_2d[i + 1]]
                        tri_depths = [depths[0], depths[i], depths[i + 1]]
                        tri_colors = [face.vertex_colors[0], face.vertex_colors[i], face.vertex_colors[i + 1]]
                        self.draw_triangle_with_lighting(screen, tri_points, tri_depths, tri_colors, use_zbuffer=True)
        
        # Копируем буфер на экран
        pygame.surfarray.blit_array(screen, self.z_buffer.color_buffer.swapaxes(0, 1))

    

    def rotate_light_around_object(self, center, angle_degrees=45):
        angle_rad = math.radians(angle_degrees)
        
        light_vector = self.light_position - center
        
        new_x = light_vector.x * math.cos(angle_rad) - light_vector.z * math.sin(angle_rad)
        new_z = light_vector.x * math.sin(angle_rad) + light_vector.z * math.cos(angle_rad)
        
        self.light_position = Point3D(
            new_x + center.x,
            self.light_position.y,
            new_z + center.z
        )
    
    def clear_z_buffer(self):
        self.z_buffer.clear()

    def get_lighting_info_text(self):
        """Возвращает текстовую информацию о состоянии освещения"""
        lighting_text = "Lighting ON" if self.use_lighting else "Lighting OFF"
        shading_text = f"Shading: {self.shading_mode}" if self.use_lighting else "No Shading"
        return lighting_text, shading_text