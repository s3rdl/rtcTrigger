# Boom Remote App

Capacitor-based native BLE app for Android and iPhone.

## Why

This app uses native BLE instead of Web Bluetooth, so it works on iPhone and Android without relying on browser BLE support.

## Setup

```bash
nvm use
npm install
```

## Native Projects

Add platforms once:

```bash
npx cap add android
npx cap add ios
```

## Run

Android:

```bash
npm run android
```

iPhone:

```bash
npm run ios
```

These commands:

- build the web app with Vite
- sync Capacitor assets and plugins
- open Android Studio or Xcode

## iOS Notes

The repository already adds Bluetooth usage descriptions to `ios/App/App/Info.plist`:

- `NSBluetoothAlwaysUsageDescription`
- `NSBluetoothPeripheralUsageDescription`

Open the Xcode project here:

```text
client/app/ios/App/App.xcodeproj
```

Then:

1. choose your iPhone as the run target
2. set a signing team in the `App` target under `Signing & Capabilities`
3. press `Run`

If iOS asks for Bluetooth permission, allow it.

## Android Notes

Android will prompt for Bluetooth and nearby-device permissions when the app scans.

## BLE Behavior

The app scans for the custom Boom Remote BLE service and then uses the same commands as the web UI.
