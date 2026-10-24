# Wiese, Wind und gebundener Normalstart

Prüftag: 24. Oktober 2026. Ausgangsstand: `5c2260f`. Der Kandidat ergänzt die
vorhandene native Astralis-/Bewohner-Linie; die historische Ausgangsversion
und ihr lokales Paket bleiben erhalten. Der genaue Quellen-, Bild- und
Paketstand steht in [QA/native-meadow.json](../QA/native-meadow.json).

## Was im Spiel entsteht

Vier vorbereitete Meshvarianten enthalten jeweils 128 unterschiedlich
parametrisierte, gekrümmte und verjüngte Halme. Die Halme besitzen eine
transversale Faltung und geschlossene Spitzen; die Blüten bestehen ebenfalls
aus Dreiecksgeometrie. Das Wiesenmaterial ist opak und zweiseitig. Es verwendet
keine kamerafolgenden Alphakarten für die neuen Halme.

Die Windfunktion arbeitet auf unverformten Weltkoordinaten, Zeit, Halmhöhe und
einer gebundenen Phase. Ihr quadratischer Höhenfaktor hält die Wurzel fest.
CPU-Funktion und Shader verwenden denselben Quelltextvertrag; Gleitkomma-
Rundung auf der GPU und die tatsächlichen GPU-Positionen wurden nicht
zurückgelesen. Ein Distanzübergang senkt ferne Halme zur Wurzel ab.

Die Platzierung nutzt eine begrenzte Menge von 31 × 31 × 4 Slots. Weltzellen
bestimmen Artvariante, Platzierung und Phase unabhängig von der Besuchsfolge.
Pro Frame werden höchstens 24 Zellen mit bis zu 96 Bodenabfragen erneuert.
Jeder neue Standort wird an der tatsächlich kollidierenden Geländeoberfläche
ausgerichtet. Im Archivbereich bleibt Platz für Bauwerk und Treppe.

Gras- und Felsgewichte sowie ein statisches Feuchtefeld verbinden
Bodenmaterial und Vegetationsauswahl. Der Boden verwendet die vorhandenen
CC0-Waldbodenkarten; die Farnzahl und ihre bisher übergroße Skalierung wurden
verringert. Das ist eine Gestaltungshilfe, kein Pflanzenwachstums-, Wetter-
oder vollständiges Bodenchemie-Modell.

## Was geprüft wurde

- Der portable C++-Test untersucht Geometrie, Phasen, feste Wurzeln,
  Windgrenzen und acht absichtlich fehlerhafte Gegenmodelle. Er läuft mit
  LLVM und MSVC sowie in den Windows-/Linux-Quellchecks.
- Ein nativer RHI-Lauf prüft 16 Wurzelpunkte gegen die Gelände-Dreiecke.
  Die zulässige Abweichung beträgt fünf Zentimeter; ein um 50 Zentimeter
  angehobener synthetischer Stand muss an derselben Regel scheitern.
  Diese Stichprobe ist keine vollständige Prüfung jedes Halms.
- Die tatsächliche Auswahl des isolierten Testspielstands wird im nativen
  Lauf geprüft. `HalvethMotionCapture` wurde in die bestehende Auswahl
  aufgenommen; private Spielstände werden für die Aufnahmen nicht benötigt.
- Die bestehenden Prüfpfade für Treppe, Buch, Flug, geschlossene Tür,
  Sichtverdeckung, Warnung, Verfolgung, kurzes Festhalten und Freigabe wurden
  erneut ausgeführt. Ihre Fixtures bleiben als solche bezeichnet.
- Der Perspektivkorridor prüft 48 aus der nativen Geometrie abgeleitete Punkte
  über vier deklarierte mathematische Projektionen. Eine absichtlich
  veränderte Beobachtung wird verworfen. Das prüft den Projektionsvertrag;
  tatsächliche Pixel, Sichtbarkeit und menschliches Spielgefühl benötigen
  jeweils eigene Beobachtungen.

