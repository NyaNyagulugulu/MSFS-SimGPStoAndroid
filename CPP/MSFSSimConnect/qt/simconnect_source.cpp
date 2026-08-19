#include "simconnect_source.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QNetworkProxy>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTimer>
#include <QUdpSocket>

#ifdef HAVE_SIMCONNECT
#include <SimConnect.h>

namespace {
struct NativeSimData {
    double latitude, longitude, altitude, heading;
    double pitch, roll, groundSpeed, airSpeed;
};

void CALLBACK simDispatch(SIMCONNECT_RECV *message, DWORD, void *context) {
    if (!message || message->dwID != SIMCONNECT_RECV_ID_SIMOBJECT_DATA) return;
    auto *source = static_cast<SimConnectSource *>(context);
    const auto *received = reinterpret_cast<SIMCONNECT_RECV_SIMOBJECT_DATA *>(message);
    const auto *native = reinterpret_cast<const NativeSimData *>(&received->dwData);
    FlightData data;
    data.latitude = native->latitude; data.longitude = native->longitude;
    data.altitude = native->altitude; data.heading = native->heading;
    data.pitch = native->pitch; data.roll = native->roll;
    data.groundSpeed = native->groundSpeed; data.airSpeed = native->airSpeed;
    emit source->dataReceived(data);
}
}
#else
namespace {
struct ProtonLaunchConfig {
    QString program;
    QStringList arguments;
    QProcessEnvironment environment;
    QString workingDirectory;
    QString description;
};

QStringList steamRoots() {
    QStringList roots;
    const auto add = [&roots](const QString &root) {
        if (!root.isEmpty() && !roots.contains(root) && QDir(root).exists()) roots.append(root);
    };
    add(qEnvironmentVariable("STEAM_ROOT"));
    add(qEnvironmentVariable("STEAM_COMPAT_CLIENT_INSTALL_PATH"));
    add(QDir::homePath() + "/.steam/steam");
    add(QDir::homePath() + "/.steam/debian-installation");
    add(QDir::homePath() + "/.local/share/Steam");
    add(QDir::homePath() + "/.var/app/com.valvesoftware.Steam/.local/share/Steam");
    return roots;
}

QStringList steamLibraryRoots(const QStringList &roots) {
    QStringList libraries;
    const auto add = [&libraries](const QString &path) {
        const QString clean = QDir::cleanPath(path);
        if (!clean.isEmpty() && !libraries.contains(clean) && QDir(clean + "/steamapps").exists()) libraries.append(clean);
    };
    for (const QString &root : roots) {
        add(root);
        const QString vdfPath = root + "/steamapps/libraryfolders.vdf";
        QFile file(vdfPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        const QString content = QString::fromUtf8(file.readAll());
        QRegularExpression re(QStringLiteral("\\\"path\\\"\\s+\\\"([^\\\"]+)\\\""));
        auto match = re.globalMatch(content);
        while (match.hasNext()) add(match.next().captured(1).replace("\\\\", "\\"));
    }
    return libraries;
}

QString manifestValue(const QString &manifestPath, const QString &key) {
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    const QString content = QString::fromUtf8(file.readAll());
    const QRegularExpression re(QStringLiteral("\\\"%1\\\"\\s+\\\"([^\\\"]*)\\\"").arg(QRegularExpression::escape(key)));
    const auto match = re.match(content);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString protonFromPrefix(const QString &compatData) {
    QFile file(compatData + "/config_info");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QRegularExpression("[\\r\\n]"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString marker = QStringLiteral("/files/");
        const int filesIndex = line.indexOf(marker);
        const int commonIndex = line.lastIndexOf(QStringLiteral("/steamapps/common/"), filesIndex);
        if (commonIndex < 0 || filesIndex < 0) continue;
        const QString protonDir = line.left(filesIndex).trimmed();
        const QString candidate = protonDir + "/proton";
        if (QFileInfo(candidate).isExecutable()) return candidate;
    }
    return {};
}

bool locateProton(const QStringList &roots, const QString &appId, ProtonLaunchConfig *config, QString *reason) {
    const QString configuredProton = qEnvironmentVariable("MSFS_PROTON_COMMAND");
    const QString configuredData = qEnvironmentVariable("MSFS_COMPAT_DATA_PATH");
    const QString configuredInstall = qEnvironmentVariable("MSFS_COMPAT_INSTALL_PATH");
    QString compatData;
    QString installPath;
    QString steamRoot;
    QString proton;

    if (!configuredData.isEmpty() && QDir(configuredData + "/pfx").exists()) compatData = configuredData;
    for (const QString &library : steamLibraryRoots(roots)) {
        const QString candidateData = library + "/steamapps/compatdata/" + appId;
        if (compatData.isEmpty() && QDir(candidateData + "/pfx").exists()) {
            compatData = candidateData;
            steamRoot = library;
        }
        const QString manifest = library + "/steamapps/appmanifest_" + appId + ".acf";
        if (installPath.isEmpty() && QFileInfo::exists(manifest)) {
            const QString installDir = manifestValue(manifest, "installdir");
            if (!installDir.isEmpty()) installPath = library + "/steamapps/common/" + installDir;
        }
        if (proton.isEmpty()) {
            const QStringList candidates = {
                library + "/steamapps/common/Proton - Experimental/proton",
                library + "/steamapps/common/Proton 9.0/proton",
                library + "/steamapps/common/Proton 8.0/proton"
            };
            for (const QString &candidate : candidates) {
                if (QFileInfo(candidate).isExecutable()) { proton = candidate; break; }
            }
        }
    }
    if (steamRoot.isEmpty() && !roots.isEmpty()) steamRoot = roots.first();
    if (compatData.isEmpty()) {
        if (reason) *reason = QStringLiteral("未找到 MSFS 2024 的 Proton prefix（AppID %1）。请先通过 Steam 启动一次游戏，或设置 MSFS_COMPAT_DATA_PATH。\n已检查：%2").arg(appId, roots.join("、"));
        return false;
    }
    if (configuredProton.isEmpty()) {
        const QString prefixProton = protonFromPrefix(compatData);
        if (!prefixProton.isEmpty()) proton = prefixProton;
    }
    if (installPath.isEmpty()) installPath = configuredInstall;
    if (!configuredProton.isEmpty()) proton = configuredProton;
    if (proton.isEmpty()) {
        if (reason) *reason = QStringLiteral("已找到 MSFS Proton prefix，但未找到 Proton 启动器。请设置 MSFS_PROTON_COMMAND。\nPrefix：%1").arg(compatData);
        return false;
    }
    config->program = proton;
    config->arguments = {QStringLiteral("run")};
    config->environment = QProcessEnvironment::systemEnvironment();
    config->environment.insert(QStringLiteral("WINEPREFIX"), compatData + "/pfx");
    config->environment.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), compatData);
    if (!steamRoot.isEmpty()) config->environment.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"), steamRoot);
    if (!installPath.isEmpty()) {
        config->environment.insert(QStringLiteral("STEAM_COMPAT_INSTALL_PATH"), installPath);
        config->workingDirectory = installPath;
    }
    config->environment.insert(QStringLiteral("SteamAppId"), appId);
    config->environment.insert(QStringLiteral("SteamGameId"), appId);
    config->description = QStringLiteral("Proton：%1\nPrefix：%2").arg(proton, compatData + "/pfx");
    return true;
}
}
#endif

