import pygame
import numpy as np
import math
import sys

class Point3D:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
    
    def to_array(self):
        return np.array([self.x, self.y, self.z, 1])
    
    @classmethod
    def from_array(cls, arr):
        return cls(arr[0], arr[1], arr[2])
    
    def __sub__(self, other):
        return Point3D(self.x - other.x, self.y - other.y, self.z - other.z)
    
    def dot(self, other):
        return self.x * other.x + self.y * other.y + self.z * other.z

class Face:
    def __init__(self, points, color=(255, 255, 255)):
        self.points = points
        self.color = color
    
    def apply_transform(self, transform_matrix):
        transformed_points = []
        for point in self.points:
            point_array = point.to_array()
            transformed_array = np.dot(transform_matrix, point_array)
            if transformed_array[3] != 0:
                transformed_array = transformed_array / transformed_array[3]
            transformed_points.append(Point3D.from_array(transformed_array))
        return Face(transformed_points, self.color)
    
    def get_center(self):
        x = sum(p.x for p in self.points) / len(self.points)
        y = sum(p.y for p in self.points) / len(self.points)
        z = sum(p.z for p in self.points) / len(self.points)
        return Point3D(x, y, z)
    
    def get_normal(self):
        if len(self.points) < 3:
            return Point3D(0, 0, 1)
        
        v1 = self.points[1] - self.points[0]
        v2 = self.points[2] - self.points[0]
        
        nx = v1.y * v2.z - v1.z * v2.y
        ny = v1.z * v2.x - v1.x * v2.z
        nz = v1.x * v2.y - v1.y * v2.x
        
        length = math.sqrt(nx*nx + ny*ny + nz*nz)
        if length > 0:
            nx /= length
            ny /= length
            nz /= length
        
        return Point3D(nx, ny, nz)
    
    def is_visible(self, camera_position=Point3D(0, 0, -5)):
        normal = self.get_normal()
        center = self.get_center()
        
        view_vector = camera_position - center
        
        length = math.sqrt(view_vector.x**2 + view_vector.y**2 + view_vector.z**2)
        if length > 0:
            view_vector = Point3D(
                view_vector.x / length,
                view_vector.y / length, 
                view_vector.z / length
            )
        
        dot_product = normal.dot(view_vector)
        return dot_product < 0

class Polyhedron:
    def __init__(self, faces):
        self.faces = faces
        self.transform_matrix = np.identity(4)
    
    def apply_transform(self, transform_matrix):
        self.transform_matrix = np.dot(transform_matrix, self.transform_matrix)
    
    def reset_transform(self):
        self.transform_matrix = np.identity(4)
    
    def get_transformed_faces(self):
        transformed_faces = []
        for face in self.faces:
            transformed_face = face.apply_transform(self.transform_matrix)
            transformed_faces.append(transformed_face)
        return transformed_faces

class Octahedron(Polyhedron):
    def __init__(self, size=1):
        # Вершины октаэдра
        s = size
        vertices = [
            Point3D(0, s, 0),   # Верх
            Point3D(0, -s, 0),  # Низ
            Point3D(s, 0, 0),   # Право
            Point3D(-s, 0, 0),  # Лево
            Point3D(0, 0, s),   # Перед
            Point3D(0, 0, -s)   # Зад
        ]
        
        # Грани октаэдра (8 треугольников) с правильным порядком вершин
        faces = [
            # Верхние грани
            Face([vertices[0], vertices[4], vertices[2]], (255, 0, 0)),    # Верх-перед-право
            Face([vertices[0], vertices[2], vertices[5]], (0, 255, 0)),    # Верх-право-зад
            Face([vertices[0], vertices[5], vertices[3]], (0, 0, 255)),    # Верх-зад-лево
            Face([vertices[0], vertices[3], vertices[4]], (255, 255, 0)),  # Верх-лево-перед
            
            # Нижние грани  
            Face([vertices[1], vertices[2], vertices[4]], (255, 0, 255)),  # Низ-право-перед
            Face([vertices[1], vertices[5], vertices[2]], (0, 255, 255)),  # Низ-зад-право
            Face([vertices[1], vertices[3], vertices[5]], (128, 128, 255)),# Низ-лево-зад
            Face([vertices[1], vertices[4], vertices[3]], (255, 128, 0))   # Низ-перед-лево
        ]
        
        super().__init__(faces)

