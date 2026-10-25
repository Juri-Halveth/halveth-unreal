# Örtliche Hautschichten und sanfte Blickreaktion

Dieser Kandidat baut auf `209b01c` auf. Die acht erwachsenen FBX-Modelle,
Quelltexturen, Morphs, Gewichte und UVs bleiben unverändert. Der vorherige
Figurenstand und seine lokalen Aufnahmen bleiben als eigener Prüfstand erhalten.

## Örtlich gebundene Oberfläche

`generate_skin_layers.py` erzeugt pro Modell einen 2048×2048-RGBA-Atlas.
Die vier linearen Datenkanäle beschreiben Lippen, Wangen, obere Lider und
ölige Gesichtszonen. Die Gewichte entstehen an der tatsächlichen exportierten
Restgeometrie und ihren benannten Mund-/Augenknochen. Baryzentrische
Rasterisierung trägt sie auf die vorhandenen Haut-UVs; eine kleine Randfüllung
schützt die Texturfilterung. Es werden keine Quellhautfotos übermalt.

Der Import verwendet `TC_Masks`, `SRGB=false`, erhaltenes Alpha und UV0.
Der Materialgraph setzt RGB und Alpha ausdrücklich wieder zu RGBA zusammen.
Ein zusätzlicher Texturzugriff steuert zurückhaltende Pigmentmultiplikatoren,
örtliche Rauheit und begrenzte Lippenfeinstruktur. `MakeupStrength=0` erhält
die vorherige Grundfarbe und Rauheit; der Standardwert ist `0.85`.
`LipPigment`, `CheekPigment`, `LidPigment` und `MicroDetailStrength` sind eigene
Materialparameter. Die bisherige `.18`-Porennormalstärke wird nicht pauschal erhöht.

Da das Material auf denselben Hautdreiecken und UVs liegt, folgt seine Färbung
Skelett, Morphs und dem bestehenden gemeinsamen TissueField. Die Masken sind
gestaltete Kosmetikzonen, keine gemessenen biologischen Schichten.
Der Bootstrap prüft Masken- und FBX-Digests gemeinsam. Der normale Starter
bindet zusätzlich jede PNG-Datei direkt; ein Manifestname allein genügt nicht.

## Blick und Ausdruck

Eine neutrale Augenmitte im Körperrahmen bestimmt die absolute Zielrichtung.
Die bereits gedrehte Augenpose wird nicht als neues Ziel rückgekoppelt.
Sichtweite, Blicksektor und ein begrenzter Sichtliniencheck entscheiden,
ob ein Ziel tatsächlich wahrgenommen wird. Eine Wand kann die Sicht unterbrechen.

Augen nähern sich dem absoluten Ziel schneller; der Kopf folgt nach kurzer
Verzögerung mit eigenem Kanal. Die Augenknochen erhalten nur die Differenz
zwischen Augen- und Kopfzustand. Winkel bleiben begrenzt. Fehlende Sicht
lässt beide Kanäle zur neutralen Haltung zurückkehren. Kopf und Augen sind
damit nicht länger an ein festes 92/8-Verhältnis gekoppelt.

Kontaktmimik wird separat zeitlich geglättet; die eigentliche Sichtentscheidung
bleibt unverändert boolesch. Blinzeln und Sprechen haben nur einen normalen
Morph-Schreiber in der AnimInstance. Atem-, Gewichts- und Fußsolver bleiben
erhalten. Das sind lokale Spielreaktionen, keine biologische Sensorik.

## Gebundene Prüfwege

Der isolierte Materialvergleich verwendet
`-HalvethMotionCapture -HalvethAppearanceCapture -HalvethSkinLayersCapture`.
`Preview/SkinLayers` enthält je Modell ein Paar mit Stärke 0/0.85, gleichem
Identitätsseed, gleicher tatsächlicher Pose, Kamera und EV12-Beleuchtung.
Das Live-Hautfeld ist in dieser Materialfixture ausdrücklich deaktiviert.
`QA/skin-layer-frames.tsv` und native Logzeilen binden die Aufnahmen.
Andere Figuren einschließlich der eigenen Körperpräsenz sind ausschließlich
in diesem Studioprozess ausgeblendet; die normale Welt bleibt vollständig sichtbar.

`-HalvethMotionCapture -HalvethGazeCapture` bewegt dagegen die tatsächliche
Spielerkamera um einen lebenden, stationären Guide, spricht ihn über die
normale Interaktion an und setzt kurz eine sichtbare blockierende Wand ein.
`Preview/Gaze`, `QA/gaze-frames.tsv` und `QA/native-gaze.json` halten den Lauf
fest. Das Live-Hautfeld bleibt dabei aktiv. Kamerawechsel sind Teil dieser
deklarierten Fixture; sie ist keine gewöhnliche ungeschnittene Spielersitzung.
Controllerzustände und zuletzt ausgewertete Knochenquaternionen sind getrennte
Beobachtungen; die TSV ist kein lückenloser synchroner GPU-Trace.

Die Aufnahme fordert 15 Bilder pro Simulationssekunde bei festem 30-Hz-Schritt
an. Erst die gespeicherten Zeitstempel bestimmen die erreichte Samplingrate.
Screenshot-Kodierung und Simulation sind kein normaler Spiel-FPS-Benchmark.
Eine Sichtabnahme bleibt von Build, Tests und numerischen Witnesses getrennt.

`scripts/capture_skin_gaze.ps1 -Mode Layers` beziehungsweise `-Mode Gaze`
startet diese Prozesse im Hintergrund, prüft Binär-/Payload-Bindung und erhält
ältere Bilder samt Timingdaten vor einer Wiederholung. Der abschließende
`verify_skin_gaze_candidate.py` verlangt gleiche tatsächliche A/B-Transforms,
korrekte Seeds, vollständige zeitgebundene Bilder, normale Startprobe und
den nativen Gaze-/GPU-Feld-Witness. Er erteilt keine grafische Qualitätsabnahme.

Der Perspektivkorridor prüft zusätzlich die IDs von 24 gebundenen
Restlandmarken in vier deklarierten Projektionen samt fehlerhafter Gegenprobe.
Dieser kleine Test betrifft das deklarierte Zahlenmodell, nicht jeden
gerenderten Hautpixel. Die eigenständige UV-Probe und native A/B-Bilder sind
andere Prüfungen.

Die Modelle und ihre Masken tragen weiterhin ihre gebundenen CC0-Hinweise;
Generatoren und Runtime-Code sind MIT. Die Wiesen-Lizenzzuordnung ändert sich
in diesem Inkrement nicht.

Technische Referenzen: Epic beschreibt
[Masken und Kanalpackung](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-texture-masks-in-unreal-engine)
und das [Subsurface-Profil](https://dev.epicgames.com/documentation/en-us/unreal-engine/subsurface-profile-shading-model-in-unreal-engine).
Diese Verfahren sind Ausgangspunkte; sie bescheinigen keine konkrete Bildqualität.
