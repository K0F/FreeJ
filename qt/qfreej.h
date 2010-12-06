#ifndef QFREEJ_H
#define QFREEJ_H
#include <QMessageBox>

#include <QMainWindow>
#include <text_layer.h>
#include <context.h>
#include <linklist.h>
#include <QMdiArea>
#include <QqWidget.h>
#include <QLayout>
#include <Sound.h>

class QTimer;
class QqTabWidget;

namespace Ui {
    class Qfreej;
}

class Qfreej : public QWidget {
    Q_OBJECT
public:
    Qfreej(QWidget *parent = 0);
    ~Qfreej();

    QTimer *poller;
    Context *getContext();
    bool getStartState();
    void createMenuGenerator();

public slots:
    void addLayer();
    void updateInterface();
    void addTextLayer();
    void startStreaming();
    void addGenerator(QAction*);
    void openSoundDevice();

protected:
    void closeEvent(QCloseEvent*);

private:
    void init();
    Context *freej;
    ViewPort *screen;
    bool startstate;
    int fps;
    QqTabWidget *tabWidget;
    QTabBar *myTabBar;
    int number;
    TextLayer *textLayer;   //see to erase this later
    QGridLayout *grid;
    QHBoxLayout *layoutH;
    QMenu* menuGenerator;
    Sound *m_snd;
};



#endif // QFREEJ_H
