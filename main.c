#include <GL/glut.h>
#include <math.h>
#include <stdio.h>

#define res 1                  // 0=160x120 1=360x240 4=640x480
#define SW 160 * res           // screen width
#define SH 120 * res           // screen height
#define SW2 (SW / 2)           // half of screen width
#define SH2 (SH / 2)           // half of screen height
#define pixelScale 4 / res     // OpenGL pixel scale
#define GLSW (SW * pixelScale) // OpenGL window width
#define GLSH (SH * pixelScale) // OpenGL window height
#define numSect 4
#define numWall 16
//------------------------------------------------------------------------------
typedef struct {
  int fr1, fr2; // frame 1 frame 2, to create constant frame rate
} time;
time T;

typedef struct {
  int w, s, a, d; // move up, down, left, right
  int sl, sr;     // strafe left, right
  int m;          // move up, down, look up, down
} keys;
keys K;

typedef struct {
  float cos[360];
  float sin[360];
} math;
math M;

typedef struct {
  int x, y, z;
  int a;
  int l;
} player;
player P;

typedef struct {
  int x1, y1;
  int x2, y2;
  int c;
} walls;
walls Walls[30];

typedef struct {
  int wS, wE;
  int z1, z2;
  int x, y;
  int d;
  int c1, c2;
  int surf[SW];
  int surface;
} sectors;
sectors Sectors[30];

//------------------------------------------------------------------------------

void drawPixel(int x, int y, int r, int g,
               int b) // draw a pixel at x/y with rgb
{
  glColor3ub(r, g, b);
  glBegin(GL_POINTS);
  glVertex2i(x * pixelScale + 2, y * pixelScale + 2);
  glEnd();
}

void movePlayer() {
  // move up, down, left, right
  if (K.a == 1 && K.m == 0) {
    P.a -= 4;
    if (P.a < 0) {
      P.a += 360;
    }
  }
  if (K.d == 1 && K.m == 0) {
    P.a += 4;
    if (P.a > 359) {
      P.a -= 360;
    }
  }
  int dX = M.sin[P.a] * 10;
  int dY = M.cos[P.a] * 10;

  if (K.w == 1 && K.m == 0) {
    P.x += dX;
    P.y += dY;
  }
  if (K.s == 1 && K.m == 0) {
    P.x -= dX;
    P.y -= dY;
  }
  // strafe left, right
  if (K.sr == 1) {
    P.x += dX;
    P.y -= dY;
  }
  if (K.sl == 1) {
    P.x -= dX;
    P.y += dY;
  }
  // move up, down, look up, look down
  if (K.a == 1 && K.m == 1) {
    P.l -= 1;
  }
  if (K.d == 1 && K.m == 1) {
    P.l += 1;
  }
  if (K.w == 1 && K.m == 1) {
    P.z -= 4;
  }
  if (K.s == 1 && K.m == 1) {
    P.z += 4;
  }
}

void clearBackground() {
  int x, y;
  for (y = 0; y < SH; y++) {
    for (x = 0; x < SW; x++) {
      drawPixel(x, y, 0, 60, 130);
    } // clear background color
  }
}

void clipBehindPlayer(int *x1, int *y1, int *z1, int x2, int y2, int z2) {
  float dA = *y1;
  float dB = y2;
  float d = dA - dB;
  if (d == 0) {
    d = 1;
  }
  float s = dA / (dA - dB);
  *x1 = *x1 + s * (x2 - (*x1));
  *y1 = *y1 + s * (y2 - (*y1));
  if (*y1 == 0) {
    *y1 = 1;
  }
  *z1 = *z1 + s * (z2 - (*z1));
}

void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int c, int s) {
  int x, y;
  int dYb = b2 - b1;
  int dYt = t2 - t1;
  int dX = x2 - x1;
  if (dX == 0) {
    dX = 1;
  }
  int xS = x1;

  // X Clip
  if (x1 < 1) {
    x1 = 1;
  }
  if (x2 < 1) {
    x2 = 1;
  }
  if (x1 > SW - 1) {
    x1 = SW - 1;
  }
  if (x2 > SW - 1) {
    x2 = SW - 1;
  }

  for (x = x1; x < x2; x++) {
    int y1 = dYb * (x - xS + 0.5) / dX + b1;
    int y2 = dYt * (x - xS + 0.5) / dX + t1;
    // Y Clip
    if (y1 < 1) {
      y1 = 1;
    }
    if (y2 < 1) {
      y2 = 1;
    }
    if (y1 > SH - 1) {
      y1 = SH - 1;
    }
    if (y2 > SH - 1) {
      y2 = SH - 1;
    }

    if (Sectors[s].surface == 1) {
      Sectors[s].surf[x] = y1;
      continue;
    }

    if (Sectors[s].surface == 2) {
      Sectors[s].surf[x] = y2;
      continue;
    }

    if (Sectors[s].surface == -1) {
      for (y = Sectors[s].surf[x]; y < y1; y++) {
        drawPixel(x, y, 255, 0, 0);
      }
    }

    if (Sectors[s].surface == -2) {
      for (y = y2; y < Sectors[s].surf[x]; y++) {
        drawPixel(x, y, 255, 0, 0);
      }
    }

    for (y = y1; y < y2; y++) {
      drawPixel(x, y, 0, 255, 255);
    }
  }
}

