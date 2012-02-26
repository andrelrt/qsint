#include "barchartplotter.h"
#include "axisbase.h"


namespace QSint
{


BarChartPlotter::BarChartPlotter(QWidget *parent) :
    PlotterBase(parent)
{
    m_axisX = new AxisBase(Qt::Horizontal, this);
    m_axisY = new AxisBase(Qt::Vertical, this);

    setBarType(Stacked);

    m_axisX->setType(AxisBase::AxisModel);

    setBarSize(-INT_MAX, INT_MAX);
    setBarScale(0.5);
    setBarOpacity(1.0);

    setZeroLinePen(QPen(Qt::white));
}


void BarChartPlotter::setBarType(BarChartType type)
{
    m_type = type;
}


void BarChartPlotter::setBarSize(int min, int max)
{
    m_barsize_min = qMax(min, 0);
    m_barsize_max = qMax(min, max);
}


void BarChartPlotter::setBarScale(double scale)
{
    if (scale < 0.1)
        m_scale = 0.1;
    else
        m_scale = qMin(scale, 1.0);
}


void BarChartPlotter::setBarOpacity(double value)
{
    if (value < 0.0)
        m_opacity = 0;
    else
        m_opacity = qMin(value, 1.0);
}


void BarChartPlotter::setZeroLinePen(const QPen &pen)
{
    m_zeroLinePen = pen;
}


void BarChartPlotter::drawContent(QPainter &p)
{
    if (!m_model || !m_axisX || !m_axisY)
        return;

    int p_start, p_end;
    m_axisX->calculatePoints(p_start, p_end);

    // draw zero line
    int p_y = m_axisY->toView(0);

    p.setPen(m_zeroLinePen);
    p.drawLine(p_start, p_y, p_end, p_y);

    int count = m_model->columnCount();
    if (!count)
        return;

    int row_count = m_model->rowCount();
    if (!row_count)
        return;

    int p_offs = double(p_end - p_start) / count;

    int bar_size = p_offs * m_scale;

    if (bar_size > m_barsize_max)
        bar_size = qMin(m_barsize_max, p_offs);
    else if (bar_size < m_barsize_min)
        bar_size = qMin(m_barsize_min, p_offs);


    switch (m_type)
    {
    case Stacked:
        StackedBarPainter::draw(this, p, count, row_count, p_start, p_offs, bar_size);
        break;

    case Columns:
        ColumnBarPainter::draw(this, p, count, row_count, p_start, p_offs, bar_size);
        break;

    case Trend:
        TrendPainter::draw(this, p, count, row_count, p_start, p_offs, bar_size);
        break;

    } // switch
}


void BarChartPlotter::drawSegment(QPainter &p, QRect rect,
                                  const QModelIndex &index, double /*value*/,
                                  bool isHighlighted)
{
    if (isHighlighted)
    {
        p.setPen(highlightPen());
        p.setBrush(highlightBrush());
    }
    else
    {
        QBrush brush(qvariant_cast<QBrush>(model()->headerData(index.row(), Qt::Vertical, Qt::BackgroundRole)));

        p.setPen(itemPen());
        p.setBrush(brush);
    }

    if (barType() == Trend)
    {
        p.drawEllipse(rect.topLeft(), 3, 3);
        return;
    }

    p.drawRect(rect);
}


void BarChartPlotter::drawValue(QPainter &p, QRect rect,
                                  const QModelIndex &index, double value,
                                  bool isHighlighted)
{
    int flags = Qt::AlignCenter;
    QString text(QString::number(value));
    QRect textRect(p.fontMetrics().boundingRect(text));

    switch (barType())
    {
        case Columns:
            if (value < 0)
            {
                flags = Qt::AlignHCenter | Qt::AlignTop;
                rect.setTop(rect.bottom());
                rect.setHeight(100);
            }
            else
            {
                flags = Qt::AlignHCenter | Qt::AlignBottom;
                rect.setTop(rect.top()-100);
                rect.setHeight(100);
            }
            break;

        case Stacked:
            if (isHighlighted)
            {
                if (textRect.height() > rect.height() || textRect.width() > rect.width())
                {
                    drawHighlightedValueFrame(p, rect, textRect);
                }
            }
            break;

        case Trend:
            rect.setSize(textRect.size());
            rect.moveLeft(rect.left() - rect.width()/2);
            rect.moveTop(rect.top() - rect.height()/2);
            if (value < 0)
                rect.moveTop(rect.top() + rect.height());
            else
                rect.moveTop(rect.top() - rect.height());

            if (isHighlighted)
            {
                drawHighlightedValueFrame(p, rect, textRect);
            }
            break;
    }

    if (isHighlighted)
    {
        p.setPen(QPen(highlightTextColor()));
        //p.setBrush(highlightBrush());
    }
    else
    {
        QPen pen(qvariant_cast<QColor>(model()->headerData(index.row(), Qt::Vertical, Qt::ForegroundRole)));
        //QBrush brush(qvariant_cast<QBrush>(model()->headerData(index.row(), Qt::Vertical, Qt::BackgroundRole)));

        p.setPen(pen);
        //p.setBrush(brush);
    }

    p.drawText(rect, flags, text);
}


void BarChartPlotter::drawHighlightedValueFrame(QPainter &p, const QRect &rect, const QRect &textRect)
{
    p.setPen(QPen(highlightTextColor()));
    p.setBrush(highlightBrush());
    QRect backRect(textRect.adjusted(-3,-3,3,3));
    backRect.moveLeft(rect.center().x() - backRect.width()/2);
    backRect.moveTop(rect.center().y() - backRect.height()/2);
    p.drawRect(backRect);
}


void BarChartPlotter::BarPainter::drawBarItem(QPainter &p, QRect rect,
                                              const QPen &pen, const QBrush &brush,
                                              const QModelIndex &/*index*/, double /*value*/)
{
    p.setPen(pen);
    p.setBrush(brush);
    p.drawRect(rect);
}


void BarChartPlotter::BarPainter::drawValueText(QPainter &p, QRect rect, int flags,
                                                const QPen &pen, const QBrush &brush,
                                                const QModelIndex &/*index*/, double value)
{
    p.setPen(pen);
    p.setBrush(brush);
    p.drawText(rect, flags, QString::number(value));
}


void BarChartPlotter::StackedBarPainter::draw(
    BarChartPlotter *plotter,
    QPainter &p,
    int count,
    int row_count,
    int p_start,
    int p_offs,
    int bar_size)
{
    bool isHighlight = false;
    double valueHl;
    QModelIndex indexHl;
    QRect rectHl;

    for (int i = 0; i < count; i++)
    {
        int p_d = p_start + p_offs*i + (p_offs-bar_size)/2;

        double acc_value = 0;
        int p_y = plotter->axisY()->toView(0);

        double neg_value = 0;
        int p_ny = p_y;

        p.setOpacity(plotter->barOpacity());

        for (int j = 0; j < row_count; j++)
        {
            QRect itemRect;

            const QModelIndex index(plotter->model()->index(j, i));
            double value = plotter->model()->data(index).toDouble();
            if (value < 0)
            {
                neg_value += value;

                int p_h = plotter->axisY()->toView(neg_value);

                itemRect = QRect(p_d, p_ny, bar_size, p_h-p_ny);

                p_ny = p_h;
            }
            else
            {
                acc_value += value;

                int p_h = plotter->axisY()->toView(acc_value);

                itemRect = QRect(p_d, p_h, bar_size, p_y-p_h);

                p_y = p_h;
            }

            // check for highlight
            if (!isHighlight && !plotter->mousePos().isNull() && itemRect.contains(plotter->mousePos()))
            {
                isHighlight = true;
                valueHl = value;
                indexHl = index;
                rectHl = itemRect;
            }
            else
            {
                plotter->drawSegment(p, itemRect, index, value, false);
                plotter->drawValue(p, itemRect, index, value, false);
            }
        }
    }

    if (isHighlight)
    {
        plotter->setIndexUnderMouse(indexHl);

        plotter->drawSegment(p, rectHl, indexHl, valueHl, true);
        plotter->drawValue(p, rectHl, indexHl, valueHl, true);
    }
    else
        plotter->setIndexUnderMouse(QModelIndex());
}


void BarChartPlotter::ColumnBarPainter::draw(
    BarChartPlotter *plotter,
    QPainter &p,
    int count,
    int row_count,
    int p_start,
    int p_offs,
    int bar_size)
{
    int single_bar_size = bar_size/row_count;
    if (!single_bar_size)
        return;

    bool isHighlight = false;
    double valueHl;
    QModelIndex indexHl;
    QRect rectHl;

    for (int i = 0; i < count; i++)
    {
        int p_d = p_start + p_offs*i + (p_offs-bar_size)/2;

        int p_y = plotter->axisY()->toView(0);

        p.setOpacity(plotter->barOpacity());

        for (int j = 0; j < row_count; j++)
        {
            const QModelIndex index(plotter->model()->index(j, i));
            double value = plotter->model()->data(index).toDouble();

            int p_h = plotter->axisY()->toView(value);

            QRect itemRect;

            if (value < 0)
            {
                itemRect = QRect(p_d, p_y, single_bar_size, p_h-p_y);
            }
            else
            {
                itemRect = QRect(p_d, p_h, single_bar_size, p_y-p_h);
            }

            // check for highlight
            if (!isHighlight && !plotter->mousePos().isNull() && itemRect.contains(plotter->mousePos()))
            {
                isHighlight = true;
                valueHl = value;
                indexHl = index;
                rectHl = itemRect;
            }
            else
            {
                plotter->drawSegment(p, itemRect, index, value, false);
                plotter->drawValue(p, itemRect, index, value, false);
            }

            p_d += single_bar_size;
        }
    }

    if (isHighlight)
    {
        plotter->setIndexUnderMouse(indexHl);

        plotter->drawSegment(p, rectHl, indexHl, valueHl, true);
        plotter->drawValue(p, rectHl, indexHl, valueHl, true);
    }
    else
        plotter->setIndexUnderMouse(QModelIndex());
}


void BarChartPlotter::TrendPainter::draw(
    BarChartPlotter *plotter,
    QPainter &p,
    int count,
    int row_count,
    int p_start,
    int p_offs,
    int /*bar_size*/)
{
    bool isHighlight = false;
    double valueHl;
    QModelIndex indexHl;
    QRect rectHl;

    p.save();
    p.setRenderHint(QPainter::Antialiasing);

    p.setOpacity(plotter->barOpacity());

    p.setFont(plotter->font());

    for (int j = 0; j < row_count; j++)
    {
        QPen pen(qvariant_cast<QColor>(plotter->model()->headerData(j, Qt::Vertical, Qt::ForegroundRole)));
        QBrush brush(qvariant_cast<QBrush>(plotter->model()->headerData(j, Qt::Vertical, Qt::BackgroundRole)));

        QPolygon points;

        for (int i = 0; i < count; i++)
        {
            const QModelIndex index(plotter->model()->index(j, i));
            double value = plotter->model()->data(index).toDouble();

            int x = p_start + p_offs*i + p_offs/2;
            int y = plotter->axisY()->toView(value);
            QRect itemRect(x,y,1,1);

            points.append(itemRect.topLeft());

            // check for higlight
            if (!isHighlight && !plotter->mousePos().isNull() && QRect(x-3, y-3, 7, 7).contains(plotter->mousePos()))
            {
                isHighlight = true;
                valueHl = value;
                indexHl = index;
                rectHl = itemRect;
            }
            else
                plotter->drawValue(p, itemRect, index, value, false);
        }

        p.setPen(QPen(brush, 2));
        p.drawPolyline(points);

        //p.setPen(pen);
        //p.setBrush(brush);
        for (int i = 0; i < count; i++)
        {
            const QModelIndex index(plotter->model()->index(j, i));
            if (index != indexHl)
            {
                double value = plotter->model()->data(index).toDouble();
                plotter->drawSegment(p, QRect(points.at(i), QSize(1,1)), index, value, false);
            }

            //p.drawEllipse(points.at(i), 3, 3);
        }
    }

    if (isHighlight)
    {
        plotter->setIndexUnderMouse(indexHl);

        plotter->drawSegment(p, rectHl, indexHl, valueHl, true);
        plotter->drawValue(p, rectHl, indexHl, valueHl, true);
    }
    else
        plotter->setIndexUnderMouse(QModelIndex());

    p.restore();
}


} // namespace
