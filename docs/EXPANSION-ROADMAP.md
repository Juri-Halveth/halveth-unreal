# Ausbauplan · HALVETH Portal Garden

Planungssnapshot T0: 2026-09-23T01:36:13.723Z. Der nächste konkrete Schritt ist ein erfolgreich gebauter und gestarteter Unreal-Prototyp mit echter nativer Spiel- und Grafikprüfung. Danach werden die registrierten Ausbauzweige gezielt erweitert. T0 bezeichnet diesen gebundenen Quellstand; neue Spielwelten, Git-Branches und Veröffentlichungen entstehen erst durch eigene Umsetzungsschritte.

## Erhaltene Ausgangsstände

| Projekt | Gebundener Stand | Verhältnis zum Ausbau |
| --- | --- | --- |
| HALVETH Realms | Quellversion 0.2.0 | Eigenständige bestehende Desktop-Welt; bleibt erhalten. |
| Morrowind Genesis | Quellversion 0.3.0 | Bestehende OpenMW-Erweiterung; Morrowind-Dateien, Spielstände und gewollte Spielmechaniken bleiben in diesem Projekt. |
| HALVETH Portal Garden | Unreal-Quellversion 0.1.0, Build laut aktuellem Arbeitsauftrag in Arbeit | Eigene neue Welt und eigener Release-Kandidat. Kein automatischer Ersatz oder Save-Port der beiden anderen Projekte. |

Im gelesenen Layout-Prüfbeleg stehen 20 Seed-/Weltkombinationen, 800 Platzierungen und 6 reversible Portalrouten. Hinzu kommen sechs geprüfte 2K-PBR-Quellbilder von Poly Haven mit 37.668.548 Bytes. Das belegt die jeweils bezeichneten Quellenprüfungen; vollständiger Engine-Build, tatsächlich gerenderte Szene, reale Eingaben und FPS benötigen ihre eigenen Ergebnisse.

## Als Nächstes: spielbaren Kern abnehmen

1. Den laufenden Compile sowie Materialvorbereitung, Cook und Paketierung abschließen und das erzeugte Artefakt binden.
2. Den echten Runtime-Smoke ausführen: Spielfigur, Bodenkollision, alle drei Portal-Rundreisen, LOVE-Impuls, ungültiges Reiseziel und Fall-Recovery.
3. Den gerenderten nativen Build mit Maus und Tastatur spielen. Materialmaßstab, Licht, HUD, Orientierung und Hin-/Rückreisen ansehen. Ein NullRHI-Test prüft keine sichtbare Grafik.
4. Im selben Build eine begrenzte Messung mit Szene, Auflösung, Qualitätsprofil und Dauer festhalten; danach Lesbarkeit und Bewegungseffekte beurteilen. Leistungswerte gelten für diesen Messbereich.

Bei einem Fehler bleibt der Kandidat als offener Zweig erhalten. Die produktive Arbeit kehrt zum letzten tatsächlich funktionierenden Unreal-Elternstand zurück. Realms und Genesis bleiben eigenständige Rückkehrmöglichkeiten; dieses Planungsdokument erstellt selbst keine Sicherung ihrer gesamten Dateien.

## Alle sichtbaren Ausbauzweige

