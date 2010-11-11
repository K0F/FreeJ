/*  Qfreej
 *  (c) Copyright 2010 fred_99
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */



#include <QqFiltersApplied.h>
#include <iostream>
#include <QWidget>
#include <QDragEnterEvent>
#include <QAction>
#include <QLabel>
#include <QDebug>
#include <QString>
#include <QByteArray>

using namespace std;

QqSlider::QqSlider(int idx, FilterInstance *filter, QWidget *parent) : QSlider()
{
    paramNumber = idx;
    filterI = filter;
    setOrientation(Qt::Horizontal);
//    double* val = (double *) filterI->parameters[paramNumber]->get ();
    double val = 0.0;
    int value = (int)(val * 100.0);
    this->setValue(value);
    //filterI->parameters[paramNumber]->set(&val);
    connect(this, SIGNAL(sliderMoved(int)), this, SLOT(changeValue(int)));
}

QqSlider::~QqSlider()
{
}

void QqSlider::changeValue(int value)
{
    double nbr = (double)value / 100.0;
    //to see if it does something ... and it does :)
    filterI->parameters[paramNumber]->set((void *)&nbr);
    filterI->get_layer()->screen->clear();  //cleans the screen
}

QqCheck::QqCheck(int idx, FilterInstance *filter, QWidget *parent) : QCheckBox(parent)
{
    m_paramNumber = idx;
    m_filterI = filter;
    bool* val = (bool *) m_filterI->parameters[m_paramNumber]->get ();
    this->setDown(*val);
    connect(this, SIGNAL(stateChanged(int)), this, SLOT(changeValue(int)));
}

void QqCheck::changeValue(int state)
{
    bool bol;
    if (state)
    {
        bol = true;
        m_filterI->parameters[m_paramNumber]->set((void *)&bol);
    }
    else
    {
        bol = false;
        m_filterI->parameters[m_paramNumber]->set((void *)&bol);
    }
    m_filterI->get_layer()->screen->clear();  //cleans the screen
}

QqColor::QqColor(int idx, FilterInstance *filter, QWidget *parent) : QGroupBox(parent)
{
    m_paramNumber = idx;
    m_filterI = filter;
    QGridLayout *layout = new QGridLayout();
    this->setTitle(filter->parameters[idx]->name);

    double rgb[3];
//    double *rgb_init;
/*
    it seems that there is no default value, so don't use
    rgb_init = (double *) m_filterI->parameters[m_paramNumber]->get ();
    rgb[0] = rgb_init[0];
    rgb[1] = rgb_init[1];
    rgb[2] = rgb_init[2];
*/

    rgb[0] = 0.5;
    rgb[1] = 0.5;
    rgb[2] = 0.5;

    QSlider *red = new QSlider();
    red->setOrientation(Qt::Horizontal);
    connect(red, SIGNAL(sliderMoved(int)), this, SLOT(changeRed(int)));
    red->setRange(0, 255);
    int value = (int)(rgb[0] * 255.0);
    red->setValue(value);
    red->setSliderDown(true);
    layout->addWidget(red, 0, 0);

    QSlider *green = new QSlider();
    green->setOrientation(Qt::Horizontal);
    connect(green, SIGNAL(sliderMoved(int)), this, SLOT(changeGreen(int)));
    green->setSliderDown(true);
    green->setRange(0, 255);
    value = (int)(rgb[1] * 255.0);
    green->setValue(value);
    layout->addWidget(green, 1, 0);

    QSlider *blue = new QSlider();
    blue->setOrientation(Qt::Horizontal);
    connect(blue, SIGNAL(sliderMoved(int)), this, SLOT(changeBlue(int)));
    blue->setSliderDown(true);
    blue->setRange(0, 255);
    value = (int)(rgb[2] * 255.0);
    blue->setValue(value);
    layout->addWidget(blue, 2, 0);
    this->setLayout(layout);
 }

void QqColor::changeColor (int val, int idx)
{
    double rgb[3];
    double *rgb_init;
    rgb_init = (double *) m_filterI->parameters[m_paramNumber]->get ();
    rgb[0] = rgb_init[0];
    rgb[1] = rgb_init[1];
    rgb[2] = rgb_init[2];
    rgb[idx] = val/255.0;
    m_filterI->parameters[m_paramNumber]->set((void *)rgb);
    m_filterI->get_layer()->screen->clear();  //cleans the screen
}

