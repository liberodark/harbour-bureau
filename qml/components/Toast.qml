import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: toast

    function show(text) {
        label.text = text
        opacity = 1.0
        hideTimer.restart()
    }

    anchors {
        horizontalCenter: parent.horizontalCenter
        bottom: parent.bottom
        bottomMargin: Theme.itemSizeLarge
    }
    width: Math.min(parent.width - 2 * Theme.horizontalPageMargin, label.implicitWidth + 2 * Theme.paddingLarge)
    height: label.height + 2 * Theme.paddingMedium
    radius: Theme.paddingMedium
    color: Theme.rgba(Theme.highlightDimmerColor, 0.9)
    opacity: 0.0
    visible: opacity > 0
    z: 100
    Behavior on opacity { FadeAnimation {} }

    Label {
        id: label
        anchors.centerIn: parent
        width: Math.min(implicitWidth, toast.parent.width - 2 * Theme.horizontalPageMargin - 2 * Theme.paddingLarge)
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.highlightColor
    }

    Timer {
        id: hideTimer
        interval: 2500
        onTriggered: toast.opacity = 0.0
    }
}
