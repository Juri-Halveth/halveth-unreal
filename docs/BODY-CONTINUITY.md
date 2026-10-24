# Körperkontinuität und elastischer Träger

Dieser Kandidat bearbeitet zwei konkrete Fehlerklassen: sichtbaren Austausch
naher Bewohner und die starre Kopplung einer Oberfläche an Animationsziele.
Er erweitert den nativen Spielpfad. Die vorhandenen Charakteroberflächen und
Skelette bleiben vorläufig die Gestaltungsquelle; eine neue menschliche Anatomie,
Zellen, Proteine, Organe und ein biologischer Organismus sind nicht implementiert.

## Bewohner behalten ihren Zustand

Die frühere Rangliste konnte einen vorhandenen Körper entfernen, sobald andere
Bewohner näher waren. Jetzt belegen vorhandene Körper zuerst ihre Plätze.
Sichtbare und kameraseitig ungeklärte Körper werden erhalten. Nach dem initialen
Szenenaufbau werden neue Körper nur außerhalb des konservativ erweiterten
Spielerkameravolumens zugelassen. Bei 18 Körpern warten neue Kandidaten.

Eintritt: 50 m. Austritt: über 65 m und mindestens zwei Sekunden außerhalb der
Ansicht, beziehungsweise über 100 m außerhalb der Ansicht. Eine laufende
Wachinteraktion schützt den beteiligten Körper. Wiederaufnahme frühestens nach
drei Sekunden. Das ist ein begrenztes Verfahren mit 4096 virtuellen Einträgen,
keine unbegrenzte vollständig physische Bevölkerung.

Position und Modellzeit werden gemeinsam übergeben. Höhe, Blickrichtung,
Animationszeit und Gangphase bleiben erhalten. Eine zurückkehrende Wache wird
nicht erneut an der ursprünglichen Pforte platziert. Weltwechsel bleiben
ausdrückliche Szenenübergänge; sie sind von gewöhnlicher Budgetverwaltung getrennt.

## Gewebe und sichtbare Oberfläche

Ein Körper besitzt 13 grobe Regionen mit je vier massiven Knoten. Ein eigener
XPBD-Kern integriert Nachgiebigkeit, sechs Kantenbedingungen und eine
vorzeichenbehaftete Volumenbedingung bei festen 120-Hz-Teilschritten. Parameter
sind authored Spielparameter, keine Messwerte menschlichen Gewebes.

Der Träger arbeitet im jeweiligen Knochenrahmen. Weltgravitation und gefilterte
Beschleunigung des Knochenzentrums werden in diesen Rahmen übertragen. Die
Anregung ist ausdrücklich auf 1500 cm/s² begrenzt. Die Animation bewegt die
Referenzstruktur; der Solver integriert die elastischen Restbewegungen.
Eigenständige Muskelaktoren, Gewebekontakt, Gelenkknorpel, innere Organe und
Materialermüdung sind damit nicht simuliert.

Die CPU übergibt ein kleines RGBA32F-Feld. Das Charakter-Material verformt die
vorhandene GPU-Oberfläche über dieses Feld. Der originale GPU-Pfad für Atmung,
Gesicht, Kleidung und Haar-Morphs bleibt verfügbar. Haut und Stoff erhalten
verschiedene Verformungsstärken. Das ist keine Überarbeitung von Gesichtsform,
Hauttexturen oder Subsurface-Shading; diese sichtbaren Baustellen bleiben offen.

Alle 13 Regionen beteiligen sich mit stetigen Gewichten. Eine Auswahl nur der
zwei nächsten Regionen wurde verworfen: Ein Wechsel des zweiten Nachbarn kann
sonst einen endlichen Sprung in der Oberfläche verursachen. Höchstens sechs
nahe Körper nutzen den zusätzlichen Träger. Beim Budgetwechsel klingt die
wirkliche Verformungsamplitude aus, bevor der Platz frei wird.

## Prüfungen und Rückweg

Die portablen Tests prüfen Solverreaktion, Volumen, Rückkehr, unterschiedliche
Zeitschritte, Beschleunigungsrahmen, unveränderte Ablehnung ungültiger Daten,
Bewohnerübergabe und ein dichtes Feld-Sampling. Gegenkontrollen enthalten einen
starren Kopierpfad, die alte Nachbarrangliste und den alten veralteten Zeitstempel.

Der native Feldtest liest denselben HLSL-Feldoperator aus einem Float-Renderziel
zurück und vergleicht ihn mit dem CPU-Erwartungswert. Das belegt Feldübertragung
und Shader-Auswertung innerhalb dieser Probe. Es ist keine Messung aller
fertigen, skinned Körperpixel. Der explizit getrennte Kontrolllauf trennt Solver
und GPU-Feld; er muss scheitern. Native Aufnahmen zeigen den sichtbaren Kontext.

Eine erste CPU-Oberflächenvariante wurde lokal verworfen: zu teuer und bei
abrupten Posewechseln instabil. Sie gehört nicht zum aktiven Renderpfad.
Der vorherige Quellstand ist Git-Commit `5556dd3`; das vorherige Windows-Paket
bleibt lokal gesichert. Originalmedien und ältere Pakete werden nicht gelöscht.

## Quellen und nächste sichtbare Arbeit

- [XPBD, Originalpublikation von Macklin, Müller und Chentanez](https://matthias-research.github.io/pages/publications/XPBD.pdf)
- [Epic: Chaos Flesh Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/chaos-flesh-overview)
- [Epic: Chaos Flesh Quickstart](https://dev.epicgames.com/documentation/en-us/unreal-engine/chaos-flesh-quickstart)

Diese Quellen beschreiben vorhandene Verfahren. Dieser Kandidat integriert
einen eigenen kleinen Träger; er ist keine Behauptung, XPBD oder Chaos Flesh
erfunden zu haben. Die nächsten gestalterischen Schritte sind individuelle
Anatomie, natürliche Haut-/Augenmaterialien, Kontaktverformung und eine
beobachtete bessere Ganzkörperbewegung. Ein bestandener Funktionstest ersetzt
deren sichtbare Abnahme nicht.
