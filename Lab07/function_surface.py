import pygame
import numpy as np
import math
from common import Point3D, Face, Polyhedron

class FunctionSurface(Polyhedron):
    def __init__(self, func_str="math.sin(x) * math.cos(y)", x_range=(-2, 2), y_range=(-2, 2), divisions=20):
        super().__init__()
        self.func_str = func_str
        self.x_range = x_range
        self.y_range = y_range
        self.divisions = divisions
        self.generate_surface()
    
    def generate_surface(self):
        """Генерирует поверхность на основе функции"""
        self.faces = []
        self.vertices = []
        
        x_min, x_max = self.x_range
        y_min, y_max = self.y_range
        
        # Создаем сетку точек
        vertices_grid = []
        for i in range(self.divisions + 1):
            row = []
            for j in range(self.divisions + 1):
                x = x_min + (x_max - x_min) * i / self.divisions
                y = y_min + (y_max - y_min) * j / self.divisions
                
                try:
                    # Безопасное вычисление функции
                    z = eval(self.func_str, {"math": math, "x": x, "y": y})
                    if not isinstance(z, (int, float)) or math.isnan(z) or math.isinf(z):
                        z = 0
                except:
                    z = 0
                
                point = Point3D(x, y, z)
                self.vertices.append(point)
                row.append(point)
            vertices_grid.append(row)
        
        # Создаем грани (квадраты из двух треугольников)
        for i in range(self.divisions):
            for j in range(self.divisions):
                # Первый треугольник
                face1 = Face([
                    vertices_grid[i][j],
                    vertices_grid[i+1][j],
                    vertices_grid[i][j+1]
                ], (100, 150, 200))
                
                # Второй треугольник
                face2 = Face([
                    vertices_grid[i+1][j],
                    vertices_grid[i+1][j+1],
                    vertices_grid[i][j+1]
                ], (100, 150, 200))
                
                self.faces.extend([face1, face2])
        
        print(f"Сгенерирована поверхность с {len(self.faces)} гранями")