void QqColor::changeRed(int val)
{
    changeColor (val,0);
}

void QqColor::changeGreen(int val)
{
    changeColor (val,1);
}

void QqColor::changeBlue(int val)
{
    changeColor (val,2);
}

QqPos::QqPos(int idx, FilterInstance *filter, QWidget *parent) : QGroupBox(parent)
{
    m_paramNumber = idx;
    m_filterI = filter;
    QGridLayout *layout = new QGridLayout();
    this->setTitle(filter->parameters[idx]->name);

    double pos[2];
//    double *pos_init;
/*
    it seems that there is no default value, so I don't use
    pos_init = (double *) m_filterI->parameters[m_paramNumber]->get ();
    pos[0] = pos_init[0];
    pos[1] = pos_init[1];
*/
    //default values
    pos[0] = 0.0;
    pos[1] = 0.0;

    QSlider *x = new QSlider(this);
    x->setOrientation(Qt::Horizontal);
    connect(x, SIGNAL(sliderMoved(int)), this, SLOT(changeX(int)));
    x->setRange(0, 1000);
    int value = (int)(pos[0] * 1000.0);
    x->setValue(value);
    x->setSliderDown(true);
    layout->addWidget(x, 0, 0);

    QSlider *y = new QSlider(this);
    y->setOrientation(Qt::Horizontal);
    connect(y, SIGNAL(sliderMoved(int)), this, SLOT(changeY(int)));
    y->setSliderDown(true);
    y->setRange(0, 1000);
    value = (int)(pos[1] * 1000.0);
    y->setValue(value);
    layout->addWidget(y, 1, 0);

    this->setLayout(layout);
 }

void QqPos::changePos (int val, int idx)
{
    double pos[2];
    double *pos_init;
    pos_init = (double *) m_filterI->parameters[m_paramNumber]->get ();
    pos[0] = pos_init[0];
    pos[1] = pos_init[1];
    pos[idx] = val/1000.0;
    m_filterI->parameters[m_paramNumber]->set((void *)pos);
    m_filterI->get_layer()->screen->clear();  //cleans the screen
}

void QqPos::changeX(int val)
{
    changePos (val,0);
}

void QqPos::changeY(int val)
{
    changePos (val,1);
}

QqFilter::QqFilter(FilterInstance *filterI) : QListWidgetItem()
{
    setText(filterI->name);
    filterIn = filterI;
    filterParam = new QqFilterParam(filterI);
}

QqFilter::~QqFilter()
{
    delete filterParam;
}

QqFilterParam::QqFilterParam(FilterInstance *filter) : QWidget()
{
    filterI = filter;
    QVBoxLayout *layoutV = new QVBoxLayout;
    QLabel *name;
    Layer* lay = filterI->get_layer();
    QString layS = lay->get_filename();
    QString title;
    title = filterI->name;
    title.append(" ");
    if (layS.count() > 1)
    {
        title.append(lay->get_filename());
    }
    else
    {
        title.append("text");
    }
    setWindowTitle(title);

    for (int i=1; i <= filterI->parameters.len(); ++i)
    {
        QHBoxLayout* layoutH = new QHBoxLayout;
        if (filterI->parameters[i]->type == Parameter::NUMBER)
        {
            name = new QLabel(filterI->parameters[i]->name);
            layoutH->addWidget(name);
            QqSlider *slider = new QqSlider(i, filterI, this);
            slider->setSliderDown(true);
            slider->setRange(0,100);
            layoutH->addWidget(slider);
            layoutV->addLayout(layoutH);
            setLayout(layoutV);
            this->show();
            qDebug() << "NUMBER";
        }
        else if (filterI->parameters[i]->type == Parameter::BOOL)
        {
            name = new QLabel(filterI->parameters[i]->name);
            layoutH->addWidget(name);
            QqCheck *check = new QqCheck(i, filterI, this);
            check->setChecked(false);
            layoutH->addWidget(check);
            layoutV->addLayout(layoutH);
            setLayout(layoutV);
            this->show();
        }
        else if (filterI->parameters[i]->type == Parameter::COLOR)
        {
            QqColor *color = new QqColor(i, filterI, this);
            layoutH->addWidget(color);
            layoutV->addLayout(layoutH);
            this->show();
        }
        else if (filterI->parameters[i]->type == Parameter::POSITION)
        {
            QqPos *pos = new QqPos(i, filterI, this);
            layoutH->addWidget(pos);
            layoutV->addLayout(layoutH);
            this->show();

            qDebug() << "size POSITION:" << filterI->parameters[i]->value_size;
        }
        else if (filterI->parameters[i]->type == Parameter::STRING)
        {
            qDebug() << "size STRING:" << filterI->parameters[i]->value_size;
        }
    }
}