QString FlightData::toWireFormat() const {
    return QString::asprintf("%.6f,%.6f,%.1f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
        latitude, longitude, altitude, heading, pitch, roll, groundSpeed, airSpeed);
}

SimConnectSource::SimConnectSource(QObject *parent) : QObject(parent), m_timer(new QTimer(this)) {
    m_timer->setInterval(5);
    connect(m_timer, &QTimer::timeout, this, [this] {
#ifdef HAVE_SIMCONNECT
        if (!m_handle) {
            HANDLE handle = nullptr;
            if (SUCCEEDED(SimConnect_Open(&handle, "MSFS Sender", nullptr, 0, 0, 0))) {
                m_handle = handle;
                SimConnect_AddToDataDefinition(handle, 0, "PLANE LATITUDE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE LONGITUDE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE ALTITUDE", "feet");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE HEADING DEGREES TRUE", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE PITCH DEGREES", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "PLANE BANK DEGREES", "degrees");
                SimConnect_AddToDataDefinition(handle, 0, "GPS GROUND SPEED", "meters per second");
                SimConnect_AddToDataDefinition(handle, 0, "AIRSPEED INDICATED", "meters per second");
                SimConnect_RequestDataOnSimObject(handle, 0, 0, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SIM_FRAME);
                emit connectionChanged(true, tr("已连接到 MSFS SimConnect"));
            }
        } else if (FAILED(SimConnect_CallDispatch(static_cast<HANDLE>(m_handle), simDispatch, this))) {
            SimConnect_Close(static_cast<HANDLE>(m_handle));
            m_handle = nullptr;
            emit connectionChanged(false, tr("与 MSFS SimConnect 的连接已断开"));
        }
#else
        if (m_wineProcess || !m_wineSocket) return;
        const QString bridge = qEnvironmentVariable("MSFS_SIMCONNECT_BRIDGE").isEmpty()
            ? QCoreApplication::applicationDirPath() + "/MSFS-SimConnect-WineBridge.exe"
            : qEnvironmentVariable("MSFS_SIMCONNECT_BRIDGE");
        if (!QFileInfo::exists(bridge)) {
            emit connectionChanged(false, tr("未找到 Wine SimConnect 桥接程序：%1").arg(bridge));
            m_timer->stop();
            return;
        }
        ProtonLaunchConfig launch;
        QString reason;
        if (!locateProton(steamRoots(), qEnvironmentVariable("MSFS_STEAM_APP_ID", QStringLiteral("2537590")), &launch, &reason)) {
            emit connectionChanged(false, reason);
            m_timer->stop();
            return;
        }
        m_wineProcess = new QProcess(this);
        m_wineProcess->setProgram(launch.program);
        launch.arguments.append({bridge, QStringLiteral("--port"), QString::number(m_wineSocket->localPort())});
        if (!launch.workingDirectory.isEmpty()) {
            launch.arguments.append({QStringLiteral("--simconnect-dir"), launch.workingDirectory});
        }
        m_wineProcess->setArguments(launch.arguments);
        m_wineProcess->setProcessEnvironment(launch.environment);
        if (!launch.workingDirectory.isEmpty()) m_wineProcess->setWorkingDirectory(launch.workingDirectory);
        connect(m_wineProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            emit connectionChanged(false, tr("无法启动 MSFS Proton SimConnect 桥接程序：%1").arg(m_wineProcess->errorString()));
        });
        connect(m_wineProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this](int, QProcess::ExitStatus) { emit connectionChanged(false, tr("MSFS Proton SimConnect 桥接程序已退出")); });
        m_wineProcess->start();
        emit connectionChanged(false, tr("正在使用 MSFS 的 Proton 环境连接 SimConnect…\n%1").arg(launch.description));
        m_timer->stop();
