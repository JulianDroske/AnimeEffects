#include <QFile>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDomDocument>
#include <QJsonDocument>
#include <qstandardpaths.h>
#include <QDesktopServices>
#include "qprocess.h"
#include "util/TextUtil.h"
#include "cmnd/BasicCommands.h"
#include "cmnd/ScopedMacro.h"
#include "core/ObjectNodeUtil.h"
#include "ctrl/CmndName.h"
#include "gui/MainMenuBar.h"
#include "gui/MainWindow.h"
#include "gui/ResourceDialog.h"
#include "gui/EasyDialog.h"
#include "gui/KeyBindingDialog.h"
#include "gui/GeneralSettingDialog.h"
#include "gui/MouseSettingDialog.h"
#include "util/NetworkUtil.h"

namespace gui
{
//-------------------------------------------------------------------------------------------------
QDomDocument getVideoExportDocument()
{
    QFile file("./data/encode/VideoEncode.txt");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << file.errorString();
        return QDomDocument();
    }

    QDomDocument prop;
    QString errorMessage;
    int errorLine = 0;
    int errorColumn = 0;
    if (!prop.setContent(&file, false, &errorMessage, &errorLine, &errorColumn))
    {
        qDebug() << "invalid xml file. "
                 << file.fileName()
                 << errorMessage << ", line = " << errorLine
                 << ", column = " << errorColumn;
        return QDomDocument();
    }
    file.close();

    return prop;
}