Der ruhige Renderlauf misst neun Simulationssekunden nach drei Sekunden
Aufwärmzeit bei fester Kamera, ohne Screenshots und mit weiterlaufenden
Bewohnern. Auf dem lokalen Prüfgerät lagen Median und 95. Perzentil bei
16,899 beziehungsweise 42,512 Millisekunden (428 Intervalle, Qualitätsprofil
1, 60-FPS-Grenze, 2560 × 1440). Das ist ein begrenzter Szenenlauf. Er ersetzt
keine Messung beim Reisen durch alle vier Welten oder auf anderer Hardware.
Aufnahme-Läufe mit PNG-Schreiben werden gesondert geführt.

## Start- und Quellenkette

`Start-Astralis.cmd` verwendet das getrennte lokale Paket
`.local/meadow-package/Windows`. Das zuvor geprüfte Paket unter
`.local/living-world-package/Windows` wird nicht überschrieben.
Der Starter bindet Quellen, die tatsächlich verwendete Himmelsdatei,
ausgewählte Vorbereitungs-/Lizenzdateien, beide EXE-Dateien und die gekochten
Pak-/IoStore-Dateien. Eine unpassende Bindung veranlasst Vorbereitung und Bau.

Das Projekt verwendet für den lokalen Cook eine Dateiablage. Dadurch ist
zwischen Cook-Ende und Archivierung kein fortlaufender Zen-Cookdienst nötig.
Das ist eine Projekteinstellung; globale Windows- und Engine-Einstellungen
werden nicht verändert.

Der Sonnenhelfer veröffentlicht seine validierten Daten jetzt sowohl in der
kompatiblen Quellenablage als auch unter `Content/sky/ephemeris.json`, dem
tatsächlichen nativen Lesepfad. Am Prüftag wurde der öffentliche
NASA/JPL-Horizons-Verlauf für den **24.10.2026** abgerufen: 25 stündliche
Samples für einen ausdrücklich virtuellen Ort bei 45° N, 0° E. Das ist weder
der Standort des Nutzers noch ein Wetterdienst. Der native Tageszyklus
verwendet weiterhin die dokumentierte Interpolation und tägliche Wiederholung.

Der frische Quell-Export enthält nun die 30 ausdrücklich ausgewählten
VOID-Bauquellen, die sein Standard-Bootstrap benötigt. Unveröffentlichte
Arbeitsreste und generierte Engine-/Content-Bäume gehören nicht dazu.
Der Download prüft sein Bytebudget vor jedem Schreibvorgang, hält den
Manifest-Gesamtdeckel ein und veröffentlicht erst passende Größe und SHA-256.
Offline-Gegenproben prüfen Fehlerbereinigung und legitime Downloads.

## Sichtbare offene Arbeit

Die vier Landschaften sind weiterhin jeweils 600 × 600 Meter groß. Bäume
verwenden denselben vorhandenen Scan, Bewohner drei geteilte Grundmodelle.
Fernbewuchs, Baumvielfalt, Wege, Materialübergänge und die Bodenschichtung
brauchen weitere Gestaltungsarbeit. Die Wiese ist kein grabbares Weltvolumen.
Die vorhandenen kleinen dynamischen Bodenfelder bleiben getrennt davon.
Bewohner besitzen lokale verfasste Routinen und Antworten; eine vollständige
Ökonomie, freie Sprachmodelldialoge und dauerhafte soziale Erinnerungen sind
damit nicht hergestellt.

Die geprüfte aktuelle UE-5.8-Richtung
[Nanite Foliage](https://dev.epicgames.com/documentation/unreal-engine/nanite-foliage?lang=en-US)
verbindet Assemblies, Voxels und Skinning und ist dort als experimentell
gekennzeichnet. Sie bleibt eine eigene Ausbauoption. Dieser Kandidat verwendet
die begrenzte klassische Instanzierung und die vorhandene Schattenkonfiguration.

## Rechte und kommerzielle Verwendung

[LICENSES.md](../LICENSES.md) ordnet nur die neuen eigenen Beiträge in sechs
Dateien der unverändert übernommenen PIRL 2.0 zu. Bestehende MIT-Erlaubnisse,
überlappende MIT-Teile, CC0-Assets und Epics Bedingungen bleiben erhalten.
Der kommerzielle Vertragspfad gilt nur für die zugeordneten schutzfähigen
Beiträge im tatsächlich kontrollierten Umfang. Das ist keine Exklusivität
über Ideen, mathematische Methoden oder enthaltenes Fremdmaterial.
