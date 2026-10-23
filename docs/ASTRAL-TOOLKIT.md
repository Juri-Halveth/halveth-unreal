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
Er ruft Windows PowerShell über deren eindeutigen Systempfad auf, damit ein
anders aufgelöster Befehl im Suchpfad den Start nicht verändert. Das Hilfsskript
bindet außerdem deren eigenes Utility-Modul, damit ein von einer anderen
Anwendung geerbter Modulsuchpfad den Hashvergleich nicht verhindert.
Beim ersten Start baut der Helfer bei Bedarf das Paket. Dafür werden dieselben
UE-5.8-/Windows-C++-Voraussetzungen und vorhandenen Assetquellen wie im
Hauptprojekt verwendet. Spätere Starts vergleichen den Quellfingerprint.
Das aktuelle lokale Entwicklungspaket liegt unter `.local/astral-sky-package/Windows`.
Der Vorgänger unter `.local/astral-package/Windows` bleibt getrennt erhalten.

- WASD/Maus: gehen und schauen; Leertaste: springen.
- Der Blick kann den Zenit und Nadir durchqueren. Die Kamerarotation bleibt als
  Quaternion erhalten; die Kapsel der gehenden Figur bleibt aufrecht.
  Nach einer vollständigen Drehung liegt dieselbe Blickrichtung wieder vor.
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
./scripts/astral.ps1 -Mode ViewProbe
./scripts/astral.ps1 -Mode NightCapture
```

## Durchgehender Blick und Nachthimmel

Die Video-Rückmeldung vom 23.10. zeigte ab etwa Sekunde 42 denselben regelmäßig
verteilten Sternbereich. Der alte Renderer verteilte 240 leuchtende Kugeln nach
einem Fibonacci-Muster in einer oberen Halbkugel. Der Standard-Kameramanager
begrenzte zusätzlich den vertikalen Blick auf knapp unter 90 Grad. Das Video
enthält keine gemessenen Mausdeltas; die Zuordnung des Anschlags wird deshalb
zusätzlich über den nativen Controller-Eingabepfad geprüft.

`ViewSkyMath.h` enthält die fortlaufende Quaternion-Rotation und einen
deterministischen Zufallsgenerator für 1.800 Sternrichtungen auf der ganzen
Kugel. Ein einziges kollisionsfreies Mesh bündelt die Lichtpunkte. Das
Sternmaterial blendet jeden Punkt radial bis zum Rand aus; der quadratische
Träger wird dort vollständig durchsichtig. Entfernung und Winkelgröße sind
getrennt: Die virtuelle Geometrie liegt zehn Kilometer entfernt und übernimmt
die Kameraverschiebung, während ihre Rotation von der Spielzeit abhängt.
Die Blickdrehung wird nicht auf die Sterne übertragen.

Das ist ein gestalteter Sternhimmel, kein realer Sternkatalog. Sein langsamer
Zeitverlauf und die bläuliche Nachtaufhellung sind gestalterische Modelle.
Die datierte Sonnenbahn bleibt separat gebunden. Die Nachtbelichtung hat eine
untere Grenze von EV100 7,5; die Umgebung erhält 100 Lux gerichtetes Hilfslicht.
Die Pfortenlampen sinken nachts auf acht Prozent ihrer Tagesintensität,
emittierende Pfortenmaterialien auf 18 Prozent. Diese Werte dienen der
Lesbarkeit und werden nicht als gemessene Mondbeleuchtung ausgegeben.
Die Sonne verwendet Atmosphärenkanal 0, das Nacht-Hilfslicht Kanal 1 ohne
sichtbare Scheibe. Der Engine-Standard hatte beide Richtungslichter in Kanal 0
mit weißer Scheibe eingetragen und damit einen festen Lichtfleck erzeugt.

`ViewProbe` führt echte Controller-Eingaben aus, misst die resultierende
Kamera und vergleicht die Kamerakomponente erst nach deren Aktualisierung.
Es verlangt mehrere Polüberquerungen, durchgehende vertikale und horizontale
Drehung, aufrechte Körperkollision und die Übernahme einer explizit neu
gesetzten Blickrichtung. Die alte Engine-Begrenzung dient als Gegenprobe.
`NightCapture` erzeugt sechs gerenderte Ansichten mit festem Zeitanker,
einschließlich einer Überquerung des Zenits über denselben Eingabepfad.
Die Modi verwenden den Test-Spielstand; neue Figurenjournale haben eigene IDs.
Eine erfolgreiche Testsequenz ersetzt nicht die Sichtprüfung mit der Maus
oder die menschliche Abnahme der Grafik.

Der eigene Nachweis ist `QA/native-free-look-sky.json`. Frühere
Astralis-Nachweise behalten ihren damaligen Quell- und Paketbezug.

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