QqFilterParam::~QqFilterParam()
{
}

QqFiltersListApplied::QqFiltersListApplied(Layer *lay, QWidget* parent) : QListWidget(parent)
{
    setToolTip("Filters applied on the layer. You can change the order by D&D\nalso double click to hide parameters");
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);

    connect(this, SIGNAL(itemPressed(QListWidgetItem *)), this, SLOT(setElement(QListWidgetItem*)));
    draggItemFilter = NULL;
    layer = lay;
    textLayer = NULL;

    QAction *remFilter= new QAction("remove filter",this);
    remFilter->setShortcut(Qt::Key_Delete);
    addAction(remFilter);
    connect(remFilter,SIGNAL(triggered()),this,SLOT(removeFilter()));
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(showParamWindow(QListWidgetItem*)));
}

QqFiltersListApplied::QqFiltersListApplied(TextLayer *lay, QWidget *parent) : QListWidget(parent)
{
    setToolTip("Filters applied on the layer. You can change the order by D&D\nalso double click to hide parameters");
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);

    connect(this, SIGNAL(itemPressed(QListWidgetItem *)), this, SLOT(setElement(QListWidgetItem*)));
    draggItemFilter = NULL;
    layer = NULL;
    textLayer = lay;

    QAction *remFilter= new QAction("remove filter",this);
    remFilter->setShortcut(Qt::Key_Delete);
    addAction(remFilter);
    connect(remFilter,SIGNAL(triggered()),this,SLOT(removeFilter()));
    connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(showParamWindow(QListWidgetItem*)));
}

QqFiltersListApplied::~QqFiltersListApplied()
{
}

void QqFiltersListApplied::setElement(QListWidgetItem *item)
{
    draggItemFilter = (QqFilter *)item;
}

void QqFiltersListApplied::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void QqFiltersListApplied::dropEvent(QDropEvent * event)
{
    if (draggItemFilter)
    {
        char temp[256];
        QByteArray fich = draggItemFilter->text().toAscii();
        strcpy (temp,fich.data());
        int idx = 0;
        int currentDropRow = row(itemAt(event->pos()));

        currentDropRow++;
        if (layer)
        {
            FilterInstance *filter = layer->filters.search(temp, &idx);
            if (currentDropRow >= 1 && currentDropRow <= layer->filters.len())
            {
                filter->move(currentDropRow);
            }
            else
            {
                draggItemFilter = NULL;
                return;
            }
        }
        else if (textLayer)
        {
            FilterInstance *filter = textLayer->filters.search(temp, &idx);
            if (currentDropRow >= 1 && currentDropRow <= textLayer->filters.len())
            {
                filter->move(currentDropRow);
            }
            else
            {
                draggItemFilter = NULL;
                return;
            }
        }
        else
        {
            cout << "pas de filtre à déplacer ?????" << endl;
            draggItemFilter = NULL;
            return;
        }
    }
    draggItemFilter = NULL;
    QListWidget::dropEvent(event);  // pour ne pas manger le signal
}

void QqFiltersListApplied::removeFilter()
{
    if (hasFocus() && count() >= 1 && currentRow() >=0)
    {
        int idx;
        QqFilter *item = (QqFilter *)currentItem();
        char temp[256];
        QByteArray fich = item->text().toAscii();
        strcpy (temp,fich.data());
        if (layer)
        {
            FilterInstance *filter = layer->filters.search(temp, &idx);
            delete item;
            filter->rem();
        }
        else if (textLayer)
        {
            FilterInstance *filter = textLayer->filters.search(temp, &idx);
            delete item;
            filter->rem();
        }
        else
        {
            cout << "Erreur : Impossible de supprimer le filtre : " << item->text().toStdString() << endl;
            return;
        }
    }
}

void QqFiltersListApplied::showParamWindow(QListWidgetItem *item)
{
    QqFilter *filter = (QqFilter *)item;
    if (filter->filterParam->isHidden())
    {
        filter->filterParam->setVisible(true);
        qDebug() << "double on ";
    }
    else
        filter->filterParam->close();
}