class FunctionInputPanel:
    def __init__(self, screen):
        self.screen = screen
        self.visible = False
        self.font = pygame.font.Font(None, 24)
        self.small_font = pygame.font.Font(None, 20)
        
        # Параметры по умолчанию
        self.function = "math.sin(x) * math.cos(y)"
        self.x_min = "-2"
        self.x_max = "2"
        self.y_min = "-2"
        self.y_max = "2"
        self.divisions = "20"
        
        self.active_field = None
        self.fields = {
            "function": {"label": "Функция f(x,y):", "value": self.function, "rect": None},
            "x_min": {"label": "X min:", "value": self.x_min, "rect": None},
            "x_max": {"label": "X max:", "value": self.x_max, "rect": None},
            "y_min": {"label": "Y min:", "value": self.y_min, "rect": None},
            "y_max": {"label": "Y max:", "value": self.y_max, "rect": None},
            "divisions": {"label": "Разбиений:", "value": self.divisions, "rect": None}
        }
        
        self.ok_rect = None
        self.cancel_rect = None
        
    def show(self):
        self.visible = True
        self.active_field = "function"
        
    def hide(self):
        self.visible = False
        self.active_field = None
        
    def handle_event(self, event):
        if not self.visible:
            return None
            
        if event.type == pygame.MOUSEBUTTONDOWN:
            mouse_pos = event.pos
            
            # Проверяем клик по полям ввода
            for field_name, field_data in self.fields.items():
                if field_data["rect"] and field_data["rect"].collidepoint(mouse_pos):
                    self.active_field = field_name
                    return "field_click"
            
            # Проверяем кнопки
            if self.ok_rect and self.ok_rect.collidepoint(mouse_pos):
                return "ok"
            elif self.cancel_rect and self.cancel_rect.collidepoint(mouse_pos):
                return "cancel"
                
        elif event.type == pygame.KEYDOWN:
            if self.active_field:
                if event.key == pygame.K_RETURN:
                    if self.active_field == "function":
                        self.active_field = "x_min"
                    elif self.active_field == "x_min":
                        self.active_field = "x_max"
                    elif self.active_field == "x_max":
                        self.active_field = "y_min"
                    elif self.active_field == "y_min":
                        self.active_field = "y_max"
                    elif self.active_field == "y_max":
                        self.active_field = "divisions"
                    else:
                        return "ok"
                elif event.key == pygame.K_BACKSPACE:
                    self.fields[self.active_field]["value"] = self.fields[self.active_field]["value"][:-1]
                elif event.key == pygame.K_ESCAPE:
                    return "cancel"
                elif event.key == pygame.K_TAB:
                    # Переход к следующему полю
                    field_names = list(self.fields.keys())
                    current_index = field_names.index(self.active_field)
                    self.active_field = field_names[(current_index + 1) % len(field_names)]
                else:
                    # Добавляем символ в активное поле
                    self.fields[self.active_field]["value"] += event.unicode
                    
        return None
        
    def draw(self):
        if not self.visible:
            return
            
        # Рисуем полупрозрачный фон
        overlay = pygame.Surface((self.screen.get_width(), self.screen.get_height()), pygame.SRCALPHA)
        overlay.fill((0, 0, 0, 128))
        self.screen.blit(overlay, (0, 0))
        
        # Рисуем панель
        panel_width = 500
        panel_height = 400
        panel_x = (self.screen.get_width() - panel_width) // 2
        panel_y = (self.screen.get_height() - panel_height) // 2
        
        panel_rect = pygame.Rect(panel_x, panel_y, panel_width, panel_height)
        pygame.draw.rect(self.screen, (50, 50, 80), panel_rect)
        pygame.draw.rect(self.screen, (100, 100, 150), panel_rect, 2)
        
        # Заголовок
        title = self.font.render("Параметры поверхности функции", True, (255, 255, 255))
        self.screen.blit(title, (panel_x + 10, panel_y + 10))
        
        # Поля ввода
        y_offset = panel_y + 50
        field_height = 30
        field_spacing = 10
        
        for field_name, field_data in self.fields.items():
            # Метка
            label = self.small_font.render(field_data["label"], True, (255, 255, 255))
            self.screen.blit(label, (panel_x + 20, y_offset))
            
            # Поле ввода
            input_rect = pygame.Rect(panel_x + 200, y_offset, 250, field_height)
            field_data["rect"] = input_rect
            
            # Цвет поля в зависимости от активности
            if self.active_field == field_name:
                color = (100, 100, 200)
            else:
                color = (70, 70, 100)
                
            pygame.draw.rect(self.screen, color, input_rect)
            pygame.draw.rect(self.screen, (150, 150, 200), input_rect, 1)
            
            # Текст
            text = self.small_font.render(field_data["value"], True, (255, 255, 255))
            self.screen.blit(text, (input_rect.x + 5, input_rect.y + 5))
            
            y_offset += field_height + field_spacing
        
        # Кнопки
        button_width = 100
        button_height = 30
        button_y = panel_y + panel_height - 50
        
        self.ok_rect = pygame.Rect(panel_x + 100, button_y, button_width, button_height)
        self.cancel_rect = pygame.Rect(panel_x + 250, button_y, button_width, button_height)
        
        pygame.draw.rect(self.screen, (0, 150, 0), self.ok_rect)
        pygame.draw.rect(self.screen, (150, 0, 0), self.cancel_rect)
        
        ok_text = self.small_font.render("OK", True, (255, 255, 255))
        cancel_text = self.small_font.render("Отмена", True, (255, 255, 255))
        
        self.screen.blit(ok_text, (self.ok_rect.centerx - ok_text.get_width() // 2, 
                                 self.ok_rect.centery - ok_text.get_height() // 2))
        self.screen.blit(cancel_text, (self.cancel_rect.centerx - cancel_text.get_width() // 2, 
                                     self.cancel_rect.centery - cancel_text.get_height() // 2))
        
        # Подсказки
        hints = [
            "Доступные функции: math.sin, math.cos, math.tan, math.exp, math.log, math.sqrt",
            "Примеры: math.sin(x) * math.cos(y), x**2 + y**2, math.exp(-(x**2 + y**2))",
            "Tab - следующее поле, Enter - подтвердить, Esc - отмена"
        ]
        
        hint_y = panel_y + panel_height + 10
        for hint in hints:
            hint_text = self.small_font.render(hint, True, (200, 200, 100))
            self.screen.blit(hint_text, (panel_x, hint_y))
            hint_y += 20