//-------------------------------------------------------------------------------------------------
MainMenuBar::MainMenuBar(MainWindow& aMainWindow, ViaPoint& aViaPoint, GUIResources &aGUIResources, QWidget* aParent)
    : QMenuBar(aParent)
    , mProcess()
    , mViaPoint(aViaPoint)
    , mProject()
    , mProjectActions()
    , mShowResourceWindow()
    , mVideoFormats()
    , mGUIResources(aGUIResources)
{
    // load the list of video formats from a setting file.
    loadVideoFormats();

    MainWindow* mainWindow = &aMainWindow;

    QMenu* fileMenu = new QMenu(tr("File"), this);
    {
        QAction* newProject     = new QAction(tr("New Project..."), this);
        QAction* openProject    = new QAction(tr("Open Project..."), this);
        QMenu* openRecent     = new QMenu(tr("Open Recent..."), this);
        {
            // Get settings
            QSettings settings;
            settings.sync();
            recentfiles = settings.value("projectloader/recents").toStringList();

            // Path placeholders
            if (recentfiles.length() != 8){
                while (recentfiles.length() != 8){
                    recentfiles.append("Path placeholder");
                }
            }
            QString firstPath = recentfiles[0];
            QString secondPath = recentfiles[1];
            QString thirdPath = recentfiles[2];
            QString fourthPath = recentfiles[3];
            QString fifthPath = recentfiles[4];
            QString sixthPath = recentfiles[5];
            QString seventhPath = recentfiles[6];
            QString eigthPath = recentfiles[7];
            QStringList list;
            list << firstPath << secondPath << thirdPath << fourthPath << fifthPath << sixthPath << seventhPath << eigthPath;

            // Path actions
            QAction * placeholderAction = new QAction(tr("No other projects..."), this);
            QAction* firstPathAction = new QAction(firstPath);
            QAction* secondPathAction = new QAction(secondPath);
            QAction* thirdPathAction = new QAction(thirdPath);
            QAction* fourthPathAction = new QAction(fourthPath);
            QAction* fifthPathAction = new QAction(fifthPath);
            QAction* sixthPathAction = new QAction(sixthPath);
            QAction* seventhPathAction = new QAction(seventhPath);
            QAction* eigthPathAction = new QAction(eigthPath);
            // Path addition
            // Better than before, but still not very DRY code.
            for (int x = 0; x <= 7; x+=1){
                if (list[x] == QString("Path placeholder")){
                    openRecent->addAction(placeholderAction);
                }
                else{
                    switch(x){
                    case 0: openRecent->addAction(firstPathAction);
                            break;
                    case 1: openRecent->addAction(secondPathAction);
                            break;
                    case 2: openRecent->addAction(thirdPathAction);
                            break;
                    case 3: openRecent->addAction(fourthPathAction);
                            break;
                    case 4: openRecent->addAction(fifthPathAction);
                            break;
                    case 5: openRecent->addAction(sixthPathAction);
                            break;
                    case 6: openRecent->addAction(seventhPathAction);
                            break;
                    case 7: openRecent->addAction(eigthPathAction);
                            break;
                    }
                }
            }

            // Connections
            connect(placeholderAction, &QAction::triggered, [=](){ qDebug() << "You've earned yourself a cookie!"; });
            connect(firstPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(firstPath); });
            connect(secondPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(secondPath); });
            connect(thirdPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(thirdPath); });
            connect(fourthPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(fourthPath); });
            connect(fifthPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(fifthPath); });
            connect(sixthPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(sixthPath); });
            connect(seventhPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(seventhPath); });
            connect(eigthPathAction, &QAction::triggered, [=](){ mainWindow->onOpenRecentTriggered(eigthPath); });
        }
        QAction* saveProject    = new QAction(tr("Save Project"), this);
        QAction* saveProjectAs  = new QAction(tr("Save Project As..."), this);
        QAction* closeProject   = new QAction(tr("Close Project"), this);
        QMenu* exportAs = new QMenu(tr("Export As"), this);
        {
            ctrl::VideoFormat gifFormat;
            gifFormat.name = "gif";
            gifFormat.label = "GIF";
            gifFormat.icodec = "ppm";

            QAction* jpgs = new QAction(tr("JPEG Sequence..."), this);
            QAction* pngs = new QAction(tr("PNG Sequence..."), this);
            QAction* gif  = new QAction(tr("GIF Animation..."), this);
            connect(jpgs, &QAction::triggered, [=](){ mainWindow->onExportImageSeqTriggered("jpg"); });
            connect(pngs, &QAction::triggered, [=](){ mainWindow->onExportImageSeqTriggered("png"); });
            connect(gif,  &QAction::triggered, [=](){ mainWindow->onExportVideoTriggered(gifFormat); });
            exportAs->addAction(jpgs);
            exportAs->addAction(pngs);
            exportAs->addAction(gif);

            for (auto format : mVideoFormats)
            {
                QAction* video = new QAction(format.label + " " + tr("Video") + "...", this);
                connect(video,  &QAction::triggered, [=](){ mainWindow->onExportVideoTriggered(format); });
                exportAs->addAction(video);
            }
        }

        mProjectActions.push_back(saveProject);
        mProjectActions.push_back(saveProjectAs);
        mProjectActions.push_back(closeProject);
        mProjectActions.push_back(exportAs->menuAction());

        connect(newProject, &QAction::triggered, mainWindow, &MainWindow::onNewProjectTriggered);
        connect(openProject, &QAction::triggered, mainWindow, &MainWindow::onOpenProjectTriggered);
        connect(saveProject, &QAction::triggered, mainWindow, &MainWindow::onSaveProjectTriggered);
        connect(saveProjectAs, &QAction::triggered, mainWindow, &MainWindow::onSaveProjectAsTriggered);
        connect(closeProject, &QAction::triggered, mainWindow, &MainWindow::onCloseProjectTriggered);

        fileMenu->addAction(newProject);
        fileMenu->addAction(openProject);
        fileMenu->addAction(openRecent->menuAction());
        fileMenu->addSeparator();
        fileMenu->addAction(saveProject);
        fileMenu->addAction(saveProjectAs);
        fileMenu->addAction(exportAs->menuAction());
        fileMenu->addSeparator();
        fileMenu->addAction(closeProject);
    }

    QMenu* editMenu = new QMenu(tr("Edit"), this);
    {
        QAction* undo = new QAction(tr("Undo"), this);
        QAction* redo = new QAction(tr("Redo"), this);

        mProjectActions.push_back(undo);
        mProjectActions.push_back(redo);

        connect(undo, &QAction::triggered, mainWindow, &MainWindow::onUndoTriggered);
        connect(redo, &QAction::triggered, mainWindow, &MainWindow::onRedoTriggered);

        editMenu->addAction(undo);
        editMenu->addAction(redo);
    }

    QMenu* projMenu = new QMenu(tr("Project"), this);
    {
        QAction* canvSize = new QAction(tr("Set canvas size..."), this);
        QAction* maxFrame = new QAction(tr("Set maximum frame count..."), this);
        QAction* loopAnim = new QAction(tr("Set animation loop..."), this);
        QAction* setFPS = new QAction(tr("Set frames per second..."), this);

        mProjectActions.push_back(canvSize);
        mProjectActions.push_back(maxFrame);
        mProjectActions.push_back(loopAnim);
        mProjectActions.push_back(setFPS);

        connect(canvSize, &QAction::triggered, this, &MainMenuBar::onCanvasSizeTriggered);
        connect(maxFrame, &QAction::triggered, this, &MainMenuBar::onMaxFrameTriggered);
        connect(loopAnim, &QAction::triggered, this, &MainMenuBar::onLoopTriggered);
        connect(setFPS, &QAction::triggered, this, &MainMenuBar::onFPSTriggered);

        projMenu->addAction(canvSize);
        projMenu->addAction(maxFrame);
        projMenu->addAction(loopAnim);
        projMenu->addAction(setFPS);
    }

    QMenu* windowMenu = new QMenu(tr("Window"), this);
    {
        QAction* resource = new QAction(tr("Resource Window"), this);
        resource->setCheckable(true);
        mShowResourceWindow = resource;

        connect(resource, &QAction::triggered, [&](bool aChecked)
        {
            if (aViaPoint.resourceDialog())
            {
                aViaPoint.resourceDialog()->setVisible(aChecked);
            }
        });

        windowMenu->addAction(resource);
    }

    QMenu* optionMenu = new QMenu(tr("Option"), this);
    {
        QAction* general = new QAction(tr("General Settings..."), this);
        QAction* mouse   = new QAction(tr("Mouse Settings..."), this);
        QAction* keyBind = new QAction(tr("Key Bindings..."), this);

        connect(general, &QAction::triggered, [&](bool)
        {
            GeneralSettingDialog *generalSettingsDialog = new GeneralSettingDialog(mGUIResources, this);
            QScopedPointer<GeneralSettingDialog> dialog(generalSettingsDialog);
            if(dialog->exec() == QDialog::DialogCode::Accepted) {
                if(generalSettingsDialog->timeFormatHasChanged())
                    this->onTimeFormatChanged();
                if(generalSettingsDialog->themeHasChanged())
                    this->mGUIResources.setTheme(generalSettingsDialog->theme());
            }
        });

        connect(mouse, &QAction::triggered, [&](bool)
        {
            QScopedPointer<MouseSettingDialog> dialog(
                        new MouseSettingDialog(aViaPoint, this));
            dialog->exec();
        });

        connect(keyBind, &QAction::triggered, [&](bool)
        {
            XC_PTR_ASSERT(aViaPoint.keyCommandMap());
            QScopedPointer<KeyBindingDialog> dialog(
                        new KeyBindingDialog(*aViaPoint.keyCommandMap(), this));
            dialog->exec();
        });

        optionMenu->addAction(general);
        optionMenu->addAction(mouse);
        optionMenu->addAction(keyBind);
    }

    QMenu* helpMenu = new QMenu(tr("Help"), this);
    {
        QAction* aboutMe = new QAction(tr("About AnimeEffects..."), this);
        connect(aboutMe, &QAction::triggered, [=]()
        {
            QMessageBox msgBox;
            msgBox.setWindowIcon(QIcon("../src/AnimeEffects.ico"));
            auto versionString = QString::number(AE_MAJOR_VERSION) + "." + QString::number(AE_MINOR_VERSION) + "." + QString::number(AE_MICRO_VERSION);
            auto formatVersionString = QString::number(AE_PROJECT_FORMAT_MAJOR_VERSION) + "." + QString::number(AE_PROJECT_FORMAT_MINOR_VERSION);
            auto platform = QSysInfo::productType();
            auto qtVersion = QString::number(QT_VERSION_MAJOR) + "." + QString::number(QT_VERSION_MINOR) + "." + QString::number(QT_VERSION_PATCH);
            platform[0] = platform[0].toUpper(); // First letter capitalization :D
            QString msgStr = tr("### AnimeEffects for ") + platform + tr(" version ") + versionString + "<br />" + tr("Original code and artwork by [Hidefuku](https://github.com/hidefuku).<br />Current development handled by the [AnimeEffectsDevs](https://github.com/AnimeEffectsDevs).");
            msgBox.setText(msgStr);
            // TODO: fix it later
            // msgBox.setTextFormat(Qt::TextFormat::MarkdownText);
            QString detail;
            detail += tr("Version: ") + versionString + "\n";
            detail += tr("Platform: ") + platform + " " + QSysInfo::productVersion() + "\n";
            detail += tr("Build ABI: ") + QSysInfo::buildAbi() + "\n";
            detail += tr("Build CPU: ") + QSysInfo::buildCpuArchitecture() + "\n";
            detail += tr("Current CPU: ") + QSysInfo::currentCpuArchitecture() + "\n";
            detail += tr("Current GPU: ") + QString(this->mViaPoint.glDeviceInfo().renderer.c_str()) + "\n";
            detail += tr("GPU Vendor: ") + QString(this->mViaPoint.glDeviceInfo().vender.c_str()) + "\n";
            detail += tr("OpenGL Version: ") + QString(this->mViaPoint.glDeviceInfo().version.c_str()) + "\n";
            detail += tr("Qt Version: ") + qtVersion + "\n";
            detail += tr("Format Version: ") + formatVersionString + "\n";
            msgBox.setDetailedText(detail);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
        });

        QAction* checkForUpdates = new QAction(tr("Check for Updates..."), this);
        connect(checkForUpdates, &QAction::triggered, [=]()
        {
            util::NetworkUtil networking;
            QString url("https://api.github.com/repos/AnimeEffectsDevs/AnimeEffects/tags");
            qDebug("--------");
            qInfo() << "Checking for updates on : " << url;
            QJsonDocument jsonResponse = networking.getJsonFrom(url);
            QString currentVersion = QString::number(AE_MAJOR_VERSION) + "." + QString::number(AE_MINOR_VERSION) + "." + QString::number(AE_MICRO_VERSION);
            /*
            qDebug()<< "Response : " << jsonResponse.toJson().data();
            qDebug()<< "Current version: " << currentVersion << "\n" << "Latest stable: " << jsonResponse[0]["name"].toString().replace("v", "");;
            */
            QString latestVersion = !jsonResponse.isEmpty()? jsonResponse[0]["name"].toString().replace("v", "") : "null";
            if (latestVersion != ""){
                QMessageBox updateBox;
                bool onLatest = false; bool onPreview = false; bool failed = false;

                // Failed
                if (latestVersion == "null"){
                    qDebug() << "Failed to get version";
                    updateBox.setWindowTitle(tr("Failed"));
                    updateBox.setText(tr("<center>Unable to get latest version. <br>Please check your internet "
                                         "connection and if you have curl or wget installed.</center>"));
                    failed = true;
                }
                // Latest version
                if (latestVersion == currentVersion){
                    qDebug() << "On latest version :" << latestVersion;
                    updateBox.setWindowTitle(tr("On latest"));
                    updateBox.setText(tr("<center>You already have the latest stable release available. <br>Version: ")
                                      + currentVersion + "</center>");
                    onLatest = true;
                }
                // Preview version
                else if (latestVersion < currentVersion){
                    qDebug() << "On preview version :" << currentVersion;

                    updateBox.setWindowTitle(tr("On preview"));
                    updateBox.setText(tr("<center>Your current version is higher than the latest stable release. "
                                         "<br>Version: ") + currentVersion + "</center>");
                    onPreview = true;
                }
                // Old version
                else{
                    qDebug() << "On version :" << currentVersion;
                    updateBox.setWindowTitle(tr("New release available"));
                    updateBox.setText(tr("<center>A new stable release is available, version: ") + latestVersion +
                                      tr(".<br>Do you wish to download it or to go to the GitHub page?</center>"));
                }

                if (onLatest || onPreview || failed){
                    updateBox.setStandardButtons(QMessageBox::Ok);
                    updateBox.setDefaultButton(QMessageBox::Ok);
                }
                else{
                     QAbstractButton* downloadButton = updateBox.addButton(tr("Download"), QMessageBox::AcceptRole);
                     QAbstractButton* gotoButton = updateBox.addButton(tr("Go to page"), QMessageBox::AcceptRole);
                     updateBox.addButton(QMessageBox::Cancel);
                     updateBox.exec();

                     if(updateBox.clickedButton() == gotoButton){
                       QDesktopServices::openUrl(QUrl("https://github.com/AnimeEffectsDevs/AnimeEffects/releases/latest"));
                     }
                     else if(updateBox.clickedButton() == downloadButton){
                         QString os = networking.os();
                         QString arch = networking.arch();
                         QString file;
                         if (os == "win"){
                             if (arch == "x86"){ file = "AnimeEffects-Windows-x86.zip"; }
                             else{ file = "AnimeEffects-Windows-x64.zip"; }
                         }
                         else if (os == "linux"){ file = "AnimeEffects-Linux.zip"; }
                         else if (os == "mac"){ file = "AnimeEffects-MacOS.zip"; }
                         QFileInfo aeUpdate = networking.downloadGithubFile("https://api.github.com/repos/AnimeEffectsDevs/AnimeEffects/releases/latest", file, 0, this);
                         QDesktopServices::openUrl(QUrl::fromLocalFile(aeUpdate.absoluteFilePath()));
                     }
                     qDebug("--------");
                     return;
                }
                updateBox.exec();
                qDebug("--------");
                return;
            }
        });

        helpMenu->addAction(aboutMe);
        helpMenu->addAction(checkForUpdates);
    }

    this->addAction(fileMenu->menuAction());
    this->addAction(editMenu->menuAction());
    this->addAction(projMenu->menuAction());
    this->addAction(windowMenu->menuAction());
    this->addAction(optionMenu->menuAction());
    this->addAction(helpMenu->menuAction());

    // reset status
    setProject(nullptr);
}

