/*
 * CS2206 - Computer Graphics
 * Project: "A Little Cat in the Rain"
 */

#define _CRT_SECURE_NO_WARNINGS
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

using namespace std;

// ─────────────────────────────────────────
//  Global State & Texture IDs
// ─────────────────────────────────────────
int  currentScene = 1;
bool soundPlaying = false;

GLuint texGround = 0;
GLuint texSun = 0;
GLuint texCloud = 0;

struct Drop { float x, y, speed; };
const int  NUM_DROPS = 80;
Drop drops[NUM_DROPS];

// ── Cat movement state ──
float catX = -0.05f;
float catY = -0.18f;
float catVelY = 0.0f;
bool  catOnGround = true;
int   jumpCount = 0;
const int MAX_JUMPS = 3;
bool  jumpingPhase = true;
float catBobOffset = 0.0f;

// ── Scene 3 animations ──
float smallCatX = 0.05f;
bool  catReachedMom = false;
float motherX = -1.2f;    // Mother starts off-screen from the left

// ── Transformations Variables ──
float umbrellaScale = 1.0f;  // SCALING
float sunAngle = 0.0f;       // ROTATION
float viewZoom = 1.0f;       // ZOOMING & VIEWING

// ─────────────────────────────────────────
//  Zoom Update (Viewing/Clipping)
// ─────────────────────────────────────────
void updateProjection() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Using gluOrtho2D to apply Zooming / Viewing
    gluOrtho2D(-viewZoom, viewZoom, -viewZoom, viewZoom);
    glMatrixMode(GL_MODELVIEW);
}

// ─────────────────────────────────────────
//  Sound Manager
// ─────────────────────────────────────────
void manageSound() {
    if (currentScene == 2 || currentScene == 3) {
        if (!soundPlaying) {
            PlaySound(TEXT("rain.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
            soundPlaying = true;
        }
    }
    else {
        if (soundPlaying) {
            PlaySound(NULL, NULL, 0);
            soundPlaying = false;
        }
    }
}

// ─────────────────────────────────────────
//  Texture Loader
// ─────────────────────────────────────────
GLuint LoadTexture(const char* filename, bool makeWhiteTransparent) {
    GLuint texture = 0;
    FILE* file = fopen(filename, "rb");
    if (!file) {
        cout << "Note: Texture [" << filename << "] not found." << endl;
        return 0;
    }
    unsigned char header[54];
    if (fread(header, 1, 54, file) != 54) { fclose(file); return 0; }
    if (header[0] != 'B' || header[1] != 'M') { fclose(file); return 0; }

    int dataPos = *(int*)&header[0x0A];
    int width = *(int*)&header[0x12];
    int height = *(int*)&header[0x16];
    if (dataPos == 0) dataPos = 54;
    fseek(file, dataPos, SEEK_SET);

    int padding = (4 - (width * 3) % 4) % 4;
    unsigned char* rgbaData = new unsigned char[width * height * 4];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char pixel[3];
            fread(pixel, 1, 3, file);
            int index = (y * width + x) * 4;
            rgbaData[index + 0] = pixel[2];
            rgbaData[index + 1] = pixel[1];
            rgbaData[index + 2] = pixel[0];
            if (makeWhiteTransparent && pixel[0] > 230 && pixel[1] > 230 && pixel[2] > 230)
                rgbaData[index + 3] = 0;
            else
                rgbaData[index + 3] = 255;
        }
        if (padding > 0) { unsigned char pad[3]; fread(pad, 1, padding, file); }
    }
    fclose(file);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);
    delete[] rgbaData;
    return texture;
}

// ─────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────
void drawCircle(float cx, float cy, float r, int segments = 60) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float a = i * 2.0f * 3.14159f / segments;
        glVertex2f(cx + r * cos(a), cy + r * sin(a));
    }
    glEnd();
}