class Icosahedron(Polyhedron):
    def __init__(self, size=1):
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
            faces.append(Face(face_points, colors[i % len(colors)]))
        
        super().__init__(faces)

class Dodecahedron(Polyhedron):
    def __init__(self, size=1):
        # Создаем икосаэдр как основу
        icosahedron = Icosahedron(size * 1.5)
        
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
        
        # Теперь для каждой вершины икосаэдра находим соответствующие вершины додекаэдра
        # Собираем все уникальные вершины икосаэдра
        icosa_vertices = []
        for face in icosahedron.faces:
            for point in face.points:
                # Проверяем на уникальность
                is_unique = True
                for v in icosa_vertices:
                    if (abs(v.x - point.x) < 0.001 and 
                        abs(v.y - point.y) < 0.001 and 
                        abs(v.z - point.z) < 0.001):
                        is_unique = False
                        break
                if is_unique:
                    icosa_vertices.append(point)
        
       
        faces = []
        colors = [
            (255, 0, 0), (255, 128, 0), (255, 255, 0), (128, 255, 0),
            (0, 255, 0), (0, 255, 128), (0, 255, 255), (0, 128, 255),
            (0, 0, 255), (128, 0, 255), (255, 0, 255), (255, 0, 128)
        ]
        
        # Для каждой вершины икосаэдра находим 5 ближайших вершин додекаэдра
        # которые образуют пятиугольную грань додекаэдра
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
            
            # Проверяем, что у нас действительно 5 вершин
            if len(closest_vertices) != 5:
                continue
            
            # Сортируем вершины в правильном порядке вокруг нормали
            # Вычисляем центр пятиугольника
            center_x = sum(v.x for v in closest_vertices) / 5
            center_y = sum(v.y for v in closest_vertices) / 5
            center_z = sum(v.z for v in closest_vertices) / 5
            center = Point3D(center_x, center_y, center_z)
            
            # Вычисляем нормаль грани
            v1 = closest_vertices[1] - closest_vertices[0]
            v2 = closest_vertices[2] - closest_vertices[0]
            normal = Point3D(
                v1.y * v2.z - v1.z * v2.y,
                v1.z * v2.x - v1.x * v2.z,
                v1.x * v2.y - v1.y * v2.x
            )
            
            # Нормализуем нормаль
            length_n = math.sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z)
            if length_n > 0:
                normal = Point3D(normal.x/length_n, normal.y/length_n, normal.z/length_n)
            
            # Создаем локальную систему координат
            # Берем произвольный вектор, не параллельный нормали
            if abs(normal.x) > 0.1 or abs(normal.y) > 0.1:
                tangent = Point3D(-normal.y, normal.x, 0)
            else:
                tangent = Point3D(0, -normal.z, normal.y)
            
            # Нормализуем касательный вектор
            length_t = math.sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z)
            if length_t > 0:
                tangent = Point3D(tangent.x/length_t, tangent.y/length_t, tangent.z/length_t)
            
            # Второй касательный вектор (бинормаль)
            binormal = Point3D(
                normal.y * tangent.z - normal.z * tangent.y,
                normal.z * tangent.x - normal.x * tangent.z,
                normal.x * tangent.y - normal.y * tangent.x
            )
            
            # Сортируем вершины по углу в плоскости грани
            def get_angle(vertex):
                # Вектор от центра к вершине
                vec = vertex - center
                # Проекция на плоскость
                x_proj = vec.dot(tangent)
                y_proj = vec.dot(binormal)
                return math.atan2(y_proj, x_proj)
            
            # Сортируем вершины по углу
            closest_vertices.sort(key=get_angle)
            
            # Проверяем, что грань выпуклая
            face_points = closest_vertices
            
            # Создаем грань
            faces.append(Face(face_points, colors[len(faces) % len(colors)]))
            
            # Останавливаемся после 12 граней
            if len(faces) >= 12:
                break
        
        super().__init__(faces)


