/***************************************************************************
 *   Copyright 2011 Yannick Motta   <yannick.motta@gmail.com>              *
 *                  Martin Heide    <martin.heide@gmx.net>                 *
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
#include "ibuddydocumentfinder.h"

#include <QMap>

namespace KDevelop {

//Our private data is entirely static, so don't need to create an
//instance of the private data class
struct IBuddyDocumentFinder::Private
{
    static QMap<QString, IBuddyDocumentFinder*>& finders()
    {
        static QMap<QString, IBuddyDocumentFinder*> finders;
        return finders;
    }
};

// ---------------- "Registry" interface --------------------------------------------
void IBuddyDocumentFinder::addFinder(const QString& mimeType, IBuddyDocumentFinder *finder)
{
    Private::finders()[mimeType] = finder;
}

void IBuddyDocumentFinder::removeFinder(const QString& mimeType)
{
    Private::finders().remove(mimeType);
}

IBuddyDocumentFinder* IBuddyDocumentFinder::finderForMimeType(const QString& mimeType)
{
    return Private::finders().value(mimeType, nullptr);
}

}
