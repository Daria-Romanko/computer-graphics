import pygame
import numpy as np
import math
from common import Point3D, Face
from common import AffineTransform

class Texture:
    def __init__(self, image_path=None, width=256, height=256):
        self.surface = None
        self.width = width
        self.height = height
        
        if image_path:
            self.load_texture(image_path)
        else:
            # Создаем текстуру по умолчанию (шахматная доска)
            self.create_default_texture()
    
    def load_texture(self, image_path):
        """Загружает текстуру из файла"""
        try:
            self.surface = pygame.image.load(image_path).convert()
            self.width = self.surface.get_width()
            self.height = self.surface.get_height()
            print(f"Текстура загружена: {image_path} ({self.width}x{self.height})")
        except Exception as e:
            print(f"Ошибка загрузки текстуры {image_path}: {e}")
            self.create_default_texture()
    
    def create_default_texture(self):
        """Создает текстуру по умолчанию (шахматная доска)"""
        self.surface = pygame.Surface((self.width, self.height))
        
        # Рисуем шахматную доску
        tile_size = 32
        for y in range(0, self.height, tile_size):
            for x in range(0, self.width, tile_size):
                if (x // tile_size + y // tile_size) % 2 == 0:
                    color = (255, 255, 255)  # белый
                else:
                    color = (128, 128, 128)  # серый
                
                pygame.draw.rect(self.surface, color, (x, y, tile_size, tile_size))
        
        
    def get_color(self, u, v):
        """Получает цвет текстуры по координатам (u, v)"""
        if self.surface is None:
            return (255, 255, 255)
        
        # Обеспечиваем циклическое повторение текстуры
        u = u % 1.0
        v = v % 1.0
        
        x = int(u * (self.width - 1))
        y = int(v * (self.height - 1))
        
        # Обеспечиваем корректные границы
        x = max(0, min(self.width - 1, x))
        y = max(0, min(self.height - 1, y))
        
        try:
            color = self.surface.get_at((x, y))
            return (color[0], color[1], color[2])
        except:
            return (255, 255, 255)


class TexturedFace(Face):
    def __init__(self, points, texture_coords, color=(255, 255, 255), texture=None):
        super().__init__(points, color)
        self.texture_coords = texture_coords  # список кортежей (u, v) для каждой вершины
        self.texture = texture
    
    def apply_transform(self, transform_matrix):
        """Применяет преобразование к грани, сохраняя координаты текстуры"""
        transformed_points = []
        for point in self.points:
            point_array = point.to_array()
            transformed_array = np.dot(transform_matrix, point_array)
            if transformed_array[3] != 0:
                transformed_array = transformed_array / transformed_array[3]
            transformed_points.append(Point3D.from_array(transformed_array))
        
        # Создаем новую текстурированную грань с теми же координатами текстуры
        new_face = TexturedFace(transformed_points, self.texture_coords, self.color, self.texture)
        new_face.vertex_colors = self.vertex_colors.copy()
        
        return new_face


class TexturedPolyhedron:
    def __init__(self, faces, texture=None):
        self.faces = faces
        self.texture = texture
        self.transform_matrix = np.identity(4)
    
    def apply_transform(self, transform_matrix):
        self.transform_matrix = np.dot(transform_matrix, self.transform_matrix)
    
    def reset_transform(self):
        self.transform_matrix = np.identity(4)
    
    def get_transformed_faces(self):
        transformed_faces = []
        for face in self.faces:
            transformed_face = face.apply_transform(self.transform_matrix)
            if self.texture and hasattr(transformed_face, 'texture'):
                transformed_face.texture = self.texture
            transformed_faces.append(transformed_face)
        return transformed_faces
    
    def get_center(self):
        """Вычисляет центр многогранника"""
        all_points = []
        for face in self.faces:
            all_points.extend(face.points)
        
        x = sum(p.x for p in all_points) / len(all_points)
        y = sum(p.y for p in all_points) / len(all_points)
        z = sum(p.z for p in all_points) / len(all_points)
        
        return Point3D(x, y, z)
    
    def scale_about_center(self, factor):
        center_point = self.get_center()
        translate_to_origin = AffineTransform.translation(-center_point.x, -center_point.y, -center_point.z)
        scale_matrix = AffineTransform.scaling(factor, factor, factor)
        translate_back = AffineTransform.translation(center_point.x, center_point.y, center_point.z)
        total_transform = np.dot(translate_back, np.dot(scale_matrix, translate_to_origin))
        self.apply_transform(total_transform)


class TexturedTetrahedron(TexturedPolyhedron):
    def __init__(self, size=1, texture=None):
        # Вершины тетраэдра
        s = size
        vertices = [
            Point3D(s, s, s),      # 0: правая-верхняя-передняя
            Point3D(-s, -s, s),    # 1: левая-нижняя-передняя  
            Point3D(-s, s, -s),    # 2: левая-верхняя-задняя
            Point3D(s, -s, -s)     # 3: правая-нижняя-задняя
        ]
        
        # координаты текстуры - покрывают всю текстуру
        tex_coords = [
            (1, 0),  # верхний правый
            (0, 1),  # нижний левый
            (0, 0),  # верхний левый
            (1, 1)   # нижний правый
        ]
        
        # Грани тетраэдра (4 треугольника)
        faces = [
            TexturedFace([vertices[0], vertices[1], vertices[2]], 
                        [tex_coords[0], tex_coords[1], tex_coords[2]], 
                        (255, 0, 0), texture),  # передняя-верхняя
            TexturedFace([vertices[0], vertices[3], vertices[1]], 
                        [tex_coords[0], tex_coords[3], tex_coords[1]], 
                        (0, 255, 0), texture),  # передняя-нижняя
            TexturedFace([vertices[0], vertices[2], vertices[3]], 
                        [tex_coords[0], tex_coords[2], tex_coords[3]], 
                        (0, 0, 255), texture),  # правая
            TexturedFace([vertices[1], vertices[3], vertices[2]], 
                        [tex_coords[1], tex_coords[3], tex_coords[2]], 
                        (255, 255, 0), texture)  # левая
        ]
        
        super().__init__(faces, texture)


class TexturedCube(TexturedPolyhedron):
    def __init__(self, size=1, texture=None):
        # Вершины куба
        s = size
        vertices = [
            Point3D(-s, -s, -s),  # 0: левая-нижняя-задняя
            Point3D(s, -s, -s),   # 1: правая-нижняя-задняя
            Point3D(s, s, -s),    # 2: правая-верхняя-задняя
            Point3D(-s, s, -s),   # 3: левая-верхняя-задняя
            Point3D(-s, -s, s),   # 4: левая-нижняя-передняя
            Point3D(s, -s, s),    # 5: правая-нижняя-передняя
            Point3D(s, s, s),     # 6: правая-верхняя-передняя
            Point3D(-s, s, s)     # 7: левая-верхняя-передняя
        ]
        
        # координаты текстуры для куба
        faces = []
        
        # Задняя грань - полная текстура
        faces.append(TexturedFace([vertices[0], vertices[1], vertices[2], vertices[3]], 
                                 [(0, 1), (1, 1), (1, 0), (0, 0)], 
                                 (255, 0, 0), texture))
        
        # Передняя грань - полная текстура
        faces.append(TexturedFace([vertices[4], vertices[7], vertices[6], vertices[5]], 
                                 [(0, 1), (0, 0), (1, 0), (1, 1)], 
                                 (0, 255, 0), texture))
        
        # Правая грань - полная текстура
        faces.append(TexturedFace([vertices[1], vertices[5], vertices[6], vertices[2]], 
                                 [(0, 1), (1, 1), (1, 0), (0, 0)], 
                                 (0, 0, 255), texture))
        
        # Левая грань - полная текстура
        faces.append(TexturedFace([vertices[0], vertices[3], vertices[7], vertices[4]], 
                                 [(1, 1), (1, 0), (0, 0), (0, 1)], 
                                 (255, 255, 0), texture))
        
        # Верхняя грань - полная текстура
        faces.append(TexturedFace([vertices[3], vertices[2], vertices[6], vertices[7]], 
                                 [(0, 1), (1, 1), (1, 0), (0, 0)], 
                                 (255, 0, 255), texture))
        
        # Нижняя грань - полная текстура
        faces.append(TexturedFace([vertices[0], vertices[4], vertices[5], vertices[1]], 
                                 [(0, 0), (0, 1), (1, 1), (1, 0)], 
                                 (0, 255, 255), texture))
        
        super().__init__(faces, texture)


class TexturedOctahedron(TexturedPolyhedron):
    def __init__(self, size=1, texture=None):
        # Вершины октаэдра
        s = size
        vertices = [
            Point3D(0, s, 0),   # 0: Верх
            Point3D(0, -s, 0),  # 1: Низ
            Point3D(s, 0, 0),   # 2: Право
            Point3D(-s, 0, 0),  # 3: Лево
            Point3D(0, 0, s),   # 4: Перед
            Point3D(0, 0, -s)   # 5: Зад
        ]
        
        # координаты текстуры для октаэдра
        # Каждая грань получает полную текстуру
        tex_coords_full = [(0, 0), (1, 0), (1, 1)]  # для треугольников
        
        # Грани октаэдра (8 треугольников)
        faces = [
            # Верхные грани
            TexturedFace([vertices[0], vertices[4], vertices[2]], 
                        tex_coords_full, 
                        (255, 0, 0), texture),
            TexturedFace([vertices[0], vertices[2], vertices[5]], 
                        tex_coords_full, 
                        (0, 255, 0), texture),
            TexturedFace([vertices[0], vertices[5], vertices[3]], 
                        tex_coords_full, 
                        (0, 0, 255), texture),
            TexturedFace([vertices[0], vertices[3], vertices[4]], 
                        tex_coords_full, 
                        (255, 255, 0), texture),
            
            # Нижние грани  
            TexturedFace([vertices[1], vertices[2], vertices[4]], 
                        tex_coords_full, 
                        (255, 0, 255), texture),
            TexturedFace([vertices[1], vertices[5], vertices[2]], 
                        tex_coords_full, 
                        (0, 255, 255), texture),
            TexturedFace([vertices[1], vertices[3], vertices[5]], 
                        tex_coords_full, 
                        (128, 128, 255), texture),
            TexturedFace([vertices[1], vertices[4], vertices[3]], 
                        tex_coords_full, 
                        (255, 128, 0), texture)
        ]
        
        super().__init__(faces, texture)


class TexturedIcosahedron(TexturedPolyhedron):
    def __init__(self, size=1, texture=None):
        # Золотое сечение
        phi = (1 + math.sqrt(5)) / 2
        
        # Вершины икосаэдра
        vertices = [
            Point3D(-1, phi, 0), Point3D(1, phi, 0), Point3D(-1, -phi, 0), Point3D(1, -phi, 0),
            Point3D(0, -1, phi), Point3D(0, 1, phi), Point3D(0, -1, -phi), Point3D(0, 1, -phi),
            Point3D(phi, 0, -1), Point3D(phi, 0, 1), Point3D(-phi, 0, -1), Point3D(-phi, 0, 1)
        ]
        
        # Нормализуем вершины
        vertices = [Point3D(v.x * size, v.y * size, v.z * size) for v in vertices]
        
        # координаты текстуры - каждая грань получает полную текстуру
        tex_coords_full = [(0, 0), (1, 0), (1, 1)]
        
        # Грани икосаэдра (20 треугольников)
        faces = []
        colors = [
            (255, 0, 0), (255, 128, 0), (255, 255, 0), (128, 255, 0),
            (0, 255, 0), (0, 255, 128), (0, 255, 255), (0, 128, 255),
            (0, 0, 255), (128, 0, 255), (255, 0, 255), (255, 0, 128),
            (255, 128, 128), (128, 255, 128), (128, 128, 255),
            (255, 255, 128), (255, 128, 255), (128, 255, 255),
            (192, 192, 192), (128, 128, 128)
        ]
        
        # Правильные треугольники для икосаэдра
        triangles = [
            [0, 11, 5], [0, 5, 1], [0, 1, 7], [0, 7, 10], [0, 10, 11],
            [1, 5, 9], [5, 11, 4], [11, 10, 2], [10, 7, 6], [7, 1, 8],
            [3, 9, 4], [3, 4, 2], [3, 2, 6], [3, 6, 8], [3, 8, 9],
            [4, 9, 5], [2, 4, 11], [6, 2, 10], [8, 6, 7], [9, 8, 1]
        ]
        
        for i, triangle in enumerate(triangles):
            face_points = [vertices[triangle[0]], vertices[triangle[1]], vertices[triangle[2]]]
            faces.append(TexturedFace(face_points, tex_coords_full, colors[i % len(colors)], texture))
        
        super().__init__(faces, texture)


class TexturedDodecahedron(TexturedPolyhedron):
    def __init__(self, size=1, texture=None):
        # Создаем икосаэдр как основу
        icosahedron = TexturedIcosahedron(size * 1.5)
        
        # Получаем центры всех граней икосаэдра - это будут вершины додекаэдра
        dodeca_vertices = []
        for face in icosahedron.faces:
            center = face.get_center()
            # Нормализуем до единичной сферы и масштабируем
            length = math.sqrt(center.x**2 + center.y**2 + center.z**2)
            if length > 0:
                center = Point3D(
                    center.x / length * size * 0.7,
                    center.y / length * size * 0.7,
                    center.z / length * size * 0.7
                )
            dodeca_vertices.append(center)
        
        # Для каждой вершины икосаэдра находим 5 ближайших вершин додекаэдра
        # которые образуют пятиугольную грань додекаэдра
        faces = []
        colors = [
            (255, 0, 0), (255, 128, 0), (255, 255, 0), (128, 255, 0),
            (0, 255, 0), (0, 255, 128), (0, 255, 255), (0, 128, 255),
            (0, 0, 255), (128, 0, 255), (255, 0, 255), (255, 0, 128)
        ]
        
        # Получаем все уникальные вершины икосаэдра
        icosa_vertices = []
        vertex_set = set()
        for face in icosahedron.faces:
            for point in face.points:
                point_key = (round(point.x, 3), round(point.y, 3), round(point.z, 3))
                if point_key not in vertex_set:
                    vertex_set.add(point_key)
                    icosa_vertices.append(point)
        
        # координаты текстуры для пятиугольников
        pentagon_tex_coords = [(0.5, 0), (1, 0.3), (0.8, 1), (0.2, 1), (0, 0.3)]
        
        # Для каждой вершины икосаэдра создаем пятиугольную грань
        for i, icosa_vertex in enumerate(icosa_vertices):
            # Находим расстояния до всех вершин додекаэдра
            distances = []
            for j, dodeca_vertex in enumerate(dodeca_vertices):
                dx = dodeca_vertex.x - icosa_vertex.x
                dy = dodeca_vertex.y - icosa_vertex.y
                dz = dodeca_vertex.z - icosa_vertex.z
                dist = math.sqrt(dx*dx + dy*dy + dz*dz)
                distances.append((dist, j, dodeca_vertex))
            
            # Сортируем по расстоянию и берем 5 ближайших
            distances.sort(key=lambda x: x[0])
            closest_vertices = [vertex for dist, idx, vertex in distances[:5]]
            
            if len(closest_vertices) != 5:
                continue
            
            # Сортируем вершины в правильном порядке вокруг нормали
            center = Point3D(
                sum(v.x for v in closest_vertices) / 5,
                sum(v.y for v in closest_vertices) / 5,
                sum(v.z for v in closest_vertices) / 5
            )
            
            # Вычисляем нормаль
            v1 = closest_vertices[1] - closest_vertices[0]
            v2 = closest_vertices[2] - closest_vertices[0]
            normal = v1.cross(v2).normalize()
            
            # Создаем локальную систему координат
            if abs(normal.x) > 0.1 or abs(normal.y) > 0.1:
                tangent = Point3D(-normal.y, normal.x, 0).normalize()
            else:
                tangent = Point3D(0, -normal.z, normal.y).normalize()
            
            binormal = normal.cross(tangent).normalize()
            
            # Сортируем вершины по углу
            def get_angle(vertex):
                vec = (vertex - center).normalize()
                x_proj = vec.dot(tangent)
                y_proj = vec.dot(binormal)
                return math.atan2(y_proj, x_proj)
            
            # Сортируем вершины
            sorted_vertices = sorted(closest_vertices, key=get_angle)
            
            # Создаем грань с улучшенными текстурными координатами
            faces.append(TexturedFace(sorted_vertices, pentagon_tex_coords, 
                                    colors[len(faces) % len(colors)], texture))
            
            if len(faces) >= 12:
                break
        
        super().__init__(faces, texture)


class TextureRenderer:
    def __init__(self, lighting_system=None):
        self.lighting = lighting_system
    
    def barycentric_coords(self, x, y, points_2d):
        """Вычисляет барицентрические координаты для точки (x, y) относительно треугольника"""
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
    
    def draw_textured_triangle(self, screen, points_2d, tex_coords, texture, face_color=None, 
                              use_lighting=False, vertex_colors=None, z_buffer=None, depths=None,
                              camera_position=None, use_perspective=True):
        """Отрисовывает текстурированный треугольник с перспективно-корректной интерполяцией"""
        if len(points_2d) != 3 or len(tex_coords) != 3:
            return
        
        # Находим ограничивающий прямоугольник
        min_x = max(0, int(min(p[0] for p in points_2d)))
        max_x = min(screen.get_width() - 1, int(max(p[0] for p in points_2d)))
        min_y = max(0, int(min(p[1] for p in points_2d)))
        max_y = min(screen.get_height() - 1, int(max(p[1] for p in points_2d)))
        
        if min_x >= max_x or min_y >= max_y:
            return
        
        # Для перспективно-корректной интерполяции используем 1/z
        if use_perspective and depths is not None:
            inv_depths = [1.0 / max(d, 0.001) for d in depths]
        else:
            inv_depths = [1.0, 1.0, 1.0]
        
        # Рисуем пиксели с перспективно-корректной интерполяцией текстуры
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                w1, w2, w3 = self.barycentric_coords(x, y, points_2d)
                
                # Проверяем, находится ли точка внутри треугольника
                if w1 >= -0.001 and w2 >= -0.001 and w3 >= -0.001:
                    
                    if use_perspective and depths is not None:
                        # Перспективно-корректная интерполяция текстурных координат
                        inv_z = w1 * inv_depths[0] + w2 * inv_depths[1] + w3 * inv_depths[2]
                        if abs(inv_z) < 1e-10:
                            continue
                        
                        # Интерполируем u/z и v/z
                        u_over_z = (w1 * tex_coords[0][0] * inv_depths[0] + 
                                   w2 * tex_coords[1][0] * inv_depths[1] + 
                                   w3 * tex_coords[2][0] * inv_depths[2])
                        v_over_z = (w1 * tex_coords[0][1] * inv_depths[0] + 
                                   w2 * tex_coords[1][1] * inv_depths[1] + 
                                   w3 * tex_coords[2][1] * inv_depths[2])
                        
                        # Восстанавливаем u и v
                        u = u_over_z / inv_z
                        v = v_over_z / inv_z
                    else:
                        # Линейная интерполяция (для аксонометрии)
                        u = w1 * tex_coords[0][0] + w2 * tex_coords[1][0] + w3 * tex_coords[2][0]
                        v = w1 * tex_coords[0][1] + w2 * tex_coords[1][1] + w3 * tex_coords[2][1]
                    
                    # Получаем цвет из текстуры
                    tex_color = texture.get_color(u, v)
                    
                    # Применяем освещение, если включено
                    if use_lighting and vertex_colors and self.lighting:
                        # Интерполируем цвет освещения
                        light_color = self.lighting.interpolate_color(w1, w2, w3, vertex_colors)
                        
                        # Вычисляем яркость как среднее значение RGB
                        brightness = (light_color[0] + light_color[1] + light_color[2]) / (3 * 255.0)
                        
                        # Применяем яркость к текстуре
                        final_color = (
                            min(255, int(tex_color[0] * brightness)),
                            min(255, int(tex_color[1] * brightness)),
                            min(255, int(tex_color[2] * brightness))
                        )
                    else:
                        final_color = tex_color
                    
                    # Обработка Z-буфера
                    if z_buffer is not None and depths is not None:
                        if use_perspective:
                            # Для перспективы используем корректную интерполяцию глубины
                            z = 1.0 / inv_z
                        else:
                            # Для аксонометрии - линейную интерполяцию
                            z = w1 * depths[0] + w2 * depths[1] + w3 * depths[2]
                        
                        if 0 <= x < z_buffer.width and 0 <= y < z_buffer.height:
                            if z < z_buffer.buffer[y, x]:
                                z_buffer.buffer[y, x] = z
                                z_buffer.color_buffer[y, x] = final_color
                    else:
                        # Без Z-буфера
                        if 0 <= x < screen.get_width() and 0 <= y < screen.get_height():
                            screen.set_at((x, y), final_color)
    
    def draw_textured_face_with_zbuffer(self, screen, face, project_3d_to_2d, camera_position,
                                       z_buffer, use_lighting=False, use_perspective=True):
        """Отрисовывает текстурированную грань с Z-буфером"""
        points_2d = []
        depths = []
        
        # Проецируем точки и вычисляем глубины
        for point in face.points:
            projected = project_3d_to_2d(point)
            points_2d.append(projected)
            depth = (point - camera_position).length()
            depths.append(depth)
        
        # Разбиваем грань на треугольники для отрисовки
        if len(points_2d) >= 3:
            if len(points_2d) == 3:
                self.draw_textured_triangle(
                    screen, points_2d, face.texture_coords, face.texture,
                    face.color, use_lighting, face.vertex_colors, 
                    z_buffer, depths, camera_position, use_perspective
                )
            else:
                for i in range(1, len(points_2d) - 1):
                    tri_points = [points_2d[0], points_2d[i], points_2d[i + 1]]
                    tri_tex_coords = [face.texture_coords[0], face.texture_coords[i], face.texture_coords[i + 1]]
                    tri_depths = [depths[0], depths[i], depths[i + 1]]
                    tri_colors = None
                    if face.vertex_colors:
                        tri_colors = [face.vertex_colors[0], face.vertex_colors[i], face.vertex_colors[i + 1]]
                    
                    self.draw_textured_triangle(
                        screen, tri_points, tri_tex_coords, face.texture,
                        face.color, use_lighting, tri_colors,
                        z_buffer, tri_depths, camera_position, use_perspective
                    )
    
    def draw_textured_face_without_zbuffer(self, screen, face, project_3d_to_2d, camera_position,
                                          use_lighting=False, use_perspective=True):
        """Отрисовывает текстурированную грань без Z-буфера"""
        points_2d = []
        depths = []
        
        # Проецируем точки и вычисляем глубины
        for point in face.points:
            projected = project_3d_to_2d(point)
            points_2d.append(projected)
            depth = (point - camera_position).length()
            depths.append(depth)
        
        # Разбиваем грань на треугольники для отрисовки
        if len(points_2d) >= 3:
            # Для треугольников отрисовываем напрямую
            if len(points_2d) == 3:
                self.draw_textured_triangle(
                    screen, points_2d, face.texture_coords, face.texture,
                    face.color, use_lighting, face.vertex_colors, 
                    None, depths, camera_position, use_perspective
                )
            else:
                # Для многоугольников разбиваем на треугольники
                for i in range(1, len(points_2d) - 1):
                    tri_points = [points_2d[0], points_2d[i], points_2d[i + 1]]
                    tri_tex_coords = [face.texture_coords[0], face.texture_coords[i], face.texture_coords[i + 1]]
                    tri_depths = [depths[0], depths[i], depths[i + 1]]
                    tri_colors = None
                    if face.vertex_colors:
                        tri_colors = [face.vertex_colors[0], face.vertex_colors[i], face.vertex_colors[i + 1]]
                    
                    self.draw_textured_triangle(
                        screen, tri_points, tri_tex_coords, face.texture,
                        face.color, use_lighting, tri_colors,
                        None, tri_depths, camera_position, use_perspective
                    )
    
    def draw_textured_face(self, screen, face, project_3d_to_2d, camera_position, 
                          use_zbuffer=False, z_buffer=None, use_lighting=False, use_perspective=True):
        """Отрисовывает текстурированную грань"""
        if not hasattr(face, 'texture_coords') or face.texture is None:
            return
        
        if use_zbuffer and z_buffer is not None:
            # Используем Z-буфер
            self.draw_textured_face_with_zbuffer(screen, face, project_3d_to_2d, camera_position, 
                                               z_buffer, use_lighting, use_perspective)
        else:
            # Без Z-буфера
            self.draw_textured_face_without_zbuffer(screen, face, project_3d_to_2d, camera_position,
                                                  use_lighting, use_perspective)