void MainMenuBar::setProject(core::Project* aProject)
{
    mProject = aProject;

    if (mProject)
    {
        for (auto action : mProjectActions)
        {
            action->setEnabled(true);
        }
    }
    else
    {
        for (auto action : mProjectActions)
        {
            action->setEnabled(false);
        }
    }
}

void MainMenuBar::setShowResourceWindow(bool aShow)
{
    mShowResourceWindow->setChecked(aShow);
}

void MainMenuBar::loadVideoFormats()
{
    using util::TextUtil;

    QDomDocument doc = getVideoExportDocument();
    QDomElement domRoot = doc.firstChildElement("video_encode");

    // for each format
    QDomElement domFormat = domRoot.firstChildElement("format");
    while (!domFormat.isNull())
    {
        ctrl::VideoFormat format;
        // neccessary attribute
        format.name = domFormat.attribute("name");
        if (format.name.isEmpty()) continue;
        // optional attributes
        format.label = domFormat.attribute("label");
        format.icodec = domFormat.attribute("icodec");
        format.command = domFormat.attribute("command");
        if (format.label.isEmpty()) format.label = format.name;
        if (format.icodec.isEmpty()) format.icodec = "png";
        // add one format
        mVideoFormats.push_back(format);

        // for each codec
        QDomElement domCodec = domFormat.firstChildElement("codec");
        while (!domCodec.isNull())
        {
            ctrl::VideoCodec codec;
            // neccessary attribute
            codec.name = domCodec.attribute("name");
            if (codec.name.isEmpty()) continue;
            // optional attributes
            codec.label = domCodec.attribute("label");
            codec.icodec = domCodec.attribute("icodec");
            codec.command = domCodec.attribute("command");
            if (codec.label.isEmpty()) codec.label = codec.name;
            if (codec.icodec.isEmpty()) codec.icodec = format.icodec;
            if (codec.command.isEmpty()) codec.command = format.command;
            {
                auto hints = TextUtil::splitAndTrim(domCodec.attribute("hint"), ',');
                for (auto hint : hints)
                {
                    if      (hint == "lossless"   ) codec.lossless    = true;
                    else if (hint == "transparent") codec.transparent = true;
                    else if (hint == "colorspace" ) codec.colorspace  = true;
                    else if (hint == "gpuenc"     ) codec.gpuenc      = true;
                }
            }
            codec.pixfmts = TextUtil::splitAndTrim(domCodec.attribute("pixfmt"), ',');

            // add one codec
            mVideoFormats.back().codecs.push_back(codec);

            // to next sibling
            domCodec = domCodec.nextSiblingElement("codec");
        }
        // to next sibling
        domFormat = domFormat.nextSiblingElement("format");
    }
}

