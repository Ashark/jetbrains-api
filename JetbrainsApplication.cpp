#include "JetbrainsApplication.h"
#include "ConfigKeys.h"
#include <KSharedConfig>
#include <KConfigCore/KConfigGroup>
#include <QtGui/QtGui>

JetbrainsApplication::JetbrainsApplication(const QString &desktopFilePath, bool fileWatcher) :
        QFileSystemWatcher(nullptr), fileWatcher(fileWatcher),desktopFilePath(desktopFilePath) {
    KConfigGroup config = KSharedConfig::openConfig(desktopFilePath)->group("Desktop Entry");
    iconPath = config.readEntry("Icon");
    executablePath = config.readEntry("Exec").remove("%u").remove("%f");
    name = config.readEntry("Name");
    shortName = QString(name)
            .remove(" Edition")
            .remove(" Professional")
            .remove(" Ultimate")
            .remove(" + JBR11")
            .remove(" RC")
            .remove(" EAP")
            .replace("Community", "CE");

    // Allow the user to search for both names like Android Studio
    auto nameList = filterApplicationName(QString(name))
            .remove(" Professional")
            .remove(" Community")
            .remove(" Ultimate")
            .remove(" Edu")
            .split(" ");
    if (nameList.count() > 0) {
        nameArray[0] = nameList.at(0);
        if (nameList.count() == 2) {
            nameArray[1] = nameList.at(1);
        }
    }
    if (fileWatcher) {
        connect(this, &QFileSystemWatcher::fileChanged, this, &JetbrainsApplication::configChanged);
    }
}


void JetbrainsApplication::parseXMLFile(QString content, QString *debugMessage) {
    // Recent folders are in recentProjectDirectories.xml or in recentProjects.xml located
    // If the method is triggered by the file watcher the content is provided
    if (content.isEmpty()) {
        if (this->configFolder.isEmpty()) return;
        QFile f(this->configFolder + "recentProjectDirectories.xml");
        if (!f.exists()) {
            QFile f2(this->configFolder + "recentProjects.xml");
            if (!f2.open(QIODevice::ReadOnly)) {
                f2.close();
                if (debugMessage) {
                    debugMessage->append("No config file found for " + this->name + " " + this->desktopFilePath + "\n");
                }
                return;
            }
            if (fileWatcher) {
                this->addPath(f2.fileName());
            }
            if (debugMessage) {
                debugMessage->append("Config file found for " + this->name + " " + f2.fileName() + "\n");
            }
            content = f2.readAll();
            f2.close();
        } else {
            if (f.open(QIODevice::ReadOnly)) {
                content = f.readAll();
                f.close();
                if (fileWatcher) {
                    this->addPath(f.fileName());
                }
                if (debugMessage) {
                    debugMessage->append("Config file found for " + this->name + " " + f.fileName() + "\n");
                }
            }
        }
    }

    if (content.isEmpty()) return;

    // Go to RecentDirectoryProjectsManager component
    QXmlStreamReader reader(content);
    reader.readNextStartElement();
    reader.readNextStartElement();

    // Go through elements until the recentPaths option element is selected
    for (int i = 0; i < 4; ++i) {
        reader.readNextStartElement();
        if (reader.name() != QLatin1String("option")
        || reader.attributes().value(QLatin1String("name")) != QLatin1String("recentPaths")) {
            reader.skipCurrentElement();
        } else {
            reader.name();
            break;
        }
    }

    // Extract paths from XML element
    while (reader.readNextStartElement()) {
        if (reader.name() == QLatin1String("option")) {
            QString recentPath = reader.attributes()
                    .value(QLatin1String("value"))
                    .toString()
                    .replace(QLatin1String("$USER_HOME$"), QDir::homePath());
            if (QDir(recentPath).exists()) {
                this->recentlyUsed.append(recentPath);
            }
            reader.readElementText();
        }
    }
    if (debugMessage) {
        debugMessage->append("=====Recently used project folder for " + this->name + "=====\n");
        if (!recentlyUsed.isEmpty()) {
            for (const auto &recent: qAsConst(recentlyUsed)) {
                debugMessage->append("    " + recent + "\n");
            }
        } else {
            debugMessage->append("    NO PROJECTS\n");
        }
        debugMessage->append("\n");
    }
}

void JetbrainsApplication::parseXMLFiles(QList<JetbrainsApplication *> &apps, QString *debugMessage) {
    for (auto &app: qAsConst(apps)) {
        app->parseXMLFile(QString(), debugMessage);
    }
}

QList<JetbrainsApplication *> JetbrainsApplication::filterApps(QList<JetbrainsApplication *> &apps, QString *debugMessage) {
    QList<JetbrainsApplication *> notEmpty;
    if (debugMessage) {
        debugMessage->append("========== Filter Jetbrains Apps ==========\n");
    }
    for (auto const &app: qAsConst(apps)) {
        if (!app->recentlyUsed.empty()) {
            notEmpty.append(app);
        } else if (debugMessage) {
            debugMessage->append("Found not projects for: " + app->name + "\n");
            delete app;
        }
    }
    return notEmpty;
}

