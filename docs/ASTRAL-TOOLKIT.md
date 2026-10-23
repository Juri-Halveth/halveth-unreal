# Astralis — Referenzen als räumlicher Spielebaukasten

Juri Janovski · HALVETH / LUCINET · Entwicklung ab 23.10.2026.

Die 16 vom Projektinhaber bereitgestellten Illustrationen verbinden räumliche
Architektur, Materialkontrast, Bibliothek, Portale und Figuren. Der neue
Astralbereich ist eine Erweiterung des nativen Unreal-Spiels im Startgebiet.
Er steht auf dem bestehenden Gelände und verwendet dessen Spielfigur, Eingaben,
Zaubersystem und Buchleser.

Die Originalbilder bleiben privat. [Das Referenzinventar](ASTRAL-REFERENCE-RECIPES.json)
enthält pro Bild Dateikennung, SHA-256, Abmessungen, sichtbare Merkmale und eine
technische Umsetzungshypothese. Alle 16 Standbilder wurden berücksichtigt.
Bildluminanz ist relativ zur sRGB-Datei gemessen; ein Standbild verrät weder
verdeckte Geometrie noch Bewegungsmechanik, Beleuchtungswerte in Lux oder
ursprünglichen Quellcode. Die beschriebenen Mechanismen sind unser Entwurf.

## Spiel und Bedienung

`Start-Astralis.cmd` startet das lokale Windows-Spiel an der neuen Treppe.
Beim ersten Start baut der Helfer bei Bedarf das Paket. Dafür werden dieselben
UE-5.8-/Windows-C++-Voraussetzungen und vorhandenen Assetquellen wie im
Hauptprojekt verwendet. Spätere Starts vergleichen den Quellfingerprint.
Das lokale Entwicklungspaket liegt unter `.local/astral-package/Windows`.

- WASD/Maus: gehen und schauen; Leertaste: springen.
- Auf der Plattform vor der Pforte dreimal mit Abstand **E** drücken.
  Jeweils ein Ring wird ausgerichtet; die massiven Türflügel öffnen sich.
- Nach dem Öffnen erneut **E**: ins obere Archiv wechseln.
- Die geöffnete Kreisfläche zeigt eine zusätzliche Live-Ansicht der Archivszene.
  Ihre Kamera übernimmt Blickrichtung, Sichtwinkel und Verschiebung der
  Spielfigur zwischen zwei gleich ausgerichteten Schnittstellen.
- **E** am oberen Ankunftskreis: zurückkehren.
- **E** am aufgeschlagenen Buch: den vorhandenen Buchleser öffnen.
- **B**: vorhandene Bibliothek; **Q**: Zauber wählen, **linke Maustaste**: wirken.
  Ein erfolgreicher Zauber aktiviert kurz den räumlichen Fokus.
- Nach der dritten Ringausrichtung **J**: Levitation beginnen oder beenden.
  WASD und Blickrichtung steuern; Leertaste steigt, Strg sinkt, Shift beschleunigt.
  Die kurze räumliche Spur folgt ausschließlich gemessenen Spielerpositionen.
  Sie wird nach 1,2 Sekunden entfernt und bei einem Teleportsprung verworfen.

Der Fokus ist ein Artefakt vor der Kamera. Anatomische Spielerhände sind in
diesem Schritt noch nicht vorhanden. Der Übergang versetzt die Spielfigur an
einen vorab auf Kapselkollision geprüften Zielpunkt. Der Blick ins Archiv ist
bereits gerendert; automatisches, nahtloses Hindurchgehen und beliebig gedrehte
oder rekursive Portale bleiben eigene offene Ausbaustufen.

## Wiederverwendbare Werkzeuge

| Baustein | Quelle | Zweck und aktueller Umfang |
|---|---|---|
| Geschlossene Körper | `Source/HALVETHRealms/Public/AstralGeometry.h` | Abgeschrägte Blöcke, Torus, Bogensteine und facettierte Kristalle; tragende Teile haben Volumen. |
| Portalzustand | derselbe portable Header | Drei Ausrichtungen, Eingabeabstand, zeitstabile Öffnung und Freigabeschwelle. |
| Verknüpfte Ansicht | `linkedObserver`, `UpdateAperture`, `M_AstralAperture` | Translationsgebundene Kamera und bildschirmbezogener Ausschnitt auf einer kreisförmigen Schnittstelle. |
| Räumlicher Ort | `HALVETHAstralSanctum.cpp` | Fundament, Treppe, Fugen, Ringgeometrie, Leuchten, Gewölbe, Bücher, Lesepult und obere Terrasse. |
| Mineralrelief | `scripts/generate_astral_mineral.py` | Deterministische periodische Normal-/Rauheitskarten; eigene synthetische Pixel, keine Referenzkopien. |
| Materialbau | `AstralPrepareCommandlet.cpp` | Weltgebundene Farbvariation, flächenbezogene UVs, Normalen, Rauheit und metallische Einlagen. |
| Referenzarbeit | `scripts/extract_astral_references.py` | Lokale Messung und getrennte Beobachtungen/Umsetzungsideen für alle 16 Bilder. |
| Nativer Lauf | `scripts/astral.ps1` | Lokaler Paketbau, normaler Start, isolierter Gehtest und gerenderte Aufnahmen. |
| Levitation und Spur | `HALVETHCharacter.cpp`, `AstralGeometry.h` | Begrenzter Flug mit Kapselkollision und Luftwiderstandsmodell; geschlossener, zeitlich begrenzter Schlauch aus der geflogenen Bahn. |

