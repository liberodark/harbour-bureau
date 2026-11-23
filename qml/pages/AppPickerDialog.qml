import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property string title
    property string acceptLabel: qsTr("Ajouter")
    property bool showNameField: false
    property var disabledPaths: []
    property string disabledHint
    property var placedPaths: []
    property string placedHint

    property var selection: []
    readonly property string folderName: nameField.text

    function isSelected(path) {
        return selection.indexOf(path) >= 0
    }

    function toggle(path) {
        var copy = selection.slice()
        var i = copy.indexOf(path)
        if (i >= 0)
            copy.splice(i, 1)
        else
            copy.push(path)
        selection = copy
    }

    allowedOrientations: Orientation.Portrait
    canAccept: selection.length > 0

    Component.onCompleted: appModel.filterText = ""
    onDone: appModel.filterText = ""

    Column {
        id: header
        width: parent.width

        DialogHeader {
            title: dialog.selection.length > 0
                   ? dialog.title + " (" + dialog.selection.length + ")"
                   : dialog.title
            acceptText: dialog.acceptLabel
        }

        TextField {
            id: nameField
            visible: dialog.showNameField
            width: parent.width
            placeholderText: qsTr("Nom du dossier")
            label: qsTr("Nom du dossier")
            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false
        }

        SearchField {
            id: searchField
            width: parent.width
            placeholderText: qsTr("Rechercher une application")
            EnterKey.iconSource: "image://theme/icon-m-enter-close"
            EnterKey.onClicked: focus = false
            onTextChanged: appModel.filterText = text
        }
    }

    SilicaListView {
        id: listView

        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true
        model: appModel

        delegate: ListItem {
            id: item

            readonly property bool disabledApp: dialog.disabledPaths.indexOf(desktopPath) >= 0
            readonly property bool placedApp: !disabledApp && dialog.placedPaths.indexOf(desktopPath) >= 0
            readonly property bool selected: dialog.selection.indexOf(desktopPath) >= 0

            width: listView.width
            contentHeight: Theme.itemSizeMedium
            enabled: !disabledApp
            highlighted: down || selected

            Image {
                id: icon
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSizeMedium
                height: width
                sourceSize.width: width
                sourceSize.height: height
                source: iconSource
                asynchronous: true
                opacity: item.disabledApp ? Theme.opacityLow : 1.0
            }

            Column {
                anchors {
                    left: icon.right
                    leftMargin: Theme.paddingMedium
                    right: check.left
                    rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                Label {
                    width: parent.width
                    truncationMode: TruncationMode.Fade
                    color: item.disabledApp ? Theme.secondaryColor
                                            : (item.highlighted ? Theme.highlightColor : Theme.primaryColor)
                    text: searchField.text.length > 0
                          ? Theme.highlightText(name, searchField.text, Theme.highlightColor)
                          : name
                }

                Label {
                    width: parent.width
                    visible: text.length > 0
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                    text: item.disabledApp ? dialog.disabledHint
                                           : (item.placedApp ? dialog.placedHint
                                                             : (isAndroid ? qsTr("Android") : ""))
                }
            }

            Image {
                id: check
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                source: "image://theme/icon-m-acknowledge"
                visible: item.selected
            }

            onClicked: dialog.toggle(desktopPath)
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: qsTr("Aucune application")
        }

        VerticalScrollDecorator {}
    }
}
