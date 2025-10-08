/*
  Trabalho 1 - OpenGL (Versão Melhorada)
  Esfera com movimento 3D aleatório, iluminação dinâmica, 
  câmera interativa, rastro visual e múltiplos modos.
  
  Compilar (Linux):
    gcc trabalho1_opengl.c -o trabalho1_opengl -lGL -lGLU -lglut -lm
  Compilar (Windows, MinGW):
    gcc trabalho1_opengl.c -o trabalho1_opengl -lopengl32 -lglu32 -lfreeglut -lm
*/

#include <GL/freeglut.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

// ==================== CONSTANTES ====================
#define MAX_TRAIL 100
#define PI 3.14159265359f

// ==================== ESTRUTURAS ====================
typedef struct {
    float pos[3];
    float alpha;
} TrailPoint;

// ==================== VARIÁVEIS GLOBAIS ====================
static float spherePos[3] = {0.0f, 0.0f, 0.0f};
static float sphereDir[3];
static float speed = 0.015f;
static float sphereColor[3] = {0.12f, 0.56f, 1.0f};
static float boxLimit = 0.72f;

static TrailPoint trail[MAX_TRAIL];
static int trailIndex = 0;
static int trailEnabled = 1;
static int trailCounter = 0;

static int paused = 0;
static int showStats = 1;
static int wireframeMode = 0;

static float cameraDistance = 3.5f;
static float cameraAngleX = 0.0f;
static float cameraAngleY = 0.0f;
static int cameraMode = 0;

static int lightingEnabled = 1;
static float lightRotation = 0.0f;

static int bounceCount = 0;
static float distanceTraveled = 0.0f;

// ==================== PROTÓTIPOS ====================
void setRandomDirection();
void addTrailPoint();
void updateTrail();
void drawTrail();
void drawText(float x, float y, const char* text);
void drawHUD();
void initGL();
void redesenha(void);
void idleFunc(void);
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);

// ==================== IMPLEMENTAÇÕES ====================

void setRandomDirection() {
    sphereDir[0] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    sphereDir[1] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    sphereDir[2] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

    float len = sqrtf(sphereDir[0]*sphereDir[0] + 
                      sphereDir[1]*sphereDir[1] + 
                      sphereDir[2]*sphereDir[2]);
    if (len > 0.001f) {
        sphereDir[0] /= len;
        sphereDir[1] /= len;
        sphereDir[2] /= len;
    }
}

void addTrailPoint() {
    trail[trailIndex].pos[0] = spherePos[0];
    trail[trailIndex].pos[1] = spherePos[1];
    trail[trailIndex].pos[2] = spherePos[2];
    trail[trailIndex].alpha = 1.0f;
    trailIndex = (trailIndex + 1) % MAX_TRAIL;
}

void updateTrail() {
    int i;
    for (i = 0; i < MAX_TRAIL; i++) {
        trail[i].alpha *= 0.98f;
    }
}

void drawTrail() {
    int i;
    if (!trailEnabled) return;
    
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (i = 0; i < MAX_TRAIL; i++) {
        if (trail[i].alpha > 0.01f) {
            glPushMatrix();
            glTranslatef(trail[i].pos[0], trail[i].pos[1], trail[i].pos[2]);
            glColor4f(sphereColor[0], sphereColor[1], sphereColor[2], trail[i].alpha * 0.5f);
            glutSolidSphere(0.04, 16, 8);
            glPopMatrix();
        }
    }
    
    glDisable(GL_BLEND);
    if (lightingEnabled) glEnable(GL_LIGHTING);
}

void drawText(float x, float y, const char* text) {
    glDisable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 700, 0, 600);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(x, y);
    
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *text);
        text++;
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    if (lightingEnabled) glEnable(GL_LIGHTING);
}

void drawHUD() {
    char buffer[128];
    float y = 570.0f;
    
    if (!showStats) return;
    
    sprintf(buffer, "FPS: ~60 | Colisoes: %d | Distancia: %.1f", bounceCount, distanceTraveled);
    drawText(10, y, buffer); y -= 20;
    
    sprintf(buffer, "Velocidade: %.3f | Camera: %s", speed, 
            cameraMode == 0 ? "Estatica" : (cameraMode == 1 ? "Auto-Rotacao" : "Seguir"));
    drawText(10, y, buffer); y -= 20;
    
    sprintf(buffer, "Rastro: %s | Iluminacao: %s | Wireframe: %s", 
            trailEnabled ? "ON" : "OFF",
            lightingEnabled ? "ON" : "OFF",
            wireframeMode ? "ON" : "OFF");
    drawText(10, y, buffer);
    
    drawText(10, 40, "ESPACO=Pausa | R=Nova Dir | T=Rastro | L=Luz | W=Wire");
    drawText(10, 20, "C=Camera | +/-=Veloc | H=HUD | Setas=Camera | ESC=Sair");
}