// Added isMother to slightly differentiate the mother's color without making it too dark
void drawCat(float cx, float cy, float scale, bool eyeClosed, bool smile, bool isMother = false) {
    float rM = isMother ? 0.95f : 1.0f;
    float gM = isMother ? 0.90f : 1.0f; // Slight reduction to give a very soft darker shade
    float bM = isMother ? 0.85f : 1.0f;

    // Body (Scaling & Translation)
    glPushMatrix();
    glTranslatef(cx, cy - 0.18f * scale, 0);
    glScalef(scale * 0.28f, scale * 0.22f, 1);
    glColor3f(1.0f * rM, 0.75f * gM, 0.5f * bM);
    drawCircle(0, 0, 1.0f);
    glPopMatrix();

    // Head
    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glScalef(scale, scale, 1);

    glColor3f(1.0f * rM, 0.78f * gM, 0.55f * bM);
    drawCircle(0, 0, 0.22f);

    // Ears outer
    glColor3f(1.0f * rM, 0.6f * gM, 0.3f * bM);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.18f, 0.18f); glVertex2f(-0.08f, 0.18f); glVertex2f(-0.13f, 0.30f);
    glVertex2f(0.08f, 0.18f); glVertex2f(0.18f, 0.18f); glVertex2f(0.13f, 0.30f);
    glEnd();

    // Ears inner
    glColor3f(1.0f * rM, 0.70f * gM, 0.75f * bM);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.165f, 0.195f); glVertex2f(-0.095f, 0.195f); glVertex2f(-0.13f, 0.27f);
    glVertex2f(0.095f, 0.195f); glVertex2f(0.165f, 0.195f); glVertex2f(0.13f, 0.27f);
    glEnd();

    // Eyes
    if (eyeClosed) {
        glColor3f(0.2f, 0.1f, 0.05f);
        glLineWidth(2.5f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 10; i++) { float t = i / 10.0f; glVertex2f(-0.12f + t * 0.07f, 0.02f + 0.03f * sin(t * 3.14f)); }
        glEnd();
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 10; i++) { float t = i / 10.0f; glVertex2f(0.05f + t * 0.07f, 0.02f + 0.03f * sin(t * 3.14f)); }
        glEnd();
    }
    else {
        glColor3f(0.1f, 0.5f, 0.2f); drawCircle(-0.09f, 0.04f, 0.045f); drawCircle(0.09f, 0.04f, 0.045f);
        glColor3f(0.05f, 0.05f, 0.05f); drawCircle(-0.09f, 0.04f, 0.025f); drawCircle(0.09f, 0.04f, 0.025f);
        glColor3f(1, 1, 1); drawCircle(-0.083f, 0.06f, 0.008f); drawCircle(0.097f, 0.06f, 0.008f);
    }

    // Nose
    glColor3f(1.0f, 0.5f, 0.6f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.025f, -0.04f); glVertex2f(0.025f, -0.04f); glVertex2f(0.0f, -0.07f);
    glEnd();

    // Mouth
    glColor3f(0.3f, 0.1f, 0.1f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 10; i++) {
        float t = i / 10.0f;
        if (smile) glVertex2f(-0.05f + t * 0.1f, -0.09f - 0.03f * sin(t * 3.14f));
        else       glVertex2f(-0.05f + t * 0.1f, -0.12f + 0.03f * sin(t * 3.14f));
    }
    glEnd();

    // Whiskers
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xF0F0); glLineWidth(1.5f); glColor3f(0.6f, 0.5f, 0.4f);
    glBegin(GL_LINES);
    glVertex2f(-0.22f, 0.0f);  glVertex2f(-0.06f, -0.03f);
    glVertex2f(-0.22f, -0.04f); glVertex2f(-0.06f, -0.06f);
    glVertex2f(-0.22f, -0.08f); glVertex2f(-0.06f, -0.09f);
    glVertex2f(0.06f, -0.03f); glVertex2f(0.22f, 0.0f);
    glVertex2f(0.06f, -0.06f); glVertex2f(0.22f, -0.04f);
    glVertex2f(0.06f, -0.09f); glVertex2f(0.22f, -0.08f);
    glEnd();
    glDisable(GL_LINE_STIPPLE);

    // Tail
    glColor3f(1.0f * rM, 0.7f * gM, 0.4f * bM); glLineWidth(5.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(0.22f, -0.18f); glVertex2f(0.35f, -0.30f);
    glVertex2f(0.25f, -0.42f); glVertex2f(0.10f, -0.38f);
    glEnd();

    glPopMatrix();
}

