# -*- coding: utf-8 -*-
import pygame
import random

# 初始化pygame
pygame.init()

# 屏幕设置
WIDTH, HEIGHT = 800, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption(u"打砖块")

# 颜色定义
WHITE = (255, 255, 255)
RED = (255, 0, 0)
BLUE = (0, 0, 255)
YELLOW = (255, 255, 0)

# 挡板设置
paddle_width = 100
paddle_height = 20
paddle_x = (WIDTH - paddle_width) // 2
paddle_y = HEIGHT - 50

# 球设置
ball_radius = 10
ball_x = WIDTH // 2
ball_y = HEIGHT // 2
ball_dx = 5 * random.choice([-1, 1])
ball_dy = -5

# 砖块设置
brick_width = 75
brick_height = 30
bricks = []
for i in range(5):
    for j in range(10):
        bricks.append(pygame.Rect(j * (brick_width + 5) + 30, 
                                i * (brick_height + 5) + 50, 
                                brick_width, brick_height))

# 游戏循环
clock = pygame.time.Clock()
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    # 移动挡板
    keys = pygame.key.get_pressed()
    if keys[pygame.K_LEFT] and paddle_x > 0:
        paddle_x -= 8
    if keys[pygame.K_RIGHT] and paddle_x < WIDTH - paddle_width:
        paddle_x += 8

    # 移动球
    ball_x += ball_dx
    ball_y += ball_dy

    # 球与墙碰撞
    if ball_x - ball_radius < 0 or ball_x + ball_radius > WIDTH:
        ball_dx *= -1
    if ball_y - ball_radius < 0:
        ball_dy *= -1

    # 球与挡板碰撞
    if (ball_y + ball_radius > paddle_y and 
        paddle_x < ball_x < paddle_x + paddle_width):
        ball_dy *= -1

    # 球与砖块碰撞
    for brick in bricks[:]:
        if brick.collidepoint(ball_x, ball_y):
            bricks.remove(brick)
            ball_dy *= -1
            break

    # 游戏结束条件
    if ball_y + ball_radius > HEIGHT:
        running = False

    # 绘制
    screen.fill((0, 0, 0))
    pygame.draw.rect(screen, BLUE, (paddle_x, paddle_y, paddle_width, paddle_height))
    pygame.draw.circle(screen, RED, (ball_x, ball_y), ball_radius)
    for brick in bricks:
        pygame.draw.rect(screen, YELLOW, brick)

    pygame.display.flip()
    clock.tick(60)

pygame.quit()