void initGL() {
    GLfloat lightAmbient[] = {0.2f, 0.2f, 0.3f, 1.0f};
    GLfloat lightDiffuse[] = {0.8f, 0.8f, 0.9f, 1.0f};
    GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    int i;
    
    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);
    
    for (i = 0; i < MAX_TRAIL; i++) {
        trail[i].alpha = 0.0f;
    }
}

void redesenha(void) {
    float camX, camY, camZ;
    float autoAngle;
    GLfloat lightPos[4];
    float i;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    if (cameraMode == 0) {
        camX = cameraDistance * sin(cameraAngleY) * cos(cameraAngleX);
        camY = cameraDistance * sin(cameraAngleX);
        camZ = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);
    } else if (cameraMode == 1) {
        autoAngle = glutGet(GLUT_ELAPSED_TIME) * 0.0003f;
        camX = cameraDistance * sin(autoAngle);
        camY = 0.5f;
        camZ = cameraDistance * cos(autoAngle);
    } else {
        camX = spherePos[0] + 2.0f;
        camY = spherePos[1] + 1.5f;
        camZ = spherePos[2] + 2.0f;
    }
    
    gluLookAt(camX, camY, camZ, 
              cameraMode == 2 ? spherePos[0] : 0.0f,
              cameraMode == 2 ? spherePos[1] : 0.0f,
              cameraMode == 2 ? spherePos[2] : 0.0f,
              0.0f, 1.0f, 0.0f);
    
    if (lightingEnabled) {
        lightRotation += 0.5f;
        lightPos[0] = 2.5f * cos(lightRotation * 0.01f);
        lightPos[1] = 2.0f;
        lightPos[2] = 2.5f * sin(lightRotation * 0.01f);
        lightPos[3] = 1.0f;
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    }
    
    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 0.25f, 0.3f);
    glBegin(GL_LINES);
    for (i = -1.0f; i <= 1.0f; i += 0.2f) {
        glVertex3f(i, -0.8f, -0.8f);
        glVertex3f(i, -0.8f, 0.8f);
        glVertex3f(-0.8f, -0.8f, i);
        glVertex3f(0.8f, -0.8f, i);
    }
    glEnd();
    if (lightingEnabled) glEnable(GL_LIGHTING);
    
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.7f, 0.8f, 0.9f);
    if (wireframeMode) {
        glutWireCube(1.6);
    } else {
        glLineWidth(2.0f);
        glutWireCube(1.6);
        glLineWidth(1.0f);
    }
    if (lightingEnabled) glEnable(GL_LIGHTING);
    glPopMatrix();
    
    drawTrail();
    
    glPushMatrix();
    glTranslatef(spherePos[0], spherePos[1], spherePos[2]);
    glColor3f(sphereColor[0], sphereColor[1], sphereColor[2]);
    
    if (wireframeMode) {
        glDisable(GL_LIGHTING);
        glutWireSphere(0.08, 32, 16);
        if (lightingEnabled) glEnable(GL_LIGHTING);
    } else {
        glutSolidSphere(0.08, 32, 16);
        glDisable(GL_LIGHTING);
        glColor3f(0.0f, 0.0f, 0.0f);
        glutWireSphere(0.081, 10, 6);
        if (lightingEnabled) glEnable(GL_LIGHTING);
    }
    glPopMatrix();
    
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(spherePos[0], spherePos[1], spherePos[2]);
    glVertex3f(spherePos[0] + sphereDir[0] * 0.3f,
               spherePos[1] + sphereDir[1] * 0.3f,
               spherePos[2] + sphereDir[2] * 0.3f);
    glEnd();
    if (lightingEnabled) glEnable(GL_LIGHTING);
    
    drawHUD();
    
    glutSwapBuffers();
}

