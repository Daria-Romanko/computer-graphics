from PIL import Image
import numpy as np
import random

'''
Задание 3.
Выполнить градиентное окрашивание произвольного треугольника,
у которого все три вершины разного цвета, используя алгоритм растеризации треугольника.
'''

# вершина треугольника. Имеет координаты х и у и цвет RGB
class Vertex:
    def __init__(self, x, y, color):
        self.x = x
        self.y = y
        self.color = color

# треугольник. Состоит из трех вершин Vertices
class Triangle:
    def __init__(self, v1, v2, v3):
        self.vertices = [v1, v2, v3]

    def get_bounding_box(self):
        x = [v.x for v in self.vertices]
        y = [v.y for v in self.vertices]
        return min(x), min(y), max(x), max(y)

# барицентричексие координаты (смю презентацию Лекция 3, слайды 42-46)
def barycentric_coordinates(x, y, v1, v2, v3):
    denom = (v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y)
    
    lambda1 = ((v2.y - v3.y) * (x - v3.x) + (v3.x - v2.x) * (y - v3.y)) / denom
    lambda2 = ((v3.y - v1.y) * (x - v3.x) + (v1.x - v3.x) * (y - v3.y)) / denom
    lambda3 = 1 - lambda1 - lambda2
    
    return lambda1, lambda2, lambda3

# рестеризация треугольника
def rasterize_triangle(triangle, width, height):
    image = Image.new('RGB', (width, height), 'black')
    pixels = image.load()
    
    x_min, y_min, x_max, y_max = triangle.get_bounding_box()
    v1, v2, v3 = triangle.vertices
    
    for y in range(y_min, y_max + 1):
        for x in range(x_min, x_max + 1):
            lambda1, lambda2, lambda3 = barycentric_coordinates(x, y, v1, v2, v3)
            
            if (lambda1 >= 0 and lambda2 >= 0 and lambda3 >= 0 and
                lambda1 <= 1 and lambda2 <= 1 and lambda3 <= 1):

                r = int(lambda1 * v1.color[0] + lambda2 * v2.color[0] + lambda3 * v3.color[0])
                g = int(lambda1 * v1.color[1] + lambda2 * v2.color[1] + lambda3 * v3.color[1])
                b = int(lambda1 * v1.color[2] + lambda2 * v2.color[2] + lambda3 * v3.color[2])
                
                pixels[x, y] = (r, g, b)
    
    return image

w, h = 800, 800
v = []
for i in range(3):
    r, g, b = random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)
    v.append(Vertex(random.randint(0, w), random.randint(0, h), (r, g, b)))

image = rasterize_triangle(Triangle(v[0], v[1], v[2]), w, h)
image.show()