#endif
    });
}

void SimConnectSource::start() {
#ifdef HAVE_SIMCONNECT
    m_timer->start();
#else
    m_wineSocket = new QUdpSocket(this);
    m_wineSocket->setProxy(QNetworkProxy::NoProxy);
    if (!m_wineSocket->bind(QHostAddress::LocalHost, 0)) {
        emit connectionChanged(false, tr("无法创建 Wine 桥接 UDP 端口：%1").arg(m_wineSocket->errorString()));
        return;
    }
    connect(m_wineSocket, &QUdpSocket::readyRead, this, [this] {
        while (m_wineSocket->hasPendingDatagrams()) {
            const QNetworkDatagram packet = m_wineSocket->receiveDatagram();
            if (!packet.senderAddress().isLoopback()) continue;
            const QString line = QString::fromUtf8(packet.data()).trimmed();
            if (line == QLatin1String("STATUS:CONNECTED")) { emit connectionChanged(true, tr("已通过 MSFS Proton 连接到 SimConnect")); continue; }
            if (line == QLatin1String("STATUS:DISCONNECTED")) { emit connectionChanged(false, tr("等待 MSFS SimConnect")); continue; }
            if (!line.startsWith(QLatin1String("DATA:"))) continue;
            const auto f = line.sliced(5).split(QLatin1Char(',')); if (f.size() != 8) continue;
            bool ok = false; FlightData d;
            d.latitude = f[0].toDouble(&ok); if (!ok) continue; d.longitude = f[1].toDouble(&ok); if (!ok) continue;
            d.altitude = f[2].toDouble(&ok); if (!ok) continue; d.heading = f[3].toDouble(&ok); if (!ok) continue;
            d.pitch = f[4].toDouble(&ok); if (!ok) continue; d.roll = f[5].toDouble(&ok); if (!ok) continue;
            d.groundSpeed = f[6].toDouble(&ok); if (!ok) continue; d.airSpeed = f[7].toDouble(&ok); if (!ok) continue;
            emit dataReceived(d);
        }
    });
    m_timer->start();
#endif
}

SimConnectSource::~SimConnectSource() {
#ifdef HAVE_SIMCONNECT
    if (m_handle) SimConnect_Close(static_cast<HANDLE>(m_handle));
#else
    if (m_wineProcess) { m_wineProcess->terminate(); m_wineProcess->waitForFinished(1000); }
#endif
}