//-------------------------------------------------------------------------------------------------
ProjectCanvasSizeSettingDialog::ProjectCanvasSizeSettingDialog(
        ViaPoint& aViaPoint, core::Project& aProject, QWidget *aParent)
    : EasyDialog(tr("Set canvas size"), aParent)
    , mViaPoint(aViaPoint)
    , mProject(aProject)
{
    // create inner widgets
    auto curSize = mProject.attribute().imageSize();
    {
        auto devInfo = mViaPoint.glDeviceInfo();
        const int maxBufferSize = std::min(
                    devInfo.maxTextureSize,
                    devInfo.maxRenderBufferSize);
        XC_ASSERT(maxBufferSize > 0);

        auto form = new QFormLayout();
        form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
        form->setLabelAlignment(Qt::AlignRight);

        auto sizeLayout = new QHBoxLayout();
        {
            mWidthBox = new QSpinBox();
            mWidthBox->setRange(1, maxBufferSize);
            mWidthBox->setValue(curSize.width());
            sizeLayout->addWidget(mWidthBox);

            mHeightBox = new QSpinBox();
            mHeightBox->setRange(1, maxBufferSize);
            mHeightBox->setValue(curSize.height());
            sizeLayout->addWidget(mHeightBox);
        }
        form->addRow(tr("Size :"), sizeLayout);

        auto group = new QGroupBox(tr("Parameters"));
        group->setLayout(form);
        this->setMainWidget(group);
    }
    this->setOkCancel();
    this->fixSize();
}

