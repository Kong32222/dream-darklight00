#include "soundbar.h"



Soundbar::Soundbar(QWidget *parent) :
    QWidget(parent), m_volume(0)
{
#if 0
    //垂直  ⊥
    setFixedSize(12, 60);   // 音柱的尺寸 (宽度6, 高度80)
#else
    // 水平
    setFixedSize(100, 15);
#endif
}


void Soundbar::setVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    m_volume = volume;
    update();
}

void Soundbar::moveSoundbar(int x, int y)
{
     move(x, y);  // 移动 Soundbar 到指定位置 (x, y)
}


void Soundbar::paintEvent(QPaintEvent *event)
{

    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 使绘制的图形边缘更加平滑和清晰。
#if 0
    int numBars = 17;
    int barWidth = width() - 2;  // 调整宽度以适应边框
    int barHeight = (height() - 2) / numBars;  // 调整高度以适应边框

    int filledBars = m_volume * numBars / 100;
    // 记得修改 QRect barRect()
#else
    int numBars = (width() / 4);  // Number of bars
    int barWidth = 4;
    int barHeight = height() - 2;
    int filledBars = m_volume * numBars / 100;
#endif

    for (int i = 0; i < numBars; ++i)
    {
        /*i * barWidth + 1：这是矩形左上角的 x 坐标。i 可能是一个循环变量，barWidth 可能是矩形的宽度，+1 可能是为了给矩形之间留出一些间隙。

        1：这是矩形左上角的 y 坐标。通常用来指定矩形在图形界面中的垂直位置。

        barWidth：这是矩形的宽度，即矩形沿 x 轴的长度。

        barHeight：这是矩形的高度，即矩形沿 y 轴的长度。

        因此，这行代码的作用是创建一个矩形对象barRect，
        其左上角位于(i * barWidth + 1, 1)，宽度为barWidth，高度为barHeight。
        在图形界面编程中，这种方式通常用于定义和管理图形元素的位置和大小。*/
#if 0
         //QRect barRect(1, height() - (i + 1) * barHeight - 1, barWidth, barHeight);
#else
         QRect barRect(i * barWidth + 1, 1, barWidth, barHeight);
#endif

        if (i < filledBars) {
            if (i < numBars / 3) {
                painter.setBrush(QColor(0, 255, 0));  // Green for low volume
            } else if (i < 2 * numBars / 3) {
                painter.setBrush(QColor(255, 255, 0));  // Yellow for medium volume
            } else {
                painter.setBrush(QColor(255, 0, 0));  // Red for high volume
            }
        } else {
            painter.setBrush(QColor(128, 128, 128));  // Gray for empty bars
        }
        painter.drawRect(barRect);
    }
}
void Soundbar::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::OpenHandCursor);
}

void Soundbar::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    unsetCursor();
}

void Soundbar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void Soundbar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging)
    {
        QPoint diff = event->pos() - m_dragStartPos;
        move(pos() + diff);

        // 打印当前窗口相对于父窗口的位置

        // DEBUG_COUT("Moving position:" << pos());
    }
}

void Soundbar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);

        // 打印最终位置
        // qDebug() << "Final position:" << pos();
    }
}
