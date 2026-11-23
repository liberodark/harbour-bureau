import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    Image {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -Theme.paddingLarge
        width: Theme.iconSizeLarge
        height: width
        sourceSize.width: width
        sourceSize.height: height
        source: "/usr/share/icons/hicolor/172x172/apps/harbour-bureau.png"
        opacity: 0.9
    }

    Label {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: Theme.paddingLarge * 2
        }
        text: qsTr("Bureau")
        color: Theme.highlightColor
    }
}