void MainMenuBar::onCanvasSizeTriggered()
{
    if (!mProject) return;

    auto curSize = mProject->attribute().imageSize();

    // create dialog
    QScopedPointer<ProjectCanvasSizeSettingDialog> dialog(
                new ProjectCanvasSizeSettingDialog(mViaPoint, *mProject, this));

    // execute dialog
    dialog->exec();
    if (dialog->result() != QDialog::Accepted) return;

    // get new canvas size
    const QSize newSize = dialog->canvasSize();
    XC_ASSERT(!newSize.isEmpty());
    if (curSize == newSize) return;

    // create commands
    {
        cmnd::ScopedMacro macro(mProject->commandStack(),
                                CmndName::tr("Change the canvas size"));

        core::Project* projectPtr = mProject;
        auto command = new cmnd::Delegatable([=]()
        {
            projectPtr->attribute().setImageSize(newSize);
            auto event = core::ProjectEvent::imageSizeChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, false);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        },
        [=]()
        {
            projectPtr->attribute().setImageSize(curSize);
            auto event = core::ProjectEvent::imageSizeChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, true);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        });
        mProject->commandStack().push(command);
    }
}

//-------------------------------------------------------------------------------------------------
ProjectMaxFrameSettingDialog::ProjectMaxFrameSettingDialog(core::Project& aProject, QWidget *aParent)
    : EasyDialog(tr("Set max frames"), aParent)
    , mProject(aProject)
    , mMaxFrameBox()
{
    // create inner widgets
    auto curMaxFrame = mProject.attribute().maxFrame();
    {
        auto form = new QFormLayout();
        form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
        form->setLabelAlignment(Qt::AlignRight);

        auto layout = new QHBoxLayout();
        {
            mMaxFrameBox = new QSpinBox();
            mMaxFrameBox->setRange(1, std::numeric_limits<int>::max());
            mMaxFrameBox->setValue(curMaxFrame);
            layout->addWidget(mMaxFrameBox);
        }
        form->addRow(tr("Max frame count :"), layout);

        auto group = new QGroupBox(tr("Parameters"));
        group->setLayout(form);
        this->setMainWidget(group);
    }

    this->setOkCancel([=](int aIndex)->bool
    {
        if (aIndex == 0)
        {
            return this->confirmMaxFrameUpdating(this->mMaxFrameBox->value());
        }
        return true;
    });
    this->fixSize();
}

