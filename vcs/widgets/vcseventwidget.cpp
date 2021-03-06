/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>                         *
 *   Copyright 2007 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "vcseventwidget.h"

#include <QAction>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

#include <KLocalizedString>

#include <interfaces/iplugin.h>

#include "ui_vcseventwidget.h"
#include "vcsdiffwidget.h"

#include "../interfaces/ibasicversioncontrol.h"
#include "../models/vcseventmodel.h"
#include "../models/vcsitemeventmodel.h"
#include "../debug.h"
#include "../vcsevent.h"
#include "../vcsjob.h"
#include "../vcsrevision.h"


namespace KDevelop
{

class VcsEventWidgetPrivate
{
public:
    explicit VcsEventWidgetPrivate( VcsEventWidget* w )
        : q( w )
    {
        m_copyAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy revision number"), q);
        m_copyAction->setShortcut(Qt::ControlModifier+Qt::Key_C);
        QObject::connect(m_copyAction, &QAction::triggered, q, [&] { copyRevision(); });
    }

    Ui::VcsEventWidget* m_ui;
    VcsItemEventModel* m_detailModel;
    VcsEventLogModel *m_logModel;
    QUrl m_url;
    QModelIndex m_contextIndex;
    VcsEventWidget* q;
    QAction* m_copyAction;
    IBasicVersionControl* m_iface;
    void eventViewCustomContextMenuRequested( const QPoint &point );
    void eventViewClicked( const QModelIndex &index );
    void jobReceivedResults( KDevelop::VcsJob* job );
    void copyRevision();
    void diffToPrevious();
    void diffRevisions();
    void currentRowChanged(const QModelIndex& start, const QModelIndex& end);
};

void VcsEventWidgetPrivate::eventViewCustomContextMenuRequested( const QPoint &point )
{
    m_contextIndex = m_ui->eventView->indexAt( point );
    if( !m_contextIndex.isValid() ){
        qCDebug(VCS) << "contextMenu is not in TreeView";
        return;
    }

    QMenu menu( m_ui->eventView );
    menu.addAction(m_copyAction);
    menu.addAction(i18n("Diff to previous revision"), q, SLOT(diffToPrevious()));
    QAction* action = menu.addAction(i18n("Diff between revisions"), q, SLOT(diffRevisions()));
    action->setEnabled(m_ui->eventView->selectionModel()->selectedRows().size()>=2);

    menu.exec( m_ui->eventView->viewport()->mapToGlobal(point) );
}

void VcsEventWidgetPrivate::currentRowChanged(const QModelIndex& start, const QModelIndex& end)
{
    Q_UNUSED(end);
    if(start.isValid())
        eventViewClicked(start);
}

void VcsEventWidgetPrivate::eventViewClicked( const QModelIndex &index )
{
    KDevelop::VcsEvent ev = m_logModel->eventForIndex( index );
    m_detailModel->removeRows(0, m_detailModel->rowCount());

    if( ev.revision().revisionType() != KDevelop::VcsRevision::Invalid )
    {
        m_ui->itemEventView->setEnabled(true);
        m_ui->message->setEnabled(true);
        m_ui->message->setPlainText( ev.message() );
        m_detailModel->addItemEvents( ev.items() );
    }else
    {
        m_ui->itemEventView->setEnabled(false);
        m_ui->message->setEnabled(false);
        m_ui->message->clear();
    }

    QHeaderView* header = m_ui->itemEventView->header();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setStretchLastSection(true);
}

void VcsEventWidgetPrivate::copyRevision()
{
    qApp->clipboard()->setText(m_contextIndex.sibling(m_contextIndex.row(), 0).data().toString());
}

void VcsEventWidgetPrivate::diffToPrevious()
{
    KDevelop::VcsEvent ev = m_logModel->eventForIndex( m_contextIndex );
    KDevelop::VcsRevision prev = KDevelop::VcsRevision::createSpecialRevision(KDevelop::VcsRevision::Previous);
    KDevelop::VcsJob* job = m_iface->diff( m_url, prev, ev.revision() );

    VcsDiffWidget* widget = new VcsDiffWidget( job );
    widget->setRevisions( prev, ev.revision() );
    QDialog* dlg = new QDialog( q );

    widget->connect(widget, &VcsDiffWidget::destroyed, dlg, &QDialog::deleteLater);

    dlg->setWindowTitle( i18n("Difference To Previous") );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    auto mainWidget = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout;
    dlg->setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    dlg->connect(buttonBox, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    dlg->connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    mainLayout->addWidget(widget);
    mainLayout->addWidget(buttonBox);

    dlg->show();
}

void VcsEventWidgetPrivate::diffRevisions()
{
    QModelIndexList l = m_ui->eventView->selectionModel()->selectedRows();
    KDevelop::VcsEvent ev1 = m_logModel->eventForIndex( l.first() );
    KDevelop::VcsEvent ev2 = m_logModel->eventForIndex( l.last() );
    KDevelop::VcsJob* job = m_iface->diff( m_url, ev1.revision(), ev2.revision() );

    VcsDiffWidget* widget = new VcsDiffWidget( job );
    widget->setRevisions( ev1.revision(), ev2.revision() );

    auto dlg = new QDialog( q );
    dlg->setWindowTitle( i18n("Difference between Revisions") );

    widget->connect(widget, &VcsDiffWidget::destroyed, dlg, &QDialog::deleteLater);

    auto mainLayout = new QVBoxLayout(dlg);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    dlg->connect(buttonBox, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    dlg->connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    mainLayout->addWidget(widget);
    dlg->show();
}

VcsEventWidget::VcsEventWidget( const QUrl& url, const VcsRevision& rev, KDevelop::IBasicVersionControl* iface, QWidget* parent )
    : QWidget(parent), d(new VcsEventWidgetPrivate(this) )
{
    d->m_iface = iface;
    d->m_url = url;
    d->m_ui = new Ui::VcsEventWidget();
    d->m_ui->setupUi(this);

    d->m_logModel = new VcsEventLogModel(iface, rev, url, this);
    d->m_ui->eventView->setModel( d->m_logModel );
    d->m_ui->eventView->sortByColumn(0, Qt::DescendingOrder);
    d->m_ui->eventView->setContextMenuPolicy( Qt::CustomContextMenu );
    QHeaderView* header = d->m_ui->eventView->header();
    header->setSectionResizeMode( 0, QHeaderView::ResizeToContents );
    header->setSectionResizeMode( 1, QHeaderView::Stretch );
    header->setSectionResizeMode( 2, QHeaderView::ResizeToContents );
    header->setSectionResizeMode( 3, QHeaderView::ResizeToContents );
    // Select first row as soon as the model got populated
    connect(d->m_logModel, &QAbstractItemModel::rowsInserted, this, [this]() {
        auto view = d->m_ui->eventView;
        view->setCurrentIndex(view->model()->index(0, 0));
    });

    d->m_detailModel = new VcsItemEventModel(this);
    d->m_ui->itemEventView->setModel( d->m_detailModel );

    connect( d->m_ui->eventView, &QTreeView::clicked,
             this, [&] (const QModelIndex& index) { d->eventViewClicked(index); } );
    connect( d->m_ui->eventView->selectionModel(), &QItemSelectionModel::currentRowChanged,
             this, [&] (const QModelIndex& start, const QModelIndex& end) { d->currentRowChanged(start, end); });
    connect( d->m_ui->eventView, &QTreeView::customContextMenuRequested,
             this, [&] (const QPoint& point) { d->eventViewCustomContextMenuRequested(point); } );
}

VcsEventWidget::~VcsEventWidget()
{
    delete d->m_ui;
    delete d;
}

}


#include "moc_vcseventwidget.cpp"
