# Bureau

An Android-style home screen for Sailfish OS (built for Sailfish OS 5.2, Jolla Phone 2026).

- Free icon grid: place your Sailfish and Android apps anywhere, empty cells included.
- Folders: drop an app on another app to create a folder, or on a folder to add it.
- Edit mode: long press an icon or an empty area. The cross removes the icon from Bureau; nothing is uninstalled.
- Can be shown as a page of the Sailfish home screen (partner space), to the right of the running apps.

## Usage

| Action | Gesture |
|---|---|
| Launch an app | Tap its icon |
| Add apps | Pull-down menu → Ajouter des applications |
| Move an icon | Long press, then drag |
| Create a folder | Drag an app onto another one (or pull-down menu → Nouveau dossier) |
| Open / rename a folder | Tap the folder, then tap its name |
| Take an app out of a folder | Inside the folder, long press → Sortir du dossier |
| Leave edit mode | "Terminé" button or tap an empty area |
| Show on the Sailfish home screen | Pull-down menu → Paramètres → Afficher Bureau sur l'accueil |

The layout is stored in `~/.local/share/fr.bureau/bureau/layout.json`.

## Building

With the Sailfish SDK Docker image:

```
docker run --rm -v $PWD:/workspace coderus/sailfishos-platform-sdk:5.2.0.15 \
  bash -c "cp -r /workspace ~/b && cd ~/b && mb2 -t SailfishOS-5.2.0.15-aarch64 build"
```

## Tests

Built and run in the i486 target of the SDK:

- `tests/tst_homemodel`: grid, folder and persistence logic.
- `tests/qmlcheck`: loads every QML file with Silica and runs a scripted scenario.
