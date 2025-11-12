import pygame
import numpy as np
import math
import sys
from common import Point3D, Face, Polyhedron, Octahedron, Icosahedron, AffineTransform, OBJLoader, ZBuffer
from surface_of_revolution import SurfaceOfRevolution, RevolutionInputPanel

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
        self.icosahedron = Icosahedron()
        
        self.current_polyhedron = self.octahedron
        self.current_polyhedron_name = "octahedron"

        # Добавляем переменные для работы с файлами
        self.custom_polyhedron = None
        self.custom_polyhedron_name = "custom"

        self.projection_type = "perspective"  # "perspective" или "axonometric"
        self.arbitrary_line_point1 = Point3D(-2, -2, -2)
        self.arbitrary_line_point2 = Point3D(2, 2, 2)
        self.show_arbitrary_line = False

        # для фигуры вращения 
        self.revolution_mode = False
        self.generatrix_points = []
        self.input_panel = RevolutionInputPanel(self.screen)

        # Z-буфер
        self.use_z_buffer = True  # По умолчанию включен
        self.z_buffer = ZBuffer(width, height)
        
    def load_custom_model(self, filename):
        """Загружает пользовательскую модель из файла OBJ"""
        try:
            loaded_polyhedron = OBJLoader.load_from_file(filename)
        
            if loaded_polyhedron:
                for face in loaded_polyhedron.faces:
                    face.points.reverse()  # меняем порядок вершин (нормаль переворачивается)

                self.custom_polyhedron = loaded_polyhedron
                self.current_polyhedron = self.custom_polyhedron
                self.current_polyhedron_name = "custom"

                correction_transform = AffineTransform.rotation_y(math.pi / 2)
                self.current_polyhedron.apply_transform(correction_transform)

                n_faces = len(self.current_polyhedron.faces)
                for i, face in enumerate(self.current_polyhedron.faces):
                    hue = (i / n_faces) * 360
                    saturation = 0.8
                    value = 0.9

                    h = hue / 60
                    j = math.floor(h)
                    f = h - j
                    p = value * (1 - saturation)
                    q = value * (1 - saturation * f)
                    t = value * (1 - saturation * (1 - f))

                    if j == 0:
                        r, g, b = value, t, p
                    elif j == 1:
                        r, g, b = q, value, p
                    elif j == 2:
                        r, g, b = p, value, t
                    elif j == 3:
                        r, g, b = p, q, value
                    elif j == 4:
                        r, g, b = t, p, value
                    else:
                        r, g, b = value, p, q

                    face.color = (int(r * 255), int(g * 255), int(b * 255))

                print(f"Модель загружена из {filename}")
                return True
            else:
                print("Ошибка загрузки модели")
                return False

        except Exception as e:
            print(f"Ошибка при загрузке: {e}")
            return False
            
    def save_current_model(self, filename):
        """Сохраняет текущую модель в файл OBJ"""
        try:
            success = OBJLoader.save_to_file(self.current_polyhedron, filename)
            if success:
                print(f"Модель сохранена в {filename}")
            else:
                print("Ошибка сохранения модели")
            return success
        except Exception as e:
            print(f"Ошибка при сохранении: {e}")
            return False

    def open_file_dialog(self, mode="load"):
        """Открывает диалог выбора файла"""
        import tkinter as tk
        from tkinter import filedialog, messagebox
        
        # Создаем скрытое окно tkinter
        root = tk.Tk()
        root.withdraw()
        
        try:
            if mode == "load":
                filename = filedialog.askopenfilename(
                    title="Загрузить модель OBJ",
                    filetypes=[("OBJ files", "*.obj"), ("All files", "*.*")]
                )
                if filename:
                    self.load_custom_model(filename)
                    
            elif mode == "save":
                filename = filedialog.asksaveasfilename(
                    title="Сохранить модель OBJ",
                    defaultextension=".obj",
                    filetypes=[("OBJ files", "*.obj"), ("All files", "*.*")]
                )
                if filename:
                    self.save_current_model(filename)
                    
        except Exception as e:
            messagebox.showerror("Ошибка", f"Ошибка работы с файлом: {e}")
        finally:
            root.destroy()

    def draw_arbitrary_line(self):
        """Рисует произвольную прямую для наглядности"""
        if not self.show_arbitrary_line:
            return
        
        p1_2d = self.project_3d_to_2d(self.arbitrary_line_point1)
        p2_2d = self.project_3d_to_2d(self.arbitrary_line_point2)
        
        pygame.draw.line(self.screen, (255, 0, 255), p1_2d, p2_2d, 2)  # Розовый
        
        # Рисуем точки
        pygame.draw.circle(self.screen, (255, 0, 0), (int(p1_2d[0]), int(p1_2d[1])), 5)
        pygame.draw.circle(self.screen, (0, 255, 0), (int(p2_2d[0]), int(p2_2d[1])), 5)

    def switch_polyhedron(self, polyhedron_type):
        """Переключение между многогранниками"""
        print(f"Attempting to switch to: {polyhedron_type}")
        
        if polyhedron_type == "octahedron":
            self.current_polyhedron = self.octahedron
            self.current_polyhedron_name = "octahedron"
        elif polyhedron_type == "icosahedron":
            self.current_polyhedron = self.icosahedron
            self.current_polyhedron_name = "icosahedron"
        else:
            return
        
        # Сбрасываем трансформации для нового многогранника
        self.current_polyhedron.reset_transform()
        self.camera_angle_x = 0
        self.camera_angle_y = 0
     
    
    def draw_with_z_buffer(self, transformed_faces, scene_transform):
        # Очищаем z-буфер и экран
        self.z_buffer.clear()
        self.screen.fill((0, 0, 0))
        
        # Определяем, используем ли перспективную коррекцию
        use_perspective = self.projection_type == "perspective"
        
        # Обрабатываем каждую грань
        for face in transformed_faces:
            # Преобразуем грань в камерное пространство
            face_cam = face.apply_transform(scene_transform)
            
            # Получаем точки и глубины
            points_2d = []
            depths = []
            
            for point in face_cam.points:
                # Проецируем точку на 2D
                projected = self.project_3d_to_2d(point)
                points_2d.append(projected)
                
                # Вычисляем глубину в камерном пространстве
                if use_perspective:
                    # Для перспективы: используем z-координату в камерном пространстве
                    # (точка уже преобразована scene_transform)
                    depth = point.z + self.camera_distance
                else:
                    # Для аксонометрии: используем z-координату
                    depth = point.z
                
                depths.append(depth)
            
            # Рисуем треугольник в z-буфере
            if len(points_2d) == 3:
                self.z_buffer.draw_triangle(points_2d, depths, face.color, use_perspective)
            elif len(points_2d) > 3:
                # Разбиваем многоугольник на треугольники
                for i in range(1, len(points_2d) - 1):
                    triangle_points = [points_2d[0], points_2d[i], points_2d[i + 1]]
                    triangle_depths = [depths[0], depths[i], depths[i + 1]]
                    self.z_buffer.draw_triangle(triangle_points, triangle_depths, face.color, use_perspective)
        
        # Копируем цветовой буфер на экран
        pygame.surfarray.blit_array(self.screen, self.z_buffer.color_buffer.swapaxes(0, 1))
        
        # Рисуем произвольную линию поверх
        self.draw_arbitrary_line()

    def project_3d_to_2d(self, point):
        if self.projection_type == "perspective":
            # Перспективная проекция
            rot_x = AffineTransform.rotation_x(self.camera_angle_x)
            rot_y = AffineTransform.rotation_y(self.camera_angle_y)
            transform = np.dot(rot_y, rot_x)
            
            point_array = point.to_array()
            transformed = np.dot(transform, point_array)
            
            # z в камерном пространстве (положительный = впереди камеры)
            z_cam = transformed[2] + self.camera_distance
            
            # Отсечение объектов позади камеры
            if z_cam <= 0.1:
                return (self.width * 10, self.height * 10)  # Выводим за экран
            
            factor = 200 / z_cam
            x = transformed[0] * factor + self.width / 2
            y = transformed[1] * factor + self.height / 2
            
            return (x, y)
        else:
            # Аксонометрическая проекция
            rot_x = AffineTransform.rotation_x(self.camera_angle_x)
            rot_y = AffineTransform.rotation_y(self.camera_angle_y)
            transform = np.dot(rot_y, rot_x)
            
            point_array = point.to_array()
            transformed = np.dot(transform, point_array)
            
            factor = 100
            x = transformed[0] * factor + self.width / 2
            y = transformed[1] * factor + self.height / 2
            
            return (x, y)

    def draw_polyhedron(self):
        # Отображаем информацию о режиме
        mode_text = "Z-buffer" if self.use_z_buffer else "Painter's algorithm"
        projection_text = "Perspective" if self.projection_type == "perspective" else "Axonometric"
        info_text = f"{self.current_polyhedron_name} - {projection_text} - {mode_text}"
        text_surface = self.font.render(info_text, True, (255, 255, 255))
        self.screen.blit(text_surface, (10, 10))
        
        if self.revolution_mode:
            self.draw_revolution_mode()
            return

        # Если используется z-буфер, рисуем с его помощью
        if self.use_z_buffer:
            transformed_faces = self.current_polyhedron.get_transformed_faces()
            rot_x = AffineTransform.rotation_x(self.camera_angle_x)
            rot_y = AffineTransform.rotation_y(self.camera_angle_y)
            scene_transform = np.dot(rot_y, rot_x)
            
            self.draw_with_z_buffer(transformed_faces, scene_transform)
        else:
            # Старый метод с сортировкой граней
            self.screen.fill((0, 0, 0))
            transformed_faces = self.current_polyhedron.get_transformed_faces()

            rot_x = AffineTransform.rotation_x(self.camera_angle_x)
            rot_y = AffineTransform.rotation_y(self.camera_angle_y)
            scene_transform = np.dot(rot_y, rot_x)

            faces_with_depth = []

            for face in transformed_faces:
                # Преобразуем грань в камерное пространство
                face_cam = face.apply_transform(scene_transform)
                center_cam = face_cam.get_center()
                
                if self.projection_type == "perspective":
                    # Для перспективы: проверка нормалей
                    normal_cam = face_cam.get_normal()
                    camera_in_camera_space = Point3D(0, 0, -self.camera_distance)
                    
                    # Вектор от камеры к центру
                    view_vec = Point3D(
                        center_cam.x - camera_in_camera_space.x,
                        center_cam.y - camera_in_camera_space.y,
                        center_cam.z - camera_in_camera_space.z
                    )
                    
                    length = math.sqrt(view_vec.x**2 + view_vec.y**2 + view_vec.z**2)
                    if length != 0:
                        view_vec = Point3D(view_vec.x / length, view_vec.y / length, view_vec.z / length)

                    dot = normal_cam.dot(view_vec)
                    if dot < 0:
                        depth = center_cam.z
                        faces_with_depth.append((depth, face))
                else:
                    # Для аксонометрии: ВСЕ грани видимы, сортируем по глубине
                    depth = center_cam.z
                    faces_with_depth.append((depth, face))

            # Сортируем грани по глубине (от дальних к ближним)
            faces_with_depth.sort(reverse=True, key=lambda x: x[0])

            for depth, face in faces_with_depth:
                points_2d = [self.project_3d_to_2d(p) for p in face.points]
                if len(points_2d) > 2:
                    try:
                        pygame.draw.polygon(self.screen, face.color, points_2d)
                        pygame.draw.polygon(self.screen, (255, 255, 255), points_2d, 1)
                    except:
                        if len(points_2d) >= 2:
                            pygame.draw.lines(self.screen, face.color, True, points_2d, 1)

            self.draw_arbitrary_line()

        # Отображаем информацию о режиме
        mode_text = "Z-buffer" if self.use_z_buffer else "Painter's algorithm"
        info_text = f"Polyhedron: {self.current_polyhedron_name} ({self.projection_type}) - {mode_text}"
        text_surface = self.font.render(info_text, True, (255, 255, 255))
        self.screen.blit(text_surface, (10, 10))

        controls_lines = [
            "1-Octahedron 2-Icosahedron, 4-Revolution",
            "R-Reset T-Translate S-Scale",
            "X/Y/Z-Rotate M/N/B-Mirror",
            "C/L-Rotation P-Projection A-ShowLine",
            "Ctrl+O-Load Ctrl+S-Save Arrows-Camera",
            "B-Z-Buffer toggle"
        ]
        small_font = pygame.font.Font(None, 24)
        for i, line in enumerate(controls_lines):
            controls_surface = small_font.render(line, True, (255, 255, 255))
            self.screen.blit(controls_surface, (10, self.height - 150 + i * 25))

        self.input_panel.draw()
        pygame.display.flip()

    
    def start_revolution_mode(self):
        self.revolution_mode = True
        self.generatrix_points = []
        self.current_polyhedron = None
        self.current_polyhedron_name = "revolution_mode"
    
    def create_revolution_figure(self, axis='y', divisions=12):
        if len(self.generatrix_points) < 2:
            return

        try:
            revolution_figure = SurfaceOfRevolution(self.generatrix_points, axis, divisions)
            self.current_polyhedron = revolution_figure
            self.current_polyhedron_name = "surface_of_revolution"
            self.revolution_mode = False
            self.input_panel.hide()
        except Exception as e:
            print(f"Ошибка при создании фигуры вращения: {e}")
            self.revolution_mode = False
            self.input_panel.hide()
    
    def draw_revolution_mode(self):
        self.screen.fill((0, 0, 0))
        
        instructions = [
            "Режим создания фигуры вращения",
            "Кликните левой кнопкой мыши для добавления точек",
            "Нажмите ENTER для завершения ввода точек", 
            "Нажмите ESC для выхода из режима",
            f"Точек добавлено: {len(self.generatrix_points)}"
        ]
        
        small_font = pygame.font.Font(None, 24)
        for i, line in enumerate(instructions):
            text = small_font.render(line, True, (255, 255, 255))
            self.screen.blit(text, (50, 50 + i * 30))
        
        for i, point in enumerate(self.generatrix_points):
            x = point.x * 50 + self.width // 2
            y = -point.y * 50 + self.height // 2
            
            pygame.draw.circle(self.screen, (255, 0, 0), (int(x), int(y)), 5)
            
            label = small_font.render(str(i+1), True, (255, 255, 255))
            self.screen.blit(label, (int(x) + 10, int(y) - 10))
        
        if len(self.generatrix_points) > 1:
            points_2d = []
            for point in self.generatrix_points:
                x = point.x * 50 + self.width // 2
                y = -point.y * 50 + self.height // 2
                points_2d.append((x, y))
            
            pygame.draw.lines(self.screen, (0, 255, 0), False, points_2d, 2)
        
        self.input_panel.draw()
        pygame.display.flip()
    
    def handle_revolution_events(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mouse_x, mouse_y = event.pos
            
            x = (mouse_x - self.width // 2) / 50
            y = -(mouse_y - self.height // 2) / 50 
            z = 0
            
            new_point = Point3D(x, y, z)
            self.generatrix_points.append(new_point)
            
            return True
        
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_RETURN:
                if len(self.generatrix_points) >= 2:
                    self.input_panel.show()
                return True
            elif event.key == pygame.K_ESCAPE:
                self.revolution_mode = False
                self.input_panel.hide()
                self.current_polyhedron = self.octahedron
                self.current_polyhedron_name = "octahedron"
                return True
        
        return False
    
    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return False
            
            if self.input_panel.visible:
                result = self.input_panel.handle_event(event)
                if result == 'ok':
                    self.create_revolution_figure(
                        self.input_panel.axis, 
                        self.input_panel.divisions
                    )
                elif result == 'cancel':
                    self.input_panel.hide()
                    self.revolution_mode = False
                    self.current_polyhedron = self.octahedron
                    self.current_polyhedron_name = "octahedron"
                continue
            
            if self.revolution_mode:
                if self.handle_revolution_events(event):
                    continue

            elif event.type == pygame.KEYDOWN:
                
                # Проверяем разные возможные коды для клавиш 1, 2, 3
                if event.key in [pygame.K_1, pygame.K_KP1]:
                    self.switch_polyhedron("octahedron")
                elif event.key in [pygame.K_2, pygame.K_KP2]:
                    self.switch_polyhedron("icosahedron")
                if event.key == pygame.K_4:
                    self.start_revolution_mode()
                    continue
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
                elif event.key == pygame.K_9:
                    self.current_polyhedron.scale_about_center(1.2)
                elif  event.key == pygame.K_0:
                    self.current_polyhedron.scale_about_center(0.8)
                elif event.key == pygame.K_m:
                    # Отражение относительно плоскости yz
                    transform = AffineTransform.reflection_yz()
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_n:
                    # Отражение относительно плоскости xy
                    transform = AffineTransform.reflection_xy()
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_b:
                    # Переключение режима z-буфера
                    self.use_z_buffer = not self.use_z_buffer
                    print(f"Z-buffer: {'enabled' if self.use_z_buffer else 'disabled'}")
                elif event.key == pygame.K_c:
                    # Вращение вокруг прямой через центр, параллельной оси X
                    transform = AffineTransform.rotation_around_line_through_center(
                        self.current_polyhedron, 'x', math.pi / 6
                    )
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_l:
                    # Вращение вокруг произвольной прямой
                    transform = AffineTransform.rotation_around_arbitrary_line(
                        self.arbitrary_line_point1, self.arbitrary_line_point2, math.pi / 6
                    )
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_p:
                    # Переключение типа проекции
                    if self.projection_type == "perspective":
                        self.projection_type = "axonometric"
                        self.camera_angle_x = math.radians(35.264)
                        self.camera_angle_y = math.radians(45)
                    else:
                        self.projection_type = "perspective"
                        # вернем "фронтальный" вид камеры
                        self.camera_angle_x = 0
                        self.camera_angle_y = 0
                elif event.key == pygame.K_a:
                    # Показать/скрыть произвольную прямую
                    self.show_arbitrary_line = not self.show_arbitrary_line
                
                # Добавляем новые обработчики для загрузки/сохранения
                if event.key == pygame.K_o:  # Ctrl+O для загрузки
                    if pygame.key.get_mods() & pygame.KMOD_CTRL:
                        self.open_file_dialog("load")
                elif event.key == pygame.K_s:  # Ctrl+S для сохранения
                    if pygame.key.get_mods() & pygame.KMOD_CTRL:
                        self.open_file_dialog("save")
        
        # Управление камерой
        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            self.camera_angle_y += 0.02
        if keys[pygame.K_RIGHT]:
            self.camera_angle_y -= 0.02
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
