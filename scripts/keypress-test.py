import pygame
pygame.init()
screen = pygame.display.set_mode((400, 300))
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        if event.type == pygame.KEYDOWN:
            print(f"Key pressed: {event.key}")
            if event.key == pygame.K_MINUS:
                print("Minus key detected")
            if event.key == pygame.K_UNDERSCORE:
                print("Underscore key detected")
            if event.key == pygame.K_KP_MINUS:
                print("Keypad minus detected")
pygame.quit()