int dist(int x1, int y1, int x2, int y2) {
  int distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
  return distance;
}

void draw3D() {
  int s, w, loop;
  int wX[4], wY[4], wZ[4];
  float CS = M.cos[P.a], SN = M.sin[P.a];

  // Sort Walls by distance
  for (s = 0; s < numSect - 1; s++) {
    for (w = 0; w < numSect - s - 1; w++) {
      if (Sectors[w].d < Sectors[w + 1].d) {
        sectors st = Sectors[w];
        Sectors[w] = Sectors[w + 1];
        Sectors[w + 1] = st;
      }
    }
  }

  for (s = 0; s < numSect; s++) {
    Sectors[s].d = 0;

    if (P.z < Sectors[s].z1) {
      Sectors[s].surface = 1;
    } else if (P.z > Sectors[s].z2) {
      Sectors[s].surface = 2;
    } else {
      Sectors[s].surface = 0;
    }
    for (loop = 0; loop < 2; loop++) {

      for (w = Sectors[s].wS; w < Sectors[s].wE; w++) {

        int x1 = Walls[w].x1 - P.x, y1 = Walls[w].y1 - P.y;
        int x2 = Walls[w].x2 - P.x, y2 = Walls[w].y2 - P.y;

        if (loop == 0) {
          int swp = x1;
          x1 = x2;
          x2 = swp;
          swp = y1;
          y1 = y2;
          y2 = swp;
        }

        wX[0] = x1 * CS - y1 * SN;
        wX[1] = x2 * CS - y2 * SN;
        wX[2] = wX[0];
        wX[3] = wX[1];

        wY[0] = y1 * CS + x1 * SN;
        wY[1] = y2 * CS + x2 * SN;
        wY[2] = wY[0];
        wY[3] = wY[1];

        Sectors[s].d += dist(0, 0, (wX[0] + wX[1]) / 2, (wY[0] + wY[1]) / 2);

        wZ[0] = Sectors[s].z1 - P.z + ((P.l * wY[0]) / 32.0);
        wZ[1] = Sectors[s].z1 - P.z + ((P.l * wY[1]) / 32.0);
        wZ[2] = wZ[0] + Sectors[s].z2;
        wZ[3] = wZ[1] + Sectors[s].z2;

        if (wY[0] < 1 && wY[1] < 1) {
          continue;
        }

        if (wY[0] < 1) {
          clipBehindPlayer(&wX[0], &wY[0], &wZ[0], wX[1], wY[1], wZ[1]);
          clipBehindPlayer(&wX[2], &wY[2], &wZ[2], wX[3], wY[3], wZ[3]);
        }

        if (wY[1] < 1) {
          clipBehindPlayer(&wX[1], &wY[1], &wZ[1], wX[0], wY[0], wZ[0]);
          clipBehindPlayer(&wX[3], &wY[3], &wZ[3], wX[2], wY[2], wZ[2]);
        }

        wX[0] = wX[0] * 200 / wY[0] + SW2;
        wY[0] = wZ[0] * 200 / wY[0] + SW2;

        wX[1] = wX[1] * 200 / wY[1] + SW2;
        wY[1] = wZ[1] * 200 / wY[1] + SW2;

        wX[2] = wX[2] * 200 / wY[2] + SW2;
        wY[2] = wZ[2] * 200 / wY[2] + SW2;

        wX[3] = wX[3] * 200 / wY[3] + SW2;
        wY[3] = wZ[3] * 200 / wY[3] + SW2;

        drawWall(wX[0], wX[1], wY[0], wY[1], wY[2], wY[3], Walls[w].c, s);
      }
      Sectors[s].d /= (Sectors[s].wE - Sectors[s].wS);
      Sectors[s].surface *= -1;
    }
  }
}

