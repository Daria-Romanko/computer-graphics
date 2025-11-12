import pygame
import numpy as np
import math
from common import Point3D, Face, Polyhedron, AffineTransform

class SurfaceOfRevolution(Polyhedron):
    def __init__(self, generatrix_points, axis = 'y', divisions = 12 ):
      self.generatrix = generatrix_points
      self.axis = axis
      self.divisions = divisions

      faces = self._create_revolution_faces()
      super().__init__(faces)

    def _create_revolution_faces(self):
        vertices = []
        faces = []

        angle_step = 2 * math.pi / self.divisions
        profile_count = len(self.generatrix)

        # Создаём все вершины
        for i in range(self.divisions):
            angle = i * angle_step
            rotation_matrix = self._get_rotation_matrix(angle)

            for vertex in self.generatrix:
                vertex_arr = vertex.to_array()
                rotated_arr = np.dot(rotation_matrix, vertex_arr)
                rotated_vertex = Point3D.from_array(rotated_arr)
                vertices.append(rotated_vertex)

        # Боковые грани
        for i in range(self.divisions):
            next_i = (i + 1) % self.divisions

            for j in range(profile_count - 1):
                v1 = i * profile_count + j
                v2 = next_i * profile_count + j
                v3 = next_i * profile_count + j + 1
                v4 = i * profile_count + j + 1

                # реверс для правильной ориентации нормалей
                face_points = [vertices[v1], vertices[v2], vertices[v3], vertices[v4]][::-1]
                face_color = self._get_color_for_face(i, j)
                faces.append(Face(face_points, face_color))

        # Верхняя и нижняя крышки
        first_pt = self.generatrix[0]
        last_pt = self.generatrix[-1]

        # Проверяем, близко ли к оси
        def near_axis(pt):
            if self.axis == 'y':
                return abs(pt.x) < 1e-6
            elif self.axis == 'x':
                return abs(pt.y) < 1e-6
            elif self.axis == 'z':
                return abs(pt.x) < 1e-6
            return False

        if not near_axis(first_pt):
            center_bottom = Point3D(0, first_pt.y, 0)
            bottom_ring = [vertices[i * profile_count + 0] for i in range(self.divisions)]
            # создаём треугольники (центр + сегмент кольца)
            for i in range(self.divisions):
                next_i = (i + 1) % self.divisions
                tri_pts = [center_bottom, bottom_ring[next_i], bottom_ring[i]][::-1]
                faces.append(Face(tri_pts, (180, 180, 180)))

        if not near_axis(last_pt):
            center_top = Point3D(0, last_pt.y, 0)
            top_ring = [vertices[i * profile_count + (profile_count - 1)] for i in range(self.divisions)]
            for i in range(self.divisions):
                next_i = (i + 1) % self.divisions
                tri_pts = [center_top, top_ring[i], top_ring[next_i]][::-1]
                faces.append(Face(tri_pts, (200, 200, 200)))

        return faces

    def _get_rotation_matrix(self, angle):
       if self.axis == 'x':
          return AffineTransform.rotation_x(angle)
       elif self.axis == 'y':
          return AffineTransform.rotation_y(angle)
       elif self.axis == 'z':
          return AffineTransform.rotation_z(angle)
       else:
          return np.identity(4)
    
    def _get_color_for_face(self, division_idx, profile_idx):
        hue = (division_idx / self.divisions) * 360
        saturation = 0.7 + 0.3 * (profile_idx / len(self.generatrix))
        value = 0.8

        h = hue / 60
        i = math.floor(h)
        f = h - i
        
        p = value * (1 - saturation)
        q = value * (1 - saturation * f)
        t = value * (1 - saturation * (1 - f))
        
        if i == 0:
            r, g, b = value, t, p
        elif i == 1:
            r, g, b = q, value, p
        elif i == 2:
            r, g, b = p, value, t
        elif i == 3:
            r, g, b = p, q, value
        elif i == 4:
            r, g, b = t, p, value
        else:
            r, g, b = value, p, q
        
        return (int(r * 255), int(g * 255), int(b * 255))
    
