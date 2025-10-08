import sys
import pygame
import math

pygame.init()

white = (255, 255, 255)
black = (0, 0, 0)

def create_board():
    pygame.init()
    screen = pygame.display.set_mode((800, 600))
    font = pygame.font.Font(None, 36)
    screen.fill(white)
    pygame.display.flip()
    return screen 

# добавить точку к многоугольнику
def add_point_to_polygon(x, y, p):
    pos = (x, y)
    p.append(pos)
    pygame.draw.circle(screen, black, pos, 5)
    if len(p) >= 2: 
        pygame.draw.line(screen, black, p[-2], pos, 1)
    pygame.display.flip()

# дорисовать последнее ребро в многоугольнике
def complete_polygon(p):
    if len(p) < 3: return
    pygame.draw.line(screen, black, p[-1], p[0], 1)
    pygame.display.flip()


# определяем, принадлежит ли точка многоугольнику
def point_in_polygon(x, y, points):
    n = len(points)
    inside = False
    p1x, p1y = points[0]
    
    for i in range(1, n + 1):
        p2x, p2y = points[i % n]
        if min(p1y, p2y) < y <= max(p1y, p2y):
            if x <= max(p1x, p2x):
                if p1y != p2y:
                    xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y) + p1x
                if p1x == p2x or x <= xinters:
                    inside = not inside
        p1x, p1y = p2x, p2y
    
    return inside

# определяем, на какой многоугольник нажали
def find_polygon(x, y, polygons):
    for p in polygons:
        if point_in_polygon(x, y, p):
            return p
    return False

# находит центр многоугольника как среднее арифметическое координат
def get_center(p):
    n = len(p)
    x, y = 0, 0
    for v in p:
        x += v[0]
        y += v[1]
    return x / n, y / n

# заново перерисовывает все многоугольники
def redraw_all_polygons(polygons):
    screen.fill(white)
    for p in polygons:
        for i in range(len(p)):
            pygame.draw.circle(screen, black, p[i], 5)
            pygame.draw.line(screen, black, p[i - 1], p[i], 1)
    pygame.display.flip()

# умножение вектора на матрицу       
def multiply_matrix(v, m):
    r = [0, 0, 0]
    for i in range(3):
        for j in range(3):
            r[i] += v[j] * m[j][i]
    return r
  
# меняет координаты многоугольника после какого-либо изменения положения
def change_coordinates(p, m):
    new_p = []
    
    for i in range(len(p)):
        c = multiply_matrix([p[i][0], p[i][1], 1], m)
        new_p.append((c[0], c[1]))
        
    return new_p

# Смещение на dx, dy
def move_dxdy(polygons, p, dx, dy):
    m = [[1, 0, 0],
         [0, 1, 0],
         [dx, dy, 1]]
    
    idx = polygons.index(p)
    polygons[idx] = change_coordinates(p, m)
    redraw_all_polygons(polygons)
    return polygons

# Поворот вокруг заданной пользователем точки или своего центра
def rotation_around_point(polygons, p, a, x = None, y = None):
    if x is None:
        x, y = get_center(p)
        
    cos_a = math.cos(math.radians(a))
    sin_a = math.sin(math.radians(a))
    
    m = [
        [cos_a, -sin_a, 0],
        [sin_a, cos_a, 0], 
        [-x * cos_a - y * sin_a + x, x * sin_a - y * cos_a + y, 1]
    ]
    
    idx = polygons.index(p)
    polygons[idx] = change_coordinates(p, m)
    redraw_all_polygons(polygons)
    return polygons

# Масштабирование относительно заданной пользователем точки или своего центра
def zooming_relative_point(polygons, p, kx, ky, x = None, y = None):
    if x is None:
        x, y = get_center(p)
        
    m = [[kx, 0, 0], 
         [0, ky, 0], 
         [(1 - kx) * x, (1 - ky) * y, 1]]
    
    idx = polygons.index(p)
    polygons[idx] = change_coordinates(p, m)
    redraw_all_polygons(polygons)
    return polygons