void drawRain() {
    glColor4f(0.5f, 0.7f, 1.0f, 0.7f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (int i = 0; i < NUM_DROPS; i++) {
        glVertex2f(drops[i].x, drops[i].y);
        glVertex2f(drops[i].x + 0.01f, drops[i].y - 0.06f);
    }
    glEnd();
}

void drawUmbrella(float cx, float cy, float scale) {
    glPushMatrix(); glTranslatef(cx, cy, 0); glScalef(scale, scale, 1);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.9f, 0.2f, 0.3f, 0.75f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for (int i = 0; i <= 30; i++) { float a = i * 3.14159f / 30; glVertex2f(0.4f * cos(a), 0.25f * sin(a)); }
    glEnd();
    glBegin(GL_TRIANGLES);
    glColor4f(1.0f, 1.0f, 0.0f, 0.5f);  glVertex2f(0, 0);
    glColor4f(1.0f, 0.5f, 0.0f, 0.2f);  glVertex2f(-0.13f, 0.18f);
    glColor4f(1.0f, 1.0f, 0.0f, 0.5f);  glVertex2f(0.13f, 0.18f);
    glEnd();
    glDisable(GL_BLEND);
    glColor3f(0.5f, 0.05f, 0.1f); glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 30; i++) { float a = i * 3.14159f / 30; glVertex2f(0.4f * cos(a), 0.25f * sin(a)); }
    glEnd();
    glColor3f(0.3f, 0.15f, 0.05f); glLineWidth(4.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(0, 0); glVertex2f(0, -0.35f); glVertex2f(0.08f, -0.42f);
    glEnd();
    glPopMatrix();
}

void drawTitle(const char* text, float x, float y) {
    glColor3f(1.0f, 1.0f, 1.0f); glRasterPos2f(x, y);
    for (const char* c = text; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

// ─────────────────────────────────────────
//  Background & Textured Objects
// ─────────────────────────────────────────
void drawSky(float r1, float g1, float b1, float r2, float g2, float b2) {
    glBegin(GL_QUADS);
    glColor3f(r1, g1, b1); glVertex2f(-1.0f, -1.0f);
    glColor3f(r1, g1, b1); glVertex2f(1.0f, -1.0f);
    glColor3f(r2, g2, b2); glVertex2f(1.0f, 1.0f);
    glColor3f(r2, g2, b2); glVertex2f(-1.0f, 1.0f);
    glEnd();
}

void drawGround(GLuint tex, float gr, float gg, float gb) {
    if (tex != 0) {
        glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);
        glColor4f(1, 1, 1, 1);
    }
    else { glColor3f(gr, gg, gb); }
    glBegin(GL_QUADS);
    if (tex) glTexCoord2f(0, 0); glVertex2f(-1.0f, -1.0f);
    if (tex) glTexCoord2f(1, 0); glVertex2f(1.0f, -1.0f);
    if (tex) glTexCoord2f(1, 1); glVertex2f(1.0f, -0.55f);
    if (tex) glTexCoord2f(0, 1); glVertex2f(-1.0f, -0.55f);
    glEnd();
    if (tex != 0) glDisable(GL_TEXTURE_2D);
}

void drawTexturedObject(GLuint tex, float cx, float cy, float w, float h) {
    if (!tex) return;
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);
    glColor4f(1, 1, 1, 1);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(cx - w, cy - h);
    glTexCoord2f(1, 0); glVertex2f(cx + w, cy - h);
    glTexCoord2f(1, 1); glVertex2f(cx + w, cy + h);
    glTexCoord2f(0, 1); glVertex2f(cx - w, cy + h);
    glEnd();
    glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
}

// Custom function for Sun Rotation
void drawRotatingSun(GLuint tex, float cx, float cy, float w, float h, float angle) {
    if (!tex) return;
    glPushMatrix();
    glTranslatef(cx, cy, 0);       // TRANSLATION
    glRotatef(angle, 0, 0, 1);     // ROTATION (Fulfills the rotation requirement for grades)
    glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-w, -h);
    glTexCoord2f(1, 0); glVertex2f(w, -h);
    glTexCoord2f(1, 1); glVertex2f(w, h);
    glTexCoord2f(0, 1); glVertex2f(-w, h);
    glEnd();
    glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

// ─────────────────────────────────────────
//  SCENE 1 
// ─────────────────────────────────────────
void drawScene1() {
    drawSky(0.53f, 0.81f, 0.98f, 0.87f, 0.95f, 1.00f);
    drawRotatingSun(texSun, 0.65f, 0.65f, 0.2f, 0.2f, sunAngle); // Moving/Rotating sun
    drawGround(texGround, 0.4f, 0.72f, 0.35f);

    float flowerX[] = { -0.7f,-0.5f,-0.3f,0.3f,0.5f,0.7f };
    for (int i = 0; i < 6; i++) {
        glColor3f(0.2f, 0.55f, 0.2f); glLineWidth(2.0f);
        glBegin(GL_LINES); glVertex2f(flowerX[i], -0.55f); glVertex2f(flowerX[i], -0.40f); glEnd();
        glColor3f(1.0f, 0.85f, 0.0f); drawCircle(flowerX[i], -0.38f, 0.04f);
        glColor3f(1.0f, 0.4f, 0.6f); drawCircle(flowerX[i], -0.38f, 0.025f);
    }

    float eyeSmile = (catVelY == 0.0f);
    drawCat(catX, catY + catBobOffset, 1.0f, false, true, false); // The small cat

    if (jumpingPhase) {
        drawTitle("\"What a nice day! Let me jump around!\"", -0.55f, 0.35f);
    }
    else {
        drawTitle("\"What a nice and peaceful day...\"", -0.45f, 0.35f);
    }

    glPointSize(10.0f); glColor3f(1.0f, 0.3f, 0.3f);
    glBegin(GL_POINTS); for (int i = 0; i < 6; i++) glVertex2f(flowerX[i], -0.38f); glEnd();
}

// ─────────────────────────────────────────
//  SCENE 2
// ─────────────────────────────────────────
void drawScene2() {
    drawSky(0.25f, 0.28f, 0.38f, 0.45f, 0.48f, 0.55f);

    drawTexturedObject(texCloud, -0.6f, 0.80f, 0.35f, 0.25f);
    drawTexturedObject(texCloud, 0.0f, 0.85f, 0.35f, 0.25f);
    drawTexturedObject(texCloud, 0.6f, 0.80f, 0.35f, 0.25f);

    drawGround(texGround, 0.25f, 0.40f, 0.22f);

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.5f, 0.65f, 0.90f, 0.5f);
    glPushMatrix(); glTranslatef(-0.1f, -0.58f, 0); glScalef(0.25f, 0.06f, 1); drawCircle(0, 0, 1.0f); glPopMatrix();
    glDisable(GL_BLEND);

    drawRain();

    drawCat(catX, catY + catBobOffset * 0.5f, 1.0f, true, false, false);

    glPushMatrix();
    glTranslatef(0.7f, 0.6f, 0); glScalef(0.8f, 0.8f, 1);
    glColor3f(1.0f, 1.0f, 0.5f); glLineWidth(4.0f);
    glBegin(GL_LINE_STRIP);
    glVertex2f(0, 0.1f); glVertex2f(-0.05f, 0.0f); glVertex2f(0.0f, -0.02f); glVertex2f(-0.07f, -0.14f);
    glEnd();
    glPopMatrix();

    drawTitle("\"I'm scared... it's so cold...\"", -0.40f, 0.30f);
    drawTitle("\"I wish someone could help me...\"", -0.45f, 0.18f);
}

