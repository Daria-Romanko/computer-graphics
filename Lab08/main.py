import pygame
import numpy as np
import math
import sys
from common import Point3D, Camera, Face, Polyhedron, Octahedron, Icosahedron, AffineTransform, OBJLoader
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
        
        self.camera = Camera(
            position=Point3D(0, 0, -5),
            target=Point3D(0, 0, 0),
            aspect_ratio=width/height
        )

        self.camera_rotation_speed = 0.03
        self.camera_move_speed = 0.1

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


        self.function_mode = False
        
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
        # Сбрасываем камеру
        self.camera.set_position(Point3D(0, 0, -5))
        self.camera.set_target(Point3D(0, 0, 0))
        self.camera_angle_x = 0
        self.camera_angle_y = 0
    
    
    def project_3d_to_2d(self, point):
        if self.projection_type == "perspective":
            # ИСПОЛЬЗУЕМ МАТРИЦУ КАМЕРЫ для перспективной проекции
            view_proj_matrix = self.camera.get_view_projection_matrix()
            
            point_array = point.to_array()
            transformed = np.dot(view_proj_matrix, point_array)
            
            # Перспективное деление
            if transformed[3] != 0:
                transformed = transformed / transformed[3]
            
            # Преобразуем в координаты экрана [-1,1] -> [0, width/height]
            x = (transformed[0] + 1) * 0.5 * self.width
            y = (1 - (transformed[1] + 1) * 0.5) * self.height
            
            return (x, y)
        else:
            # Аксонометрическая проекция (старый метод для совместимости)
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
        if self.revolution_mode:
            self.draw_revolution_mode()
            return

        self.screen.fill((0, 0, 0))
        transformed_faces = self.current_polyhedron.get_transformed_faces()

        faces_with_depth = []

        for face in transformed_faces:
            if self.projection_type == "perspective":
                # Для перспективы: проверка нормалей относительно камеры
                center = face.get_center()
                normal = face.get_normal()
                
                # Вектор от камеры к центру грани
                view_vector = center - self.camera.position
                view_vector = view_vector.normalize()
                
                dot = normal.dot(view_vector)
                if dot < 0:  # Грань видима
                    # Глубина для сортировки
                    depth = (center - self.camera.position).length()
                    faces_with_depth.append((depth, face))
            else:
                # Для аксонометрии: ВСЕ грани видимы
                center = face.get_center()
                depth = center.z
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

        # Отображаем информацию о камере
        cam_pos = self.camera.position
        info_text = f"Polyhedron: {self.current_polyhedron_name} | Camera: ({cam_pos.x:.1f}, {cam_pos.y:.1f}, {cam_pos.z:.1f})"
        text_surface = self.font.render(info_text, True, (255, 255, 255))
        self.screen.blit(text_surface, (10, 10))

        controls_lines = [
            "1-Octahedron 2-Icosahedron, 4-Revolution",
            "R-Reset T-Translate S-Scale",
            "X/Y/Z-Rotate M/N/B-Mirror",
            "C/L-Rotation P-Projection A-ShowLine",
            "WASD-Camera Move Arrows-Camera Rotate",
            "QE-Camera Up/Down Ctrl+O-Load Ctrl+S-Save" 
        ]
        small_font = pygame.font.Font(None, 24)
        for i, line in enumerate(controls_lines):
            controls_surface = small_font.render(line, True, (255, 255, 255))
            self.screen.blit(controls_surface, (10, self.height - 180 + i * 25))

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
                    # Сбрасываем камеру
                    self.camera.set_position(Point3D(0, 0, -5))
                    self.camera.set_target(Point3D(0, 0, 0))
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
                    # Отражение относительно плоскости xz
                    transform = AffineTransform.reflection_xz()
                    self.current_polyhedron.apply_transform(transform)
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
                        # Сбрасываем камеру для перспективы
                        self.camera.set_position(Point3D(0, 0, -5))
                        self.camera.set_target(Point3D(0, 0, 0))
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
        
        if self.projection_type == "perspective":
            # Вращение камеры вокруг цели
            if keys[pygame.K_LEFT]:
                self.camera.rotate_around_target(0, -self.camera_rotation_speed)
            if keys[pygame.K_RIGHT]:
                self.camera.rotate_around_target(0, self.camera_rotation_speed)
            if keys[pygame.K_UP]:
                self.camera.rotate_around_target(-self.camera_rotation_speed, 0)
            if keys[pygame.K_DOWN]:
                self.camera.rotate_around_target(self.camera_rotation_speed, 0)
            
            # Движение камеры
            if keys[pygame.K_w]:
                self.camera.move_forward(self.camera_move_speed)
            if keys[pygame.K_s]:
                self.camera.move_forward(-self.camera_move_speed)
            if keys[pygame.K_q]:
                # Move up
                current_distance = (self.camera.target - self.camera.position).length()
                new_pos = Point3D(
                    self.camera.position.x,
                    self.camera.position.y + self.camera_move_speed,
                    self.camera.position.z
                )
                # Сохраняем расстояние до цели при вертикальном движении
                direction_to_target = (self.camera.target - new_pos).normalize()
                new_target = Point3D(
                    new_pos.x + direction_to_target.x * current_distance,
                    new_pos.y + direction_to_target.y * current_distance,
                    new_pos.z + direction_to_target.z * current_distance
                )
                self.camera.set_position(new_pos)
                self.camera.set_target(new_target)
            if keys[pygame.K_e]:
                # Move down
                current_distance = (self.camera.target - self.camera.position).length()
                new_pos = Point3D(
                    self.camera.position.x,
                    self.camera.position.y - self.camera_move_speed,
                    self.camera.position.z
                )
                direction_to_target = (self.camera.target - new_pos).normalize()
                new_target = Point3D(
                    new_pos.x + direction_to_target.x * current_distance,
                    new_pos.y + direction_to_target.y * current_distance,
                    new_pos.z + direction_to_target.z * current_distance
                )
                self.camera.set_position(new_pos)
                self.camera.set_target(new_target)
        else:
            # Старое управление для аксонометрии
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
