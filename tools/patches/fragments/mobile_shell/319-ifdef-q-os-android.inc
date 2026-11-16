#ifdef Q_OS_ANDROID
class MobileBrushOptionDelegate final : public KisCategorizedItemDelegate
{
public:
    using KisCategorizedItemDelegate::KisCategorizedItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const override
    {
        auto size = KisCategorizedItemDelegate::sizeHint(option, index);
        size.setHeight(qMax(43, size.height()));
        return size;
    }
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        if(!(index.flags() & Qt::ItemIsUserCheckable)) {
            KisCategorizedItemDelegate::paint(painter,option,index);
            return;
        }
        painter->save();
        auto *style=QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelItemViewItem,&option,painter);
        QStyleOptionButton checkbox;
        checkbox.rect=QRect(option.rect.left()+6,
                            option.rect.center().y()-13,26,26);
        checkbox.state=QStyle::State_Enabled |
            (index.data(Qt::CheckStateRole).toInt()==Qt::Checked?
                QStyle::State_On:QStyle::State_Off);
        style->drawPrimitive(QStyle::PE_IndicatorCheckBox,&checkbox,painter);
        const QRect caption=option.rect.adjusted(40,0,-6,0);
        painter->setPen(option.state&QStyle::State_Selected?
                        option.palette.highlightedText().color():
                        option.palette.text().color());
        painter->drawText(caption,Qt::AlignVCenter|Qt::AlignLeft,
            option.fontMetrics.elidedText(index.data().toString(),
                                          Qt::ElideRight,caption.width()));
        painter->restore();
    }
};

// The checkbox is a direct action; opening the remainder of the row
// continues to navigate to the native detailed configuration page.
class MobileBrushOptionTapFilter final : public QObject
{
public:
    explicit MobileBrushOptionTapFilter(KisCategorizedListView *view)
        : QObject(view->viewport()),m_view(view) {}
protected:
    bool eventFilter(QObject *,QEvent *event) override
    {
        const auto type=event->type();
        if(type==QEvent::MouseButtonPress) {
            auto *mouse=static_cast<QMouseEvent*>(event);
            if(mouse->button()!=Qt::LeftButton)return false;
            const QModelIndex index=m_view->indexAt(mouse->pos());
            const QRect row=m_view->visualRect(index);
            if(!index.isValid() ||
               !(index.flags()&Qt::ItemIsUserCheckable) ||
               mouse->pos().x()>row.left()+40)return false;
            m_pressed=index;
            m_start=mouse->pos();
            return true;
        }
        if(!m_pressed.isValid())return false;
        if(type==QEvent::MouseMove)return true;
        if(type==QEvent::MouseButtonRelease) {
            auto *mouse=static_cast<QMouseEvent*>(event);
            const QModelIndex index=m_view->indexAt(mouse->pos());
            if(index==m_pressed &&
               (mouse->pos()-m_start).manhattanLength()<
                   QApplication::startDragDistance()) {
                const bool checked=index.data(Qt::CheckStateRole).toInt()==Qt::Checked;
                m_view->model()->setData(index,
                    checked?Qt::Unchecked:Qt::Checked,Qt::CheckStateRole);
            }
            m_pressed=QPersistentModelIndex();
            return true;
        }
        return false;
    }
private:
    KisCategorizedListView *m_view=nullptr;
    QPersistentModelIndex m_pressed;
    QPoint m_start;
};
#endif

struct KisPaintOpSettingsWidget::Private