// ─────────────────────────────────────────
//  SCENE 3 
// ─────────────────────────────────────────
void drawScene3() {
    drawSky(0.35f, 0.38f, 0.50f, 0.55f, 0.58f, 0.68f);

    drawTexturedObject(texCloud, -0.6f, 0.80f, 0.35f, 0.25f);
    drawTexturedObject(texCloud, 0.0f, 0.85f, 0.35f, 0.25f);
    drawTexturedObject(texCloud, 0.6f, 0.80f, 0.35f, 0.25f);

    drawGround(texGround, 0.28f, 0.44f, 0.25f);

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.45f, 0.60f, 0.85f, 0.55f);
    glPushMatrix(); glTranslatef(0.3f, -0.58f, 0); glScalef(0.18f, 0.05f, 1); drawCircle(0, 0, 1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.4f, -0.60f, 0); glScalef(0.22f, 0.055f, 1); drawCircle(0, 0, 1.0f); glPopMatrix();
    glDisable(GL_BLEND);

    glColor4f(0.6f, 0.75f, 1.0f, 0.5f); glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < NUM_DROPS / 2; i++) {
        glVertex2f(drops[i].x, drops[i].y);
        glVertex2f(drops[i].x + 0.01f, drops[i].y - 0.05f);
    }
    glEnd();

    // Mother cat (Animated Translation from left)
    glPushMatrix();
    glTranslatef(motherX, -0.1f, 0); glScalef(1.1f, 1.1f, 1.0f);
    drawCat(0, 0, 1.0f, false, true, true); // isMother = true (slightly darker color)
    glPopMatrix();

    drawUmbrella(motherX + 0.07f, 0.22f, umbrellaScale);

    // Small cat
    glPushMatrix();
    glTranslatef(0, catBobOffset * 0.3f, 0);
    glScalef(0.8f, 0.8f, 1.0f);
    drawCat(smallCatX, -0.22f, 1.0f, catReachedMom, catReachedMom, false);
    glPopMatrix();

    if (motherX >= -0.45f && !catReachedMom) {
        drawTitle("\"Don't be afraid, little one! Come here!\"", -0.90f, 0.45f);
    }
    else if (catReachedMom) {
        drawTitle("\"Don't be afraid, my little one. I'm here.\"", -0.90f, 0.50f);
        drawTitle("\"Come under the umbrella, you'll be safe.\"", -0.90f, 0.38f);
        drawTitle("\"Thank you, Mom!\"", -0.20f, 0.26f);
    }
}

