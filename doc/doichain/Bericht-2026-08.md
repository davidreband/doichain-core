# Doichain Core — Statusbericht und Vorhaben

Stand: 17. August 2026

Dieser Bericht fasst zusammen, in welchem Zustand Doichain Core vorgefunden
wurde, was inzwischen erledigt ist, was noch geplant ist und welche
Entscheidungen dafür getroffen werden müssen. Alle genannten Zahlen sind
gemessen, nicht geschätzt.

---

## 1. Ausgangslage

Im Produktivbetrieb läuft `dc0.20.1.10`, ein Fork von Bitcoin Core 0.20 aus dem
Jahr 2020. Die Muttertechnologie ist seither um **neun Hauptversionen**
weitergezogen: Bitcoin Core steht bei 31.1, Namecoin Core bei nc31.1.

Damit war Doichain das letzte veraltete Glied in der Kette. Konkret bedeutete
das:

* keine Sicherheitskorrekturen und Verbesserungen aus sechs Jahren Entwicklung,
* ein Build-System, das es nicht mehr gibt (Autotools statt CMake),
* Abhängigkeiten, die inzwischen entfallen sind (Boost) oder entfernt wurden
  (BerkeleyDB für Wallets),
* eine Codebasis, die sich mit jedem weiteren Jahr schwerer aktualisieren
  lässt.

Die Kette selbst ist klein und vollständig: rund 1,9 GB, Höhe etwa 430 900,
nicht beschnitten, mit vollem Transaktions- und Namensindex.

---

## 2. Vorgeschichte des Problems

Doichain wurde 2020 von Namecoin abgezweigt. In den 128 Commits, die den Fork
ausmachen, sind zwei Dinge vermischt: die eigentliche Doichain-Funktionalität
und Überreste der damaligen Fehlersuche.

Die Analyse ergab, dass **65 dieser 128 Commits reine Debug-Arbeit** sind —
Protokollausgaben, auskommentierte Prüfungen, Testadressen. Einiges davon ist
im Produktivcode geblieben und dort seit Jahren wirksam. Die wichtigsten Funde:

**Die Schwierigkeitsprüfung war sechs Jahre abgeschaltet.** In
`ContextualCheckBlockHeader` war die Prüfung, ob die im Block angegebene
Schwierigkeit der berechneten entspricht, auskommentiert. Ein Knoten akzeptierte
also jeden Wert. Wir haben nachgemessen: über alle 213 historischen
Anpassungspunkte stimmt die Kette exakt mit der Berechnung überein — die
Prüfung wäre nie fehlgeschlagen. Sie ist wieder aktiv.

**Drei Schutzmechanismen haben nie funktioniert.** Die Sperre gegen die
Verwendung des `d/`-Namensraums für DOI-Operationen verglich den Namen mit einer
in Anführungszeichen gesetzten Fassung und griff deshalb nie. Eine Sperre gegen
das Überschreiben bestehender DOI-Einträge ab Höhe 170 000 liest eine Variable,
die an dieser Stelle immer null ist, und greift ebenfalls nie. Und die Prüfung,
dass ein Namens-Update nicht auf einen abgelaufenen Namen zeigt, war für
DOI-Operationen auskommentiert.

**`name_doi` hatte keinen einzigen Test** — weder Unit- noch Funktionstest —
seit der Operation Einführung. Genau deshalb blieben die obigen Punkte
unbemerkt.

---

## 3. Was erledigt ist

### 3.1 Analyse und Inventarisierung

Vollständige Aufnahme aller Doichain-Änderungen gegenüber Namecoin, gegliedert
nach Netzwerkparametern, Protokoll, Speicherschicht, RPC, Wallet und
Markenanpassung. Die 128 Commits wurden auf **12 thematische Commits**
zusammengeführt; die 65 Debug-Commits sind entfallen.

Ergebnis dieser Aufräumarbeit: künftige Aktualisierungen sind ein Rebase von
zwölf lesbaren Commits, nicht das Nachvollziehen von 128 unsortierten.

### 3.2 Portierung auf Namecoin Core 31.1

