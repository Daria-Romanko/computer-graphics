import pygame
import sys

white = (255, 255, 255)
black = (0, 0, 0)


class Polygon:
    def __init__(self):
        self.points = []
        
    # добавляем точку и ребро
    def add_point(self, pos):
        self.points.append(pos)
        pygame.draw.circle(screen, black, pos, 5)
        if len(self.points) >= 2:
            pygame.draw.line(screen, black, self.points[-2], pos, 1)
        pygame.display.flip()

    # чтобы закончить рисовать полигон, соединяем последнюю точку с первой
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

# здесь выполняем все задания. В зависимости от того, на какую клавишу или кнопку мыши нажали, определяем, что конкретно нужно делать
def tasks():
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return
            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == 1: # выбрать полигон
                    # определяем, какому полигону из polygons принадлежит точка event.pos
                    # и называем этот полигон p

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
    


screen = create_board()
polygons = create_Polygons()

tasks()

pygame.quit()
sys.exit()
