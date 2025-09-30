import matplotlib.pyplot as plt
import math

# Алгоритм Брезенхема (целочисленный)
def bresenham(x0, y0, x1, y1):
    points = []  # список точек для линии
    dx = abs(x1 - x0)
    dy = abs(y1 - y0)
    sx = 1 if x0 < x1 else -1  # направление по x
    sy = 1 if y0 < y1 else -1  # направление по y
    err = dx - dy

    while True:
        points.append((x0, y0))  # добавляем текущую точку
        if x0 == x1 and y0 == y1:  # достигли конца линии
            break
        e2 = 2 * err
        if e2 > -dy:  # корректируем ошибку по x
            err -= dy
            x0 += sx
        if e2 < dx:   # корректируем ошибку по y
            err += dx
            y0 += sy
    return points


# Алгоритм Ву (с сглаживанием)
def wu(x0, y0, x1, y1):
    def fpart(x): return x - math.floor(x)  # дробная часть
    def rfpart(x): return 1 - fpart(x)      # обратная дробная часть

    def plot(x, y, c):
        pixels.append((x, y, c))  # сохраняем с "прозрачностью" c (0..1)

    pixels = []
    dx = x1 - x0
    dy = y1 - y0

    steep = abs(dy) > abs(dx)  # проверка наклонной линии
    if steep:
        x0, y0 = y0, x0
        x1, y1 = y1, x1

    if x0 > x1:  # линия слева направо
        x0, x1 = x1, x0
        y0, y1 = y1, y0

    dx = x1 - x0
    dy = y1 - y0
    gradient = dy / dx if dx != 0 else 1

    # первая точка
    xend = round(x0)
    yend = y0 + gradient * (xend - x0)
    xgap = rfpart(x0 + 0.5)
    xpxl1 = xend
    ypxl1 = math.floor(yend)

    if steep:
        plot(ypxl1,   xpxl1, rfpart(yend) * xgap)
        plot(ypxl1+1, xpxl1, fpart(yend) * xgap)
    else:
        plot(xpxl1, ypxl1,   rfpart(yend) * xgap)
        plot(xpxl1, ypxl1+1, fpart(yend) * xgap)
    intery = yend + gradient

    # последняя точка
    xend = round(x1)
    yend = y1 + gradient * (xend - x1)
    xgap = fpart(x1 + 0.5)
    xpxl2 = xend
    ypxl2 = math.floor(yend)

    if steep:
        plot(ypxl2,   xpxl2, rfpart(yend) * xgap)
        plot(ypxl2+1, xpxl2, fpart(yend) * xgap)
    else:
        plot(xpxl2, ypxl2,   rfpart(yend) * xgap)
        plot(xpxl2, ypxl2+1, fpart(yend) * xgap)

    # основные точки
    if steep:
        for x in range(xpxl1+1, xpxl2):
            plot(math.floor(intery),   x, rfpart(intery))
            plot(math.floor(intery)+1, x, fpart(intery))
            intery += gradient
    else:
        for x in range(xpxl1+1, xpxl2):
            plot(x, math.floor(intery),   rfpart(intery))
            plot(x, math.floor(intery)+1, fpart(intery))
            intery += gradient

    return pixels


# Отрисовка сравнения
if __name__ == "__main__":
    # координаты отрезка
    x0, y0, x1, y1 = 10, 10, 190, 50

    # получаем точки
    points_bres = bresenham(x0, y0, x1, y1)
    points_wu = wu(x0, y0, x1, y1)

    # создаем фигуру
    plt.figure(figsize=(10, 5))

    # Брезенхем
    plt.subplot(1, 2, 1)
    x, y = zip(*points_bres)
    plt.scatter(x, y, c="black", s=10)
    plt.title("Алгоритм Брезенхема")
    plt.gca().invert_yaxis()  # чтобы совпадало с экранной системой координат
    plt.axis("equal")

    # Ву
    plt.subplot(1, 2, 2)
    for px, py, c in points_wu:
        plt.scatter(px, py, c="black", alpha=c, s=10)
    plt.title("Алгоритм Ву (сглаживание)")
    plt.gca().invert_yaxis()
    plt.axis("equal")

    plt.show()