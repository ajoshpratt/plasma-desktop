/*
 * Copyright 2017 Xuetian Weng <wengxt@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "mousesettings.h"

#include "mousebackend.h"
#include <KSharedConfig>
#include <KConfigGroup>
#include <KGlobalSettings>

#include "../migrationlib/kdelibs4config.h"

void MouseSettings::apply(MouseBackend* backend, bool force)
{
    if (!backend) {
        return;
    }

    backend->apply(*this, force);
    handedNeedsApply = false;
}

void MouseSettings::load(KConfig *config, MouseBackend *backend)
{
    // TODO: what's a good threshold default value
    int threshold = 0;
    handed = MouseHanded::Right;
    double accel = 1.0;
    QString profile;
    if (backend) {
        backend->load();
        auto handedOnServer = backend->handed();
        handedEnabled = handedOnServer != MouseHanded::NotSupported;
        if (handedEnabled) {
            handed = handedOnServer;
        }
        accel = backend->accelRate();
        threshold = backend->threshold();
    }

    KConfigGroup group = config->group("Mouse");
    double a = group.readEntry("Acceleration", -1.0);
    if (a == -1)
        accelRate = accel;
    else
        accelRate = a;

    int t = group.readEntry("Threshold", -1);
    if (t == -1)
        thresholdMove = threshold;
    else
        thresholdMove = t;

    QString key = group.readEntry("MouseButtonMapping");
    if (key == "RightHanded")
        handed = MouseHanded::Right;
    else if (key == "LeftHanded")
        handed = MouseHanded::Left;
    reverseScrollPolarity = group.readEntry("ReverseScrollPolarity", false);
    handedNeedsApply = false;

    // SC/DC/AutoSelect/ChangeCursor
    group = config->group("KDE");
    doubleClickInterval = group.readEntry("DoubleClickInterval", 400);
    dragStartTime = group.readEntry("StartDragTime", 500);
    dragStartDist = group.readEntry("StartDragDist", 4);
    wheelScrollLines = group.readEntry("WheelScrollLines", 3);

    singleClick = group.readEntry("SingleClick", KDE_DEFAULT_SINGLECLICK);
}

void MouseSettings::save(KConfig *config)
{
    KSharedConfig::Ptr kcminputProfile = KSharedConfig::openConfig("kcminputrc");
    KConfigGroup kcminputGroup(kcminputProfile, "Mouse");
    kcminputGroup.writeEntry("Acceleration",accelRate);
    kcminputGroup.writeEntry("Threshold",thresholdMove);
    if (handed == MouseHanded::Right) {
        kcminputGroup.writeEntry("MouseButtonMapping",QString("RightHanded"));
    } else {
        kcminputGroup.writeEntry("MouseButtonMapping",QString("LeftHanded"));
    }
    kcminputGroup.writeEntry("ReverseScrollPolarity", reverseScrollPolarity);
    kcminputGroup.sync();

    KSharedConfig::Ptr profile = KSharedConfig::openConfig("kdeglobals");
    KConfigGroup group(profile, "KDE");
    group.writeEntry("DoubleClickInterval", doubleClickInterval, KConfig::Persistent);
    group.writeEntry("StartDragTime", dragStartTime, KConfig::Persistent);
    group.writeEntry("StartDragDist", dragStartDist, KConfig::Persistent);
    group.writeEntry("WheelScrollLines", wheelScrollLines, KConfig::Persistent);
    group.writeEntry("SingleClick", singleClick, KConfig::Persistent);

    group.sync();
    config->sync();

    Kdelibs4SharedConfig::syncConfigGroup(QLatin1String("Mouse"), "kcminputrc");
    Kdelibs4SharedConfig::syncConfigGroup(QLatin1String("KDE"), "kdeglobals");

    KGlobalSettings::self()->emitChange(KGlobalSettings::SettingsChanged, KGlobalSettings::SETTINGS_MOUSE);
}