bool ProjectMaxFrameSettingDialog::confirmMaxFrameUpdating(int aNewMaxFrame) const
{
    XC_ASSERT(aNewMaxFrame > 0);

    auto curMaxFrame = mProject.attribute().maxFrame();
    if (curMaxFrame <= aNewMaxFrame) return true;

    if (!core::ObjectNodeUtil::thereAreSomeKeysExceedingFrame(
                mProject.objectTree().topNode(), aNewMaxFrame))
    {
        return true;
    }

    auto message1 = tr("Frame value cannot be set.");
    auto message2 = tr("One or more keys exceed the specified frame value.");
    QMessageBox::warning(nullptr, tr("Operation Error"), message1 + "\n" + message2);
    return false;
}

void MainMenuBar::onMaxFrameTriggered()
{
    if (!mProject) return;

    auto curMaxFrame = mProject->attribute().maxFrame();

    // create dialog
    QScopedPointer<ProjectMaxFrameSettingDialog> dialog(
                new ProjectMaxFrameSettingDialog(*mProject, this));

    // execute dialog
    dialog->exec();
    if (dialog->result() != QDialog::Accepted) return;

    // get new canvas size
    const int newMaxFrame = dialog->maxFrame();
    XC_ASSERT(newMaxFrame > 0);
    if (curMaxFrame == newMaxFrame) return;

    // create commands
    {
        cmnd::ScopedMacro macro(mProject->commandStack(),
                                CmndName::tr("Change the max frame"));

        core::Project* projectPtr = mProject;
        auto command = new cmnd::Delegatable([=]()
        {
            projectPtr->attribute().setMaxFrame(newMaxFrame);
            auto event = core::ProjectEvent::maxFrameChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, false);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        },
        [=]()
        {
            projectPtr->attribute().setMaxFrame(curMaxFrame);
            auto event = core::ProjectEvent::maxFrameChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, true);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        });
        mProject->commandStack().push(command);
    }
}

