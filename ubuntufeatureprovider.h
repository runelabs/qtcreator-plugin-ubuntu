/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Juhapekka Piiroinen <juhapekka.piiroinen@canonical.com>
 */

#ifndef UBUNTUFEATUREPROVIDER_H
#define UBUNTUFEATUREPROVIDER_H

#include <coreplugin/featureprovider.h>
#include "ubuntuversionmanager.h"

namespace Ubuntu {
namespace Internal {
class UbuntuFeatureProvider : public Core::IFeatureProvider
{
    Q_OBJECT

public:
    UbuntuFeatureProvider() {}

    Core::FeatureSet availableFeatures(const QString &platformName) const;
    QStringList availablePlatforms() const;
    QString displayNameForPlatform(const QString &string) const;
    
};
}
}

#endif // UBUNTUFEATUREPROVIDER_H
