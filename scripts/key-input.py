# importing all the required libraries
import pygame
from pygame.locals import *
from sys import exit
 
# initiating pygame library to use it's 
# functions
pygame.init()
 
# declaring windows/surface width and height
size = width, height = 740, 480
screen = pygame.display.set_mode(size)
 
# loads a new image from a file and convert()
# will create a copy of image on surface
img = pygame.image.load("char.png").convert()
 
# declaring value to variables
x, y = 0, 0
move_x, move_y = 0, 0
 
while True:
    print("start of event loop")
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            # pygame.QUIT deactivates pygame
            exit()
            # exit() is sys function used to 
            # kill the program
             
     # KEYDOWN event will be triggered everytime 
    # we press a button
    if event.type == KEYDOWN:  
       
        if event.key == K_LEFT:  
            move_x = -0.3  # object moves -0.3 towards x axis
            print("pressed LEFT")
        elif event.key == K_RIGHT:  
            move_x = +0.3  # object moves 0.3 towards x axis
            print("pressed RIGHT")
        elif event.key == K_UP: 
            move_y = -0.3  # object moves -0.3 towards y axis
            print("pressed UP")
        elif event.key == K_DOWN: 
            move_y = +0.3  # object moves 0.3 towards y axis
            print("pressed DOWN")
             
        # K_LCTRL event will be triggered everytime we
        # press left CTRL button
        elif event.key == K_LCTRL:
           
            # declaring new image file to update image
            # everytime left CTRL is pressed
            img = pygame.image.load("char1.png")
            pygame.display.update()  # update image
        elif event.key == K_BACKSPACE:
           
            # this the default file we declared in start 
            # and it will restore it everytime we press
            # backspace
            img = pygame.image.load("char.png")
            pygame.display.update()  # update image
             
     # it will get triggered when left key is released
    if event.type == KEYUP:
        if event.key == K_LEFT:
            move_x = 0  # movement stops
        elif event.key == K_RIGHT:  
            move_x = 0  # movement stops
        elif event.key == K_UP: 
            move_y = 0  # movement stops
        elif event.key == K_DOWN:  
            move_y = 0  # movement stops
            """KEYUP event will be triggered when the release the keys 
            and x,y coordinates will not change anymore"""
 
    x += move_x
    y += move_y
    # updating coordinate values of x,y
    screen.fill((255, 255, 255))
     
    # the function will fill the background with white color
    screen.blit(img, (x, y))
     
    # blit() function will copy image file to x,y coordinates.
    pygame.display.update()
    # draw the objects on screen
