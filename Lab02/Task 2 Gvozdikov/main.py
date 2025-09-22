import cv2
import matplotlib.pyplot as plt

image = cv2.imread('dog.webp')

image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

R, G, B = image_rgb[:,:,0], image_rgb[:,:,1], image_rgb[:,:,2]

plt.figure(figsize=(12,4))

plt.subplot(1,3,1)
plt.imshow(R, cmap='Reds')
plt.title('Красный канал')
plt.axis('off')

plt.subplot(1,3,2)
plt.imshow(G, cmap='Greens')
plt.title('Зеленый канал')
plt.axis('off')

plt.subplot(1,3,3)
plt.imshow(B, cmap='Blues')
plt.title('Синий канал')
plt.axis('off')

plt.show()

colors = ('r','g','b')
channels = (R,G,B)

plt.figure(figsize=(8,5))
for chan, color in zip(channels, colors):
    hist = cv2.calcHist([chan],[0],None,[256],[0,256])
    plt.plot(hist, color=color)
    plt.xlim([0,256])

plt.title('Гистограммы каналов RGB')
plt.xlabel('Интенсивность')
plt.ylabel('Количество пикселей')
plt.show()