def line_intersection(a, b, c, d):
    n_x = -(d[1] - c[1])
    n_y = d[0] - c[0]
    
    numerator = -(n_x * (a[0] - c[0]) + n_y * (a[1] - c[1]))
    denominator = n_x * (b[0] - a[0]) + n_y * (b[1] - a[1])
    
    if abs(denominator) < 1e-10:
        return None
    
    t = numerator / denominator
    
    if t < 0 or t > 1:
        return None
    
    intersection_x = a[0] + t * (b[0] - a[0])
    intersection_y = a[1] + t * (b[1] - a[1])

    def is_between(v1, v2, value):
        return min(v1, v2) <= value <= max(v1, v2)
    
    if (is_between(a[0], b[0], intersection_x) and 
        is_between(a[1], b[1], intersection_y) and
        is_between(c[0], d[0], intersection_x) and 
        is_between(c[1], d[1], intersection_y)):
        return (intersection_x, intersection_y)
    else:
        return None

def point_side_of_edge(a, b, c):
    """
    Определяет, где находится точка c относительно направленного ребра ab.
    Возвращает:
      1  — точка слева
      -1 — точка справа
      0 — точка на линии
    """
    xa, ya = a
    xb, yb = b
    xc, yc = c
    
    det = (xb - xa) * (yc - ya) - (yb - ya) * (xc - xa)
    if abs(det) < 1e-9:
        return 0
    elif det > 0:
        return 1  # слева
    else:
        return -1 # справа

def point_in_convex_polygon(pt, polygon):
    """
    Проверяет, принадлежит ли точка pt выпуклому полигону polygon.
    Возвращает True/False.
    Примечание: точка принадлежит ему, 
                если она всегда находится с одной стороны всех его рёбер.
    """
    n = len(polygon)
    if n < 3:
        return False
    
    prev_side = None
    for i in range(n):
        a = polygon[i]
        b = polygon[(i + 1) % n]
        side = point_side_of_edge(a, b, pt)
        if side == 0:
            continue
        if prev_side is None:
            prev_side = side
        elif prev_side != side:
            return False
    return True