class RevolutionInputPanel:
    def __init__(self, screen):
        self.screen = screen
        self.width = 380
        self.height = 180
        self.x = (screen.get_width() - self.width) // 2
        self.y = 30
        self.rect = pygame.Rect(self.x, self.y, self.width, self.height)
        self.visible = False
            
        self.axis = 'y'
        self.divisions = 12
        self.active_input = None
        self.divisions_text = str(self.divisions)
        
        self.font = pygame.font.Font(None, 28)
        self.small_font = pygame.font.Font(None, 24)
        
        self.bg_color = (25, 25, 35)
        self.accent_color = (220, 220, 230)
        self.highlight_color = (80, 150, 255)
        
        self.axis_buttons = [
            {'rect': pygame.Rect(self.x + 80, self.y + 50, 60, 32), 'axis': 'x', 'label': 'X'},
            {'rect': pygame.Rect(self.x + 160, self.y + 50, 60, 32), 'axis': 'y', 'label': 'Y'},
            {'rect': pygame.Rect(self.x + 240, self.y + 50, 60, 32), 'axis': 'z', 'label': 'Z'}
        ]
        
        self.divisions_input_rect = pygame.Rect(self.x + 140, self.y + 100, 60, 32)
        
        self.ok_button = pygame.Rect(self.x + 90, self.y + 140, 80, 32)
        self.cancel_button = pygame.Rect(self.x + 210, self.y + 140, 80, 32)
        
    def show(self):
        self.visible = True
        
    def hide(self):
        self.visible = False
        
    def draw(self):
        if not self.visible:
            return
            
        pygame.draw.rect(self.screen, self.bg_color, self.rect)
        pygame.draw.rect(self.screen, self.accent_color, self.rect, 1)
        
        title = self.small_font.render("Revolution Parameters", True, self.accent_color)
        self.screen.blit(title, (self.x + 20, self.y + 15))
        
        axis_text = self.small_font.render("Axis:", True, self.accent_color)
        self.screen.blit(axis_text, (self.x + 20, self.y + 55))
        
        for button in self.axis_buttons:
            if self.axis == button['axis']:
                pygame.draw.rect(self.screen, self.highlight_color, button['rect'])
            else:
                pygame.draw.rect(self.screen, self.bg_color, button['rect'])
            
            pygame.draw.rect(self.screen, self.accent_color, button['rect'], 1)
            
            label = self.small_font.render(button['label'], True, 
                                         self.accent_color if self.axis != button['axis'] else self.bg_color)
            label_rect = label.get_rect(center=button['rect'].center)
            self.screen.blit(label, label_rect)
        
        divisions_text = self.small_font.render("Segments:", True, self.accent_color)
        self.screen.blit(divisions_text, (self.x + 20, self.y + 105))
        
        if self.active_input == 'divisions':
            pygame.draw.rect(self.screen, self.highlight_color, self.divisions_input_rect)
        else:
            pygame.draw.rect(self.screen, self.bg_color, self.divisions_input_rect)
            
        pygame.draw.rect(self.screen, self.accent_color, self.divisions_input_rect, 1)
        
        divisions_label = self.small_font.render(self.divisions_text, True, 
                                               self.accent_color if self.active_input != 'divisions' else self.bg_color)
        divisions_rect = divisions_label.get_rect(center=self.divisions_input_rect.center)
        self.screen.blit(divisions_label, divisions_rect)
        
        pygame.draw.rect(self.screen, self.bg_color, self.ok_button)
        pygame.draw.rect(self.screen, self.accent_color, self.ok_button, 1)
        ok_text = self.small_font.render("OK", True, self.accent_color)
        ok_rect = ok_text.get_rect(center=self.ok_button.center)
        self.screen.blit(ok_text, ok_rect)
        
        pygame.draw.rect(self.screen, self.bg_color, self.cancel_button)
        pygame.draw.rect(self.screen, self.accent_color, self.cancel_button, 1)
        cancel_text = self.small_font.render("CANCEL", True, self.accent_color)
        cancel_rect = cancel_text.get_rect(center=self.cancel_button.center)
        self.screen.blit(cancel_text, cancel_rect)
        
    def handle_event(self, event):
        if not self.visible:
            return None
            
        if event.type == pygame.MOUSEBUTTONDOWN:
            for button in self.axis_buttons:
                if button['rect'].collidepoint(event.pos):
                    self.axis = button['axis']
            
            if self.divisions_input_rect.collidepoint(event.pos):
                self.active_input = 'divisions'
            else:
                self.active_input = None
            
            if self.ok_button.collidepoint(event.pos):
                try:
                    self.divisions = int(self.divisions_text) if self.divisions_text else 12
                    if self.divisions < 3:
                        self.divisions = 3
                    elif self.divisions > 100:
                        self.divisions = 100
                    return 'ok'
                except ValueError:
                    self.divisions_text = "12"
                    self.divisions = 12
            
            if self.cancel_button.collidepoint(event.pos):
                return 'cancel'
        
        elif event.type == pygame.KEYDOWN and self.active_input == 'divisions':
            if event.key == pygame.K_RETURN:
                self.active_input = None
            elif event.key == pygame.K_BACKSPACE:
                self.divisions_text = self.divisions_text[:-1]
            elif event.key == pygame.K_ESCAPE:
                self.active_input = None
            elif event.unicode.isdigit():
                if len(self.divisions_text) < 3:
                    new_text = self.divisions_text + event.unicode
                    if new_text and int(new_text) <= 100:
                        self.divisions_text = new_text
        
        return None