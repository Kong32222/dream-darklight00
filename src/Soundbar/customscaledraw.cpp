#include "customscaledraw.h"

#if 0
void SetAxisColor(QwtPlot *plot, const QColor &color)
{
    // x轴 y轴
    QwtScaleWidget *scaleWidget = plot->axisWidget(QwtPlot::yLeft);
    QwtScaleWidget *scaleWidget2 = plot->axisWidget(QwtPlot::xBottom);
    if (scaleWidget)
    {
        QPalette palette = scaleWidget->palette();
        palette.setColor(QPalette::WindowText, color);
        scaleWidget->setPalette(palette);
    }
    if (scaleWidget2)
    {
        QPalette palette = scaleWidget2->palette();
        palette.setColor(QPalette::WindowText, color);
        scaleWidget2->setPalette(palette);
    }
}
#endif 