QStringList JetbrainsApplication::getAdditionalDesktopFileLocations() {
    const QStringList additionalDesktopFileLocations = {
            // AUR applications
            QStringLiteral("/usr/share/applications/rubymine.desktop"),
            QStringLiteral("/usr/share/applications/pycharm-professional.desktop"),
            QStringLiteral("/usr/share/applications/pycharm-eap.desktop"),
            QStringLiteral("/usr/share/applications/charm.desktop"),
            QStringLiteral("/usr/share/applications/rider.desktop"),
            // Snap applications
            QStringLiteral("/var/lib/snapd/desktop/applications/clion_clion.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/datagrip_datagrip.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/goland_goland.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/pycharm-community_pycharm-community.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/pycharm-educational_pycharm-educational.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/pycharm-professional_pycharm-professional.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/rubymine_rubymine.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/webstorm_webstorm.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/intellij-idea-community_intellij-idea-community.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/intellij-idea-educational_intellij-idea-educational.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/intellij-idea-ultimate_intellij-idea-ultimate.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/phpstorm_phpstorm.desktop"),
            QStringLiteral("/var/lib/snapd/desktop/applications/rider_rider.desktop"),
    };
    QStringList validFiles;
    for (const auto &additionalFile: additionalDesktopFileLocations) {
        if (QFile::exists(additionalFile)) {
            validFiles.append(additionalFile);
        }
    }

    return validFiles;
}

QMap<QString, QString>
JetbrainsApplication::getInstalledApplicationPaths(const KConfigGroup &customMappingConfig, QString *debugMessage) {
    QMap<QString, QString> applicationPaths;

    // Manually, locally or with Toolbox installed
    const QString localPath = QDir::homePath() + "/.local/share/applications/";
    const QDir localJetbrainsApplications(localPath, {"jetbrains-*"});
    if (debugMessage) {
        debugMessage->append("========== Locally Installed Jetbrains Applications ==========\n");
    }
    auto entries = localJetbrainsApplications.entryList();
    entries.removeOne("jetbrains-toolbox.desktop");
    for (const auto &item: qAsConst(entries)) {
        if (!item.isEmpty()) {
            applicationPaths.insert(filterApplicationName(KSharedConfig::openConfig(localPath + item)->
                    group("Desktop Entry").readEntry("Name")), localPath + item);
            if (debugMessage) {
                debugMessage->append(localPath + item + "\n");
            }
        }
    }
    // Globally installed
    const QString globalPath = "/usr/share/applications/";
    const QDir globalJetbrainsApplications(globalPath, {"jetbrains-*"});
    if (debugMessage) {
        debugMessage->append("========== Globally Installed Jetbrains Applications ==========\n");
    }
    auto globalEntries = globalJetbrainsApplications.entryList();
    globalEntries.removeOne("jetbrains-toolbox.desktop");
    for (const auto &item: qAsConst(globalEntries)) {
        if (!item.isEmpty()) {
            applicationPaths.insert(filterApplicationName(KSharedConfig::openConfig(globalPath + item)->
                    group("Desktop Entry").readEntry("Name")), globalPath + item);
            if (debugMessage) {
                debugMessage->append(globalPath + item + "\n");
            }
        }
    }

    // AUR/Snap/Other  installed
    if (debugMessage) {
        debugMessage->append("========== AUR/Snap/Other Installed Jetbrains Applications ==========\n");
    }
    for (const auto &item : getAdditionalDesktopFileLocations()) {
        if (!item.isEmpty()) {
            applicationPaths.insert(filterApplicationName(KSharedConfig::openConfig(item)->
                    group("Desktop Entry").readEntry("Name")), item);
            if (debugMessage) {
                debugMessage->append(item + "\n");
            }
        }
    }

    // Add manually configured entries
    if (debugMessage) {
        debugMessage->append("========== Manually Configured Jetbrains Applications ==========\n");
    }
    if (!customMappingConfig.isValid()) {
        return applicationPaths;
    }
    for (const auto &mappingEntry: customMappingConfig.entryMap().toStdMap()) {
        if (QFile::exists(mappingEntry.first) && QFile::exists(mappingEntry.second)) {
            applicationPaths.insert(filterApplicationName(KSharedConfig::openConfig(mappingEntry.first)->
                    group("Desktop Entry").readEntry("Name")), mappingEntry.first);
            if (debugMessage) {
                debugMessage->append(mappingEntry.first + "\n");
                debugMessage->append(mappingEntry.second + "\n");
            }
        }
    }

    return applicationPaths;
}

QString JetbrainsApplication::filterApplicationName(const QString &name) {
    return QString(name)
        .remove(QLatin1String(" Release"))
        .remove(QLatin1String(" Edition"))
        .remove(QLatin1String(" + JBR11"))
        .remove(QLatin1String(" RC"))
        .remove(QLatin1String(" EAP"));
}

QString JetbrainsApplication::formatOptionText(const QString &formatText, const QString &dir, const QString &path) {
    QString txt = QString(formatText)
        .replace(QLatin1String(FormatString::PROJECT), dir)
        .replace(QLatin1String(FormatString::APPNAME), this->name)
        .replace(QLatin1String(FormatString::APP), this->shortName);
    if (txt.contains(QLatin1String(FormatString::DIR))) {
        txt.replace(QLatin1String(FormatString::DIR), QString(path).replace(QDir::homePath(), QLatin1String("~")));
    }
    return txt;
}

QDebug operator<<(QDebug d, const JetbrainsApplication *app)
{
    return d
        << " name: " << app->name
        << " desktopFilePath: " << app->desktopFilePath
        << " executablePath: " << app->executablePath
        << " configFolder: " << app->configFolder
        << " iconPath: " << app->iconPath
        << " shortName: " << app->shortName
        << " recentlyUsed: " << app->recentlyUsed;
}
