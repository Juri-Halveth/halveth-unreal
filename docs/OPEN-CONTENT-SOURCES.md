# Offene Quellen · konkrete Ansatzpunkte für HALVETH 0.2

Stand **23.09.2026**. Dieses Register enthält acht ausgewählte Katalog- oder
Werksquellen und daraus entwickelte Spielideen. **Importierte Inhalte: 0.**
In dieser Quellenrunde wurden Anbieterinformationen, Metadaten, Lizenzbedingungen
und einzelne Mechanikbeschreibungen gelesen. Keine fremden Bücher, Regelwerke, Datensätze,
Bilder oder Sounds wurden als Dateien heruntergeladen, ins Spiel eingebaut oder
zum Training eingelesen. Die bestehenden HALVETH-Buchtexte bleiben eigene Texte.

Die Vorschläge ergänzen [KNOWLEDGE-DESIGN.md](KNOWLEDGE-DESIGN.md). Sie sind
Designkandidaten, keine bereits implementierten Fähigkeiten und kein Anspruch,
das gesamte Wissen eines Archivs erfasst zu haben. Das maschinenlesbare Gegenstück
ist [source-register.json](source-register.json); alle acht Einträge tragen
`imported: false`.

## Acht gebundene Quellen

| ID / konkrete Quelle | Lizenz und exakter Umfang | Eigene HALVETH-Schraube |
| --- | --- | --- |
| **SRC_SRD_521** · Wizards of the Coast, [D&D SRD 5.2.1](https://www.dndbeyond.com/srd) | **CC BY 4.0** für dieses SRD. Die offizielle Seite nennt 5.2.1 als aktuellen Download; englisch veröffentlicht am 01.05.2025, deutsch am 08.12.2025. Seite zuletzt aktualisiert am 02.03.2026. Es ist keine Freigabe aller D&D-Bücher, Welten, Logos oder Figuren. | `skills.practiceRank` beeinflusst eine eigene, begrenzte `spells.manaCost`. Vorschlag: `max(1, baseCost - floor(practiceRank / 3))`. Diese Formel ist unser Balancingvorschlag, keine abgeschriebene SRD-Regel. |
| **SRC_FATE_CORE** · Evil Hat, [offizielle Fate-Lizenzroute und SRDs](https://fate-srd.com/official-licensing-fate) | **CC BY 3.0 Unported** für die über diese Route bezeichneten offiziellen SRD-Dateien. Die Lizenzseite fordert ausdrücklich diese Dateien als Arbeitsgrundlage, nicht die für das Web angepasste Abschrift. Marken und Logos haben eigene Bedingungen. | `npc.dispositionTags` und `dialogue.requiredTags`: Ein eigener Charakterzug wie „bewahrt Versprechen“ eröffnet eine passende Antwort oder Hilfsaufgabe. Die [Aspektbeschreibung](https://fate-srd.com/fate-core/aspects-fate-points) dient als Mechanikreferenz; es wird kein Fate-Punktesystem importiert. |
| **SRC_BASIC_FANTASY_4E** · Chris Gonnerman, [Basic Fantasy 4e Online-SRD, Release 128](https://basicfantasy.org/srd/) | **CC BY-SA 4.0** für die dort bezeichneten Texte, Karten, Diagramme, Tabellen und Formulare. Viele übrige Illustrationen sind ausdrücklich ausgenommen. R128 bezeichnet den gelesenen Online-Stand, nicht die Behauptung des neuesten gesamten Projektreleases. | `lantern.lightRadiusCm`, `lantern.coneAngleDeg` und `lantern.fuelUnits`: Eine Werkbank lässt Reichweite gegen Verbrauch abwägen. Die [Ausrüstungsreferenz](https://www.basicfantasy.org/srd/equipment.html) regt die Variablen an; HALVETH erhält eigene Werte, Texte und Modelle. |
| **SRC_GRIMM_1812** · Jacob und Wilhelm Grimm, [Kinder- und Haus-Märchen, Band 1, Berlin 1812, DTA-Werknachweis](https://www.deutschestextarchiv.de/book/show/grimm_maerchen01_1812) | Der **deutsche Original-Reintext** wird von den [DTA-Bedingungen, Abschnitt 2](https://www.deutschestextarchiv.de/doku/nutzungsbedingungen) als gemeinfrei getrennt behandelt. Die elektronisch annotierten Fassungen sind als **CC BY-SA 4.0** ausgewiesen; Bilddigitalisate haben gesonderte Bedingungen. Keine moderne Übersetzung und keine Illustration ist dadurch pauschal freigegeben. | `mythMotif.promiseState` mit `UNMADE`, `OFFERED`, `KEPT`, `DECLINED`: Eigene Märchenaufträge lassen ein freiwillig gegebenes Versprechen den Dialog ändern. Die konkrete Handlung und jeder Satz werden für HALVETH neu geschrieben. |
| **SRC_MET_OPEN_ACCESS** · [The Met Collection API](https://metmuseum.github.io/) | **CC0** für die vom Museum freigegebenen Metadaten und entsprechend ausgewiesenen Open-Access-Abbildungen. Metadaten enthalten auch Angaben zu noch geschützten Werken; ein vorhandener Katalogeintrag allein gibt dessen Bild nicht frei. Vor einem späteren Bildimport müssen Objekt-ID, OA-Status und Bildquelle feststehen. | `craft.materialClass` und `prop.surfaceFinish`: Klassifikation und Materialangaben können eine eigene Keramik-/Metall-Werkbank ordnen. Eine spätere Bildreferenz wird einzeln an `sourceObjectId` gebunden. |
| **SRC_COMMONS_BOTANY** · [Wikimedia Commons: Botanical illustrations](https://commons.wikimedia.org/wiki/Category:Botanical_illustrations) | **Lizenz je Datei**, keine gemeinsame CC0-Freigabe. Die [Commons-Nutzungsanleitung](https://commons.wikimedia.org/wiki/Commons:Reusing_content_outside_Wikimedia/licenses) verlangt die jeweils auf der Dateiseite ausgewiesenen Bedingungen. Noch keine einzelne Abbildung ausgewählt. | `codex.plantDiagramSourceId` und `plants.visualTraitTags`: Später könnte eine konkret geprüfte Zeichnung den Pflanzenkodex unterstützen. Bereits jetzt können eigene, neu gezeichnete Blattformen als klar erkennbare Crafting-Zutaten dienen. |
| **SRC_FREESOUND_PAGE_151220** · OwlStorm, [Page Turn (1), Sound 151220](https://freesound.org/people/OwlStorm/sounds/151220/) | **CC0**, laut Lizenzangabe des Uploaders im konkreten Eintrag. Metadaten nennen 0,588 Sekunden, WAV, Stereo, 44,1 kHz. Audio wurde weder geladen noch angehört; Klangqualität und Rechtekette sind damit nicht unabhängig geprüft. Andere Freesound-Dateien können andere Bedingungen tragen. | `bookAudio.pageTurnCue`, `bookAudio.gainDb` und `accessibility.pageSoundEnabled`: Ein optionaler, leiser Seitenklang folgt genau einem tatsächlichen Seitenwechsel. Er erzeugt weder XP noch einen Lesenachweis. |
| **SRC_GBIF_BACKBONE** · GBIF Secretariat, [Backbone Taxonomy, DOI 10.15468/39omei](https://www.gbif.org/en/dataset/d7dddbf4-2cf0-4f39-9b2a-bb099caae36c) | **CC BY 4.0** laut Datensatzkarte. Gebundene Veröffentlichung: 28.08.2023, Metadatenstand 17.11.2023. Es handelt sich um eine synthetische taxonomische Klassifikation, nicht um gemessene lokale Häufigkeiten oder ein vollständiges Ökologiemodell. | `ecology.taxonKey`, `ecology.familyKey` und `ecology.selectionGroup`: Verwandtschaftsgruppen können die Auswahl einer abwechslungsreichen fiktiven Flora strukturieren. Feuchtigkeit, Wachstum und Vorkommen bleiben eigene Spielparameter, bis passende ökologische Daten gesondert belegt sind. |

## Lizenzwege bleiben voneinander getrennt

**D&D:** SRD 5.1 bietet eine historische OGL- und eine CC-Route; neue SRD-Versionen
werden laut [offizieller SRD-Seite](https://www.dndbeyond.com/srd) unter CC BY 4.0
veröffentlicht. Das Register wählt **5.2.1 / CC BY 4.0**. Es aktiviert nicht
nebenbei OGL 1.0a und verwendet keine DMsGuild-Lizenz. Bei tatsächlicher Adaption
sind die Attribution aus genau dieser SRD-Fassung und die vorgenommenen Änderungen
zu dokumentieren. Eine bloße Inhaltsähnlichkeit zu einem anderen D&D-Buch ersetzt
die Zugehörigkeit zum lizenzierten SRD nicht.

**Fate:** Die [CC-Anleitung](https://fate-srd.com/official-licensing-fate/cc) nennt
CC BY 3.0 und eigene Attributionsblöcke je SRD. Diese sind bei einer tatsächlichen
Übernahme zu verwenden. Der [Evil-Hat-Versionsführer](https://evilhat.com/wp-content/uploads/2022/03/Fate-Version-Guide-Spheres-Differences-Licensing-Selections-2021.pdf)
bestätigt die offenen SRD-Routen; er gibt nicht sämtliche Fate-Settings oder
lizenzierten Franchise-Produkte frei.

**ShareAlike-Inhalte:** Basic-Fantasy-Adaptionen und DTA-Annotationen lassen sich
nicht einfach als eigenes MIT-Material deklarieren. Umfang einer Adaption,
Attribution, ShareAlike und Vereinbarkeit mit dem konkreten Auslieferungsformat
müssen vor einer Übernahme geklärt sein. Dieses Register übernimmt nur
Quellverweise und formuliert eigene Designvorschläge; es importiert keine dieser
Textfassungen. Die Rechte an fremden Materialien werden durch das Projekt-LICENSE
nicht umgeschrieben.

**Einzelmedien:** CC0, CC BY und CC BY-SA sind verschiedene Freigaben. Bei
Freesound kommen zudem CC BY-NC und ältere Lizenztypen vor, wie die
[Anbieter-FAQ](https://freesound.org/help/faq/) erläutert. Eine Freigabe des
konkreten Seitenklangs überträgt sich nicht auf den übrigen Katalog. Für eine
kommerzielle Ausgabe werden NC-Dateien ohne zusätzliche passende Erlaubnis nicht
als normale offene Assets eingeplant.

## Grimm, Gutenberg, Tolkien und Lovecraft: Werk und Ausgabe benennen

Der ausgewählte Grimm-Anker ist ausdrücklich **Band 1, deutscher Originaltext,
1812**. Das häufig gefundene [Project-Gutenberg-eBook 2591](https://www.gutenberg.org/ebooks/2591)
ist dagegen als **Englisch** und „Public domain in the USA“ verzeichnet. Es wird
nicht als deutscher Originaltext oder als weltweite Freigabe behandelt. Eine
moderne Übersetzung kann eigene Rechte tragen; für Deutschland ist dafür unter
anderem [§ 3 UrhG](https://www.gesetze-im-internet.de/urhg/__3.html) relevant.
Die allgemeine deutsche Schutzfrist und ihre Berechnung stehen in
[§ 64](https://www.gesetze-im-internet.de/urhg/__64.html) und
[§ 69 UrhG](https://www.gesetze-im-internet.de/urhg/__69.html).

Beim DTA sind Reintext, Annotationen und Scans ausdrücklich verschiedene
Gegenstände. Die Nutzungsbedingungen enthalten zugleich ältere Formulierungen zu
nichtkommerzieller Verwendung und den neueren CC-BY-SA-4.0-Verweis. Hier ist nur
die ausdrückliche Reintextregel der ausgewählte Weg; ein späterer Import einer
anderen Fassung braucht seine eigene Klärung. Der DTA-Einzelabruf der ersten
Textseite war in dieser Recherche nicht verfügbar. Ein bestimmter Originalsatz
oder eine textkritisch geprüfte Lesart wird deshalb nicht behauptet.

**Tolkien ist keine offene Materialquelle dieses Registers.** Die
[Tolkien-Estate-FAQ](https://www.tolkienestate.com/frequently-asked-questions-and-links/)
beschreibt Schutz- und Erlaubnisanforderungen für Texte, Bilder, Figuren,
Sprachen und weitere Gestaltung. Für ein konkretes Vorhaben müssen Werk,
Veröffentlichung, Rechteart, Land und gegebenenfalls die zuständige Lizenzstelle
feststehen. Eigene fantastische Sprachen, Landschaften und Geschichten können
neu entwickelt werden, ohne Tolkien-Texte, Namen, Karten oder Designs zu übernehmen.
Eine aktuelle umfassende Freigabe wird hier nicht behauptet.

**Auch „Lovecraft“ ist keine pauschale Lizenz.** Der
[Brown-University-Bestandsnachweis](https://www.riamco.org/render?eadid=US-RPB-mslovecraft&view=access)
unterscheidet ausdrücklich Länder sowie veröffentlichte und unveröffentlichte
Werke. Sein starrer US-Stichtag „vor 1923“ ist kein aktueller vollständiger
Prüfmaßstab für 2026. In diesem Lauf wurde kein einzelner Lovecraft-Titel samt
Erstveröffentlichung, Ausgabe und Übersetzung zur Nutzung ausgewählt. Für einen
solchen Titel wäre als Nächstes genau diese Rechtekette zu prüfen; Archivzugang
allein genügt nicht. Für HALVETH bleiben unheimliche Entdeckungen und kosmische
Weite als eigene Gestaltungsideen möglich.

Tolkien, Lovecraft und der englische Gutenberg-Vergleich sind damit
**Rechte-/Ausgabenabgrenzungen**, keine zusätzlichen importierbaren Einträge neben
den acht oben ausgewählten Quellen.

## Drei neue, sehr kurze Lore-Seeds

Die folgenden Sätze sind neue HALVETH-Formulierungen, keine Quellenzitate und
keine Tatsachenbehauptungen über wirkliche Wesen:

1. **Der Hüter der Zusagen:** „Der Archivhüter trägt keine Schlüssel. Jeder
   gehaltene Satz öffnet ihm einen anderen Weg.“ Motivbezug: beschreibende
   Eigenschaften beeinflussen Handlungsmöglichkeiten in der
   [Fate-Aspektmechanik](https://fate-srd.com/fate-core/aspects-fate-points).
   Hook: `npc.dispositionTags` öffnet einen Dialogzweig nach einem erfüllten Auftrag.
2. **Die sparsame Sonne:** „Die Leuchte schenkt dem Garten nur so viel Morgen,
   wie ihre Reisende heute braucht.“ Motivbezug: Lichtkegel und begrenzter Vorrat
   in der [Basic-Fantasy-Ausrüstungsreferenz](https://www.basicfantasy.org/srd/equipment.html).
   Hook: Die Werkbank tauscht größeren Lichtkegel gegen höheren Brennstoffverbrauch.
3. **Der Garten der Verwandten:** „Zwei Blüten tragen denselben Namen. Erst das
   Blatt zeigt, welchen Weg ihre Familien nahmen.“ Motivbezug: Namen und
   Klassifikationsbeziehungen der [GBIF-Taxonomie](https://www.gbif.org/en/dataset/d7dddbf4-2cf0-4f39-9b2a-bb099caae36c).
   Hook: Eine eigene Beobachtungsaufgabe unterscheidet visuelle Merkmale; sie
   behauptet keine naturwissenschaftlich vollständige Artbestimmung.

## Nächste konkrete Übernahme

Für 0.2 ist zuerst eine der eigenen Variablenänderungen im nativen Spiel zu
testen: beispielsweise ein merklich veränderter Lichtkegel bei zuvor angezeigten
Kosten. Eine spätere Materialübernahme ergänzt einen Einzelbeleg mit
`sourceID`, Werk-/Datei-ID, Version, Lizenz-URL, Attributionszeile, Änderungsnotiz,
Bytezahl, SHA-256 und Zielpfad. Erst nach tatsächlicher Übernahme und Prüfung
wechselt dieser konkrete Eintrag zu `imported: true`. Ein Registereintrag allein
ändert keine Runtime und erzeugt kein Asset.

Die Recherche ist ein begrenzter Stand: Anbieter- und Werksmetadaten wurden über
Exa gezielt geprüft, keine vollständigen Kataloge. Für Klang, Grafik, Lernnutzen
und Performance fehlen jeweils die späteren konkreten Spiel- und Medientests.
