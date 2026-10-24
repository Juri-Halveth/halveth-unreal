# Entspanntere Bewohnerbewegung

Dieser Kandidat folgt auf den geprÃ¼ften Wiesenstand `a8bc948`. Er verÃ¤ndert
die gemeinsame Skelettbewegung der drei vorhandenen Figuren und das Verhalten
ihrer KollisionskÃ¶rper. Das vorherige Paket bleibt in seiner eigenen Ablage.

![Native Nahansicht mit entspannterer Armhaltung](../Preview/RESIDENT_POSTURE.png)

[Kurze native Gehsequenz, GIF, etwa 4,6 MB](../Preview/RESIDENT_MOTION.gif)

## Was sich verÃ¤ndert

- Die Handziele erhalten eine entspannte Ellenbogenbeugung von 25 Grad in
  Ruhe und bis zu 45 Grad beim Gestikulieren. Eine kurze gefilterte
  Nachbewegung lÃ¤sst HÃ¤nde und seitliche Gewichtsverlagerung nachlaufen.
- Die FuÃŸkurve behÃ¤lt beim Abheben und Aufsetzen ihre Position und
  Geschwindigkeit. Die lokale RÃ¼ckwÃ¤rtsbewegung in der Standphase bleibt
  erhalten; bloÃŸes Abbremsen auf lokale Geschwindigkeit null wÃ¼rde den FuÃŸ
  relativ zur laufenden Figur am Boden verschieben.
- Die Animation liest das tatsÃ¤chliche Bewegungstempo in jedem eigenen
  Update. Sie wartet dafÃ¼r nicht mehr auf den selteneren Bewohnerzyklus.
- Bewohner und Begleiter sind keine begehbaren Treppen. Neue Bewohner werden
  bei blockiertem Platz ausgelassen, statt durch die Kollisionskorrektur auf
  andere KÃ¶rper angehoben zu werden. Ihre Bodensuche verwendet statische
  Weltgeometrie.

## Was tatsÃ¤chlich geprÃ¼ft ist

Der portable C++-Test prÃ¼ft 10.000 Gelenkketten und die ÃœbergÃ¤nge der
Schrittphasen. Er unterscheidet die alte Kurve und ein scheinbar glattes
Gegenmodell, das in Weltkoordinaten am Boden rutscht. Diese analytische
KontaktprÃ¼fung setzt flachen Boden, konstantes Tempo und konstantes
Bewegungsgewicht voraus.

Der native Renderlauf wertet 1.800 Arm- und 1.800 FuÃŸpunkte der drei Begleiter
aus. Alle Idle-Animationen sind geladen. Die gemessene Ellenbogenbeugung
reicht von 25 bis knapp 45 Grad. Die ausgewerteten KnÃ¶chel treffen ihre
zuletzt fertig berechneten IK-Ziele innerhalb der ausgegebenen MessprÃ¤zision.
Das ist keine globale Messung von FuÃŸrutschen oder menschlicher NatÃ¼rlichkeit.

Ein eigener nativer Bewohnerlauf enthÃ¤lt 14 kÃ¶rperliche Bewohner, davon
13 mit gemessener Bewegung. Kein untersuchter Bewohner verwendet einen
anderen Pawn als Bewegungsboden. Die Engine verweigert das Hochsteigen;
die im isolierten Test kurz eingeschaltete Gegenprobe erlaubt es wieder und
wird anschlieÃŸend zurÃ¼ckgesetzt. Flug, geschlossene TÃ¼r, Sichtverdeckung,
Warnung, tatsÃ¤chliche Verfolgung, Festhalten und Freigabe bestehen weiter.

Die [Quellen-, Paket- und Aufnahmequittung](../QA/native-resident-motion.json)
bindet die geÃ¤nderten Dateien, ausfÃ¼hrbaren Dateien, gekochten Pakete,
300 native PNG-Anfragen und die ausgegebenen Medien. Der Aufnahmelauf benutzt
einen festen Simulationsschritt von 30 Hz; daraus entstehen tatsÃ¤chlich
15 Screenshot-Anfragen pro Simulationssekunde. PNG-Schreiben und die
verfassten Szenenwechsel machen ihn ungeeignet als Leistungsbenchmark.

## Haut und noch offene Gestaltungsarbeit

Ein getrennter Versuch aktivierte die bereits angelegten Subsurface-EingÃ¤nge
des Hautmaterials. Die Gesichter wurden dabei zu hell und verloren sichtbare
Konturen. Dieser Versuch wurde nach BildprÃ¼fung verworfen; der endgÃ¼ltige
Materialquelltext entspricht dem Ausgangsstand. Gleiche Aufnahmeeinstellungen
bedeuten dabei keine identische komplette Bewohneranordnung.

Die drei geteilten Modelle, Kleidung und Hautbilder sind weiterhin vorhanden.
Individuellere Gesichter, Hautdetails in Normalen und Rauigkeit, mehr Mimik,
vertikales Nachgeben des Beckens und vollstÃ¤ndig simulierte Kleidung sind
eigene offene Schritte. Die aktuelle Ã„nderung ist eine Skelett- und
Kontaktkorrektur, keine Knorpel- oder Weichteilsimulation.

## Start und RÃ¼ckweg

`Start-Astralis.cmd` startet den Kandidaten aus seinem eigenen Checkout.
FÃ¼r die neue ausfÃ¼hrbare Datei ist ein Neustart des Spielfensters nÃ¶tig.
Die vorherige Wiesenfassung und die vorangegangenen Desktop-VerknÃ¼pfungen
bleiben lokal erhalten. Bestehende MIT-Erlaubnisse sowie die enge
Sechs-Dateien-Zuordnung der Wiesenwerkzeuge bleiben unverÃ¤ndert.

Der spätere explizite Neubau besitzt dieselben gebundenen Quellen und gekochten
Spieldaten, aber eine neu verlinkte EXE. Die Aufnahmequittung erhält deshalb
die ursprüngliche Medienbindung und eine eigene Renderprüfung der installierten
EXE. Der Paket-Cache besteht außerdem einen nativen Treppen-/Buch-/Fluglauf
mit einem absichtlich nicht vorhandenen Engine-Pfad.

Ein zusätzlicher Windows-Lauf zeigte eine Kollision zweier gleichzeitiger
Asset-Veröffentlichungen auf dasselbe Ziel. Nur der kurze atomare
Veröffentlichungsschritt wird nun serialisiert; der Transfer bleibt parallel.
Die bestehende Gegenprobe besteht 64 weitere lokale Wiederholungen.
