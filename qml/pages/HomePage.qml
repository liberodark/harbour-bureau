import QtQuick 2.6
import Sailfish.Silica 1.0
import "../components"

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    property bool editMode: false

    readonly property int columns: homeModel.columns
    readonly property real cellWidth: Math.floor(width / columns)
    readonly property real cellHeight: Math.round(cellWidth * (homeModel.showLabels ? 1.3 : 1.05))
    readonly property real gridTop: Theme.paddingLarge * 2
    readonly property int screenRows: Math.max(1, Math.floor((flick.height - gridTop) / cellHeight))
    readonly property int visibleRows: Math.max(homeModel.usedRows + (editMode ? 1 : 0),
                                                screenRows,
                                                dragId >= 0 ? hoverRow + 2 : 0)

    property int dragId: -1
    property int hoverRow: -1
    property int hoverCol: -1
    property real grabX: 0
    property real grabY: 0
    property real lastSceneX: 0
    property real lastSceneY: 0
    property int scrollDirection: 0
    readonly property bool hoverOccupied: dragId >= 0 && hoverRow >= 0
                                          && homeModel.entryAt(hoverRow, hoverCol) >= 0
                                          && homeModel.entryAt(hoverRow, hoverCol) !== dragId

    property int pendingDropId: -1
    property int pendingDropRow: -1
    property int pendingDropCol: -1

    function launchApp(path, missing) {
        if (missing) {
            toast.show(qsTr("Application introuvable. Elle a peut-être été désinstallée."))
            return
        }
        if (!bridge.launch(path))
            toast.show(qsTr("Impossible de lancer l'application"))
    }

    function addApplications() {
        var dialog = pageStack.push(Qt.resolvedUrl("AppPickerDialog.qml"), {
            title: qsTr("Ajouter au bureau"),
            acceptLabel: qsTr("Ajouter"),
            disabledPaths: homeModel.allApps(),
            disabledHint: qsTr("Déjà sur le bureau")
        })
        if (!dialog)
            return
        dialog.accepted.connect(function() {
            var added = homeModel.addApps(dialog.selection)
            if (added > 0)
                flick.scrollToBottom()
        })
    }

    function newFolder() {
        var dialog = pageStack.push(Qt.resolvedUrl("AppPickerDialog.qml"), {
            title: qsTr("Nouveau dossier"),
            acceptLabel: qsTr("Créer"),
            showNameField: true,
            placedPaths: homeModel.allApps(),
            placedHint: qsTr("Sera déplacée dans le dossier")
        })
        if (!dialog)
            return
        dialog.accepted.connect(function() {
            homeModel.createFolder(dialog.folderName, dialog.selection)
        })
    }

    function addToFolder(folderId) {
        var dialog = pageStack.push(Qt.resolvedUrl("AppPickerDialog.qml"), {
            title: qsTr("Ajouter au dossier"),
            acceptLabel: qsTr("Ajouter"),
            disabledPaths: homeModel.folderPaths(folderId),
            disabledHint: qsTr("Déjà dans ce dossier"),
            placedPaths: homeModel.allApps(),
            placedHint: qsTr("Sera déplacée dans le dossier")
        })
        if (!dialog)
            return
        dialog.accepted.connect(function() {
            homeModel.addToFolder(folderId, dialog.selection)
        })
    }

    function startDrag(cell, pressX, pressY, entry) {
        editMode = true
        dragProxy.entryType = entry.entryType
        dragProxy.label = entry.name
        dragProxy.iconSource = entry.iconSource || ""
        dragProxy.folderApps = entry.folderApps || []
        dragProxy.missing = entry.missing
        dragProxy.x = cell.x
        dragProxy.y = cell.y
        grabX = pressX
        grabY = pressY
        hoverRow = entry.entryRow
        hoverCol = entry.entryColumn
        dragId = entry.entryId
    }

    function updateDragPosition() {
        var p = grid.mapFromItem(null, lastSceneX, lastSceneY)
        dragProxy.x = p.x - grabX
        dragProxy.y = p.y - grabY
        var cx = dragProxy.x + cellWidth / 2
        var cy = dragProxy.y + cellHeight / 2
        hoverCol = Math.max(0, Math.min(columns - 1, Math.floor(cx / cellWidth)))
        hoverRow = Math.max(0, Math.floor(cy / cellHeight))
    }

    function moveDrag(sceneX, sceneY) {
        lastSceneX = sceneX
        lastSceneY = sceneY
        updateDragPosition()

        var f = flick.mapFromItem(null, sceneX, sceneY)
        var edge = cellHeight * 0.6
        if (f.y < edge && !flick.atYBeginning)
            scrollDirection = -1
        else if (f.y > flick.height - edge)
            scrollDirection = 1
        else
            scrollDirection = 0
    }

    function endDrag(commit) {
        var id = dragId
        scrollDirection = 0
        if (commit && id >= 0 && hoverRow >= 0) {
            pendingDropId = id
            pendingDropRow = hoverRow
            pendingDropCol = hoverCol
            dropTimer.restart()
        }
        dragId = -1
        hoverRow = -1
        hoverCol = -1
    }

    Timer {
        id: dropTimer
        interval: 0
        onTriggered: {
            var result = homeModel.moveEntry(page.pendingDropId, page.pendingDropRow, page.pendingDropCol)
            page.pendingDropId = -1
            if (result === "folder")
                toast.show(qsTr("Dossier créé. Touchez-le pour le renommer."))
            else if (result === "added")
                toast.show(qsTr("Ajoutée au dossier"))
        }
    }

    Timer {
        id: autoScrollTimer
        interval: 16
        repeat: true
        running: page.dragId >= 0 && page.scrollDirection !== 0
        onTriggered: {
            var step = page.scrollDirection * Theme.paddingLarge
            var maxY = Math.max(0, flick.contentHeight - flick.height)
            flick.contentY = Math.max(0, Math.min(maxY, flick.contentY + step))
            page.updateDragPosition()
        }
    }

    RemorsePopup { id: remorse }

    SilicaFlickable {
        id: flick

        function scrollToBottom() {
            contentY = Math.max(0, contentHeight - height)
        }

        anchors.fill: parent
        contentHeight: Math.max(height, page.gridTop + grid.height + Theme.itemSizeLarge)
        interactive: page.dragId < 0

        PullDownMenu {
            enabled: page.dragId < 0
            MenuItem {
                text: qsTr("Paramètres")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: page.editMode ? qsTr("Terminer la modification") : qsTr("Modifier la disposition")
                onClicked: page.editMode = !page.editMode
            }
            MenuItem {
                text: qsTr("Nouveau dossier")
                onClicked: page.newFolder()
            }
            MenuItem {
                text: qsTr("Ajouter des applications")
                onClicked: page.addApplications()
            }
        }

        ViewPlaceholder {
            enabled: homeModel.count === 0 && !page.editMode
            text: qsTr("Votre bureau est vide")
            hintText: qsTr("Tirez vers le bas pour ajouter des applications")
        }

        Item {
            id: grid

            y: page.gridTop
            width: page.columns * page.cellWidth
            height: page.visibleRows * page.cellHeight
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                anchors.fill: parent
                onPressAndHold: page.editMode = true
                onClicked: if (page.editMode) page.editMode = false
            }

            Repeater {
                model: page.editMode ? page.visibleRows * page.columns : 0
                Rectangle {
                    x: (index % page.columns) * page.cellWidth + Theme.paddingSmall
                    y: Math.floor(index / page.columns) * page.cellHeight + Theme.paddingSmall
                    width: page.cellWidth - 2 * Theme.paddingSmall
                    height: page.cellHeight - 2 * Theme.paddingSmall
                    radius: Theme.paddingMedium
                    color: Theme.rgba(Theme.highlightBackgroundColor, 0.07)
                    border.width: Math.max(1, Math.round(Theme.pixelRatio))
                    border.color: Theme.rgba(Theme.highlightColor, 0.15)
                }
            }

            Rectangle {
                visible: page.dragId >= 0 && page.hoverRow >= 0
                x: page.hoverCol * page.cellWidth + Theme.paddingSmall
                y: page.hoverRow * page.cellHeight + Theme.paddingSmall
                width: page.cellWidth - 2 * Theme.paddingSmall
                height: page.cellHeight - 2 * Theme.paddingSmall
                radius: Theme.paddingMedium
                color: Theme.rgba(Theme.highlightBackgroundColor, page.hoverOccupied ? 0.45 : 0.25)
            }

            Repeater {
                model: homeModel

                delegate: Item {
                    id: cell

                    x: model.entryColumn * page.cellWidth
                    y: model.entryRow * page.cellHeight
                    width: page.cellWidth
                    height: page.cellHeight
                    opacity: page.dragId === model.entryId ? 0.2 : 1.0

                    Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }
                    Behavior on y { NumberAnimation { duration: 180; easing.type: Easing.InOutQuad } }

                    HomeIcon {
                        anchors.fill: parent
                        entryType: model.entryType
                        label: model.name
                        iconSource: model.iconSource
                        folderApps: model.folderApps
                        missing: model.missing
                        showLabel: homeModel.showLabels
                        down: cellMouse.pressed && cellMouse.containsMouse && page.dragId < 0
                        dropTarget: page.dragId >= 0 && page.dragId !== model.entryId
                                    && page.hoverRow === model.entryRow && page.hoverCol === model.entryColumn
                    }

                    MouseArea {
                        id: cellMouse

                        property bool suppressClick: false
                        property real pressX: 0
                        property real pressY: 0

                        anchors.fill: parent
                        preventStealing: page.dragId === model.entryId

                        onPressed: {
                            suppressClick = false
                            pressX = mouse.x
                            pressY = mouse.y
                            holdTimer.restart()
                        }
                        onPositionChanged: {
                            if (page.dragId === model.entryId) {
                                var s = cellMouse.mapToItem(null, mouse.x, mouse.y)
                                page.moveDrag(s.x, s.y)
                            } else if (holdTimer.running
                                       && (Math.abs(mouse.x - pressX) > Theme.startDragDistance
                                           || Math.abs(mouse.y - pressY) > Theme.startDragDistance)) {
                                holdTimer.stop()
                            }
                        }
                        onReleased: {
                            holdTimer.stop()
                            if (page.dragId === model.entryId) {
                                suppressClick = true
                                page.endDrag(true)
                            }
                        }
                        onCanceled: {
                            holdTimer.stop()
                            if (page.dragId === model.entryId)
                                page.endDrag(false)
                        }
                        onClicked: {
                            if (suppressClick || page.dragId >= 0)
                                return
                            if (model.entryType === "folder") {
                                folderOverlay.open(model.entryId)
                            } else if (!page.editMode) {
                                page.launchApp(model.desktopPath, model.missing)
                            }
                        }

                        Timer {
                            id: holdTimer
                            interval: page.editMode ? 250 : 550
                            onTriggered: {
                                cellMouse.suppressClick = true
                                page.startDrag(cell, cellMouse.pressX, cellMouse.pressY, {
                                    entryId: model.entryId,
                                    entryType: model.entryType,
                                    name: model.name,
                                    iconSource: model.iconSource,
                                    folderApps: model.folderApps,
                                    missing: model.missing,
                                    entryRow: model.entryRow,
                                    entryColumn: model.entryColumn
                                })
                            }
                        }
                    }

                    Rectangle {
                        visible: page.editMode && page.dragId < 0
                        width: Theme.iconSizeSmallPlus
                        height: width
                        radius: width / 2
                        x: Theme.paddingSmall
                        y: Theme.paddingSmall
                        color: Theme.rgba(Theme.highlightDimmerColor, 0.85)
                        border.width: Math.max(1, Math.round(Theme.pixelRatio))
                        border.color: Theme.rgba(Theme.primaryColor, 0.6)

                        Image {
                            anchors.centerIn: parent
                            width: Theme.iconSizeSmall
                            height: width
                            sourceSize.width: width
                            sourceSize.height: height
                            source: "image://theme/icon-s-clear-opaque-cross"
                        }

                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -Theme.paddingMedium
                            onClicked: {
                                var id = model.entryId
                                if (model.entryType === "folder") {
                                    remorse.execute(qsTr("Suppression du dossier « %1 »").arg(model.name), function() {
                                        homeModel.removeEntry(id)
                                    })
                                } else {
                                    homeModel.removeEntry(id)
                                }
                            }
                        }
                    }
                }
            }

            HomeIcon {
                id: dragProxy
                visible: page.dragId >= 0
                width: page.cellWidth
                height: page.cellHeight
                showLabel: homeModel.showLabels
                scale: 1.1
                z: 10
            }
        }

        VerticalScrollDecorator {}
    }

    Button {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.paddingLarge
        }
        visible: page.editMode && page.dragId < 0 && !folderOverlay.opened
        text: qsTr("Terminé")
        onClicked: page.editMode = false
    }

    FolderOverlay {
        id: folderOverlay
        onLaunchRequested: {
            folderOverlay.close()
            page.launchApp(path, missing)
        }
        onAddRequested: page.addToFolder(folderId)
    }

    Toast { id: toast }

    onStatusChanged: {
        if (status === PageStatus.Activating)
            bridge.reloadPartnerSpace()
    }
}
