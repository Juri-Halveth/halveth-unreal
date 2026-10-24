# Lebendige Welt: freier Flug, Bewohner und Verwarnungen

Juri Janovski · HALVETH / LUCINET · lokaler Entwicklungsschritt vom 24.10.2026.

Der Spielentwurf erlaubt Flug von Anfang an, auch im Archiv. Ein Raum, eine
Treppe oder eine frühere Ringaufgabe verleiht dafür keine gesonderte Lizenz.
Massive Wände, Türen, Boden und Körper bleiben physische Hindernisse. **J**
schaltet die Levitation um; WASD und Blick lenken, Leertaste steigt, Strg sinkt,
Shift beschleunigt. Das vorhandene Flugmodell hat endliche Geschwindigkeit,
Kapselkollision und eine begrenzte Welt. Ein sichtbarer Besen und anatomische
Spielerhände bleiben eigene Arbeiten.

## Ein erster zusammenhängender Spielablauf

Bewohner gehen zu wechselnden örtlichen Zielen, ruhen abhängig von ihrer
Tagesroutine oder halten für ein Gespräch an. Ihre Kennung bestimmt
Modellwahl, geringe Größenvariation, Tempo, Reaktionszeit, Animationsphase,
Antworttyp und Schicht. Die vier Antworttypen sind freundlich, direkt,
zurückhaltend und neugierig. **E** spricht eine nahe Figur an, wenn zwischen
den beiden kein Hindernis liegt. Der Blick und die Sprechgeste reagieren auf
die aktuelle Situation. Die Antworten sind verfasste Spieltexte.
Die schmale Eingangstreppe bleibt beim Einsetzen neuer Körper frei; nahe
Fußgänger erhalten Platz durch eine seitliche Ausweichbewegung. Vor dem
Neuaufbau eines Gebiets werden die alten Bewohnerkörper entfernt.

Langsamer Flug ist erlaubt. Bei mehr als 8,5 Metern pro Sekunde innerhalb von
4,2 Metern eines Bewohners prüft eine Wache ihre eigene Sicht. Dazu gehören
Entfernung, Blickrichtung und ein Raycast gegen die wirkliche Kollision.
Eine Wand unterbricht diese Sicht. Die Wache kann zunächst verwarnen; sechs
Sekunden später beginnt bei fortgesetztem gefährlichem Flug die Verfolgung.
Eine Sekunde sicheren Verhaltens während der Verwarnung beendet sie. Ein
einzelner kurzer Framewechsel setzt sie nicht zurück. Acht Sekunden ohne Sicht
beenden die Verfolgung.

Ein Fangimpuls benötigt Sicht und höchstens fünf Meter Abstand. Die Wache
muss sich mit CharacterMovement durch die Szene bewegen. Die anschließende
Lähmung dauert maximal 2,2 Sekunden. In der Luft hält sie den Flug an, danach
kehrt die Steuerung automatisch zurück. Ein fallender Charakter wird während
dieser kurzen Zeit gehalten und fällt anschließend weiter. Der Spieler darf
sich weiterhin umsehen. Der Ablauf verursacht selbst keinen Schaden,
Geldabzug oder Inventarverlust und führt kein Strafregister. Eine gemeinsame
Pause von 25 Sekunden verhindert mehrere unmittelbar gestapelte Fänge.
Der sichtbare kurze Lichtstrahl stellt den erfolgreichen Fang dar; er ist
kein separat ballistisch simuliertes Projektil.

## Die Welt muss nicht jeden Menschen gleichzeitig zeichnen

| Ebene | Tatsächlicher Umfang | Aktualisierung |
|---|---|---|
| Bewohnerregister | 4.096 einzelne Zustände, auf vier Gebiete verteilt | Eine Kohorte von 512 pro Sekunde; bei vollständigem Umlauf ungefähr acht Sekunden |
| Körper in der Nähe | Höchstens 18 zusätzliche Skeletal Characters, einschließlich bis zu zwei Wachen | Bewegung und Kollision pro Frame, Ziel-/Sichtentscheidung fünfmal pro Sekunde |
| Ursprüngliche Begleiter | Scarlet, Lucinet und Rachel | Ihr vorhandener Bewegungs- und Dialogpfad bleibt separat |
| Animation | Budgeted Skeletal Meshes mit Entfernungssignifikanz | Engine-Budgetziel 3 ms, reduzierte Aktualisierung nach Signifikanz |