//-------------------------------------------------------------------------------------------------
ProjectLoopSettingDialog::ProjectLoopSettingDialog(core::Project& aProject, QWidget* aParent)
    : EasyDialog(tr("Set loop"), aParent)
{
    // create inner widgets
    auto curLoop = aProject.attribute().loop();
    {
        auto form = new QFormLayout();
        form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
        form->setLabelAlignment(Qt::AlignRight);

        auto layout = new QHBoxLayout();
        {
            mLoopBox = new QCheckBox();
            mLoopBox->setChecked(curLoop);
            layout->addWidget(mLoopBox);
        }
        form->addRow(tr("Loop animation :"), layout);

        auto group = new QGroupBox(tr("Parameters"));
        group->setLayout(form);
        this->setMainWidget(group);
    }

    this->setOkCancel();
    this->fixSize();
}

void MainMenuBar::onLoopTriggered()
{
    if (!mProject) return;

    auto curLoop = mProject->attribute().loop();

    // create dialog
    QScopedPointer<ProjectLoopSettingDialog> dialog(new ProjectLoopSettingDialog(*mProject, this));

    // execute dialog
    dialog->exec();
    if (dialog->result() != QDialog::Accepted) return;

    // get new loop setting
    const bool newLoop = dialog->isCheckedLoopBox();
    if (curLoop == newLoop) return;

    // create commands
    {
        cmnd::ScopedMacro macro(mProject->commandStack(),
                                CmndName::tr("Change loop settings"));

        core::Project* projectPtr = mProject;
        auto command = new cmnd::Delegatable([=]()
        {
            projectPtr->attribute().setLoop(newLoop);
            auto event = core::ProjectEvent::loopChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, false);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        },
        [=]()
        {
            projectPtr->attribute().setLoop(curLoop);
            auto event = core::ProjectEvent::loopChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, true);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        });
        mProject->commandStack().push(command);
    }
}