void display() {
  int x, y;
  if (T.fr1 - T.fr2 >= 50) // only draw 20 frames/second
  {
    clearBackground();
    movePlayer();
    draw3D();

    T.fr2 = T.fr1;
    glutSwapBuffers();
    glutReshapeWindow(GLSW, GLSH); // prevent window scaling
  }

  T.fr1 = glutGet(GLUT_ELAPSED_TIME); // 1000 Milliseconds per second
  glutPostRedisplay();
}

void KeysDown(unsigned char key, int x, int y) {
  if (key == 'w' == 1) {
    K.w = 1;
  }
  if (key == 's' == 1) {
    K.s = 1;
  }
  if (key == 'a' == 1) {
    K.a = 1;
  }
  if (key == 'd' == 1) {
    K.d = 1;
  }
  if (key == 'm' == 1) {
    K.m = 1;
  }
  if (key == ',' == 1) {
    K.sr = 1;
  }
  if (key == '.' == 1) {
    K.sl = 1;
  }
}
void KeysUp(unsigned char key, int x, int y) {
  if (key == 'w' == 1) {
    K.w = 0;
  }
  if (key == 's' == 1) {
    K.s = 0;
  }
  if (key == 'a' == 1) {
    K.a = 0;
  }
  if (key == 'd' == 1) {
    K.d = 0;
  }
  if (key == 'm' == 1) {
    K.m = 0;
  }
  if (key == ',' == 1) {
    K.sr = 0;
  }
  if (key == '.' == 1) {
    K.sl = 0;
  }
}

int loadSectors[] = {
    // wall start, wall end, z1 height, z2 height, bottom color, top color
    0,  4,  0, 40, 2, 3, // sector 1
    4,  8,  0, 40, 4, 5, // sector 2
    8,  12, 0, 40, 6, 7, // sector 3
    12, 16, 0, 40, 0, 1, // sector 4
};
int loadWalls[] = {
    // x1,y1, x2,y2, color
    0,  0,  32, 0,  0, 32, 0,  32, 32, 1, 32, 32, 0,  32, 0, 0,  32, 0,  0,  1,

    64, 0,  96, 0,  2, 96, 0,  96, 32, 3, 96, 32, 64, 32, 2, 64, 32, 64, 0,  3,

    64, 64, 96, 64, 4, 96, 64, 96, 96, 5, 96, 96, 64, 96, 4, 64, 96, 64, 64, 5,

    0,  64, 32, 64, 6, 32, 64, 32, 96, 7, 32, 96, 0,  96, 6, 0,  96, 0,  64, 7,
};
void init() {
  int x;
  for (x = 0; x < 360; x++) {
    M.cos[x] = cos(x / 180.0 * M_PI);
    M.sin[x] = sin(x / 180.0 * M_PI);
  }

  P.x = 70;
  P.y = -110;
  P.z = 20;
  P.a = 0;
  P.l = 0;

  int s, w, v1 = 0, v2 = 0;
  for (s = 0; s < numSect; s++) {
    Sectors[s].wS = loadSectors[v1 + 0];
    Sectors[s].wE = loadSectors[v1 + 1];
    Sectors[s].z1 = loadSectors[v1 + 2];
    Sectors[s].z2 = loadSectors[v1 + 3] - loadSectors[v1 + 2];
    Sectors[s].c1 = loadSectors[v1 + 4];
    Sectors[s].c2 = loadSectors[v1 + 5];
    v1 += 6;
    for (w = Sectors[s].wS; w < Sectors[s].wE; w++) {
      Walls[w].x1 = loadWalls[v2 + 0];
      Walls[w].y1 = loadWalls[v2 + 1];
      Walls[w].x2 = loadWalls[v2 + 2];
      Walls[w].y2 = loadWalls[v2 + 3];
      Walls[w].c = loadWalls[v2 + 4];
      v2 += 5;
    }
  }
}

int main(int argc, char *argv[]) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowPosition(GLSW / 2, GLSH / 2);
  glutInitWindowSize(GLSW, GLSH);
  glutCreateWindow("");
  glPointSize(pixelScale);      // pixel size
  gluOrtho2D(0, GLSW, 0, GLSH); // origin bottom left
  init();
  glutDisplayFunc(display);
  glutKeyboardFunc(KeysDown);
  glutKeyboardUpFunc(KeysUp);
  glutMainLoop();
  return 0;
}
