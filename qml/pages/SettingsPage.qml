import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Paramètres") }

            SectionHeader { text: qsTr("Accueil de Sailfish") }

            TextSwitch {
                text: qsTr("Afficher Bureau sur l'accueil")
                description: bridge.otherPartnerApps > 0
                             ? qsTr("Bureau devient une page de l'accueil, à droite des applications ouvertes. Les %n autre(s) page(s) ajoutée(s) à l'accueil seront retirées.", "", bridge.otherPartnerApps)
                             : qsTr("Bureau devient une page de l'accueil, à droite des applications ouvertes.")
                automaticCheck: false
                checked: bridge.partnerSpaceEnabled
                onClicked: {
                    var enabling = !bridge.partnerSpaceEnabled
                    var ok = enabling ? bridge.enablePartnerSpace() : bridge.disablePartnerSpace()
                    if (!ok) {
                        errorLabel.visible = true
                        return
                    }
                    errorLabel.visible = false
                    remorse.execute(qsTr("Redémarrage de l'accueil"), function() {
                        bridge.restartHomescreen()
                    })
                }
            }

            Label {
                id: errorLabel
                visible: false
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.errorColor
                text: qsTr("Impossible de modifier le réglage de l'accueil.")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("L'accueil redémarre pour appliquer le changement : l'écran clignote quelques secondes, c'est normal.")
            }

            Item { width: 1; height: Theme.paddingLarge }

            ButtonLayout {
                Button {
                    text: qsTr("Redémarrer l'accueil")
                    onClicked: remorse.execute(qsTr("Redémarrage de l'accueil"), function() {
                        bridge.restartHomescreen()
                    })
                }
            }

            SectionHeader { text: qsTr("Grille") }

            Slider {
                id: columnsSlider
                width: parent.width
                minimumValue: 3
                maximumValue: 6
                stepSize: 1
                value: homeModel.columns
                valueText: value
                label: qsTr("Icônes par ligne")
                onDownChanged: if (!down) homeModel.columns = value
            }

            TextSwitch {
                text: qsTr("Afficher le nom des applications")
                checked: homeModel.showLabels
                onCheckedChanged: homeModel.showLabels = checked
            }

            SectionHeader { text: qsTr("Bureau") }

            ButtonLayout {
                Button {
                    text: qsTr("Tout effacer")
                    onClicked: remorse.execute(qsTr("Effacement du bureau"), function() {
                        homeModel.clearAll()
                    })
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryHighlightColor
                text: qsTr("Retire toutes les icônes et tous les dossiers de Bureau. Aucune application n'est désinstallée.")
            }

            SectionHeader { text: qsTr("À propos") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
                text: qsTr("Bureau %1 — licence GPLv3.").arg(bridge.version)
            }
        }

        VerticalScrollDecorator {}
    }
}
