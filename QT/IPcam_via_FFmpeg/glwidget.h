#ifndef GLWIDGET_H
#define GLWIDGET_H
/* --------------------------------------------------- */
#include <QGLWidget>
#include <QTimer>
/* --------------------------------------------------- */
class GLwidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit GLwidget(QWidget *parent = 0);
    void set_reflashtime(int t);
protected :
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    int obj_height;
    int obj_width;
    GLuint obj_texture;
    int Stream_height;
    int Stream_width;
    QTimer* flush_timer ;
private:

public slots:
    void slots_set_picture(unsigned char *color, int SizeWidth, int SizeHeight);
    void slots_Display();
};
/* --------------------------------------------------- */
#endif // GLWIDGET_H
