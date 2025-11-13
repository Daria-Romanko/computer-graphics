import pygame
import numpy as np
import math
import sys
from common import Point3D, Face, Polyhedron, Octahedron, Icosahedron, AffineTransform, OBJLoader
from surface_of_revolution import SurfaceOfRevolution, RevolutionInputPanel
from function_surface import FunctionInputPanel, FunctionSurface
from z_buffer import ZBuffer
from camera import Camera
from lighting import Lighting

class PolyhedronRenderer:
    def __init__(self, width=800, height=600):
        pygame.init()
        self.width = width
        self.height = height
        self.screen = pygame.display.set_mode((width, height))
        pygame.display.set_caption("3D Polyhedra Viewer")
        self.clock = pygame.time.Clock()
        self.font = pygame.font.Font(None, 36)
        
        self.obj_color = (255, 100, 100) #(255, 100, 255) (255, 255, 100)

        self.camera = Camera(
            position=Point3D(0, 0, -5),
            target=Point3D(0, 0, 0),
            up_vector=Point3D(0, 1, 0),
            fov=60,
            aspect_ratio=width/height,
            near_plane=0.1,
            far_plane=100.0
        )

        self.camera_rotation_speed = 0.03
        self.camera_move_speed = 0.3
        
        self.octahedron = Octahedron()
        self.icosahedron = Icosahedron()
        
        self.current_polyhedron = self.octahedron
        self.current_polyhedron_name = "octahedron"

        self.custom_polyhedron = None
        self.custom_polyhedron_name = "custom"

        self.projection_type = "perspective"
        self.arbitrary_line_point1 = Point3D(-2, -2, -2)
        self.arbitrary_line_point2 = Point3D(2, 2, 2)
        self.show_arbitrary_line = False

        # для фигуры вращения 
        self.revolution_mode = False
        self.generatrix_points = []
        self.input_panel = RevolutionInputPanel(self.screen)

        # для поверхности функции
        self.function_mode = False
        self.function_panel = FunctionInputPanel(self.screen)

        # Z-буфер и освещение
        self.use_z_buffer = True
        self.lighting = Lighting(width, height)

# ЗАГРУЗКА И СОХРАНЕНИЕ    
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

                for face in self.current_polyhedron.faces:
                    face.color = self.obj_color
                    face.vertex_colors = [self.obj_color] * len(face.points)

                correction_transform = AffineTransform.rotation_y(math.pi / 2)
                self.current_polyhedron.apply_transform(correction_transform)

                self.reset_camera()

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