Die statischen Bauteile werden nach Material in wenige Procedural-Mesh-Gruppen
zusammengefasst. Türflügel, drei Ringe, Fokus und die kurze Flugspur werden
bewegt. Kleine lokale Lichtquellen haben hier keine eigenen Schattenkarten.
Die Vegetationsverteilung reserviert den tatsächlichen Gebäudegrundriss,
damit Bäume und Farne nicht in Regale oder Treppen wachsen.

Die zweite Kamera rendert nur bei offener Pforte, vor deren Vorderseite und
innerhalb von 22 Metern. Ihr Budget beträgt höchstens zwölf Aktualisierungen
pro Sekunde bei 960 × 540 im 16:9-Fenster. Andere Fensterverhältnisse passen die
Höhe an. Bilineare Filterung vermeidet harte vergrößerte Texel. Dieser zweite
Renderpfad spart zusätzliche dynamische Schatten, Lumenreflexionen und
Volumennebel; sein Beleuchtungsumfang entspricht daher noch nicht vollständig
dem Hauptbild. Die Aufnahme kann während schneller Kamerabewegung nachlaufen.

Die Referenzwirkung kommt aus mehreren unterschiedlichen Mitteln: starke
Silhouette, echte Vorder-/Hinterkanten, sichtbare Fugen, passende Größen,
Reflexionsunterschiede, Tiefenstaffelung, örtliche Lichtquellen und Bewegung
mit einem benannten Auslöser. Mehr zufällige Partikel ersetzt diese Mittel
nicht. Bewohnte schwebende Inseln, Wasserfälle, Gewächse an der
Architektur und individuelle Begleiter bleiben sichtbare weitere Arbeitsäste.

## Prüfung und Grenzen

Der portable C++-Test prüft geschlossene Kanten, positive Volumen,
nicht entartete Dreiecke, absichtlich geöffnete Gegenmodelle, Eingabeabstand
und gleiche Öffnungsentwicklung bei 30 und 144 Simulationsschritten pro Sekunde.
Für drei Beobachterpositionen prüft er 96 korrespondierende Strahlen zwischen
den beiden gleich ausgerichteten Schnittstellen. Eine absichtlich um zehn
Zentimeter versetzte Gegenkamera wird in allen 96 Fällen unterschieden. Das
prüft den verwendeten Kameraoperator; es ist kein Vergleich aller gerenderten
Pixel oder sämtlicher möglichen Portalkonfigurationen.

Die nativen Modi verwenden einen isolierten Testslot. `Probe` bewegt die
Spielfigur mit ihrem CharacterMovement über die Treppe, prüft Bodenauflage und
beide Portalankünfte, öffnet und schließt das Archivbuch und prüft geflogene
Strecke, Höhengewinn, Spurerzeugung und Rückkehr zur Schwerkraft.
`Capture` erzeugt sechs tatsächliche Spielansichten einschließlich Flugspur und
erfasst Frameintervalle nach dem Aufwärmen. Ein Capture ist eine begrenzte
Messung, kein Langzeitbenchmark oder Nachweis einer vollständig fertigen Welt.
Die Fensterauflösung beträgt 2560 × 1440. Das gewählte ausgewogene Profil
verwendet 85 Prozent interne Renderauflösung mit TSR; es ist keine Messung bei
1440p ohne Upscaling. Der Shader erzeugt Oberflächenrelief über Normalen,
keine geometrisch ausgegrabenen Bodenschichten.

```powershell
./scripts/astral.ps1 -Mode Build
./scripts/astral.ps1 -Mode Probe
./scripts/astral.ps1 -Mode Capture
```

Quellcode und eigene Mineralbilder stehen unter der MIT-Lizenz des Projekts.
Die ursprünglichen Illustrierungsdateien sind kein Bestandteil der
Veröffentlichung. Engine- und andere vorhandene Assetlizenzen gelten weiterhin
jeweils für ihre eigenen Bestandteile. Das erzeugte Windows-Paket ist ein
lokaler Entwicklungsstand; dieser Quellbeitrag veröffentlicht keine
Engine-Binaries und ersetzt keine gesonderte Distributionsprüfung.

## Primärquellen zur Technik

- [Epic: Instanced Static Mesh Component](https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-static-mesh-component-in-unreal-engine)
  beschreibt das Bündeln wiederholter Meshes und die Unterschiede zwischen
  statischen und bewegten Instanzen. Unser Schritt bündelt generierte statische
  Geometrie nach Material; die bestehenden Naturobjekte verwenden HISMs.
- [Epic: Local Fog Volumes](https://dev.epicgames.com/documentation/unreal-engine/local-fog-volumes-in-unreal-engine)
  unterscheidet lokale Nebelbereiche von aufwendigerem beleuchtetem
  Volumennebel. Für künftige Portalschwaden ist ein örtlicher, begrenzter Effekt
  der passende nächste Versuchsaufbau. Dieser Schritt fügt keinen globalen
  Nebelpass hinzu.
