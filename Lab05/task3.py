"""
bezier_editor.py
Интерактивный редактор составной кубической кривой Безье (tkinter).

Управление:
- ЛКМ на пустом месте: добавить новую опорную точку.
- ЛКМ на точке + drag: переместить точку.
- ПКМ (правый клик) на точке: удалить точку.
- Клавиша C: очистить все точки.
- Клавиша H: показать/скрыть подсказку.
- Клавиша Q или Esc: выйти.

Сегменты: берутся каждые 3 точки: (P0,P1,P2,P3), (P3,P4,P5,P6), ...
Требование для полного сегмента: нужно как минимум 4 точки; для k сегментов — N = 3*k + 1.
"""

import tkinter as tk
from math import hypot

# Настройки визуала
CANVAS_W, CANVAS_H = 1000, 700
POINT_RADIUS = 6
SELECT_RADIUS = 10
BEZIER_SAMPLES = 80  # точек на сегмент для отрисовки (увеличьте для более гладкой кривой)

class BezierEditor:
    def __init__(self, root):
        self.root = root
        root.title("Составная кубическая кривая Безье — редактор")
        self.canvas = tk.Canvas(root, width=CANVAS_W, height=CANVAS_H, bg="white")
        self.canvas.pack(fill=tk.BOTH, expand=True)

        self.points = []  # список контрольных точек [(x,y), ...]
        self.drag_index = None
        self.show_help = True

        # бинды мыши и клавиатуры
        self.canvas.bind("<Button-1>", self.on_left_down)
        self.canvas.bind("<B1-Motion>", self.on_left_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_left_up)
        self.canvas.bind("<Button-3>", self.on_right_click)  # правая кнопка
        self.root.bind("<Key>", self.on_key)

        # ID объектов на canvas чтобы обновлять/удалять
        self._items = {}
        self.redraw()

    # ----- события -----
    def on_left_down(self, event):
        idx = self.find_point(event.x, event.y)
        if idx is not None:
            # начинаем тянуть существующую точку
            self.drag_index = idx
        else:
            # добавляем новую точку в конец (можно изменить вставку)
            self.points.append((event.x, event.y))
            self.drag_index = len(self.points) - 1
            self.redraw()

    def on_left_drag(self, event):
        if self.drag_index is not None:
            # двигаем точку
            self.points[self.drag_index] = (event.x, event.y)
            self.redraw()

    def on_left_up(self, event):
        self.drag_index = None

    def on_right_click(self, event):
        idx = self.find_point(event.x, event.y)
        if idx is not None:
            # удалить точку
            del self.points[idx]
            self.redraw()

    def on_key(self, event):
        key = event.keysym.lower()
        if key == 'c':
            self.points.clear()
            self.redraw()
        elif key == 'h':
            self.show_help = not self.show_help
            self.redraw()
        elif key in ('q', 'escape'):
            self.root.quit()

    # ----- вспомогательные -----
    def find_point(self, x, y):
        """Найти индекс точки в радиусе SELECT_RADIUS, иначе None."""
        for i, (px, py) in enumerate(self.points):
            if hypot(px - x, py - y) <= SELECT_RADIUS:
                return i
        return None

    @staticmethod
    def cubic_bezier_point(P0, P1, P2, P3, t):
        """Возвращает точку кубического Безье для параметра t in [0,1]."""
        u = 1 - t
        u3 = u*u*u
        u2t = 3 * (u*u) * t
        ut2 = 3 * u * (t*t)
        t3 = t*t*t
        x = u3 * P0[0] + u2t * P1[0] + ut2 * P2[0] + t3 * P3[0]
        y = u3 * P0[1] + u2t * P1[1] + ut2 * P2[1] + t3 * P3[1]
        return (x, y)

    # ----- отрисовка -----
    def redraw(self):
        c = self.canvas
        c.delete("all")

        # подсказка/инструкция
        if self.show_help:
            help_text = (
                "ЛКМ добавить/взять+перетащить точку · ПКМ удалить точку · C очистить · H показать/скрыть помощь · Q выход\n"
                "Сегменты: P0..P3, P3..P6, P6..P9, ... (нужны группы по 3k+1)."
            )
            c.create_text(10, 10, anchor="nw", text=help_text, fill="black", font=("Arial", 11))

        # отрисовать контрольный многоугольник
        if len(self.points) >= 2:
            for i in range(len(self.points)-1):
                x1,y1 = self.points[i]
                x2,y2 = self.points[i+1]
                c.create_line(x1, y1, x2, y2, fill="#999999", dash=(4,4))

        # отрисовать сами точки
        for i, (x, y) in enumerate(self.points):
            r = POINT_RADIUS
            fill = "#1f77b4"  # синий
            outline = "black"
            c.create_oval(x-r, y-r, x+r, y+r, fill=fill, outline=outline)
            # индекс точки для удобства
            c.create_text(x + 10, y - 10, text=str(i), anchor="w", font=("Arial", 9), fill="#333333")

        # отрисовать кривые Безье для каждого полного сегмента (шаг 3)
        n = len(self.points)
        seg_count = 0
        for i in range(0, n-1, 3):
            if i + 3 < n:
                P0 = self.points[i]
                P1 = self.points[i+1]
                P2 = self.points[i+2]
                P3 = self.points[i+3]

                # рисуем вспомогательные ручки (линии от P0->P1, P2->P3)
                c.create_line(P0[0], P0[1], P1[0], P1[1], fill="#cccccc", dash=(2,2))
                c.create_line(P2[0], P2[1], P3[0], P3[1], fill="#cccccc", dash=(2,2))

                # вычисляем точки кривой
                pts = []
                for k in range(BEZIER_SAMPLES+1):
                    t = k / BEZIER_SAMPLES
                    pts.append(self.cubic_bezier_point(P0, P1, P2, P3, t))

                # соединяем отрезками
                for a in range(len(pts)-1):
                    x1,y1 = pts[a]
                    x2,y2 = pts[a+1]
                    c.create_line(x1, y1, x2, y2, width=2, fill="#d62728")  # красная кривая
                seg_count += 1

        # если есть несформированный хвост (меньше 4 точек в конце) — подсветим его серым
        tail_start = (seg_count * 3)
        if tail_start < n:
            # нарисовать оставшиеся точки (если >1) соединёнными пунктиром
            if tail_start < n - 1:
                for i in range(tail_start, n-1):
                    x1,y1 = self.points[i]
                    x2,y2 = self.points[i+1]
                    c.create_line(x1, y1, x2, y2, fill="#e0e0e0", dash=(4,4))
            # подпись о том, что не хватает точек
            missing = 0
            if n - tail_start < 4:
                missing = 4 - (n - tail_start)
            if missing > 0:
                text = f"Ожидается ещё {missing} точек для завершения следующего сегмента (нужны 4)."
                c.create_text(10, CANVAS_H-10, anchor="sw", text=text, fill="#333333", font=("Arial", 11))

        # рамка состояния (количество точек, сегменты)
        status = f"Точек: {n}    Сегментов: {seg_count}"
        c.create_text(CANVAS_W-10, CANVAS_H-10, anchor="se", text=status, fill="#333333", font=("Arial", 11))


def main():
    root = tk.Tk()
    app = BezierEditor(root)
    root.mainloop()

if __name__ == "__main__":
    main()