# ОТРИСОВКА
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
        
        self.current_polyhedron.reset_transform()
        self.reset_camera()

    def reset_camera(self):
        """Сбрасывает камеру в исходное положение"""
        self.camera.set_position(Point3D(0, 0, -5))
        self.camera.set_target(Point3D(0, 0, 0))
        self.camera.up_vector = Point3D(0, 1, 0)
        self.camera.update_matrices()
    
    def project_3d_to_2d(self, point):
        if self.projection_type == "perspective":
            point_array = np.array([point.x, point.y, point.z, 1])
            
            view_point = np.dot(self.camera.view_matrix, point_array)
            
            proj_point = np.dot(self.camera.projection_matrix, view_point)
            
            if proj_point[3] != 0:
                proj_point = proj_point / proj_point[3]
            
            x = proj_point[0] * self.width / 2 + self.width / 2
            y = -proj_point[1] * self.height / 2 + self.height / 2
            
            return (x, y)
        else:
            # Для аксонометрической проекции используем ту же логику вращения, что и в камере
            # но без перспективного искажения
            direction = self.camera.position - self.camera.target
            distance = direction.length()
            
            # Используем углы камеры для согласованного вращения
            angle_x = self.camera.angle_x
            angle_y = self.camera.angle_y
            
            # Матрица вращения для аксонометрии (такая же как в камере)
            rot_x = np.array([
                [1, 0, 0],
                [0, math.cos(angle_x), -math.sin(angle_x)],
                [0, math.sin(angle_x), math.cos(angle_x)]
            ])
            rot_y = np.array([
                [math.cos(angle_y), 0, math.sin(angle_y)],
                [0, 1, 0],
                [-math.sin(angle_y), 0, math.cos(angle_y)]
            ])
            
            transform = np.dot(rot_y, rot_x)
            point_array = np.array([point.x, point.y, point.z])
            transformed = np.dot(transform, point_array)
            
            # Масштабируем и центрируем
            factor = 100
            x = transformed[0] * factor + self.width / 2
            y = transformed[1] * factor + self.height / 2
            
            return (x, y)

    def draw_polyhedron(self):
        if self.revolution_mode:
            self.draw_revolution_mode()
            return

        if self.function_mode:
            self.draw_function_mode()
            return

        self.screen.fill((0, 0, 0))
        transformed_faces = self.current_polyhedron.get_transformed_faces()

        # Применяем шейдинг Гуро если освещение включено
        if self.lighting.use_lighting and self.lighting.shading_mode == "gouraud":
            self.lighting.apply_gouraud_shading(self.current_polyhedron, self.camera.position)

        if self.use_z_buffer:
            if self.lighting.use_lighting and self.lighting.shading_mode == "gouraud":
                # отрисовка с шейдингом Гуро
                self.lighting.draw_with_z_buffer_gouraud(
                    self.screen, transformed_faces, self.project_3d_to_2d, self.camera.position
                )
            else:
                self.draw_with_z_buffer(transformed_faces)
        else:
            faces_with_depth = []

            for face in transformed_faces:
                if self.projection_type == "perspective":
                    # Перспективная проекция - проверка нормалей
                    center = face.get_center()
                    normal = face.get_normal()

                    # Вектор взгляда от камеры к центру грани
                    view_vec = (center - self.camera.position).normalize()
                    dot = normal.dot(view_vec)

                    # Отбрасываем невидимые грани
                    if dot < 0:
                        depth = (center - self.camera.position).length()
                        faces_with_depth.append((depth, face))
                else:
                    # Аксонометрическая проекция - все грани видимы
                    direction = self.camera.position - self.camera.target
                    distance = direction.length()
                    
                    angle_x = self.camera.angle_x
                    angle_y = self.camera.angle_y
                    
                    rot_x = np.array([
                        [1, 0, 0],
                        [0, math.cos(angle_x), -math.sin(angle_x)],
                        [0, math.sin(angle_x), math.cos(angle_x)]
                    ])
                    rot_y = np.array([
                        [math.cos(angle_y), 0, math.sin(angle_y)],
                        [0, 1, 0],
                        [-math.sin(angle_y), 0, math.cos(angle_y)]
                    ])
                    
                    scene_transform = np.dot(rot_y, rot_x)
                    
                    # Преобразуем грань для вычисления глубины
                    transformed_points = []
                    for point in face.points:
                        point_array = np.array([point.x, point.y, point.z])
                        transformed_array = np.dot(scene_transform, point_array)
                        transformed_point = Point3D(transformed_array[0], transformed_array[1], transformed_array[2])
                        transformed_points.append(transformed_point)
                    
                    # Создаем временную грань для вычисления центра
                    from common import Face
                    temp_face = Face(transformed_points, face.color)
                    center_cam = temp_face.get_center()
                    
                    depth = center_cam.z
                    faces_with_depth.append((depth, face))

            # Сортируем грани по глубине (от дальних к ближним)
            faces_with_depth.sort(reverse=True, key=lambda x: x[0])

            for depth, face in faces_with_depth:
                points_2d = [self.project_3d_to_2d(p) for p in face.points]
                if len(points_2d) > 2:
                    try:
                        if self.lighting.use_lighting and self.lighting.shading_mode == "gouraud":
                            # Для шейдинга Гуро рисуем треугольники с интерполяцией
                            for i in range(1, len(points_2d) - 1):
                                tri_points = [points_2d[0], points_2d[i], points_2d[i + 1]]
                                tri_colors = [face.vertex_colors[0], face.vertex_colors[i], face.vertex_colors[i + 1]]
                                
                                self.lighting.draw_triangle_with_lighting(
                                    self.screen, tri_points, [0, 0, 0], tri_colors, use_zbuffer=False
                                )
                        else:
                            # Стандартная отрисовка без освещения
                            pygame.draw.polygon(self.screen, face.color, points_2d)
                            pygame.draw.polygon(self.screen, (255, 255, 255), points_2d, 1)
                    except:
                        if len(points_2d) >= 2:
                            pygame.draw.lines(self.screen, face.color, True, points_2d, 1)

        self.draw_arbitrary_line()

        cam_pos = self.camera.position
        mode_text = "Z-buffer" if self.use_z_buffer else "Painter"
        proj_text = "Perspective" if self.projection_type == "perspective" else "Axonometric"
        lighting_text, shading_text = self.lighting.get_lighting_info_text()
        
        info_text = f"{self.current_polyhedron_name} | {proj_text} | {mode_text} | {lighting_text}"
        info_text2 = f"Camera: ({cam_pos.x:.1f}, {cam_pos.y:.1f}, {cam_pos.z:.1f})"
        info_text3 = shading_text

        text_surface = self.font.render(info_text, True, (255, 255, 255))
        text_surface2 = self.font.render(info_text2, True, (255, 255, 255))
        text_surface3 = self.font.render(info_text3, True, (255, 255, 255))
        
        self.screen.blit(text_surface, (10, 10))
        self.screen.blit(text_surface2, (10, 50))
        self.screen.blit(text_surface3, (10, 90))

        controls = [
            "1-Octa 2-Icosa 4-Revolution  R-Reset",
            "WASD-Move  Arrows-Rotate  QE-Up/Down", 
            "B-ZBuffer  P-Proj  A-Line  T-Translate",
            "XYZ-Rotate  MN-Mirror  CL-SpecialRot",
            "SPACE-Lighting  Ctrl+O-Load  Ctrl+S-Save"
        ]

        small_font = pygame.font.Font(None, 24)
        for i, line in enumerate(controls):
            text = small_font.render(line, True, (255, 255, 255))
            self.screen.blit(text, (10, self.height - 150 + i * 25))

        self.input_panel.draw()
        pygame.display.flip()

    
    def draw_with_z_buffer(self, faces):
        """Отрисовка с использованием Z-буфера (без освещения)"""
        self.lighting.z_buffer.clear()
        self.screen.fill((0, 0, 0))
        
        for face in faces:
            points_2d = []
            depths = []
            
            for point in face.points:
                projected = self.project_3d_to_2d(point)
                points_2d.append(projected)
                depth = (point - self.camera.position).length()
                depths.append(depth)
            
            if len(points_2d) >= 3:
                if len(points_2d) == 3:
                    self.lighting.draw_triangle_with_lighting(
                        self.screen, points_2d, depths, None, use_zbuffer=True, face_color=face.color
                    )
                else:
                    # Разбиваем многоугольник на треугольники
                    for i in range(1, len(points_2d) - 1):
                        tri_points = [points_2d[0], points_2d[i], points_2d[i + 1]]
                        tri_depths = [depths[0], depths[i], depths[i + 1]]
                        self.lighting.draw_triangle_with_lighting(
                            self.screen, tri_points, tri_depths, None, use_zbuffer=True, face_color=face.color
                        )
        
        # Копируем буфер на экран
        pygame.surfarray.blit_array(self.screen, self.lighting.z_buffer.color_buffer.swapaxes(0, 1))