Das Animationsbudget ist ein Zielwert der Engine, keine Garantie für die
gesamte Framezeit. Der Registertest mit 25.000 Zuständen prüft Datenlogik,
Identitäten, Routinen und Zeitregeln. Er rendert keine 25.000 Menschen.
Das Trennen von Hintergrundzustand und nahen Körpern ist der erste
Skalierungsschritt; die Zahl der im Sichtfeld körperlich dargestellten
Figuren muss nach gemessener Leistung erhöht werden.

Die Architektur nutzt den offiziellen
[Animation Budget Allocator](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-budget-allocator-in-unreal-engine).
Die konkreten API-Aufrufe sind zusätzlich gegen die installierten
UE-5.8-Header gebunden. Das Plugin verteilt Animationsarbeit, entscheidet
aber weder selbst über Tagesroutinen noch über menschliches Verhalten.

## Nachweis und Rückkehr

`LivingProbe` prüft im nativen Spiel freie Levitation vor der Ringausrichtung,
Bewegung im Archiv, die blockierende geschlossene Tür, einen absichtlich
eingesetzten Sichtblocker, Warnfrist, wirklichen Laufweg der Wache, Halt in
der Luft, unveränderte Lebenspunkte und erneute Bewegung nach Freigabe.
Die wiederholbare Vorbeiflug-Situation setzt Spielerposition und Tempo als
Testfixture; Wahrnehmung, Zustandsautomat und CharacterMovement verwenden
denselben Laufzeitpfad wie das normale Spiel.

`LivingCapture` rendert diese Folge zusätzlich. Die gemessenen Frameintervalle
umfassen auch Screenshots und Szenenwechsel; sie sind kein Langzeitbenchmark.
Der datierte [Prüfstand](../QA/native-living-world.json) bindet Quellbytes,
Paket, Ergebnisse, Bilder und verbleibende Grenzen. Testläufe verwenden einen
separaten Wissens-/Spielstandslot. Die vorhandenen Begleiterjournale schreiben
je Lauf in eine neue Datei und überschreiben keinen vorherigen Stand.

```powershell
./scripts/astral.ps1 -Mode Build
./scripts/astral.ps1 -Mode LivingProbe
./scripts/astral.ps1 -Mode LivingCapture
```

Der normale Start bleibt `Start-Astralis.cmd`. Er prüft Quell- und Binärhash
und startet das neue, getrennte lokale Paket. Eine Rückkehr zum zuvor
geprüften Himmelsstand ist durch den direkten Start des erhaltenen Pakets
unter `.local/astral-sky-package/Windows/HALVETHRealms.exe` möglich. Auch die
angenommene Quellbasis ist im Git-Verlauf erhalten.

## Was dieser Schritt noch nicht leistet

Die Bewohner benutzen drei vorhandene Grundmodelle, eine prozedural ergänzte
Bewegung und vier Antwortfamilien. Ihre Tagesaufgaben sind örtliche Routinen;
vollständige Berufe, Haushalte, Wirtschaft, Freundschaftsgedächtnis und
unbeschränkte Sprachdialoge sind damit noch nicht implementiert. Der neue
Bewohnerzustand besteht je Sitzung und wird noch nicht als eigene Bevölkerung
gespeichert. Hintergrundpositionen haben keine vollständige Kollisionsphysik.
Die lokale Ausweichsteuerung ersetzt noch kein gebietsweites Wegenetz.
Beim Wechsel vom Register zum Körper können sichtbare Übergänge auftreten.

Die Landschaft bleibt viermal 600 × 600 Meter groß. Natürlicher Boden,
reichere Äste und Blätter, bessere Fernlandschaft, Wasser und charaktereigene
Kleidung bleiben sichtbare Arbeitsäste. Dieser Schritt allein liefert keine
neue fotorealistische Welt. Eine fünffache Morrowind-Bevölkerung benötigt
zuerst einen gebundenen Vergleichsumfang; der derzeitige Registerumfang ist
kein Nachweis dafür.
