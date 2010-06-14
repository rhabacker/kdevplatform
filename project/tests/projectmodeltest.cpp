/***************************************************************************
 *   Copyright 2010 Andreas Pakulat <apaku@gmx.de>                         *
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

#include "projectmodeltest.h"
#include <QtTest/QTest>
#include <qtest_kde.h>

#include <projectmodel.h>

using KDevelop::ProjectModel;
using KDevelop::ProjectBaseItem;
using KDevelop::ProjectFolderItem;
using KDevelop::ProjectBuildFolderItem;

void ProjectModelTest::initTestCase()
{
    model = new ProjectModel( this );
}

void ProjectModelTest::init()
{
    model->clear();
}

void ProjectModelTest::testCreateFolders()
{
    QFETCH( int, itemType );
    QFETCH( KUrl, itemUrl );
    QFETCH( KUrl, expectedItemUrl );
    QFETCH( QString, expectedItemText );
    QFETCH( QStringList, expectedItemPath );
    QFETCH( int, expectedItemRow );

    ProjectBaseItem* newitem = 0;
    switch( itemType ) {
        case ProjectBaseItem::Folder:
            newitem = new ProjectFolderItem( 0, itemUrl );
            break;
        case ProjectBaseItem::BuildFolder:
            newitem = new ProjectBuildFolderItem( 0, itemUrl );
            break;
    }
    int origRowCount = model->rowCount();
    model->appendRow( newitem );
    QCOMPARE( model->rowCount(), origRowCount+1 );
    QCOMPARE( newitem->row(), expectedItemRow );
    QModelIndex idx = model->index( expectedItemRow, 0, QModelIndex() );
    QVERIFY( model->itemFromIndex( idx ) );
    QCOMPARE( model->itemFromIndex( idx ), newitem );
    QCOMPARE( newitem->text(), expectedItemText );
    QCOMPARE( newitem->url(), expectedItemUrl );
    QCOMPARE( newitem->type(), itemType );
    QCOMPARE( model->data( idx ).toString(), expectedItemText );
    QCOMPARE( model->indexFromItem( newitem ), idx );
    QCOMPARE( model->pathFromIndex( idx ), expectedItemPath );
    QCOMPARE( model->pathToIndex( expectedItemPath ), idx );
}

void ProjectModelTest::testCreateFolders_data()
{
    QTest::addColumn<int>( "itemType" );
    QTest::addColumn<KUrl>( "itemUrl" );
    QTest::addColumn<KUrl>( "expectedItemUrl" );
    QTest::addColumn<QString>( "expectedItemText" );
    QTest::addColumn<QStringList>( "expectedItemPath" );
    QTest::addColumn<int>( "expectedItemRow" );

    QTest::newRow("RootFolder")
        << (int)ProjectBaseItem::Folder
        << KUrl("file:///rootdir")
        << KUrl("file:///rootdir")
        << QString::fromLatin1("rootdir")
        << ( QStringList() << "rootdir" )
        << 0;

    QTest::newRow("RootBuildFolder")
        << (int)ProjectBaseItem::BuildFolder
        << KUrl("file:///rootdir")
        << KUrl("file:///rootdir")
        << QString::fromLatin1("rootdir")
        << ( QStringList() << "rootdir" )
        << 0;

}

QTEST_KDEMAIN( ProjectModelTest, GUI)
#include "projectmodeltest.moc"
