/* This file is part of KDevelop
Copyright 2008 Andreas Pakulat <apaku@gmx.de>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#include "session.h"

#include <QDir>
#include <QUrl>

#include <KConfigGroup>

#include <interfaces/iplugincontroller.h>
#include <interfaces/iplugin.h>
#include "core.h"
#include "sessioncontroller.h"
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <serialization/itemrepository.h>

namespace KDevelop
{

const QString Session::cfgSessionNameEntry = QStringLiteral("SessionName");
const QString Session::cfgSessionDescriptionEntry = QStringLiteral("SessionPrettyContents");
const QString Session::cfgSessionProjectsEntry = QStringLiteral("Open Projects");
const QString Session::cfgSessionOptionsGroup = QStringLiteral("General Options");

class SessionPrivate
{
public:
    SessionInfo info;
    Session* q;
    bool isTemporary;

    QUrl pluginArea( const IPlugin* plugin )
    {
        QString name = Core::self()->pluginController()->pluginInfo(plugin).pluginId();
        QUrl url = QUrl::fromLocalFile( info.path + '/' + name );
        if( !QFile::exists( url.toLocalFile() ) ) {
            QDir( info.path ).mkdir( name );
        }
        return url;
    }

    SessionPrivate( Session* session, const QString& id )
        : info( Session::parse( id, true ) )
        , q( session )
        , isTemporary( false )
    {
    }

    void updateDescription()
    {
        buildDescription( info );
        emit q->sessionUpdated( q );
    }

    static QString generatePrettyContents( const SessionInfo& info );
    static QString generateDescription( const SessionInfo& info );
    static void buildDescription( SessionInfo& info );
};

Session::Session( const QString& id, QObject* parent )
        : ISession(parent)
        , d( new SessionPrivate( this, id ) )
{
}

Session::~Session()
{
    delete d;
}

QString Session::name() const
{
    return d->info.name;
}

QList<QUrl> Session::containedProjects() const
{
    return d->info.projects;
}

QString Session::description() const
{
    return d->info.description;
}

QUrl Session::pluginDataArea( const IPlugin* p )
{
    return d->pluginArea( p );
}

KSharedConfigPtr Session::config()
{
    return d->info.config;
}

QUuid Session::id() const
{
    return d->info.uuid;
}

void Session::setName( const QString& newname )
{
    d->info.name = newname;
    d->info.config->group( QString() ).writeEntry( cfgSessionNameEntry, newname );
    d->updateDescription();
}

void Session::setContainedProjects( const QList<QUrl>& projects )
{
    d->info.projects = projects;
    d->info.config->group( cfgSessionOptionsGroup ).writeEntry( cfgSessionProjectsEntry, projects );
    d->updateDescription();
}

void Session::setTemporary(bool temp)
{
    d->isTemporary = temp;
}

bool Session::isTemporary() const
{
    return d->isTemporary;
}

QString Session::path() const
{
    return d->info.path;
}

QString SessionPrivate::generatePrettyContents( const SessionInfo& info )
{
    if( info.projects.isEmpty() )
        return QString();

    QStringList projectNames;
    projectNames.reserve( info.projects.size() );

    foreach( const QUrl& url, info.projects ) {
        IProject* project = nullptr;
        if( ICore::self() && ICore::self()->projectController() ) {
            project = ICore::self()->projectController()->findProjectForUrl( url );
        }

        if( project ) {
            projectNames << project->name();
        } else {
            QString projectName = url.fileName();
            projectName.remove( QRegExp( "\\.kdev4$", Qt::CaseInsensitive ) );
            projectNames << projectName;
        }
    }

    if( projectNames.isEmpty() ) {
        return i18n("(no projects)");
    } else {
        return projectNames.join( QStringLiteral(", ") );
    }
}

QString SessionPrivate::generateDescription( const SessionInfo& info )
{
    QString prettyContentsFormatted = generatePrettyContents( info );
    QString description;

    if( info.name.isEmpty() ) {
        description = prettyContentsFormatted;
    } else {
        description = info.name + ":  " + prettyContentsFormatted;
    }

    return description;
}

void SessionPrivate::buildDescription( SessionInfo& info )
{
    QString description = generateDescription( info );

    info.description = description;
    info.config->group( QString() ).writeEntry( Session::cfgSessionDescriptionEntry, description );
    info.config->sync();
}

SessionInfo Session::parse( const QString& id, bool mkdir )
{
    SessionInfo ret;
    QString sessionPath = SessionController::sessionDirectory(id);

    QDir sessionDir( sessionPath );
    if( !sessionDir.exists() ) {
        if( mkdir ) {
            sessionDir.mkpath(sessionPath);
            Q_ASSERT( sessionDir.exists() );
        } else {
            return ret;
        }
    }

    ret.uuid = id;
    ret.path = sessionPath;
    ret.config = KSharedConfig::openConfig( sessionPath + "/sessionrc" );

    KConfigGroup cfgRootGroup = ret.config->group( QString() );
    KConfigGroup cfgOptionsGroup = ret.config->group( cfgSessionOptionsGroup );

    ret.name = cfgRootGroup.readEntry( cfgSessionNameEntry, QString() );
    ret.projects = cfgOptionsGroup.readEntry( cfgSessionProjectsEntry, QList<QUrl>() );
    SessionPrivate::buildDescription( ret );

    return ret;
}

}