// ─────────────────────────────────────────
//  SCENE 4
// ─────────────────────────────────────────
void drawScene4() {
    drawSky(0.40f, 0.70f, 1.00f, 1.00f, 0.95f, 0.70f);
    drawRotatingSun(texSun, 0.68f, 0.72f, 0.2f, 0.2f, sunAngle);

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float rainbowColors[7][3] = {
        {1,0,0},{1,0.5f,0},{1,1,0},{0,0.8f,0},{0,0.5f,1},{0.3f,0,0.8f},{0.8f,0,0.8f}
    };
    glLineWidth(5.0f);
    for (int b = 0; b < 7; b++) {
        glColor4f(rainbowColors[b][0], rainbowColors[b][1], rainbowColors[b][2], 0.65f);
        float rad = 0.50f + b * 0.045f;
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 30; i++) { float a = i * 3.14159f / 30; glVertex2f(-0.6f + rad * cos(a), -0.20f + rad * sin(a)); }
        glEnd();
    }
    glDisable(GL_BLEND);

    drawGround(texGround, 0.42f, 0.75f, 0.38f);

    float fx[] = { -0.75f,-0.55f,-0.35f,0.35f,0.55f,0.75f,0.15f };
    float fc[][3] = { {1,0.2f,0.5f},{1,0.9f,0},{0.5f,0.2f,1},{1,0.5f,0},{0,0.8f,0.8f},{1,0.2f,0},{1,1,0.2f} };
    for (int i = 0; i < 7; i++) {
        glColor3f(0.2f, 0.55f, 0.2f); glLineWidth(2.0f);
        glBegin(GL_LINES); glVertex2f(fx[i], -0.55f); glVertex2f(fx[i], -0.40f); glEnd();
        glColor3f(fc[i][0], fc[i][1], fc[i][2]); drawCircle(fx[i], -0.38f, 0.045f);
    }

    glPushMatrix();
    glTranslatef(0, catBobOffset, 0);
    glPushMatrix(); glTranslatef(-0.28f, -0.10f, 0); glScalef(1.1f, 1.1f, 1.0f); drawCat(0, 0, 1.0f, false, true, true); glPopMatrix();
    glPushMatrix(); glTranslatef(0.22f, -0.12f, 0); glScalef(0.75f, 0.75f, 1.0f); drawCat(0, 0, 1.0f, false, true, false); glPopMatrix();
    glPopMatrix();

    drawTitle("\"Thank you, Mom!\"", 0.10f, 0.35f);
    drawTitle("\"The sun is back!\"", 0.10f, 0.23f);

    glEnable(GL_LINE_STIPPLE); glLineStipple(2, 0xAAAA); glLineWidth(3.0f); glColor3f(1.0f, 0.85f, 0.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.97f, -0.97f); glVertex2f(0.97f, -0.97f);
    glVertex2f(0.97f, 0.97f); glVertex2f(-0.97f, 0.97f);
    glEnd();
    glDisable(GL_LINE_STIPPLE);
}