void idleFunc(void) {
    float oldPos[3];
    float dx, dy, dz;
    int bounced;
    
    if (!paused) {
        oldPos[0] = spherePos[0];
        oldPos[1] = spherePos[1];
        oldPos[2] = spherePos[2];
        
        spherePos[0] += sphereDir[0] * speed;
        spherePos[1] += sphereDir[1] * speed;
        spherePos[2] += sphereDir[2] * speed;
        
        dx = spherePos[0] - oldPos[0];
        dy = spherePos[1] - oldPos[1];
        dz = spherePos[2] - oldPos[2];
        distanceTraveled += sqrtf(dx*dx + dy*dy + dz*dz);
        
        bounced = 0;
        if (spherePos[0] > boxLimit || spherePos[0] < -boxLimit) {
            sphereDir[0] = -sphereDir[0];
            spherePos[0] = spherePos[0] > 0 ? boxLimit : -boxLimit;
            bounced = 1;
        }
        if (spherePos[1] > boxLimit || spherePos[1] < -boxLimit) {
            sphereDir[1] = -sphereDir[1];
            spherePos[1] = spherePos[1] > 0 ? boxLimit : -boxLimit;
            bounced = 1;
        }
        if (spherePos[2] > boxLimit || spherePos[2] < -boxLimit) {
            sphereDir[2] = -sphereDir[2];
            spherePos[2] = spherePos[2] > 0 ? boxLimit : -boxLimit;
            bounced = 1;
        }
        
        if (bounced) {
            bounceCount++;
            sphereColor[0] = 0.12f + (bounceCount % 10) * 0.05f;
            sphereColor[2] = 1.0f - (bounceCount % 10) * 0.05f;
        }
        
        trailCounter++;
        if (trailCounter >= 2) {
            addTrailPoint();
            trailCounter = 0;
        }
        updateTrail();
        
        glutPostRedisplay();
    }
}

void reshape(int w, int h) {
    float aspect;
    if (h == 0) h = 1;
    aspect = (float)w / (float)h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, aspect, 0.1, 100.0);
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    
    switch(key) {
        case 27:
            printf("\n=== ESTATISTICAS FINAIS ===\n");
            printf("Colisoes: %d\n", bounceCount);
            printf("Distancia: %.2f unidades\n", distanceTraveled);
            glutDestroyWindow(glutGetWindow());
            exit(0);
            break;
            
        case ' ':
            paused = !paused;
            break;
            
        case 'r': case 'R':
            setRandomDirection();
            printf("Nova direcao aleatoria gerada!\n");
            break;
            
        case 't': case 'T':
            trailEnabled = !trailEnabled;
            break;
            
        case 'l': case 'L':
            lightingEnabled = !lightingEnabled;
            if (lightingEnabled) glEnable(GL_LIGHTING);
            else glDisable(GL_LIGHTING);
            break;
            
        case 'w': case 'W':
            wireframeMode = !wireframeMode;
            break;
            
        case 'c': case 'C':
            cameraMode = (cameraMode + 1) % 3;
            break;
            
        case 'h': case 'H':
            showStats = !showStats;
            break;
            
        case '+': case '=':
            speed = fminf(speed + 0.005f, 0.1f);
            break;
            
        case '-': case '_':
            speed = fmaxf(speed - 0.005f, 0.001f);
            break;
            
        case '0':
            spherePos[0] = spherePos[1] = spherePos[2] = 0.0f;
            bounceCount = 0;
            distanceTraveled = 0.0f;
            setRandomDirection();
            printf("Sistema resetado!\n");
            break;
    }
    
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    (void)x; (void)y;
    
    if (cameraMode == 0) {
        switch(key) {
            case GLUT_KEY_UP:
                cameraAngleX += 0.1f;
                break;
            case GLUT_KEY_DOWN:
                cameraAngleX -= 0.1f;
                break;
            case GLUT_KEY_LEFT:
                cameraAngleY -= 0.1f;
                break;
            case GLUT_KEY_RIGHT:
                cameraAngleY += 0.1f;
                break;
        }
        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    srand(time(NULL));
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(700, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Trabalho 1 - OpenGL 3D (Versao Melhorada)");
    
    initGL();
    setRandomDirection();
    
    printf("\n=== CONTROLES ===\n");
    printf("ESPACO    - Pausar/Retomar\n");
    printf("R         - Nova direcao aleatoria\n");
    printf("T         - Ligar/Desligar rastro\n");
    printf("L         - Ligar/Desligar iluminacao\n");
    printf("W         - Modo wireframe\n");
    printf("C         - Alternar modo de camera\n");
    printf("H         - Mostrar/Ocultar HUD\n");
    printf("+/-       - Ajustar velocidade\n");
    printf("SETAS     - Controlar camera (modo estatico)\n");
    printf("0         - Reset completo\n");
    printf("ESC       - Sair\n");
    printf("=================\n\n");
    
    glutDisplayFunc(redesenha);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutIdleFunc(idleFunc);
    
    glutMainLoop();
    
    return 0;
}
