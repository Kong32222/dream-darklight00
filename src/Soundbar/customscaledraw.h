#ifndef CUSTOMSCALEDRAW_H
#define CUSTOMSCALEDRAW_H
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_widget.h>
#include <qwt_legend.h>
#include <QDebug>

//修改 x y轴的颜色
void SetAxisColor(QwtPlot *plot, const QColor &color);

class CustomScaleDraw : public QwtScaleDraw
{
public:
    CustomScaleDraw() {}

    virtual void drawTick(QPainter *painter, double val, double len) const override
    {
        // 自定义刻度线绘制
        painter->save();
        painter->setPen(Qt::white);  // 设置刻度线颜色为 white
        QwtScaleDraw::drawTick(painter, val, len);  // 调用基类的实现
        painter->restore();
    }

    virtual void drawBackbone(QPainter *painter) const override
    {
        // 自定义主干线绘制
        painter->save();
        // painter->setPen(QPen(Qt::blue, 2));  // 设置主干线颜色为蓝色，线宽为2
        QwtScaleDraw::drawBackbone(painter);  // 调用基类的实现
        painter->restore();
    }

    virtual void drawLabel(QPainter *painter, double value) const override
    {
        // 设置颜色映射
        QColor color = Qt::white;

        // 设置画笔颜色
        painter->setPen(color);

        // 绘制刻度标签
        QwtScaleDraw::drawLabel(painter, value);
    }
    /*
     * 1. 函数用来绘制刻度线
          virtual void drawTick(QPainter *painter, double val, double len) const;
                painter：指向用于绘制的 QPainter 对象的指针 。
                val：    刻度线的值，表示在坐标轴上的位置 。
                len：    刻度线的长度 。
       通过重写这个函数，可以自定义刻度线的绘制方式，例如更改刻度线的样式、颜色或形状。

     * 2.用来绘制坐标轴的主干线
         virtual void drawBackbone(QPainter *painter) const;
         通过重写这个函数，可以自定义坐标轴主干线的绘制方式，例如更改主干线的颜色、线宽或样式

     * 3.用来绘制坐标轴上的标签
        virtual void drawLabel(QPainter *painter, double val) const;
            painter：指向用于绘制的 QPainter 对象的指针。
            val：标签的值，表示标签在坐
        通过重写这个函数，可以自定义标签的绘制方式，例如更改标签的字体、颜色、位置或格式。


    */

};



#endif // CUSTOMSCALEDRAW_H