Statt sich schrittweise über zehn Zwischenversionen zu bewegen, wurde direkt auf
31.1 portiert. Grundlage dieser Entscheidung war eine Messung: die Dateien, die
Doichain überhaupt verändert, sind im Oberlauf nahezu unangetastet geblieben.
`script/names.*` und `names/mempool.*` haben sich zwischen nc29 und nc31.1 um
**null Zeilen** verändert, `names/main.cpp` um dreizehn. Der Zwischenschritt
hätte also fast nichts eingebracht, dafür aber zehnmal die Mühe verursacht, eine
Bauumgebung von 2020 mit BerkeleyDB 4.8 herzustellen.

Der Fork besteht nun aus 18 nachvollziehbaren Commits über dem unveränderten
Oberlauf-Tag.

### 3.3 Nachweis, dass der Konsens unverändert ist

Das ist der wichtigste Punkt des ganzen Vorhabens.

Ein Knoten aus dem neuen Code hat die **gesamte Kette neu validiert** — die
ersten 309 855 Blöcke aus lokalen Blockdateien, die restlichen etwa 121 000 aus
dem laufenden Netz — und endet auf demselben Block wie der Produktivknoten:

    unser Knoten : 5d561d4c32af68a3135f66c7edbae5f9ac6320409ab5b1b517ea5298b731232f
    Produktion   : 5d561d4c32af68a3135f66c7edbae5f9ac6320409ab5b1b517ea5298b731232f

Kein einziger Konsensfehler, und das mit **wieder eingeschalteter**
Schwierigkeitsprüfung. Der Knoten hat sich dabei mit 26 Gegenstellen der alten
Version verbunden und Blöcke von ihnen bezogen — die Netzwerkkompatibilität ist
damit ebenfalls belegt.

### 3.4 Tests

|  |  |
|---|---|
| Unit-Tests | 150 von 151 Testgruppen erfolgreich |
| Funktionstests Namen | 28 von 28 erfolgreich |

Die eine rote Testgruppe ist absichtlich rot, siehe Abschnitt 5.3.

Neu geschrieben wurden die ersten Tests für `name_doi` überhaupt: Registrierung,
Aktualisierung, Registrierung ohne Namens-Eingang, Ablehnung des
`d/`-Namensraums, Transaktionsdekodierung, verkettete offene Operationen und
Reorganisation.

**Diese Tests haben unmittelbar vier Defekte gefunden**, alle im bestehenden
Code: eine fehlende Begrenzung gleichzeitig offener Operationen, eine doppelte
Zählung in derselben Begrenzung, eine Ablehnung von Aktualisierungen, deren
Vorgänger noch im Speicherpool liegt, und einen Absturz des Knotens beim
Neuregistrieren eines abgelaufenen Namens. Alle vier sind behoben.

Zusätzlich wurde eine Prüfung des gesamten Fork-Bestands gegen den alten Code
durchgeführt. Sie förderte **sechs Einstellungen** zutage, die bei der
Portierung verloren gegangen waren, darunter eine, die zu einer Spaltung des
Testnetzes geführt hätte, und 111 fest eingebaute Adressen fremder
Namecoin-Knoten.

### 3.5 Aufhebung des Namensablaufs — implementiert

Namecoin lässt Namen verfallen, damit aufgegebene Domains wieder frei werden.
Für Datensätze, die eine Identität tragen, ist das falsch, und die Zahlen auf
der Kette zeigen es deutlich:

    Namen insgesamt : 57 742
    davon abgelaufen: 57 699
    noch gültig     :      43

Bei den angestrebten zehn Minuten je Block entsprechen die 36 000 Blöcke
Ablauffrist etwa 250 Tagen. Ein Identitätseintrag verschwand also nach acht
Monaten still, wenn ihn niemand erneuerte.

Die Regel ist implementiert, getestet und dokumentiert, aber **auf keinem Netz
aktiviert** — das Verhalten ist derzeit unverändert. Zur Technik siehe
Abschnitt 4.1.

---

## 4. Was geplant ist

Zwei Änderungen stehen an. Beide sind **Hard Forks**: sie erweitern die Regeln,
alte Knoten würden die neuen Blöcke ablehnen. Alle Knoten und alle Miner müssen
bis zum Aktivierungszeitpunkt aktualisiert sein.

