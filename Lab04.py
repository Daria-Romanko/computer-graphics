import pygame
import sys

white = (255, 255, 255)
black = (0, 0, 0)

class Polygon:
    def __init__(self):
        self.points = []
        
    def add_point(self, pos):
        self.points.append(pos)
        pygame.draw.circle(screen, black, pos, 5)
        if len(self.points) >= 2:
            pygame.draw.line(screen, black, self.points[-2], pos, 1)
        pygame.display.flip()
    
    def complete(self):
        if len(self.points) >= 3:
            pygame.draw.line(screen, black, self.points[-1], self.points[0], 1)
        pygame.display.flip()

def create_board():
    pygame.init()
    screen = pygame.display.set_mode((800, 600))
    font = pygame.font.Font(None, 36)
    screen.fill(white)
    pygame.display.flip()
    return screen

def create_Polygons():
    polygon = Polygon()
    polygons = []
    
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:
                    polygon.add_point(event.pos)
                else:
                    polygon.complete()
                    polygons.append(polygon)
                    polygon = Polygon()
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_RETURN:
                    return polygons
                elif event.key == pygame.K_ESCAPE:
                    return
    
    return polygons

# определяем, принадлежит ли точка многоугольнику
def point_in_polygon(x, y, p):
    n = len(p.points)
    inside = False
    
    p1x, p1y = p.points[0]
    for i in range(1, n + 1):
        p2x, p2y = p[i % n]
        if y > min(p1y, p2y):
            if y <= max(p1y, p2y):
                if x <= max(p1x, p2x):
                    if p1y != p2y:
                        xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                    if p1x == p2x or x <= xinters:
                        inside = not inside
        p1x, p1y = p2x, p2y
    
    return inside

# определяем, на какой многоугольник нажали
def find_polygon(x, y, polygons):
    for p, v in polygons:
        if point_in_polygon(x, y, p):
            return p
    return False

# находит центр многоугольника как среднее арифметическое координат
def get_center(p):
    n = len(p.points)
    x, y = 0, 0
    
    for v in p.points:
        x += v[0]
        y += v[1]
        
    return x / n, y / n

# Смещение на dx, dy
def move_dxdy(p, dx, dy):
    m = [[  1,   0,   0],
         [  0,   1,   0],
         [-dx, -dy,   1]]

# Поворот вокруг заданной пользователем точки или своего центра
def rotation_around_point(p, a, x = false, y = false):
    if not x:
        x, y = get_center(p)
    m = [[cos(a), sin(a), 0], 
         [-sin(a), cos(a), 0], 
         [-x*cos(a) + y*sin(a) + x, -x*sin(a) - y*cos(a) + y, 1]]

# Масштабирование относительно заданной пользователем точки или своего центра
def zooming_relative_point(p, kx, ky, x = false, y = false):
    if not x:
        x, y = get_center(p)
    m = [[kx, 0, 0], 
         [0, ky, 0], 
         [(1 - kx) * x, (1 - ky) * y, 1]]

# подразумевается, что на момент выполения заданий (вращений, определений, к чему относится точка и т.д.)
# все многоугольники уже созданы и хранятся в списке polygons
def tasks():
    while True:
        comand = "" # какое действие было сделано последним
        for event in pygame.event.get():
            point = pos # запоминаем, куда наимали в последний раз
            if event.type == pygame.QUIT:
                return
            elif event.type == pygame.MOUSEBUTTONDOWN:
            
                # нажали лкм, но до этого никакой многоугольник не был выбран
                if event.button == 1 and comand != "polygon_selected": 
                    p = find_polygon(pos, polygons) # определяем, на какой многоугольник нажали
                    if not p: continue # если просто так нажали на поле - игнор
                    comand = "polygon_selected" # если нажали на многоугольник, запоминаем этот факт
                
                # если нажали лкм, и многоугольник уже был выбран раньше, значит, хотят выбрать точку
                elif event.button == 1: 
                    comand = "point_selected"
                    point = pos

            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    return
                    
                # если нажали R, выбрав точку (и предварительно многоугольник), то поворачиваем многоугольник вокруг этой точки на заданный угол
                elif event.key == pygname.K_r and comand == "point_selected":
                    rotation_around_point(p, int(input("angle: ")), point)
                    
                # если нажали R, предварительно выбрав только многоугольник, то поворачиваем многоугольник вокруг его центра на заданный угол
                elif event.key == pygname.K_r and comand == "polygon_selected":
                    rotation_around_point(p, int(input("angle: ")))
                    
                # если нажали Z, выбрав точку (и предварительно многоугольник), то масштабируем его относительно этой точки с заданными коэффициентами
                elif event.key == pygname.K_z and comand == "point_selected":
                    zooming_relative_point(p, int(input("kx: ")), int(input("ky: ")), point)
                
                # если нажали Z, предварительно выбрав только многоугольник, то масштабируем его относительно его центра с заданными коэффициентами
                elif event.key == pygname.K_z and comand == "polygon_selected":
                    zooming_relative_pointt(p, int(input("kx: ")), int(input("ky: ")))
                
                # если нажали M, предварительно выбрав многоугольник, то смещаем его на заданные dx, dy
                elif event.key == pygname.K_m and comand == "polygon_selected":
                    move_dxdy(p, int(input("dx: ")), int(input("dy: ")))
              



screen = create_board()
polygons = create_Polygons()

tasks()

pygame.quit()
sys.exit()