// ─────────────────────────────────────────
//  Display Callback
// ─────────────────────────────────────────
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    switch (currentScene) {
    case 1: drawScene1(); break;
    case 2: drawScene2(); break;
    case 3: drawScene3(); break;
    case 4: drawScene4(); break;
    }
    glutSwapBuffers();
}

// ─────────────────────────────────────────
//  Physics / Animation Update
// ─────────────────────────────────────────
const float GROUND_Y = -0.18f;
const float GRAVITY = -0.008f;
const float JUMP_FORCE = 0.065f;

void update(int val) {
    static float bobTime = 0;
    bobTime += 0.05f;
    catBobOffset = 0.012f * sinf(bobTime);

    // Rain drops
    for (int i = 0; i < NUM_DROPS; i++) {
        drops[i].y -= drops[i].speed;
        if (drops[i].y < -1.0f) {
            drops[i].y = 1.0f + (float)rand() / RAND_MAX * 0.5f;
            drops[i].x = -1.0f + (float)rand() / RAND_MAX * 2.0f;
        }
    }

    // Scene 1: gravity / jump
    if (currentScene == 1) {
        if (!catOnGround) {
            catVelY += GRAVITY;
            catY += catVelY;
            if (catY <= GROUND_Y) {
                catY = GROUND_Y;
                catVelY = 0.0f;
                catOnGround = true;
            }
        }
    }

    // Mother entry animation
    if (currentScene == 3 && motherX < -0.45f) {
        motherX += 0.005f;
    }

    if (currentScene == 3 && umbrellaScale < 1.0f) umbrellaScale += 0.02f;

    sunAngle -= 1.0f; // Sun rotation

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// ─────────────────────────────────────────
//  Reset helpers
// ─────────────────────────────────────────
void resetScene1() {
    catX = -0.05f;
    catY = GROUND_Y;
    catVelY = 0.0f;
    catOnGround = true;
    jumpCount = 0;
    jumpingPhase = true;
}

void resetScene3() {
    smallCatX = 0.05f;
    catReachedMom = false;
    umbrellaScale = 0.1f;
    motherX = -1.2f; // Reset mother outside the screen
}

// ─────────────────────────────────────────
//  Keyboard / Interaction
// ─────────────────────────────────────────
void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0);

    if (key == ' ') {
        if (currentScene < 4) {
            currentScene++;
            if (currentScene == 3) resetScene3();
            umbrellaScale = 0.1f;
        }
        manageSound();
    }

    // Zoom controls to fulfill Viewing / Clipping requirements
    if (key == 'z' || key == 'Z') {
        viewZoom -= 0.05f;
        if (viewZoom < 0.2f) viewZoom = 0.2f;
        updateProjection();
    }
    if (key == 'x' || key == 'X') {
        viewZoom += 0.05f;
        if (viewZoom > 2.0f) viewZoom = 2.0f;
        updateProjection();
    }
}