Deshalb der Vorschlag, **beide mit demselben Aktivierungszeitpunkt** in Kraft zu
setzen: eine Abstimmung mit Betreibern und Minern statt zwei.

### 4.1 Namensablauf aufheben

Die Regel lautet: Namen, deren letzte Aktualisierung auf oder über der
Aktivierungshöhe liegt, verfallen nicht mehr.

Entscheidend ist, dass die Regel an der **Aktualisierungshöhe des Namens** hängt
und nicht an der aktuellen Kettenhöhe. Die naheliegende Formulierung „ab Höhe N
verfällt nichts mehr" würde alle 57 699 bereits abgelaufenen Namen wieder als
gültig ausweisen — deren Ausgänge sind aber längst verbraucht, und der Knoten
prüft die Namensdatenbank gegen den Bestand unverbrauchter Ausgänge ab. Er
würde beim nächsten Abgleich abbrechen.

So bleibt die Vergangenheit unangetastet: historische Blöcke validieren
unverändert, Abgelaufenes bleibt abgelaufen, und wer einen abgelaufenen Namen
neu registriert, erhält einen dauerhaften Eintrag.

Der Preis der Änderung: Namensausgänge werden nie mehr freigegeben, der Bestand
unverbrauchter Ausgänge wächst also nur noch. Zur Größenordnung: der Namensraum
`bp/` allein hat 45 702 Namen über 76 000 Blöcke angelegt, etwa 0,6 Namen je
Block.

### 4.2 Schwierigkeitsanpassung nach dem Vorbild von Bitcoin Cash

**Das drängendere der beiden Vorhaben.** Die Kette steckt derzeit fest.

Gemessen am 12. August:

| | letzte 2016 Blöcke | 2016 Blöcke etwa 70 Tage früher |
|---|---|---|
| Mittlerer Abstand | 122,8 min | 12,4 min |
| Median | 27,9 min | 8,3 min |
| 90. Perzentil | 369 min | 29 min |
| länger als 6 Stunden | 10,2 % | 0,00 % |

Bis vor etwa zehn Wochen hielt die Kette ihr Ziel von zehn Minuten nahezu
genau. Jetzt liegt der Mittelwert zwölffach darüber, jeder zehnte Block braucht
über sechs Stunden.

**Die Kette kann sich nicht selbst befreien.** Das Anpassungsfenster wird in
Blöcken gezählt: werden Blöcke langsamer, dehnt sich das Fenster mit ihnen. Der
laufende Abschnitt hat 1474 Blöcke in 167 Tagen erbracht, die nächste Anpassung
liegt 542 Blöcke und damit etwa 62 Tage entfernt, und eine einzelne Anpassung
ist auf den Faktor vier begrenzt, während etwa das Sechzehnfache nötig wäre.
Drei Anpassungen hintereinander, in der Größenordnung von vier Monaten — und
das nur, wenn die Rechenleistung nicht weiter sinkt.

Genau für diese Lage hat Bitcoin Cash nach seiner Abspaltung 2017 eine
Anpassung **je Block** entwickelt. Doichains Lage ist strukturell dieselbe, nur
schärfer: die Rechenleistung stammt aus dem Mitschürfen mit Bitcoin, gehört
also anderen und verschwindet ohne Vorankündigung.

**Technisch** ist ASERT (`aserti3-2d`) zu nehmen, das seit November 2020 bei
Bitcoin Cash in Betrieb ist. Es berechnet die Schwierigkeit als
Exponentialfunktion der Abweichung zwischen tatsächlicher und planmäßiger Zeit,
gemessen ab einem Ankerblock. Es hat keinen inneren Zustand, sammelt daher
keinen Fehler an und schwingt nicht — beides Probleme der beiden Vorgänger,
die Bitcoin Cash deshalb ersetzt hat.

Für die Umsetzung liegen von Bitcoin Cash **veröffentlichte Prüfvektoren** vor.
Sie zu übernehmen nimmt das Hauptrisiko aus der Arbeit, denn ein Fehler in der
Festkomma-Arithmetik wäre mit eigenen Tests kaum zu finden.

