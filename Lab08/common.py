import numpy as np
import math
       
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
    
    def __add__(self, other):
        return Point3D(self.x + other.x, self.y + other.y, self.z + other.z)

    def cross(self, other):
        return Point3D(
            self.y * other.z - self.z * other.y,
            self.z * other.x - self.x * other.z,
            self.x * other.y - self.y * other.x
        )

    def length(self):
        return math.sqrt(self.x**2 + self.y**2 + self.z**2)

    def normalize(self):
        length = self.length()
        if length > 0:
            return Point3D(self.x/length, self.y/length, self.z/length)
        return self

    def __str__(self):
        return f"({self.x}, {self.y}, {self.z})"


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
    
    def is_visible(self, camera_position=Point3D(0, 0, -5), camera_transform=None):
        normal = self.get_normal()
        center = self.get_center()

        # если есть поворот камеры, преобразуем нормаль и центр в те же координаты
        if camera_transform is not None:
            n_arr = np.dot(camera_transform, np.array([normal.x, normal.y, normal.z, 0]))
            c_arr = np.dot(camera_transform, np.array([center.x, center.y, center.z, 1]))
            normal = Point3D.from_array(n_arr)
            center = Point3D.from_array(c_arr)

        view_vector =  center - camera_position 

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
            # Верхные грани
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


    @staticmethod
    def reflection_xy():
        """Отражение относительно плоскости XY"""
        return np.array([
            [1, 0, 0, 0],
            [0, 1, 0, 0],
            [0, 0, -1, 0],
            [0, 0, 0, 1]
        ])

    @staticmethod
    def reflection_xz():
        """Отражение относительно плоскости XZ"""
        return np.array([
            [1, 0, 0, 0],
            [0, -1, 0, 0],
            [0, 0, 1, 0],
            [0, 0, 0, 1]
        ])

    @staticmethod
    def reflection_yz():
        """Отражение относительно плоскости YZ"""
        return np.array([
            [-1, 0, 0, 0],
            [0, 1, 0, 0],
            [0, 0, 1, 0],
            [0, 0, 0, 1]
        ])
    

    @staticmethod
    def rotation_around_axis(axis, angle):
        """
        Вращение вокруг произвольной оси
        axis - единичный вектор направления оси
        angle - угол вращения
        """
        u, v, w = axis
        cos_a = math.cos(angle)
        sin_a = math.sin(angle)
        one_minus_cos = 1 - cos_a
        
        return np.array([
            [cos_a + u*u*one_minus_cos, u*v*one_minus_cos - w*sin_a, u*w*one_minus_cos + v*sin_a, 0],
            [u*v*one_minus_cos + w*sin_a, cos_a + v*v*one_minus_cos, v*w*one_minus_cos - u*sin_a, 0],
            [u*w*one_minus_cos - v*sin_a, v*w*one_minus_cos + u*sin_a, cos_a + w*w*one_minus_cos, 0],
            [0, 0, 0, 1]
        ])
    

    @staticmethod
    def rotation_around_line_through_center(polyhedron, axis, angle):
        """
        Вращение многогранника вокруг прямой, проходящей через его центр,
        параллельно выбранной координатной оси
        """
        center = polyhedron.get_center()
        
        # 1. Перенос в начало координат
        T1 = AffineTransform.translation(-center.x, -center.y, -center.z)
        
        # 2. Вращение вокруг оси
        if axis == 'x':
            R = AffineTransform.rotation_x(angle)
        elif axis == 'y':
            R = AffineTransform.rotation_y(angle)
        elif axis == 'z':
            R = AffineTransform.rotation_z(angle)
        else:
            raise ValueError("Axis must be 'x', 'y', or 'z'")
        
        # 3. Обратный перенос
        T2 = AffineTransform.translation(center.x, center.y, center.z)
        
        # Комбинированная матрица: T2 * R * T1
        return np.dot(T2, np.dot(R, T1))
    

    @staticmethod
    def rotation_around_arbitrary_line(point1, point2, angle):
        """
        Вращение вокруг произвольной прямой, заданной двумя точками
        """
        # Вектор направления прямой
        direction = Point3D(point2.x - point1.x, point2.y - point1.y, point2.z - point1.z)
        direction = direction.normalize()
        
        u, v, w = direction.x, direction.y, direction.z
        
        # 1. Перенос в начало координат (точка point1 становится началом)
        T1 = AffineTransform.translation(-point1.x, -point1.y, -point1.z)
        
        # 2. Совмещение прямой с осью Z
        # Вычисляем углы поворота
        d = math.sqrt(v*v + w*w)
        
        if d != 0:
            # Поворот вокруг X
            Rx = np.array([
                [1, 0, 0, 0],
                [0, w/d, -v/d, 0],
                [0, v/d, w/d, 0],
                [0, 0, 0, 1]
            ])
            
            # Поворот вокруг Y
            Ry = np.array([
                [d, 0, -u, 0],
                [0, 1, 0, 0],
                [u, 0, d, 0],
                [0, 0, 0, 1]
            ])
        else:
            # Если прямая уже параллельна оси X
            Rx = np.identity(4)
            if u < 0:
                Ry = AffineTransform.rotation_y(math.pi)
            else:
                Ry = np.identity(4)
        
        # 3. Вращение вокруг Z
        Rz = AffineTransform.rotation_z(angle)
        
        # 4. Обратные преобразования
        if d != 0:
            Ry_inv = np.linalg.inv(Ry)
            Rx_inv = np.linalg.inv(Rx)
        else:
            if u < 0:
                Ry_inv = AffineTransform.rotation_y(-math.pi)
            else:
                Ry_inv = np.identity(4)
            Rx_inv = np.identity(4)
        
        # 5. Обратный перенос
        T2 = AffineTransform.translation(point1.x, point1.y, point1.z)
        
        # Комбинированная матрица: T2 * Rx_inv * Ry_inv * Rz * Ry * Rx * T1
        if d != 0:
            return np.dot(T2, np.dot(Rx_inv, np.dot(Ry_inv, np.dot(Rz, np.dot(Ry, np.dot(Rx, T1))))))
        else:
            return np.dot(T2, np.dot(Ry_inv, np.dot(Rz, np.dot(Ry, T1))))


