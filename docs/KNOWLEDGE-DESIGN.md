# Wissen und Handwerk · Entwurf für HALVETH 0.2

Stand: **2026-09-23 · DESIGN_PROPOSAL**. Der spielbare Kern soll Entdecken,
Nachschlagen und Anwenden verbinden: Eine eigene Notiz eröffnet eine Möglichkeit;
eine tatsächlich ausgeführte Handlung verändert eine Fähigkeit oder die Welt.
Die unten genannten Zahlen und Namen sind konkrete Startvorschläge für das
Balancing, keine bereits beobachteten Runtime-Ergebnisse.

Umsetzungsnotiz für 0.2.0: Den tatsächlich gebauten Umfang beschreiben
[README](../README.md) und [native Prüfungen](NATIVE-VALIDATION-0.2.0.md).
Die aktuelle Laufzeit schaltet Rezepte über eine richtige Seitenantwort frei.
Der hier vorgeschlagene gleichwertige Weg allein durch direktes Üben und eine
vollständig optionale Wissensabfrage sind noch offene Ausbauzweige. Lesedauer
erzeugt weder eine Wartepflicht noch automatische Erfahrungspunkte.

Dieser Entwurf ersetzt für den **Wissens- und Handwerkszweig** die Reihenfolge des
[T0-Ausbauplans](EXPANSION-ROADMAP.md): Nach dem nativen 0.1-Build folgt eine kleine
durchgängige Spielschleife. T0 und sein Receipt bleiben als historische Planung
erhalten. Der erhaltene Unreal-Stand 0.1, HALVETH Realms 0.2 und Morrowind Genesis
0.3 bleiben getrennte Baselines. Das Design verändert weder deren Release-Dateien
noch Morrowind-Spielstände oder gewollte bestehende Spielmechaniken. Umsetzung,
Engine-Test, sichtbarer Spieltest und Veröffentlichung von 0.2 benötigen eigene
Belege; dieses Dokument liefert dafür die Anforderungen.

## Was die Primärquellen tatsächlich beschreiben