def tasks():
    comand = "" # какое действие было сделано последним
    point = (0, 0)
    p = [] # выбранный многоугольник
    polygon = [] # создаваемый многоугольник
    polygons = [] # все многоугольники
    edge_points = []  # точки пользовательского ребра для поиска пересечений

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1:

                    # нажали лкм и мы создаем многоугольник, значит, хотят добавить точку
                    if comand == "creating_polygon":
                        add_point_to_polygon(event.pos[0], event.pos[1], polygon)

                    # нажали лкм, и это выбор многоугольника
                    elif comand == "selecting_polygon": 
                        p = find_polygon(event.pos[0], event.pos[1], polygons) # определяем, на какой многоугольник нажали
                        if p is None: continue # если просто так нажали на поле - игнор
                        comand = "polygon_selected" # если нажали на многоугольник, запоминаем этот факт

                    # нажали лкм, и это выбор точки
                    elif comand == "selecting_point": 
                        point = event.pos # запоминаем, куда нажимали в последний раз
                        comand = "point_selected"
                    
                    elif comand == "drawing_edge_for_intersection":
                        edge_points.append(event.pos)

                        redraw_all_polygons(polygons)

                        for pt in edge_points:
                            pygame.draw.circle(screen, (255, 0, 0), pt, 5)

                        if len(edge_points) == 2:
                            start, end = edge_points
                            pygame.draw.line(screen, (255, 0, 0), start, end, 2)

                            intersection_points = []
                            for poly in polygons:
                                n = len(poly)
                                for i in range(n):
                                    a = poly[i]
                                    b = poly[(i + 1) % n]
                                    inter = line_intersection(start, end, a, b)
                                    if inter is not None:
                                        intersection_points.append(inter)

                            for pt in intersection_points:
                                pygame.draw.circle(screen, (0, 255, 0), (int(pt[0]), int(pt[1])), 6)

                            if intersection_points:
                                print("Найдены точки пересечения:")
                                for pt in intersection_points:
                                    print(f"  ({pt[0]:.2f}, {pt[1]:.2f})")
                            else:
                                print("Пересечений не найдено.")

                            edge_points = []

                        pygame.display.flip()
                            
                    elif comand == "check_point_in_polygon":
                        point = event.pos
                        pygame.draw.circle(screen, (0, 0, 255), point, 5)
                        pygame.display.flip()

                        # Проверяем положение точки относительно каждого ребра каждого многоугольника
                        for poly in polygons:
                            print("\nПроверка относительно рёбер многоугольника:")
                            for i in range(len(poly)):
                                a = poly[i]
                                b = poly[(i + 1) % len(poly)]
                                side = point_side_of_edge(a, b, point)
                                if side == 1:
                                    print(f"  Точка слева от ребра {a} → {b}")
                                elif side == -1:
                                    print(f"  Точка справа от ребра {a} → {b}")
                                else:
                                    print(f"  Точка на ребре {a} → {b}")

                            # После классификации проверяем принадлежность
                            if point_in_convex_polygon(point, poly):
                                print("→ Точка внутри выпуклого многоугольника.")
                            elif point_in_polygon(point[0], point[1], poly):
                                print("→ Точка внутри невыпуклого многоугольника (метод лучей).")
                            else:
                                print("→ Точка вне многоугольников.")

                        comand = ""

                else:

                  # если нажали пкм и мы создаем многоугольник, то добавляем последнее ребро и завершаем его создание
                  if comand == "creating_polygon":
                      complete_polygon(polygon)
                      polygons.append(polygon.copy())
                      polygon.clear()
                      comand = "polygon_created"
                    
                  elif comand == "drawing_edge_for_intersection":
                      edge_points.clear()
                      screen.fill(white)
                      redraw_all_polygons(polygons)
                      comand = ""
                      print("Рисование ребра отменено")
                  
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    return
                  
                # если нажали n, значит хотят создать многоугольник
                elif event.key == pygame.K_n:
                    if comand == "drawing_edge_for_intersection":
                        redraw_all_polygons(polygons)
                    comand = "creating_polygon"

                # если нажали p, значит хотят выбрать точку
                elif event.key == pygame.K_p:
                    if comand == "drawing_edge_for_intersection":
                        redraw_all_polygons(polygons)
                    comand = "selecting_point"

                # если нажали s, значит хотят выбрать многоугольник
                elif event.key == pygame.K_s:
                    if comand == "drawing_edge_for_intersection":
                        redraw_all_polygons(polygons)
                    comand = "selecting_polygon"
                
                # если нажали r, то поворачиваем выбранный ранее многоугольник вокруг выбранной точки на заданный угол
                elif event.key == pygame.K_r and comand == "point_selected":
                    polygons = rotation_around_point(polygons, p, 10, point[0], point[1])
                    comand = "polygon_rotated"
                    
                # если нажали r, предварительно выбрав только многоугольник, то поворачиваем многоугольник вокруг его центра на заданный угол
                elif event.key == pygame.K_r and comand == "polygon_selected":
                    polygons = rotation_around_point(polygons, p, 10)
                    comand = "polygon_rotated"
                    
                # если нажали z, то масштабируем выбранный ранее многоугольник относительно выбранной точки с заданными коэффициентами
                elif event.key == pygame.K_z and comand == "point_selected":
                    polygons = zooming_relative_point(polygons, p, 1.2, 1.1, point[0], point[1])
                    comand = "polygon_zoomed"
                
                # если нажали z, предварительно выбрав только многоугольник, то масштабируем его относительно его центра с заданными коэффициентами
                elif event.key == pygame.K_z and comand == "polygon_selected":
                    polygons = zooming_relative_point(polygons, p, 1.2, 1.1)
                    comand = "polygon_zoomed"
                
                # если нажали m, предварительно выбрав многоугольник, то смещаем его на заданные dx, dy
                elif event.key == pygame.K_m and comand == "polygon_selected":
                    polygons = move_dxdy(polygons, p, 10, 10)
                    comand = "polygon_moved"
                
                # если нажали c, очищаем всю сцену
                elif event.key == pygame.K_c:
                    polygons.clear()
                    polygon.clear()
                    screen.fill(white)
                    pygame.display.flip()
                    comand = ""
                    print("Сцена отчищена")

                # если нажали i, то переходим в режим рисования ребра для поиска пересечений
                elif event.key == pygame.K_i:
                    comand = "drawing_edge_for_intersection"
                    edge_points.clear()
                    print("Режим поиска пересечений: кликните 2 раза, чтобы задать ребро.")            
            
                elif event.key == pygame.K_t:
                    comand = "check_point_in_polygon"
                    print("Режим проверки точки: кликните, чтобы выбрать точку для проверки.")
                    

screen = create_board()
tasks()
pygame.quit()
sys.exit()
