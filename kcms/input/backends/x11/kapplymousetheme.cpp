/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 * Copyright (c) 2005 Lubos Lunak <l.lunak@kde.org>
 * Copyright (c) 2017 Xuetian Weng <wengxt@gmail.com>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <QGuiApplication>
#include <QFile>

#include "mousebackend.h"

int main( int argc, char* argv[] )
{
    int ret = 0;
    QGuiApplication app(argc, argv);
    if( argc != 3 )
        return 1;
    QString theme = QFile::decodeName(argv[ 1 ]);
    QString size = QFile::decodeName(argv[ 2 ]);
    auto backend = MouseBackend::implementation();
    if (!backend || !backend->isValid()) {
        return 2;
    }

    // Note: If you update this code, update main.cpp as well.

    // use a default value for theme only if it's not configured at all, not even in X resources
    if(theme.isEmpty() && backend->currentCursorTheme().isEmpty())
    {
        theme = "breeze_cursors";
        ret = 10; // means to switch to default
    }

    backend->applyCursorTheme(theme, size.toInt());
    return ret;
}