Nebeneffekt: die in Abschnitt 5.3 beschriebene Reserve im
Schwierigkeitsgrenzwert wird gegenstandslos, weil die alte Berechnung für neue
Blöcke nicht mehr aufgerufen wird.

#### Warum die Übernahme kein Kopieren ist

Die Rechenvorschrift selbst ist überschaubar und liegt als erprobter Code vor.
Die Schwierigkeit liegt an drei anderen Stellen.

**Erstens: es ist eine Konsensregel.** Geändert wird nicht ein Programmteil,
sondern die Regel, nach der alle Knoten im Netz einen Block für gültig halten.
Alte und neue Knoten würden sich uneinig — deshalb müssen bis zum Stichtag alle
Betreiber und Miner umgestellt haben. Der Zeitpunkt muss außerdem so gewählt
sein, dass die bisherige Kette weiterhin lückenlos gültig bleibt.

**Zweitens: die Rechnung arbeitet mit Näherungswerten.** Um eine
Exponentialfunktion ohne Kommazahlen zu berechnen, verwendet Bitcoin Cash eine
Annäherung mit Festkommazahlen. Ein Fehler darin liefert keine Fehlermeldung,
sondern plausible, aber falsche Zahlen — und ein selbst geschriebener Test, der
derselben Formel folgt, würde denselben Fehler wiederholen. Das ist das
eigentliche Risiko.

Dagegen gibt es ein bewährtes Vorgehen: den Originalcode wortgetreu übernehmen
statt ihn zu verbessern, die von Bitcoin Cash veröffentlichten Prüfvektoren
nutzen, und zusätzlich gegen eine unabhängig gerechnete, exakte Fassung
vergleichen. Erst diese Kombination deckt beides ab — Übereinstimmung mit der
erprobten Umsetzung und mathematische Richtigkeit. Ergänzend prüft ein
Zufallstest die Eigenschaften, die immer gelten müssen, und ein Durchlauf über
die tatsächliche Kettengeschichte zeigt, wie sich die Regel auf echten Daten
verhalten hätte.

**Drittens: Bitcoin Cash ist eine andere Kette.** Doichain wird mitgeschürft,
Bitcoin Cash nicht. Die bestehende Berechnung enthält Besonderheiten aus dem
Mitschürfen, die für alle alten Blöcke unverändert erhalten bleiben müssen. Die
neue Regel kommt daneben, nicht an ihre Stelle.

**Einschätzung:** technisch beherrschbar, Aufwand etwa eine Arbeitswoche für die
Umsetzung samt Tests, danach mindestens zwei Wochen Beobachtung im Testnetz. Das
Risiko liegt nicht in der Programmierung, sondern in der Sorgfalt der Prüfung
und in der Abstimmung des Stichtags.

---

## 5. Offene Entscheidungen

### 5.1 Aktivierungszeitpunkt

Beide Hard Forks brauchen einen. Empfehlung: **nach Kalenderzeit**, nicht nach
Blockhöhe. Eine Höhenangabe unterstellt einen vorhersehbaren Blocktakt, und
genau der ist gestört: „in 1000 Blöcken" bedeutet beim derzeitigen Tempo 85
Tage, mit unsicherem Ausgang.

### 5.2 Halbwertszeit der Anpassung

Bitcoin Cash verwendet zwei Tage. Bei zehn Minuten Zielabstand entspricht das
288 Blöcken. Empfehlung: mit diesem erprobten Wert beginnen und einen kürzeren
erst nach Messungen im Testnetz erwägen — ein zu kurzer Wert reagiert schneller,
wird aber unruhig und anfälliger für manipulierte Zeitstempel.

### 5.3 Umgang mit dem Schwierigkeitsgrenzwert

Doichains Grenzwert liegt 74-fach über der Schwelle, ab der die
Zwischenrechnung der alten Schwierigkeitsanpassung 256 Bit überschreiten kann.
Ein Überlauf würde die Schwierigkeit unerreichbar hoch setzen und die Kette
anhalten.

Gemessen über alle 213 historischen Anpassungen ist das **nie eingetreten**; der
knappste Fall lag bei 11,2 % des kritischen Werts, ein Abstand von etwa
Faktor neun, und zwar 2018 in der Frühphase der Kette. Beim heutigen
Schwierigkeitsniveau beträgt der Abstand etwa 29 Bit.