// ----------------------------------------------------------------------------- //
ProjectFPSSettingDialog::ProjectFPSSettingDialog(core::Project& aProject, QWidget *aParent)
    : EasyDialog(tr("Set FPS"), aParent)
    , mProject(aProject)
    , mFPSBox()

{
    // create inner widgets
    auto curFPS = mProject.attribute().fps();
    {
        auto form = new QFormLayout();
        form->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
        form->setLabelAlignment(Qt::AlignRight);

        auto layout = new QHBoxLayout();
        {
            mFPSBox = new QSpinBox();
            mFPSBox->setRange(1, std::numeric_limits<int>::max());
            mFPSBox->setValue(curFPS);
            layout->addWidget(mFPSBox);
        }
        form->addRow(tr("Frames per second :"), layout);

        auto group = new QGroupBox(tr("Parameters"));
        group->setLayout(form);
        this->setMainWidget(group);
    }

    this->setOkCancel([=](int aIndex)->bool
    {
        if (aIndex == 0)
        {
            return this->confirmFPSUpdating(this->mFPSBox->value());
        }
        return true;
    });
    this->fixSize();
}

bool ProjectFPSSettingDialog::confirmFPSUpdating(int aNewFPS) const
{
    XC_ASSERT(aNewFPS > 0);
    if (aNewFPS!=0) return true;
    return false;
}

void MainMenuBar::onFPSTriggered()
{
    if (!mProject) return;

    auto curFPS = mProject->attribute().fps();

    // create dialog
    QScopedPointer<ProjectFPSSettingDialog> dialog(
                new ProjectFPSSettingDialog(*mProject, this));

    // execute dialog
    dialog->exec();
    if (dialog->result() != QDialog::Accepted) return;

    // get new canvas size
    const int newFPS = dialog->fps();
    XC_ASSERT(newFPS > 0);
    if (curFPS == newFPS) return;

    // create commands
    {
        cmnd::ScopedMacro macro(mProject->commandStack(),
                                CmndName::tr("Change the FPS"));

        core::Project* projectPtr = mProject;
        auto command = new cmnd::Delegatable([=]()
        {
            projectPtr->attribute().setFps(newFPS);
            auto event = core::ProjectEvent::maxFrameChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, false);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        },
        [=]()
        {
            projectPtr->attribute().setFps(curFPS);
            auto event = core::ProjectEvent::maxFrameChangeEvent(*projectPtr);
            projectPtr->onProjectAttributeModified(event, true);
            this->onProjectAttributeUpdated();
            this->onVisualUpdated();
        });
        mProject->commandStack().push(command);
    }
}

} // namespace gui
