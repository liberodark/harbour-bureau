import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: overlay

    property int folderId: -1
    readonly property bool opened: folderId >= 0
    readonly property var apps: (homeModel.revision, opened ? homeModel.folderApps(folderId) : [])
    readonly property string title: (homeModel.revision, opened ? homeModel.folderName(folderId) : "")
    property string actionPath: ""

    readonly property int columns: Math.min(homeModel.columns, 4)
    readonly property real cellWidth: Math.floor((panel.width - 2 * Theme.paddingMedium) / columns)
    readonly property real cellHeight: Math.round(cellWidth * 1.3)

    signal launchRequested(string path, bool missing)
    signal addRequested(int folderId)

    function open(id) {
        actionPath = ""
        folderId = id
        nameField.text = homeModel.folderName(id)
    }

    function close() {
        commitName()
        actionPath = ""
        folderId = -1
    }

    function commitName() {
        if (opened && nameField.text.trim().length > 0 && nameField.text !== title)
            homeModel.renameFolder(folderId, nameField.text)
        nameField.focus = false
    }

    Connections {
        target: homeModel
        onRevisionChanged: {
            if (overlay.opened && !homeModel.folderExists(overlay.folderId)) {
                overlay.actionPath = ""
                overlay.folderId = -1
            }
        }
    }

    anchors.fill: parent
    visible: opacity > 0
    opacity: opened ? 1.0 : 0.0
    z: 50
    Behavior on opacity { FadeAnimation { duration: 150 } }

    Rectangle {
        anchors.fill: parent
        color: Theme.rgba("black", 0.55)
        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (overlay.actionPath !== "")
                    overlay.actionPath = ""
                else
                    overlay.close()
            }
        }
    }

    Rectangle {
        id: panel
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        height: content.height + 2 * Theme.paddingMedium
        radius: Theme.paddingLarge
        color: Theme.rgba(Theme.highlightDimmerColor, 0.96)

        MouseArea {
            anchors.fill: parent
            onClicked: overlay.actionPath = ""
        }

        Column {
            id: content
            x: Theme.paddingMedium
            y: Theme.paddingMedium
            width: parent.width - 2 * Theme.paddingMedium

            TextField {
                id: nameField
                width: parent.width
                horizontalAlignment: TextInput.AlignHCenter
                font.pixelSize: Theme.fontSizeLarge
                placeholderText: qsTr("Nom du dossier")
                label: qsTr("Toucher pour renommer")
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: overlay.commitName()
                onActiveFocusChanged: if (!activeFocus) overlay.commitName()
            }

            SilicaFlickable {
                id: folderFlick
                width: parent.width
                height: Math.min(folderGrid.height, overlay.height * 0.55)
                contentHeight: folderGrid.height
                clip: contentHeight > height

                Grid {
                    id: folderGrid
                    columns: overlay.columns

                    Repeater {
                        model: overlay.apps
                        delegate: Item {
                            id: folderCell
                            width: overlay.cellWidth
                            height: overlay.cellHeight

                            HomeIcon {
                                anchors.fill: parent
                                entryType: "app"
                                label: modelData.name
                                iconSource: modelData.iconSource
                                missing: modelData.missing
                                down: cellMouse.pressed && cellMouse.containsMouse
                            }

                            MouseArea {
                                id: cellMouse
                                anchors.fill: parent
                                onClicked: {
                                    if (overlay.actionPath !== "") {
                                        overlay.actionPath = ""
                                        return
                                    }
                                    overlay.launchRequested(modelData.desktopPath, modelData.missing)
                                }
                                onPressAndHold: {
                                    var p = folderCell.mapToItem(overlay, folderCell.width / 2, folderCell.height)
                                    actionMenu.anchorX = p.x
                                    actionMenu.anchorY = p.y
                                    overlay.actionPath = modelData.desktopPath
                                }
                            }
                        }
                    }

                    Item {
                        width: overlay.cellWidth
                        height: overlay.cellHeight

                        HomeIcon {
                            anchors.fill: parent
                            entryType: "app"
                            label: qsTr("Ajouter")
                            iconSource: "image://theme/icon-m-add"
                            down: addMouse.pressed && addMouse.containsMouse
                        }
                        MouseArea {
                            id: addMouse
                            anchors.fill: parent
                            onClicked: overlay.addRequested(overlay.folderId)
                        }
                    }
                }

                VerticalScrollDecorator {}
            }
        }
    }

    Rectangle {
        id: actionMenu

        property real anchorX: 0
        property real anchorY: 0

        visible: overlay.actionPath !== ""
        width: Math.min(overlay.width - 2 * Theme.horizontalPageMargin, Theme.itemSizeExtraLarge * 3)
        height: actionColumn.height
        x: Math.max(Theme.horizontalPageMargin, Math.min(overlay.width - width - Theme.horizontalPageMargin, anchorX - width / 2))
        y: Math.min(overlay.height - height - Theme.paddingLarge, anchorY)
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.95)
        z: 10

        Column {
            id: actionColumn
            width: parent.width

            BackgroundItem {
                width: parent.width
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.paddingLarge
                    width: parent.width - 2 * Theme.paddingLarge
                    text: qsTr("Sortir du dossier")
                    truncationMode: TruncationMode.Fade
                }
                onClicked: {
                    var path = overlay.actionPath
                    overlay.actionPath = ""
                    homeModel.moveOutOfFolder(overlay.folderId, path)
                }
            }

            BackgroundItem {
                width: parent.width
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.paddingLarge
                    width: parent.width - 2 * Theme.paddingLarge
                    text: qsTr("Retirer du bureau")
                    truncationMode: TruncationMode.Fade
                }
                onClicked: {
                    var path = overlay.actionPath
                    overlay.actionPath = ""
                    homeModel.removeFromFolder(overlay.folderId, path)
                }
            }
        }
    }
}