Die entsprechende Testgruppe ist bewusst rot gelassen worden, statt sie
stillzulegen: sie meldet eine echte Eigenschaft der Parameter. Mit Abschnitt 4.2
erledigt sich die Frage von selbst.

### 5.4 Kleinere offene Punkte

Die Einordnung von DOI-Transaktionen in der grafischen Oberfläche kennt die
Operation nicht und zeigt eine Registrierung als „Name empfangen". Nicht
angefasst, weil die Oberfläche in dieser Umgebung nicht gebaut werden kann und
eine ungeprüfte Änderung schlechter wäre als ein dokumentierter Mangel.

Für die Regel aus Abschnitt 4.1 fehlen noch Unit-Tests; abgedeckt ist sie
derzeit nur durch einen Funktionstest.

---

## 6. Weiteres Vorgehen

| Schritt | Zustand |
|---|---|
| Analyse und Inventarisierung | abgeschlossen |
| Portierung auf Namecoin Core 31.1 | abgeschlossen |
| Tests, eigene wie übernommene | abgeschlossen |
| Konsensnachweis gegen den Produktivknoten | abgeschlossen |
| Aufhebung des Namensablaufs, Umsetzung | abgeschlossen, nicht aktiviert |
| ASERT, Umsetzung | offen |
| Aktivierungszeitpunkt festlegen | **Entscheidung nötig** |
| Erprobung im Testnetz | offen |
| Abstimmung mit Betreibern und Minern | offen |
| Produktivumstellung | offen |
| Bereitstellungsskripte anpassen | offen |

### Reihenfolge der Produktivumstellung

Zuerst ein **Testknoten** auf einer Kopie der Produktivdaten, nicht der
Produktivknoten selbst: das Datenformat wandelt sich einseitig, eine Rückkehr
zur alten Version ist danach nicht möglich. Abnahmekriterium ist derselbe
Vergleich wie in Abschnitt 3.3 — gleiche Höhe, gleicher Blockhash.

Dann die vier abhängigen Dienste, nach Wichtigkeit: **ElectrumX** zuerst, weil
davon alle Nutzer der Wallet-Anwendung abhängen, danach das Mitschürfen über
p2pool, dann die Benachrichtigungen der dApp, zuletzt die Weboberfläche.

Vor Änderungen an den Bereitstellungsskripten sind die tatsächlichen
Konfigurationen der laufenden Knoten mit den Vorlagen abzugleichen. Bei der
Untersuchung fielen vier Abweichungen auf, unter anderem Gebührenwerte, die um
Größenordnungen unter der Vorlage liegen. Ein Überschreiben mit der Vorlage
würde sie stillschweigend ändern.

### Aufwandsschätzung

Die Umsetzung von ASERT samt Prüfvektoren und Tests: etwa eine Arbeitswoche.
Erprobung im Testnetz: mindestens zwei Wochen Beobachtung, unabhängig vom
Aufwand. Produktivumstellung samt Diensten: wenige Tage, sobald die
Entscheidungen aus Abschnitt 5 getroffen sind.

---

## 7. Was dieses Vorhaben bereits gebracht hat

Unabhängig von den beiden noch ausstehenden Änderungen:

* Die Codebasis ist von 2020 auf den aktuellen Stand gebracht, ohne den Konsens
  zu verändern — nachgewiesen an der vollständigen Kette.
* Sechs Jahre Sicherheits- und Stabilitätsarbeit von Bitcoin Core sind
  übernommen.
* Zehn Defekte im bestehenden Code wurden gefunden und behoben, darunter zwei,
  die einen Knoten zum Abbruch bringen, und einer, der das Testnetz gespalten
  hätte.
* Der Fork besteht aus lesbaren, thematisch getrennten Commits; die nächste
  Aktualisierung ist Tagesarbeit statt eines Projekts.
* Es gibt erstmals Tests für die Doichain-eigene Operation, und eine
  Abnahmeprüfung, die vor jeder Veröffentlichung wiederholbar ist.
* Der Zustand des Fork, seine Abweichungen von Namecoin und die Begründungen
  dafür sind schriftlich festgehalten.
