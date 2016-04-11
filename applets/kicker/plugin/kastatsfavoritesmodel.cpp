/***************************************************************************
 *   Copyright (C) 2014-2015 by Eike Hein <hein@kde.org>                   *
 *   Copyright (C) 2015 by Ivan Cukic <ivan.cukic@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "kastatsfavoritesmodel.h"
#include "appentry.h"
#include "contactentry.h"
#include "fileentry.h"
#include "actionlist.h"

#include <QDebug>

#include <KLocalizedString>
#include <KConfigGroup>

#include <KActivities/Stats/Terms>
#include <KActivities/Stats/Query>
#include <KActivities/Stats/ResultModel>

namespace KAStats = KActivities::Stats;

using namespace KAStats;
using namespace KAStats::Terms;

KAStatsFavoritesModel::KAStatsFavoritesModel(QObject *parent) : ForwardingModel(parent)
, m_enabled(true)
, m_maxFavorites(-1)
, m_dropPlaceholderIndex(-1)
, m_sourceModel(nullptr)
, m_config("TESTTEST")
{
    refresh();
}

KAStatsFavoritesModel::~KAStatsFavoritesModel()
{
}

QString KAStatsFavoritesModel::description() const
{
    return i18n("Favorites");
}

QVariant KAStatsFavoritesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }

    const QString url =
        ForwardingModel::data(index, ResultModel::ResourceRole).toString();

    qDebug() << "URL" << url;

    // const casts are bad, but we can not achieve this
    // with the standard 'mutable' members for lazy evaluation
    const AbstractEntry *entry =
        const_cast<KAStatsFavoritesModel*>(this)->favoriteFromId(url);

    if (!entry) {
        return "NULL!";
    }

    if (role == Qt::DisplayRole) {
        return entry->name();
    } else if (role == Qt::DecorationRole) {
        return entry->icon();
    } else if (role == Kicker::DescriptionRole) {
        return entry->description();
    } else if (role == Kicker::FavoriteIdRole) {
        return entry->id();
    } else if (role == Kicker::UrlRole) {
        return entry->url();
    } else if (role == Kicker::HasActionListRole) {
        return entry->hasActions();
    } else if (role == Kicker::ActionListRole) {
        return entry->actions();
    }

    return QVariant();
}

// int KAStatsFavoritesModel::rowCount(const QModelIndex& parent) const
// {
// }

bool KAStatsFavoritesModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    if (row < 0 || row >= rowCount()) {
        return false;
    }

    const QString url =
        ForwardingModel::data(index(row, 0), ResultModel::ResourceRole).toString();

    return m_entries.contains(url) ? m_entries[url]->run(actionId, argument)
                                   : false;
}

bool KAStatsFavoritesModel::enabled() const
{
    return m_enabled;
}

int KAStatsFavoritesModel::maxFavorites() const
{
    return m_maxFavorites;
}

void KAStatsFavoritesModel::setMaxFavorites(int max)
{
}

void KAStatsFavoritesModel::setEnabled(bool enable)
{
    if (m_enabled != enable) {
        m_enabled = enable;

        emit enabledChanged();
    }
}

QStringList KAStatsFavoritesModel::favorites() const
{
    return QStringList();
}

void KAStatsFavoritesModel::setFavorites(const QStringList& favorites)
{
}

bool KAStatsFavoritesModel::isFavorite(const QString &id) const
{
    qDebug() << "isFavorite" << id;
    return m_entries.contains(validateUrl(id));
}

void KAStatsFavoritesModel::addFavorite(const QString &id, int index)
{
    // TODO: Inspect where this is used
    Q_UNUSED(index)

    QString scheme;
    const QString url = validateUrl(id, &scheme);

    m_sourceModel->linkToActivity(
        QUrl(url), Activity::current(),
        Agent(agentForScheme(scheme)));
}

void KAStatsFavoritesModel::removeFavorite(const QString &id)
{
    QString scheme;
    const QString url = validateUrl(id, &scheme);

    m_sourceModel->unlinkFromActivity(
        QUrl(url), Activity::current(),
        Agent(agentForScheme(scheme)));
}

void KAStatsFavoritesModel::moveRow(int from, int to)
{
    const QString url =
        ForwardingModel::data(index(from, 0), ResultModel::ResourceRole).toString();

    qDebug() << "Moving " << url << "(" << from << ") to" << to;

    m_sourceModel->setResultPosition(validateUrl(url), to);
}

int KAStatsFavoritesModel::dropPlaceholderIndex() const
{
    return m_dropPlaceholderIndex;
}

void KAStatsFavoritesModel::setDropPlaceholderIndex(int index)
{
    if (m_dropPlaceholderIndex != index) {
        m_dropPlaceholderIndex = index;

    }
}

AbstractModel *KAStatsFavoritesModel::favoritesModel()
{
    return this;
}

void KAStatsFavoritesModel::refresh()
{
    qDebug() << "Refreshing the model";
    QObject *oldModel = sourceModel();

    auto query = LinkedResources
                    | Agent {
                        "org.kde.plasma.favorites.applications",
                        "org.kde.plasma.favorites.contacts"
                      }
                    | Type::any()
                    | Activity::current()
                    | Limit(15);

    m_sourceModel = new ResultModel(query, "org.kde.plasma.favorites");

    QModelIndex index;

    if (m_sourceModel->canFetchMore(index)) {
        m_sourceModel->fetchMore(index);
    }

    setSourceModel(m_sourceModel);

    delete oldModel;
}

AbstractEntry *KAStatsFavoritesModel::favoriteFromId(const QString &id)
{
    if (!m_entries.contains(id)) {
        const QUrl url(id);
        const QString &s = url.scheme();

        AbstractEntry *entry = nullptr;

        if (s == QStringLiteral("applications")
                || s == QStringLiteral("preferred")
                || (s.isEmpty() && id.contains(QStringLiteral(".desktop")))) {
            entry = new AppEntry(this, id);
        } else if (s == QStringLiteral("ktp")) {
            entry = new ContactEntry(this, id);
        } else if (url.isValid() && !url.scheme().isEmpty()) {
            entry = new FileEntry(this, url);
        }

        m_entries[id] = entry;
    }

    return m_entries[id];
}

QString KAStatsFavoritesModel::validateUrl(const QString &url, QString * scheme) const
{
    const QUrl qurl(url);

    QString s; // needed only when scheme is null

    if (!scheme) {
        scheme = &s;
    }

    *scheme = qurl.scheme();

    if (scheme->isEmpty() && url.contains(".desktop")) {
        *scheme = "applications";
        return "applications://" + url;

    } else {
        return url;
    }
}

QString KAStatsFavoritesModel::agentForScheme(const QString &scheme) const
{
    if (scheme == QStringLiteral("applications")
            || scheme == QStringLiteral("preferred")) {
        return "org.kde.plasma.favorites.applications";

    } else if (scheme == QStringLiteral("ktp")) {
        return "org.kde.plasma.favorites.contacts";

    }

    return QString();
}
