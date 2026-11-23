import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: root

    property string entryType: "app"
    property string label
    property string iconSource
    property var folderApps: []
    property bool missing: false
    property bool showLabel: true
    property bool down: false
    property bool dropTarget: false
    property real iconSize: Math.min(Theme.iconSizeLauncher, width * 0.78)

    readonly property real _labelHeight: showLabel ? labelItem.implicitHeight + Theme.paddingSmall : 0

    Item {
        id: iconBox
        width: root.iconSize
        height: root.iconSize
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.max(0, Math.round((root.height - root.iconSize - root._labelHeight) / 2))
        scale: root.dropTarget ? 1.15 : 1.0
        Behavior on scale { NumberAnimation { duration: 120 } }

        Image {
            id: appImage
            anchors.fill: parent
            visible: root.entryType === "app"
            source: root.entryType === "app" ? (root.iconSource || "image://theme/icon-launcher-default") : ""
            sourceSize.width: width
            sourceSize.height: height
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
            opacity: root.missing ? Theme.opacityLow : 1.0
            onStatusChanged: {
                if (status === Image.Error && source != "image://theme/icon-m-sailfish")
                    source = "image://theme/icon-m-sailfish"
            }
        }

        Rectangle {
            anchors.fill: parent
            visible: root.entryType === "folder"
            radius: width * 0.24
            color: Theme.rgba(Theme.highlightDimmerColor, 0.55)
            border.width: Math.max(1, Math.round(Theme.pixelRatio))
            border.color: Theme.rgba(Theme.primaryColor, root.dropTarget ? 0.8 : 0.25)

            Grid {
                anchors.centerIn: parent
                columns: 2
                spacing: Math.round(parent.width * 0.06)

                Repeater {
                    model: root.entryType === "folder" ? Math.min(4, root.folderApps ? root.folderApps.length : 0) : 0
                    Image {
                        width: Math.round(iconBox.width * 0.34)
                        height: width
                        sourceSize.width: width
                        sourceSize.height: height
                        source: root.folderApps[index].iconSource
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        smooth: true
                    }
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: Theme.highlightBackgroundColor
            opacity: root.down ? Theme.highlightBackgroundOpacity : 0.0
        }
    }

    Label {
        id: labelItem
        visible: root.showLabel
        anchors {
            top: iconBox.bottom
            topMargin: Theme.paddingSmall
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingSmall
            rightMargin: Theme.paddingSmall
        }
        text: root.label
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        maximumLineCount: 1
        font.pixelSize: Theme.fontSizeTiny
        color: root.down || root.dropTarget ? Theme.highlightColor : Theme.primaryColor
        opacity: root.missing ? Theme.opacityLow : 1.0
    }
}
