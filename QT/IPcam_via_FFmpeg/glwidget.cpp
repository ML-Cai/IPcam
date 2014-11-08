#include "glwidget.h"
#include <math.h>
#include <stdio.h>
/* -------------------------------------------------------- */
extern int VOICE_DATA[] ;
extern int VOICE_DATA_COUNT;
extern double FFT_DATA[];
extern int VOICE_DATA_1s[] ;
extern int VOICE_DATA_1s_COUNT ;

extern int CZR[];
extern int CZR_count ;
extern int AMP[] ;
extern int Background_means ;


const float PI = 3.14159f / 180.0f;
/* -------------------------------------------------------- */
GLwidget::GLwidget(QWidget *parent) :
    QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    obj_texture =0;
    flush_timer =NULL;
    initializeGL();
}
/* -------------------------------------------------------- */
void GLwidget::initializeGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
}
/* -------------------------------------------------------- */
void GLwidget::resizeGL(int width, int height)
{
    obj_height = height;
    obj_width = width;
}
/* -------------------------------------------------------- */
/* slots */
void GLwidget::slots_Display()
{
    paintGL();
    updateGL();
}
/* -------------------------------------------------------- */
void GLwidget::set_reflashtime(int t)
{
    if(flush_timer ==NULL) {
        flush_timer = new QTimer;
        connect(flush_timer, SIGNAL(timeout()),  this , SLOT(slots_Display()));
    }
    if(t ==0) {
        /* set flush timer */
        flush_timer->stop();
    }
    else {
        flush_timer->start(t);
    }
}
/* -------------------------------------------------------- */
void GLwidget::slots_set_picture(unsigned char *color, int SizeWidth, int SizeHeight)
{
    makeCurrent();

    if(obj_texture ==0) glGenTextures(1, &obj_texture);

    glBindTexture(GL_TEXTURE_2D, obj_texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, SizeWidth , SizeHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, color);
    glBindTexture(GL_TEXTURE_2D, 0);

    Stream_width = SizeWidth;
    Stream_height = SizeHeight;
}
/* -------------------------------------------------------- */
void GLwidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, Stream_width, Stream_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, Stream_width, 0, Stream_height, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glPushMatrix();
        glBindTexture(GL_TEXTURE_2D, obj_texture);
        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTexCoord2f(0, 0);	glVertex3f(0.0f, Stream_height, 0.0f);
        glTexCoord2f(0, 1);	glVertex3f(0.0f, 0 , 0.0f);
        glTexCoord2f(1, 1);	glVertex3f(Stream_width,  0, 0.0f);
        glTexCoord2f(1, 0);	glVertex3f(Stream_width, Stream_height, 0.0f);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
    glPopMatrix();
}