# ФИГУРА ВРАЩЕНИЯ
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
            self.reset_camera()
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

# ГРАФИК ФУНКЦИИ
    def start_function_mode(self):
        """Запускает режим создания поверхности функции"""
        self.function_mode = True
        self.function_panel.show()

    def create_function_surface(self):
        """Создает поверхность на основе введенных параметров"""
        try:
            # Получаем параметры из панели
            func_str = self.function_panel.fields["function"]["value"]
            x_min = float(self.function_panel.fields["x_min"]["value"])
            x_max = float(self.function_panel.fields["x_max"]["value"])
            y_min = float(self.function_panel.fields["y_min"]["value"])
            y_max = float(self.function_panel.fields["y_max"]["value"])
            divisions = int(self.function_panel.fields["divisions"]["value"])
            
            # Создаем поверхность
            self.function_surface = FunctionSurface(func_str, (x_min, x_max), (y_min, y_max), divisions)
            self.current_polyhedron = self.function_surface
            self.current_polyhedron_name = "function_surface"
            self.function_mode = False
            self.function_panel.hide()
            
            # Сбрасываем камеру для новой фигуры
            self.reset_camera()
            
            print(f"Создана поверхность функции: {func_str}")
            
        except Exception as e:
            print(f"Ошибка при создании поверхности: {e}")
            self.function_mode = False
            self.function_panel.hide()

    def draw_function_mode(self):
        """Отрисовка в режиме создания поверхности функции"""
        self.screen.fill((0, 0, 0))
        
        instructions = [
            "Режим создания поверхности функции",
            "Задайте параметры в панели ввода",
            "Нажмите 3 для выхода из режима без создания",
            "Используйте Tab для перехода между полями",
            "Enter для подтверждения, Esc для отмены"
        ]
        
        small_font = pygame.font.Font(None, 24)
        for i, line in enumerate(instructions):
            text = small_font.render(line, True, (255, 255, 255))
            self.screen.blit(text, (50, 50 + i * 30))
        
        self.function_panel.draw()
        pygame.display.flip()

    def handle_function_events(self, event):
        """Обработка событий в режиме функции"""
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_3:
                self.function_mode = False
                self.function_panel.hide()
                return True
                
        result = self.function_panel.handle_event(event)
        if result == 'ok':
            self.create_function_surface()
            return True
        elif result == 'cancel':
            self.function_mode = False
            self.function_panel.hide()
            return True
            
        return False