class OBJLoader:
    @staticmethod
    def load_from_file(filename, default_color=(255, 255, 255)):
        """Загружает модель из файла OBJ с автоматическим назначением цветов по направлениям"""
        vertices = []
        faces = []
        
        try:
            with open(filename, 'r') as file:
                lines = file.readlines()
                
            # Сначала читаем все вершины
            for line in lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                    
                parts = line.split()
                if parts[0] == 'v' and len(parts) >= 4:
                    x = float(parts[1])
                    y = float(parts[2])
                    z = float(parts[3])

                    vertices.append(Point3D(x, -y, z))
          
            
            # Теперь читаем грани
            for line in lines:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                parts = line.split()
                if parts[0] == 'f':
                    face_vertices = []
                    for part in parts[1:]:
                        vertex_index = part.split('/')[0]
                        if vertex_index.isdigit():
                            idx = int(vertex_index) - 1
                            if 0 <= idx < len(vertices):
                                face_vertices.append(vertices[idx])
                    
                    if len(face_vertices) >= 3:
                        # Автоматически определяем цвет по нормали грани
                        color = OBJLoader._get_face_color(face_vertices)
                        faces.append(Face(face_vertices, color))
                    
            return Polyhedron(faces)
            
        except Exception as e:
            print(f"Ошибка загрузки файла {filename}: {e}")
            return None
    
    @staticmethod
    def _get_face_color(face_vertices):
        """Определяет цвет грани по ее нормали"""
        if len(face_vertices) < 3:
            return (255, 255, 255)
        
        # Вычисляем нормаль
        v1 = face_vertices[1] - face_vertices[0]
        v2 = face_vertices[2] - face_vertices[0]
        normal = v1.cross(v2)
        normal = normal.normalize()
        
        # Цвета для разных направлений
        if abs(normal.y) > 0.7:  # Верх/низ
            if normal.y > 0:
                return (255, 50, 50)    # Красный - верх
            else:
                return (50, 255, 255)   # Голубой - низ
        elif abs(normal.x) > 0.7:  # Лево/право
            if normal.x > 0:
                return (50, 255, 50)    # Зеленый - право
            else:
                return (50, 50, 255)    # Синий - лево
        elif abs(normal.z) > 0.7:  # Перед/зад
            if normal.z > 0:
                return (255, 255, 50)   # Желтый - перед
            else:
                return (255, 50, 255)   # Пурпурный - зад
        else:
            # Наклонные грани - оттенки серого
            brightness = int(128 + normal.y * 64)
            return (brightness, brightness, brightness)
            
    @staticmethod
    def save_to_file(polyhedron, filename):
        """Сохраняет многогранник в файл OBJ"""
        try:
            with open(filename, 'w') as file:
                file.write("# Exported from 3D Polyhedra Viewer\n")
                
                # Собираем все уникальные вершины
                all_vertices = []
                vertex_to_index = {}
                
                for face in polyhedron.faces:
                    for vertex in face.points:
                        vertex_key = f"{vertex.x:.6f}_{vertex.y:.6f}_{vertex.z:.6f}"
                        if vertex_key not in vertex_to_index:
                            all_vertices.append(vertex)
                            vertex_to_index[vertex_key] = len(all_vertices)
                
                # Записываем вершины
                for vertex in all_vertices:
                    file.write(f"v {vertex.x:.6f} {vertex.y:.6f} {vertex.z:.6f}\n")
                
                # Записываем грани
                for face in polyhedron.faces:
                    file.write("f")
                    for vertex in face.points:
                        vertex_key = f"{vertex.x:.6f}_{vertex.y:.6f}_{vertex.z:.6f}"
                        file.write(f" {vertex_to_index[vertex_key]}")
                    file.write("\n")
                    
            return True
            
        except Exception as e:
            print(f"Ошибка сохранения файла {filename}: {e}")
            return False