class AffineTransform:
    @staticmethod
    def translation(dx, dy, dz):
        return np.array([
            [1, 0, 0, dx],
            [0, 1, 0, dy],
            [0, 0, 1, dz],
            [0, 0, 0, 1]
        ])
    
    @staticmethod
    def rotation_x(angle):
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        return np.array([
            [1, 0, 0, 0],
            [0, cos_a, -sin_a, 0],
            [0, sin_a, cos_a, 0],
            [0, 0, 0, 1]
        ])
    
    @staticmethod
    def rotation_y(angle):
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        return np.array([
            [cos_a, 0, sin_a, 0],
            [0, 1, 0, 0],
            [-sin_a, 0, cos_a, 0],
            [0, 0, 0, 1]
        ])
    
    @staticmethod
    def rotation_z(angle):
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        return np.array([
            [cos_a, -sin_a, 0, 0],
            [sin_a, cos_a, 0, 0],
            [0, 0, 1, 0],
            [0, 0, 0, 1]
        ])
    
    @staticmethod
    def scaling(sx, sy, sz):
        return np.array([
            [sx, 0, 0, 0],
            [0, sy, 0, 0],
            [0, 0, sz, 0],
            [0, 0, 0, 1]
        ])

class PolyhedronRenderer:
    def __init__(self, width=800, height=600):
        pygame.init()
        self.width = width
        self.height = height
        self.screen = pygame.display.set_mode((width, height))
        pygame.display.set_caption("3D Polyhedra Viewer")
        self.clock = pygame.time.Clock()
        self.font = pygame.font.Font(None, 36)
        
        self.camera_distance = 5
        self.camera_angle_x = 0
        self.camera_angle_y = 0
        
        # Создаем экземпляры всех многогранников
        self.octahedron = Octahedron()
        self.icosahedron = Icosahedron(0.8)
        self.dodecahedron = Dodecahedron(0.6)
        
        self.current_polyhedron = self.octahedron
        self.current_polyhedron_name = "octahedron"
    
    def switch_polyhedron(self, polyhedron_type):
        """Переключение между многогранниками"""
        print(f"Attempting to switch to: {polyhedron_type}")
        
        if polyhedron_type == "octahedron":
            self.current_polyhedron = self.octahedron
            self.current_polyhedron_name = "octahedron"
        elif polyhedron_type == "icosahedron":
            self.current_polyhedron = self.icosahedron
            self.current_polyhedron_name = "icosahedron"
        elif polyhedron_type == "dodecahedron":
            self.current_polyhedron = self.dodecahedron
            self.current_polyhedron_name = "dodecahedron"
        else:
            return
        
        # Сбрасываем трансформации для нового многогранника
        self.current_polyhedron.reset_transform()
        self.camera_angle_x = 0
        self.camera_angle_y = 0
     
    def project_3d_to_2d(self, point):
        rot_x = AffineTransform.rotation_x(self.camera_angle_x)
        rot_y = AffineTransform.rotation_y(self.camera_angle_y)
        transform = np.dot(rot_y, rot_x)
        
        point_array = point.to_array()
        transformed = np.dot(transform, point_array)
        
        z = transformed[2] + self.camera_distance
        if z == 0:
            z = 0.001
            
        factor = 200 / z
        x = transformed[0] * factor + self.width / 2
        y = transformed[1] * factor + self.height / 2
        
        return (x, y)
    
    def draw_polyhedron(self):
        self.screen.fill((0, 0, 0))
        
        # Получаем трансформированные грани текущего многогранника
        transformed_faces = self.current_polyhedron.get_transformed_faces()
        
        # Временно отключаем удаление невидимых граней
        visible_faces = []
        for face in transformed_faces:
            visible_faces.append(face)
        
        # Сортируем грани по глубине для правильного отображения
        faces_with_depth = []
        for face in visible_faces:
            center = face.get_center()
            rot_x = AffineTransform.rotation_x(self.camera_angle_x)
            rot_y = AffineTransform.rotation_y(self.camera_angle_y)
            transform = np.dot(rot_y, rot_x)
            
            center_array = center.to_array()
            transformed_center = np.dot(transform, center_array)
            depth = transformed_center[2] + self.camera_distance
            
            faces_with_depth.append((depth, face))
        
        # Сортируем от дальних к ближним
        faces_with_depth.sort(reverse=True, key=lambda x: x[0])
        
        # Рисуем грани
        for depth, face in faces_with_depth:
            points_2d = [self.project_3d_to_2d(p) for p in face.points]
            
            if len(points_2d) > 2:
                try:
                    pygame.draw.polygon(self.screen, face.color, points_2d)
                    pygame.draw.polygon(self.screen, (255, 255, 255), points_2d, 1)
                except:
                    # Если есть проблемы с отрисовкой, рисуем контур
                    if len(points_2d) >= 2:
                        pygame.draw.lines(self.screen, face.color, True, points_2d, 1)
        
        # Отображаем информацию
        info_text = f"Polyhedron: {self.current_polyhedron_name}"
        text_surface = self.font.render(info_text, True, (255, 255, 255))
        self.screen.blit(text_surface, (10, 10))
        
        controls_text = "1-Octahedron 2-Icosahedron 3-Dodecahedron R-Reset"
        controls_surface = pygame.font.Font(None, 24).render(controls_text, True, (255, 255, 255))
        self.screen.blit(controls_surface, (10, self.height - 30))
        
        pygame.display.flip()
    
    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False
            elif event.type == pygame.KEYDOWN:
                
                # Проверяем разные возможные коды для клавиш 1, 2, 3
                if event.key in [pygame.K_1, pygame.K_KP1]:
                    self.switch_polyhedron("octahedron")
                elif event.key in [pygame.K_2, pygame.K_KP2]:
                    self.switch_polyhedron("icosahedron")
                elif event.key in [pygame.K_3, pygame.K_KP3]:
                    self.switch_polyhedron("dodecahedron")
                elif event.key == pygame.K_r:
                    self.current_polyhedron.reset_transform()
                    self.camera_angle_x = 0
                    self.camera_angle_y = 0
                elif event.key == pygame.K_t:
                    transform = AffineTransform.translation(0.5, 0, 0)
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_s:
                    transform = AffineTransform.scaling(1.2, 1.2, 1.2)
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_x:
                    transform = AffineTransform.rotation_x(math.pi / 8)
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_y:
                    transform = AffineTransform.rotation_y(math.pi / 8)
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_z:
                    transform = AffineTransform.rotation_z(math.pi / 8)
                    self.current_polyhedron.apply_transform(transform)
        
        # Управление камерой
        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            self.camera_angle_y -= 0.02
        if keys[pygame.K_RIGHT]:
            self.camera_angle_y += 0.02
        if keys[pygame.K_UP]:
            self.camera_angle_x -= 0.02
        if keys[pygame.K_DOWN]:
            self.camera_angle_x += 0.02
        
        return True
    
    def run(self):
        running = True
        while running:
            running = self.handle_events()
            self.draw_polyhedron()
            self.clock.tick(60)
        
        pygame.quit()

if __name__ == "__main__":
    renderer = PolyhedronRenderer()
    renderer.run()