# ОБРАБОТКА СОБЫТИЙ
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
            
            if self.function_panel.visible:
                if self.handle_function_events(event):
                    continue
            
            if self.revolution_mode:
                if self.handle_revolution_events(event):
                    continue
            
            if self.function_mode:
                if self.handle_function_events(event):
                    continue

            elif event.type == pygame.KEYDOWN:

                if event.key in [pygame.K_1, pygame.K_KP1]:
                    self.switch_polyhedron("octahedron")
                elif event.key in [pygame.K_2, pygame.K_KP2]:
                    self.switch_polyhedron("icosahedron")
                elif event.key in [pygame.K_3, pygame.K_KP3]:
                    self.start_function_mode()
                    continue
                elif event.key in [pygame.K_4, pygame.K_KP4]:
                    self.start_revolution_mode()
                    continue
                elif event.key in [pygame.K_5, pygame.K_KP5]:
                    center = self.current_polyhedron.get_center()
                    self.lighting.rotate_light_around_object(center, 45)
                elif event.key == pygame.K_r:
                    self.current_polyhedron.reset_transform()
                    # Сбрасываем камеру
                    self.reset_camera()
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
                    transform = AffineTransform.reflection_yz()
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_n:
                    transform = AffineTransform.reflection_xy()
                    self.current_polyhedron.apply_transform(transform)
                elif event.key == pygame.K_b:
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
                    else:
                        self.projection_type = "perspective"
                    print(f"Projection type: {self.projection_type}")
                elif event.key == pygame.K_a:
                    # Показать/скрыть произвольную прямую
                    self.show_arbitrary_line = not self.show_arbitrary_line
                elif event.key == pygame.K_SPACE:
                    # Переключение освещения
                    self.lighting.set_lighting_enabled(not self.lighting.use_lighting)
                    print(f"Освещение: {'включено' if self.lighting.use_lighting else 'выключено'}")
                
                if event.key == pygame.K_o:  # Ctrl+O для загрузки
                    if pygame.key.get_mods() & pygame.KMOD_CTRL:
                        self.open_file_dialog("load")
                elif event.key == pygame.K_s:  # Ctrl+S для сохранения
                    if pygame.key.get_mods() & pygame.KMOD_CTRL:
                        self.open_file_dialog("save")
        
        # Управление камерой
        keys = pygame.key.get_pressed()
        
        if keys[pygame.K_LEFT]:
            self.camera.rotate_around_target(0, -self.camera_rotation_speed)
        if keys[pygame.K_RIGHT]:
            self.camera.rotate_around_target(0, self.camera_rotation_speed)
        if keys[pygame.K_UP]:
            self.camera.rotate_around_target(-self.camera_rotation_speed, 0)
        if keys[pygame.K_DOWN]:
            self.camera.rotate_around_target(self.camera_rotation_speed, 0)
        
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