# Datei- und Versionslizenzen

Diese Karte ordnet konkrete Beiträge zu. Sie ersetzt keine früher wirksame
Freigabe und beansprucht keine Rechte außerhalb des jeweiligen Beitrags.

## Bestehender Stand

Der veröffentlichte Ausgangsstand
`5c2260f85d5275c15082c5c7d38ae51a143a5688` und seine bereits unter MIT
freigegebenen überlappenden Anteile behalten die Erlaubnisse aus [LICENSE](LICENSE).
Die bestehenden Dateien einschließlich ihrer Integrationsänderungen bleiben
unter ihren bisherigen Lizenzen. Eine neue Karte erzeugt keine nachträgliche
Zahlungspflicht für bereits erlaubte Nutzung.

## Neue Meadow-Beiträge

Die folgenden sechs erstmals in der Meadow-Linie hinzugefügten Quelldateien
ordnen ausschließlich ihre neuen eigenen, schutzfähigen Beiträge der
[HALVETH Public-Interest Research License 2.0](LICENSE-HALVETH-PIRL-2.0.md) zu:

| Datei | Neuer Beitrag |
| --- | --- |
| `Source/HALVETHRealms/Public/MeadowModel.h` | Deterministische Halmgeometrie und gebundene Windfunktion |
| `Source/HALVETHRealms/Public/HALVETHMeadow.h` | Schnittstelle des begrenzten Wiesenfensters |
| `Source/HALVETHRealms/Private/HALVETHMeadow.cpp` | An Weltzellen gebundene Instanzen, Bodenprüfung und native Aufnahmen |
| `Source/HALVETHRealmsEditor/Public/MeadowPrepare.h` | Schnittstelle der neuen Asset-Vorbereitung |
| `Source/HALVETHRealmsEditor/Private/MeadowPrepare.cpp` | Neue Wiesenmaterialien, Halme und Blütengeometrie |
| `tests/meadow_model_test.cpp` | Zugehörige geometrische und dynamische Gegenproben |

Die Zuordnung beginnt mit der ersten veröffentlichten Fassung dieser Karte
und der sechs Dateien. Die exakten Quelldigests stehen in
[HALVETH-RIGHTS.json](HALVETH-RIGHTS.json). Frühere wirksame Grants bleiben
auch dann erhalten, wenn ein überlappender Anteil in einer neuen Datei steht.

`MeadowPrepare.cpp` verwendet bestehende HALVETH-MIT-Muster für das Speichern
von Unreal-Paketen und das Anlegen einer `MeshDescription`. Diese überlappenden
Teile behalten die MIT-Erlaubnisse und den unveränderten MIT-Lizenztext.
Allgemeine Unreal-APIs, Datenstrukturen und übliche Programmiermuster werden
nicht als ausschließliches HALVETH-Eigentum beansprucht.

PIRL erlaubt die dort beschriebenen nichtkommerziellen Nutzungen im
öffentlichen Interesse. Eine kommerzielle Nutzung, die rechtlich eine
Erlaubnis benötigt, erfordert für die zugeordneten neuen Beiträge eine
gesonderte vorherige schriftliche Vereinbarung. Kontakt:
`security@halveth.de`. Download, Fork oder Nutzung erzeugen allein weder eine
vereinbarte Vergütung noch eine automatische Beteiligungsquote.

Dies ist für die zugeordneten Beiträge eine Source-available-Lizenz. Das
gemischt lizenzierte Gesamtprojekt wird nicht pauschal als vollständig unter
MIT oder als vollständig OSI-Open-Source bezeichnet.

## Fremdmaterial, Daten und Darstellungen

- Poly-Haven-Modelle und Texturen bleiben CC0; ihre Herkunft steht in den
  Asset-Inventaren und [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
- MakeHuman-Quellen und ausdrücklich freigegebene Charakterassets behalten
  ihre vorhandenen CC0-Zuordnungen und Urheberhinweise.
- Unreal Engine, seine APIs, mitgelieferte Inhalte und Runtime-Komponenten
  behalten Epics und die jeweiligen fremden Bedingungen.
- Messdaten, Fakten, allgemeine Ideen, mathematische Methoden, Namen und
  Marken werden durch diese Karte nicht exklusiv lizenziert.
- Ein generiertes Asset oder Bildschirmbild überträgt keine Rechte an
  enthaltenem Fremdmaterial. Ein Runtime-Paket kombiniert die jeweils
  zugeordneten Bedingungen, ohne sie zu einer neuen Gesamtlizenz zu machen.

Die PIRL-Fassung wird unverändert übernommen. Diese Karte ist eine begrenzte
Rechtezuordnung, keine Patent-, Zertifikats- oder Erfolgsbehauptung und keine
Garantie für Rechtsdurchsetzung oder Einnahmen.