| Quelle / gebundene Fassung | Belegte Mechanik | Bedeutung für das Design |
| --- | --- | --- |
| Bethesda, [Morrowind GOTY PC-Handbuch](https://assets.ctfassets.net/rporu91m20dc/6lHLSvBn8WoeCou2i0IAqa/77f3c4493ad458ab5a692d593f1af8a7/manual_mwgoty_pc_en-us.pdf), gedruckte S. 26 | Fähigkeiten wachsen durch erfolgreiche Anwendung, Training und besondere Bücher. | Wissenserwerb und Handlung können unterschiedliche Zugänge zu derselben Fähigkeit bieten. |
| Bethesda, [Skyrim Legendary Edition PC-Handbuch](https://assets.ctfassets.net/rporu91m20dc/1poliwondakI2QwKQMY2QI/b79ecee4541dd1ba7c7a85b453f0e9b2/manual_skyrim-le_pc_en-us.pdf), gedruckte S. 5, 8 und 20 | Ein Fertigkeitsbuch verbessert eine Fähigkeit beim ersten Lesen; Zauberbücher lehren Zauber und werden dabei verbraucht. | Einmalige inhaltliche Entdeckung kann eine klare Freischaltung auslösen. |
| Keen Games, [Enshrouded FAQ](https://enshrouded.com/en-US/faq), Abruffassung | Persönliche Rezepte, gefundene Lore und Charakterfortschritt sind vom Weltstand getrennt; viele andere Rezepte hängen an freigeschalteten NPCs und Stationen der jeweiligen Welt. Bauen und Geländeformen sind Teil des Spiels. | Wissen gehört zur Figur; die Möglichkeit, es anzuwenden, kann zusätzlich einen konkreten Ort benötigen. |
| Keen Games, [Forging the Path](https://enshrouded.com/en-US/news/enshrouded-forging-the-path-is-live), 21.04.2026, Update 8 / 0.9.1.0 | Haupt- und Nebenaufträge werden unterschieden, Item-Sammlungen im Journal zusammengefasst und lange Rezeptlisten auf zusätzliche Stationen verteilt. | Ein lesbares Journal verbindet Entdeckung, Bau und Handwerk mit einem erkennbaren nächsten Schritt. |
| Blizzard, [Dragonflight: Making It with Professions](https://news.blizzard.com/en-us/article/23876529/dragonflight-making-it-with-professions), Dragonflight-Systembeschreibung | Handwerksaufträge, Spezialisierungswissen, Rezeptschwierigkeit und die Qualität von Ergebnis und Zutaten werden getrennt geführt. | Ein hilfreicher Auftrag und eine nachvollziehbare Spezialisierung geben dem Herstellen einen Zweck. |
| Blizzard, [Dragonflight Preview: More on Professions](https://news.blizzard.com/en-us/article/23826545/dragonflight-preview-more-on-professions), damalige Vorschau | Handwerksstationen bündeln Tätigkeiten; Recrafting überarbeitet vorhandene Ausrüstung mit einem Teil der ursprünglichen Materialien. | Weiterentwickeln eines vorhandenen Gegenstands kann wertvoller sein als Serienproduktion zum Wegwerfen. |
| Gameforge, [Metin2 Skills Guide](https://gameforge.com/en-GB/games/metin2-skills-window-skill-guide.html), Abruffassung ohne ausgewiesenes Veröffentlichungsdatum | Fertigkeitsbücher sind ein eigener Ausbauweg neben Fertigkeitspunkten; das beschriebene System begrenzt das Büchertraining durch Wartezeiten. | Ein Buch kann eine konkrete Fähigkeit adressieren. HALVETH verwendet dafür eindeutige Inhalte und Anwendungsschritte statt einer übernommenen Tageswartezeit. |

Die Handbücher beschreiben ihre jeweiligen Editionen, die Blizzard-Texte das
Dragonflight-System einschließlich einer damaligen Vorschau. Sie belegen keine
unveränderten Werte aller späteren Erweiterungen. Die Enshrouded-Patchnotiz ist
an ihre genannte Version gebunden. Alle Quellen stammen vom Entwickler oder
Publisher; kommerzielle Produktbeschreibungen sind keine unabhängigen Studien
über Lernen. Fan-Wikis, Presseartikel und Community-Anleitungen wurden nicht als
Belege verwendet. Es werden Mechanikideen neu gestaltet; Texte, Namen, Bilder,
Musik, Modelle und Rezeptdaten dieser Spiele werden nicht in HALVETH übernommen.

## Sechs eigene Mechaniken

### 1. Archivnotizen mit einmaligem spielerischem Nutzen

**Anregung:** Die einmalige Buchwirkung aus dem
[Skyrim-Handbuch](https://assets.ctfassets.net/rporu91m20dc/1poliwondakI2QwKQMY2QI/b79ecee4541dd1ba7c7a85b453f0e9b2/manual_skyrim-le_pc_en-us.pdf).

In jeder bestehenden Portalwelt liegt zunächst ein eigener kurzer Text. Beispiele
sind „Wärme im Gewebe“, „Licht im Kristall“ und „Ein guter Wegweiser“. Im Lesefenster
steht eine gleichwertige Kurzfassung. Mit **Notiz merken** übernimmt die Figur
den zugehörigen Eintrag dauerhaft ins Journal. Die Notiz eröffnet ein Rezept,
ein Gesprächsthema oder eine sichtbare Bauoption. Das Buch bleibt zum Nachschlagen
erhalten; seine Geschichte darf länger sein als die Kurzfassung.

Die Belohnung hängt an einer stabilen `knowledgeId`. Ein zweites Exemplar,
wiederholtes Öffnen, ein erneuter Portalbesuch oder eine redaktionell überarbeitete
Fassung erzeugen keine zweite Freischaltung. Ein Texteingriff erhält eine eigene
`documentVersion`, ohne automatisch eine neue Belohnungs-ID zu schaffen.

**Prüfbare Wirkung:** Nach dem ersten Merken erscheint genau ein neuer
Journaleintrag und eine benannte Option. Wiederholung und Save/Reload erhalten
denselben Zustand. Die UI meldet „Notiz verfügbar“, nicht „Text verstanden“.

### 2. Theorie eröffnet Möglichkeiten, Praxis verbessert ihre Anwendung

**Anregung:** Anwendung, Training und Bücher im
[Morrowind-Handbuch](https://assets.ctfassets.net/rporu91m20dc/6lHLSvBn8WoeCou2i0IAqa/77f3c4493ad458ab5a692d593f1af8a7/manual_mwgoty_pc_en-us.pdf)
und der gesonderte Bücherweg im
[Gameforge-Guide](https://gameforge.com/en-GB/games/metin2-skills-window-skill-guide.html).

HALVETH startet mit drei Praxisgebieten: **Fürsorge**, **Resonanz** und
**Handwerk**. Eine Notiz eröffnet eine zugehörige Übung. Fortschritt folgt einem
belegten Spielereignis: LOVE stellt tatsächlich fehlende Lebenspunkte wieder her,
Spark trifft ein aktives Übungsziel oder ein Gegenstand wird erfolgreich
hergestellt. Ein Tastendruck, ein Fehlwurf oder LOVE bei voller Gesundheit zählt
für diese Übung nicht als Fortschritt.

Ein Trainingsauftrag enthält endlich viele benannte Ziele. Bereits erfüllte
Ziele bleiben markiert; dieselbe Kristall-Regeneration setzt sie nicht zurück.
Freies Üben bleibt jederzeit möglich. Der nächste Rang wird über neue Aufgaben
oder eine andere Anwendung sichtbar erreichbar, statt durch endloses Wiederholen
derselben Aktion. Assistierte Eingabe darf dasselbe gültige Spielereignis erzeugen.

**Prüfbare Wirkung:** Einmal erfüllte Übungsziele verbessern einen angezeigten
Parameter, etwa die verfügbare Rezeptvariante. Ein wiederholtes identisches
Abschlussereignis vergibt keine zweite Rangbelohnung.

### 3. Rezepte verbinden Wissen, Material und einen passenden Arbeitsplatz

**Anregung:** Ortsabhängige Herstellung in der
[Enshrouded-FAQ](https://enshrouded.com/en-US/faq) und nachvollziehbare
Rezeptanforderungen in [Blizzards Handwerkssystem](https://news.blizzard.com/en-us/article/23876529/dragonflight-making-it-with-professions).

Eine eigene **Gartenwerkbank** zeigt zwei erste Rezepte: ein Hilfspäckchen und
eine Wegleuchte. Die Oberfläche zeigt Wissen, Zutaten, Arbeitsplatz und Ergebnis
vor dem Herstellen. Fehlende Voraussetzungen erhalten eine konkrete Beschreibung
wie „Notiz: Wärme im Gewebe“ oder „Noch eine Faser“. Ein bekanntes Rezept bleibt
im Journal sichtbar, auch wenn die Figur gerade ohne Werkbank reist.

Herstellen ist eine einzelne Zustandsänderung: Voraussetzungen erneut prüfen,
Zutaten abziehen, Ergebnis hinzufügen und denselben Vorgang speichern. Bei
abgebrochener oder ungültiger Herstellung werden keine Zutaten abgezogen. Die
erste Qualitätsstufe ist zuverlässig nutzbar; v0.2 benötigt keinen Zufallsverlust.
Inventar-Mengen bleiben ganze, begrenzte Werte.

**Prüfbare Wirkung:** Ein gültiger Craft verbraucht exakt die angezeigten Zutaten
und erzeugt exakt das angekündigte Ergebnis. Doppelklick und erneute Verarbeitung
derselben Transaktions-ID duplizieren weder Ergebnis noch Kosten.

### 4. Wissen verändert einen bewohnbaren Ort

**Anregung:** Bauen, Stationen und übersichtliche Sammlungsaufgaben in
[Enshrouded](https://enshrouded.com/en-US/news/enshrouded-forging-the-path-is-live).

Ein Guide bittet um eine Wegleuchte an einer dunklen Abzweigung. Nach Lesen oder
Kurzfassung, Merken und Herstellen setzt die Figur die Leuchte an einen sichtbaren
Baupunkt. Dort entsteht tatsächlich eine beleuchtete, kollidierende oder bewusst
kollisionsfreie Spielkomponente gemäß Objekttyp. Der Guide kommentiert die
Veränderung; das Journal zeigt den abgeschlossenen Auftrag. Für v0.2 genügt ein
Baupunkt mit einer eigenen einfachen Geometrie. Freies Voxelbauen bleibt ein
separater späterer Ausbau.

Bereits vor Annahme erfüllte Voraussetzungen werden erkannt. Die Reihenfolge
„erst entdecken, später beauftragen“ darf keinen Questzustand blockieren.
Rückbau erhält eine ausdrückliche, vorher sichtbare Erstattungsregel.

**Prüfbare Wirkung:** Der Ort ändert sich sichtbar und bleibt nach Laden verändert.
Eine zusätzliche Leuchte am selben Baupunkt und wiederholte Questbelohnung werden
durch dessen gespeicherten Zustand verhindert.

### 5. Aufträge machen Spezialisierung zu einer Hilfe für andere Figuren

**Anregung:** Handwerksaufträge und Spezialisierung bei
[Blizzard](https://news.blizzard.com/en-us/article/23876529/dragonflight-making-it-with-professions).

Ein lokales Auftragsbrett bietet kleine authored Aufgaben: ein Guide braucht ein
Hilfspäckchen, ein anderer eine Leuchte. Die Figur kann zuerst Fürsorge oder
Handwerk vertiefen; die jeweils andere Richtung bleibt später erreichbar.
Auftragstext, Stückzahl, bereitgestellte Materialien und Belohnung sind sichtbar.
Der Auftrag kann Zutaten bereitstellen, damit er ohne vorheriges Ressourcen-Grinden
spielbar bleibt. Er wird anhand seiner eindeutigen `orderId` einmal abgerechnet.

Die Figur erhält etwas Konkretes zurück: ein neues Dialogthema, eine dekorative
Bauvariante oder einen Übungsschritt. Für die erste Version genügt ein lokaler
NPC-Auftrag. Spielerhandel, Server und wirtschaftliche Märkte sind eigene spätere
Implementierungen.

**Prüfbare Wirkung:** Die Abgabe entfernt exakt die geforderten Gegenstände,
verändert Auftrag und NPC-Dialog und vergibt ihre Belohnung einmal. Abbruch vor
Abgabe erhält das Inventar. Eine frühere Notiz bleibt bei einer anderen
Spezialisierungswahl erhalten.

### 6. Bestehende Dinge durch neue Erkenntnisse überarbeiten

**Anregung:** Das Überarbeiten vorhandener Ausrüstung im
[Dragonflight-Handwerksentwurf](https://news.blizzard.com/en-us/article/23826545/dragonflight-preview-more-on-professions).

Nach zwei passenden Notizen und einer praktischen Aufgabe erhält eine bestehende
Wegleuchte eine neue Variante, etwa einen größeren Lichtkegel oder eine andere
Farbe. Eine Vorschau zeigt Wirkung und Kosten. Die Änderung bleibt an derselben
`itemInstanceId` oder `buildSiteId`; neue Kenntnis führt zu einer überprüfbaren
Eigenschaftsänderung am bereits genutzten Objekt. Bestehende nützliche Eigenschaften
gehen in diesem ersten Überarbeitungsschritt nicht verloren.

Diese Mechanik folgt auf den getesteten Grund-Craft. Sie kann als nächster
0.2-Ausbau zurückgestellt werden, wenn Persistenz und atomare Herstellung noch
fehlen. Wiedereinstieg: Grund-Craft und Save/Reload sind im nativen Build bestanden.

**Prüfbare Wirkung:** Vorschau und angewandte Werte stimmen überein. Fehlende
Zutaten lassen das Original unverändert; ein wiederholter Commit dupliziert das
Objekt nicht.

## Lesen, Pause und Zugänglichkeit

Lesezeit ist höchstens ein lokales **Engagementsignal**. Eine sichtbare Seite,
ein Klick, Scrollen oder verstrichene Sekunden beweisen weder Aufmerksamkeit noch
Verständnis. Der Spielcode darf daraus keinen Wissensnachweis über die Person
ableiten. Auch eine bestandene Spielaufgabe belegt zunächst ihren definierten
Spielzustand, keine allgemeine Kompetenz außerhalb des Spiels.

Für 0.2 gilt:

- Freischaltungen verwenden einmalige Inhalts-IDs und explizite Spielaktionen.
  Zeitablauf, geöffnetes Buch, pausiertes Spiel und Rückkehr nach Ablenkung erzeugen
  **keine XP, Materialien oder Gegenstände**. Ein Mindestlesetimer ist unnötig.
- Langsames Lesen, kurze Sitzungen oder eine Pause kosten keinen Fortschritt.
  Ein optionaler Zähler heißt etwa „aktive Anzeigezeit“, bleibt ausblendbar und
  hat keinen Einfluss auf Belohnungen. Pausen und Hintergrundbetrieb stoppen ihn;
  fehlende Mausbewegung wird nicht als Unaufmerksamkeit gewertet.
- Kurzfassung, Volltext und eine später ergänzte Vorlesefunktion führen zu
  denselben Freischaltungen. Die Vorlesefunktion ist eine geplante Erweiterung,
  kein Nachweis bereits vorhandener Audioausgabe.
- Das Lesefenster unterstützt Tastaturbedienung, sichtbaren Fokus, vergrößerbare
  Schrift, ausreichenden Kontrast, erneutes Nachschlagen und Schließen ohne Verlust.
  Bewegte Hintergründe und Glüheffekte müssen reduzierbar sein.
- Eine optionale Inhaltsfrage gibt Hilfen und Wiederholungen; sie ist kein
  Pflicht-Gate mit Zeitstrafe. Ihre Antwort darf nur den konkreten Spielauftrag
  auswerten. Die gleichwertige direkte Übung bleibt erreichbar.

Die Regeln sind HALVETH-Designentscheidungen. Die herangezogenen Spielquellen
werden nicht als Beleg für psychologische Wirksamkeit oder als Accessibility-Studie
ausgegeben.

## Kleiner implementierbarer 0.2-Schnitt

Der erste vollständige Ablauf umfasst **drei eigene Notizen, drei Praxisgebiete,
zwei Rezepte, eine Werkbank und einen Hilfsauftrag mit einem Baupunkt**. Dieselben
drei vorhandenen Portalwelten liefern Orte und Figuren. Ein neues großes Biom,
ein Sprachmodell und zusätzliche Grafikdownloads sind dafür keine Voraussetzung.

Ein vorgeschlagener versionierter Save führt `knowledgeIds`, `exerciseResults`,
`recipeIds`, `inventory`, `orders`, `buildSites` und bereits verarbeitete
Transaktions-IDs. Inhaltstitel und freie Notiztexte sind keine Belohnungsschlüssel.
Wissen und Figurendaten sind vom Zustand einzelner Baupunkte getrennt. Der
Welt-Seed bleibt ein Generierungsparameter und erhält keine Wallet-Bedeutung.
Migration, neue Dateiablage und Umgang mit beschädigten Saves werden ausdrücklich
implementiert; sie entstehen nicht durch das Vorhandensein dieser Feldliste.

Die erste native Abnahme spielt den Ablauf durch: Notiz finden und merken,
Übung ausführen, Material erhalten, Hilfspäckchen oder Leuchte herstellen,
Auftrag erfüllen, speichern, beenden und laden. Gesonderte Gegenproben prüfen
wiederholtes Lesen, reine Wartezeit, Pause, LOVE bei voller Gesundheit, doppelte
Craft-Anfrage, unzureichende Materialien, Auftrag nach vorheriger Entdeckung und
erneute Abgabe nach Laden. Eine ungültige Craft-Anfrage verändert weder Inventar
noch Welt. Der Abschlusszustand ist im HUD, am Baupunkt und nach Save/Reload sichtbar.

Die tatsächlichen Resultate gehören in den 0.2-Testbeleg mit Build- und
Quellstand. Ein NullRHI-Test kann Zustandslogik prüfen; Textlesbarkeit,
Tastaturfokus, sichtbare Leuchte und Spielgefühl benötigen den gerenderten nativen
Build. Dieser Entwurf ersetzt diese Beobachtungen nicht.

## Rechercheumfang

Exa wurde am 23.09.2026 für vier Spielefamilien verwendet: sechs gezielte
Suchläufe mit insgesamt 30 angeforderten Ergebnisplätzen und sieben direkt
abgerufenen Primärdokumenten. Die relevanten Abschnitte wurden gezielt gelesen;
dies ist kein vollständiger Vergleich aller Fassungen oder Systeme der vier
Spielreihen. Treffer von Fan-Wikis und Presse wurden nach Quellenart verworfen.
Neue Entwicklerdokumentation, eine geänderte Zielversion oder native
Spieltestergebnisse sind konkrete Anlässe, die jeweiligen Designentscheidungen
erneut zu prüfen.