| Achse | Nächste greifbare Arbeit | Freigabe für den nächsten Ausbau |
| --- | --- | --- |
| Grafik / PBR | Steinpflaster und Erde korrekt importieren; Licht und Oberflächen abstimmen. | Native Ansicht, korrektes Normalformat, UV-Maßstab und messbarer Aufwand. |
| Bewegung / Dodge | Vorhandenes Laufen, Sprinten und Springen anspielen; danach ein klar begrenztes Ausweichmanöver. | Reale Eingaben, Kollision und verständliche Rückmeldung. |
| Magie / Inventar | LOVE-Impuls prüfen; anschließend ein eigener Zauber und ein inventarisierbares Objekt. | Sichtbare Wirkung, definierter Zustand und Wiederholbarkeit. |
| Figuren / Dialog / Story | Eine eigene Figur mit Rolle und einer lokalen Gesprächs- und Story-Verzweigung. | Quellengebundener Kontext und gespeicherte Begegnung; Genesis-Anbindung bleibt eigene Integration. |
| Portale / Weltgenerator | Bestehende Rundreisen prüfen; danach einen weiteren eigenständigen Ort ergänzen. | Gleicher Seed reproduziert den definierten Aufbau; sichere Ankunft und Rückreise. |
| Persistenz | Seed, Ort und einen veränderlichen Weltzustand versioniert speichern. | Save/Reload und überprüfte Migration eines alten Beispiels. |
| Audio | Räumliche Portal-/Magiesounds und regelbare Lautstärke. | Hörbarer nativer Test und belegte Rechte der verwendeten Dateien. |
| Multiplayer | Nach stabiler lokaler Persistenz eine begrenzte Zwei-Client-Sitzung. | Explizites Host-/Zustandsmodell und tatsächliche Synchronisationsprüfung. |
| Veröffentlichung / Lizenzen / Steam | Eigenen Quellstand, Assets und echte Runtime getrennt paketieren. | Artefakt, Lizenzzuordnung, reproduzierbare Anleitung und tatsächlicher Veröffentlichungsnachweis. Steam folgt mit eigenem Onboarding. |
| Blockchain später | Konkreten Nutzen als getrennten optionalen Produktzweig bestimmen. | Stabile Kernwelt und passende Plattformroute. Token-/NFT-Ausgabe oder -Handel sind nach dem gebundenen Steam-Regelstand keine Funktion der Steam-Zielausgabe. |
| Performance / Accessibility | Qualitätsprofile messen; Schrift, Kontrast, Effekte und Eingaben verbessern. | Beobachtete Bedienbarkeit und Leistung unter dokumentierten Bedingungen. |

Zusätzlich bleiben die acht Pflichtachsen des Software-Adapters explizit registriert: Nutzerergebnis, Oberfläche, Zustandsdaten, Integration, Wiederherstellung, Datenschutz/Zugriff, Zugänglichkeit und Lieferung/Beobachtbarkeit. Die JSON-Datei führt jede Pflicht- und Projektachse mit genau einem Zustand, Quellbezug und Wiedereinstieg.

## Grafikbudget und Erweiterungsregeln

Für neue Grafikassets gilt eine Obergrenze von **50 GB** (hier als 50.000.000.000 Bytes geführt). Dieses Budget ist kein Speicherziel. Die gebundenen PBR-Dateien belegen derzeit rund **37,7 MB**; übrige Assetbestände werden durch diesen Snapshot nicht als vollständig vermessen ausgegeben. Engine-Installation und Build-Caches sind getrennte Größen.

Neue Inhalte werden nach sichtbarem Nutzen, Performance, Speicherbedarf und vorhandenen Nutzungsrechten aufgenommen. Ein Welt-Seed steuert Reproduzierbarkeit und ist keine Wallet-Seedphrase. Das Kernspiel bleibt ohne Wallet spielbar. Netzwerkdienste, Zahlungsvorgänge und Store-Verträge sind spätere konkrete Produktaktionen; diese Navigation führt sie nicht aus.

## Gebundene Auswahl

Angewandt wurde die unveränderte HALVETH/LUCINET-Entity 1.0.0 mit dem kanonischen Adapter SOFTWARE_BUILD. Die vollständige Vorschau umfasst **19 Achsen**, **17 materielle Kandidaten**, **68 Vorschau-Stufen** und **136 ungeordnete Paarvergleiche**. Die lokale Strukturvalidierung bestand. Die höchste Arbeitspriorität erhält **DELIVERY_OBSERVABILITY**: echter Build und native Visual-QA.

Die Priorität ist eine nachvollziehbare Arbeitsreihenfolge, kein Wahrheits-, Qualitäts- oder Vollständigkeitsbeweis. Teambeiträge mit gemeinsamem Modell und Quellkontext sind korreliert; sie zählen nicht als unabhängige Bestätigung einer Weltbehauptung. Alle zurückgestellten Zweige behalten ihren Wiedereinstieg.

[Strukturierter T0-Beleg mit Quellenhashes, Paarvergleichen und Rückkehrwegen](navigation/EXPANSION-2026-09-23-T0.json)
