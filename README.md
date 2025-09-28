Ugrajeni ukazi:

  Ukaz: debug level
    Opcijski številčni argument level podaja nivo razhroščevanja (debug level).
    Če argument ni podan, se izpiše trenutni nivo razhroščevanja.
    Če pa je podan, se nastavi nivo razhroščevanja na podani level.
    Izpiše argumente, če je nivo razhroščevanja višji od 0.
    
  Ukaz: prompt poziv
    S tem ukazovom izpišemo ali nastavimo izpis pozivnika (prompt).
    Če argument ni podan, potem se izpiše trenutni poziv.
    Če je podan, pa se nastavi nov pozivnik. Podano ime lahko vsebuje do 8 znakov.

  Ukaz: status
    Izpiše se izhodni status zadnjega izvedenega ukaza.

  Ukaz: exit status
    Izvajanje lupine se zaključi s podanim izhodnim statusom.
    Če argument ni podane, se lupina konča s statusom zadnjega izvedenega ukaza.

  Ukaz: help
    Izpiše spisek podprtih ukazov.

  Ukaz: print args...
    Izpiše podane argumente na standardni izhod (brez končnega skoka v novo vrstico).

  Ukaz: echo args...
    Kot ukaz print, le da izpiše še skok v novo vrstico.

  Ukaz: len args...
    Izpiše skupno dolžino vseh argumentov (kot nizi).

  Ukaz: sum args...
    Sešteje svoje argumente (cela števila) in izpiše vsoto.

  Ukaz: calc arg1 op arg2
    Nad argumentoma arg1 in arg2 izvede operacijo op in izpiše rezultat.
    Argumenta sta števili.
    Operacija je lahko +, -, *, /, %.

  Ukaz: basename arg
    Izpiše osnovno ime podane poti arg, podobno kot ukaz basename.

  Ukaz: dirname arg
    Izpiše imenik podane poti arg, podobno kot ukaz dirname.

  Ukaz: dirch imenik
    Zamenjava trenutnega delovnega imenika (working directory).
    Če imenika ne podamo, skoči na korenski imenik.

  Ukaz: dirwd mode
    Izpis trenutnega delovnega imenika.
    Če je mode enako full se izpiše celotna pot.
    Če je mode enako base, se izpiše le osnova imena (basename).
    Če argumenta ne podamo, se privzame base.

  Ukaz: dirmk imenik
    Ustvarjanje podanega imenika.
  
  Ukaz: dirrm imenik
    Brisanje podanega imenika.
  
  Ukaz: dirls imenik
    Preprost izpis vsebine imenika.
    Izpišejo se le imena datotek, ločena z dvema presledkoma.
    Če imena imenika ne podamo, se privzame trenutni delovni imenik.

  Ukaz: rename izvor ponor
    Preimenovanje datoteke izvor v ponor.

  Ukaz: unlink ime
    Odstrani datoteko s podanim imenom.

  Ukaz: remove ime
    Odstranjevanje datoteke ali imenika s podanim imenom.

  Ukaz: linkhard cilj ime
    Ustvari trdo povezavo s podanim imenom na cilj.

  Ukaz: linksoft cilj ime
    Ustvari simbolično povezavo s podanim imenom na cilj.

  Ukaz: linkread ime
    Izpiše cilj podane simbolične povezave.

  Ukaz: linklist ime
    V trenutnem delovnem imeniku poišče vse trde povezave na datoteko s podanim imenom.

  Ukaz: cpcat izvor ponor
    Znana ukaza cp in cat združena v enega.

  Ukaz: pid
    Izpis PID procesa lupine.

  Ukaz: ppid
    Izpis PID starševskega procesa lupine.
  
  Ukaz: uid
    Izpis UID uporabnika, ki je lastnik procesa lupine.
  
  Ukaz: euid
    Izpis UID uporabnika, ki je aktualni lastnik procesa lupine.

  Ukaz: gid
    Izpis GID skupine, kateri pripada procesa lupine.

  Ukaz: egid
    Izpis GID skupine, kateri aktualno pripada procesa lupine.

  Ukaz: sysinfo
    Izpiše osnovne informacije v sistemu.
    Izpiše polja: sysname, nodename, release, version, machine.

  Ukaz: proc pot
    Nastavitev poti do procfs datotečnega sistema.
    Brez argumenta se izpiše trenutna nastavitev. 
    Privzeta nastavitev je /proc.
    Če pa podamo argument, se nastavi nova pot do imenika, ki vsebuje procfs.

  Ukaz: pids
    Izpiše PIDe trenutnih procesov, ki jih pridobi iz procfs.
    Vsak pid je izpisan v svoji vrstici.

  Ukaz: pinfo
    Izpiše informacije o trenutnih procesih (PID, PPID, STANJE, IME), ki jih pridobi iz datoteke stat v procfs.

  Ukaz: waitone pid
    Počaka na otroka s podanim pidom.
    Če pida ne podamo, počaka na enega poljubnega otroka.
    Če otrok ne obstaja, bo zadnji status 0, sicer pa izhodni status končanega otroka.
  
  Ukaz: waitall
    Počaka na vse otroke.

  Ukaz: pipes "stopnja 1" "stopnja 2" "stopnja 3" ...
    Ustvari cevovod.
  