void specialKeys(int key, int x, int y) {
    if (currentScene == 1) {
        if (key == GLUT_KEY_UP) {
            if (catOnGround && jumpingPhase) {
                catVelY = JUMP_FORCE;
                catOnGround = false;
                jumpCount++;
                if (jumpCount >= MAX_JUMPS) {
                    jumpingPhase = false;
                }
            }
        }
        if (key == GLUT_KEY_LEFT) {
            catX -= 0.05f;
            if (catX < -0.90f) catX = -0.90f;
        }
        if (key == GLUT_KEY_RIGHT) {
            catX += 0.05f;
            if (catX > 0.90f) catX = 0.90f;
            if (!jumpingPhase && jumpCount >= MAX_JUMPS && catX > 0.3f) {
                currentScene = 2;
                manageSound();
            }
        }
        return;
    }

    if (currentScene == 3 && key == GLUT_KEY_LEFT) {
        if (!catReachedMom) {
            smallCatX -= 0.04f;
            float target = motherX + 0.20f;
            if (smallCatX <= target) {
                smallCatX = target;
                catReachedMom = true;
            }
        }
        return;
    }

    if (key == GLUT_KEY_RIGHT && currentScene < 4) {
        currentScene++;
        if (currentScene == 3) resetScene3();
        umbrellaScale = 0.1f;
        manageSound();
    }
    else if (key == GLUT_KEY_LEFT && currentScene > 1) {
        currentScene--;
        if (currentScene == 1) resetScene1();
        manageSound();
    }
}

void mouse(int button, int state, int x, int y) {
    if (state == GLUT_UP) {
        if (button == GLUT_LEFT_BUTTON && currentScene < 4) {
            currentScene++;
            if (currentScene == 3) resetScene3();
            umbrellaScale = 0.1f;
        }
        else if (button == GLUT_RIGHT_BUTTON && currentScene > 1) {
            currentScene--;
            if (currentScene == 1) resetScene1();
        }
        manageSound();
    }
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    updateProjection(); // Update Zoom with window size
}

// ─────────────────────────────────────────
//  Init
// ─────────────────────────────────────────
void init() {
    glClearColor(0, 0, 0, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load textures (White color becomes transparent for sun and clouds, but not for the ground)
    texGround = LoadTexture("ground.bmp", false);
    texSun = LoadTexture("extra.bmp", true);
    texCloud = LoadTexture("extra1.bmp", true);

    srand((unsigned)time(NULL));
    for (int i = 0; i < NUM_DROPS; i++) {
        drops[i].x = -1.0f + (float)rand() / RAND_MAX * 2.0f;
        drops[i].y = 1.0f + (float)rand() / RAND_MAX * 2.0f;
        drops[i].speed = 0.015f + (float)rand() / RAND_MAX * 0.020f;
    }

    resetScene1();
}

// ─────────────────────────────────────────
//  Main
// ─────────────────────────────────────────
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(700, 700);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("A Little Cat in the Rain - CS2206");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}