# Readme - Monte Carlo Tree Search în rezolvarea jocului Go
### Echipa MCTS-GO 

## Introducere:
Go este un board game de strategie jucat in doi jucatori, pe un grid de 19x19 (se poate juca si pe table mai mici, precum 9x9 pentru incepatori). Fiecare jucator are la dispozitie pietre de o singura culoare (albe sau negre) si scopul jocului este de a captura cat mai mult din tabla prin punerea de pietre pe aceasta si prin inconjurarea pietrelor adversarului pentru a captura teritoriu.

Rezolvarea jocului de Go folosind un calculator a fost o mare provocare de-a lungul timpului datorita spatiului foarte mare de explorare (estimat ca fiind 250<sup>150</sup> [1]) folosind algoritmi precum minimax sau alpha-beta pruning. Din acest motiv, o abordare uzuala pentru acest joc este utilizarea MCTS prin care se simuleaza jocul in mod aleatoriu pana intr-un anumit punct, se evalueaza starea acestuia si se propaga rezultatul inapoi pentru luarea deciziei. Acest algoritm a fost folosit si de Alpha Go [1], impreuna cu alti algoritmi de invatare automata (machine learning). 

## Descriere:
Datorita spatiului foarte mare de cautare (este un spatiu exponential!), implementarea seriala a MCTS poate dura foarte mult. Paralelizarea acestuia poate avea rezultate mult mai bune si exista la ora actuala mai multe abordari [2]. 

Codul sursă original pentru implementarea serială se găsește aici: https://github.com/zyxiaooo/GoGame

## Obiective:  
Scopul proiectului este de a alege o abordare de paralelizare si implementarea acesteia folosind OpenMP, MPI si PThreads si evaluarea performantelor.

## Backgound:
- MTCS si metode de paralelizare
- OPM, MPI, PThreads
- Algoritmi paraleli si distribuiti
- Arhitecturi de calculatoare

## Documentatie:
In continuare, este prezentata activitatea in cadrul acestui proiect, intr-o maniera mult mai detaliata.

### Saptamana 1
In prima saptamana, am rulat codul serial folosind Intel VTune 2016 de pe clusterul facultatii, timp de 8 secunde. Rezultatele profiling-ului serial, rulate pe masina dp-wn04.grid.pub.ro, sunt urmatoarele: 
![](./Profiling%20serial/mcts_go_prof_serial_1.png)
![](./Profiling%20serial/mcts_go_prof_serial_2.png)
![](./Profiling%20serial/mcts_go_prof_serial_3.png)

Se poate observa ca procesorul (single threaded) este utilizat la maxim, iar timpul cel mai mult este petrecut in `Possition::is_pass()` care determina daca in starea curenta a jocului, jucatorul curent poate sa puna o piatra pe tabla de joc sau trebuie sa paseze mutarea oponentului. De aici apare problema "ce trebuie paralelizat?". Daca paralelizezi doar acele functii, si incerci sa urmezi legea lui Amdahl, probabil nu se va obtine un speedup foarte mare, datorita limitarilor acestei legi (de exemplu, pentru un cod care este 90% paralelizabil, se obtine un speedup de doar 10 ori). Aceasta problema necesita un volum foarte mare de calcule pentru a da rezultate satisfacatoare. In urma documentarii [3], am descoperit 3 metode de paralelizare:
- leaf parallelization
- root parallelization
- tree parallelization

In metoda **'leaf parallelization'** se presupune ca fiecare fir de executie sa aiba inceapa sa simuleze, in paralel, mai multe jocuri, iar apoi printr-o operatie de reducere se calculeaza valoare finala in acel nod. Aceasta varianta aduce un minim de paralelism intrucat parcurgerea si explorarea arborelui se realizeaza serial.

In metoda **'root parallelization'** se presupune ca fiecare fir de executie are o copie a arborelui de cautare. Fiecare thread realizeaza in paralel un numar de runde de simulari monte carlo, iar in final se face 'merge' intre arborii obtinuti si threadul master ia decizia.

In metoda **'tree parallelization'** se foloseste un singur arbore care este partajat intre threaduri, iar principiul de simulare este acelasi ca la 'root parallelization'. Aceasta metoda implica mecanisme se sincronizare la nivelul nodurilor arborelui.

Ultimele doua metode prezentate aveau performante similare conform studiului de la [3] si astfel am hotarat sa alegem paralelizarea pe radacina, intrucat este mai usor de implementat si are rezultate destul de bune (o mica argumentare se gaseste si mai jos, in saptmana a doua).

### Saptamana 2
In saptamana a doua, codul serial original a suferit multe modificari pentru a fi mult mai usor de citit. Astfel, au fost facute urmatoarele modificari:
  - Aplicarea unui coding style (llvm, care e similar cu linux kernel coding style) pe codul existent
  - Aranjarea codului existent pentru a fi mai usor de urmarit diferentele intre fisierele existente, folosind diff de exemplu
  - Adaugarea posibilitatii de a activa/dezactiva afisarile la stdout (pentru a usura atat procesul de debugging, cat si cel de profiling)
  - Completat makefile deja existent cu reguli lipsa si adaugat lista dependinte la regulile existente

De asemenea, am comparat implementarile deja existente pentru 'root parallelization', 'leaf parallelization' si serial. Rezultate rulari seriale pe masina locala (Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz, 8GB DDR4 2400 MHz, Ubuntu 18.04, Linux 5.0.0-36-generic):

```text
$ time ./GoGame_vs 1 3 3 # Root parallelization 
real	4m22,504s
80 Rounds
3.2 sec/round
```

```text
$ time ./GoGame_vs 1 2 2 # Leaf parallelization
real	2m33,757s
61 Rounds
2.5 sec/round
```

```text
$ time ./GoGame_vs 1 1 1 # Serial implementation
real	1m58,234s
53 Rounds
2.2 sec/round
```

**Nota:** Rularile sunt pe diferitele metode de paralelizare deja existente (OMP, paralelizarea pe funze si pe radacina), si cea seriala. De asemenea, jocurile s-au realizat aleatoriu, si din acest motiv am calculat timpul mediu pe runda.

Cele 3 rulari au fost facute in aceleasi conditii (compilate cu -O3, si daca erau implementari paralele, cu acelasi numar de threaduri, acelasi numar de simulari pe runda) si se poate observa ca rezultatele nu sunt favorabile implementarilor paralele deja existente. Astfel, am decis ca ce este deja existent trebuie rescris pentru a avea performante mult mai bune.


### Saptamana 3
In aceasta saptamana, am implementat varianta pentru OpenMp si am adaugat documentatie pe codul deja existent pentru al face mult mai usor de inteles. Documentatia am facut-o pe baza codului. Alte activitati:
  - Imlementarea root parallelization folosind OpenMP (am folosit ca suport ce era deja existent, insa cu unele imbunatatiri)
  - Balansarea workload-ului deoarece avem de-a face cu simulari aleatoare, deci unele threaduri pot termina mult mai rapid treaba decat altii si deci apar timpi de asteptare in barierele interne OMP. Acest lucru a fost rezolvat folosind timpul in loc de numar de iteratii. Astfel, daca un thread are mai mult de lucru decat ceilalti, o sa faca mai putine simulari si astfel se va utiliza procesorul la capacitate maxima mai mult timp.
  - Adaugarea de comentarii de tip documentatie la modulele principale din cod
  - Activare si dezactivarea comentariilor cu directive de compilare (pentru a reduce numarul de instructiuni jmp din cod). In plus, compilarea se realizeaza de mai putine ori decat numarul de executii al codului
  - Posibilitatea de a realiza simulari random sau dupa regula "urmatoarea celula valida" (ultima fiind folosita pentru a avea rulari deterministice)
  - Oprirea simularii dupa un numar de 5000 de pasi datorate ciclarii care poate sa apara (ciclarea insemnand realizarea unui set de mutari valide la nesfarsit)
  - Analiza de deadlock-uri folosind Intel Inspector 2019
  - Realizarea de profiling folosind Intel VTune Amplifier 2019

Am utilizat utilitarul Intel Inspector 2019 pentru a determina mai usor daca exista posibilitatea de deadlock-uri si data race-uri, acestea fiind destul de dificil de detectat ruland de mai multe ori codul si realizand o analiza statica pe acesta. 
```test
Application: ./GoGame-master/GoGame_root_app # (versiunea de OMP)
Application parameters: 50 10 4
Analysis Type: Locate Deadlocks and Data Races
```
Rezultate date de Intel Inspector 2019 (pe masina locala cu specificatiile descrise anterior) sunt urmatoarele:
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Deadlocks_omp_1.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Deadlocks_omp_2.png?format=raw" />

Din cele doua figuri se poate deduce ca nu sunt sanse (sau daca exista, sunt foarte mici) sa apara probleme legate de data race-uri sau deadlock-uri. 

De asemenea, am folosit VTune Amplifier 2019 (pe masina locala) pentru a determina abordarea care ofera maxim de paralelism. Testele presupun rularea codului in diferite conditii (numar de iteratii/timp de durata iteratii si numar simulari aleatoare/timp pentru simulari aleatoare) si detectarea problemelor de sincronizare care pot duce la pierderea de paralelism. Conditiile de testare sunt: 8 thread-uri, jocuri aleatoare/deterministe (pe principiul 'urmatoarea celula valida'), timpi(0.5s x 0.05s)/iteratii(50 x 10). Rezultatele sunt urmatoarele:

#### 1. Utilizarea unui numar de iteratii pentru playouts si simulari aleatoare''' 
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_Rnd_omp_1.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_Rnd_omp_2.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_Rnd_omp_3.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_Rnd_omp_4.png?format=raw" />

Se poate observa ca se utilizeaza 7 din o procesoare, iar o buna perioada de timp este petrecuta in bariera interna din OpenMP (`__kmpc_barrier` -> 84.5s din 1265.8s => 6.4% din timp). In ultima figura se poate observa cel mai bine acest aspect (multe secvente rosii)

#### 2. Utilizarea unui numar de iteratii pentru playouts si simulari deterministe ("urmatoarea celula valida")''' 
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_No_Rnd_omp_1.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_No_Rnd_omp_2.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_No_Rnd_omp_3.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Iter_No_Rnd_omp_4.png?format=raw" />

Rezultatele sunt similare ca la **1.**, codul sta in bariere.

#### 3. Utilizarea unei durate de timp pentru playouts si simulari aleatoare''' 
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_Rnd_omp_1.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_Rnd_omp_2.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_Rnd_omp_3.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_Rnd_omp_5.png?format=raw" />

Introducerea timpului pare ca duce la o crestere a gradului de paralelism (nu cu foarte mult). Codul in continuare sta in bariere, dar se vad imbunatatiri (de la 6.4% la 5.2%). De asemenea, ultima figura are mai putin rosu datorat barierelor.

#### 4. Utilizarea unei durate de timp pentru playouts si simulari deterministe ("urmatoarea celula valida")''' 
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_No_Rnd_omp_1.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_No_Rnd_omp_2.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_No_Rnd_omp_3.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_Time_No_Rnd_omp_4.png?format=raw" />

Aceasta varianta pare ca este cea mai buna de pana acum. Codul nu mai sta in bariere si se observa ca se utilizeaza peste 7.5 din cele 8 core-uri. De asemenea, workload-ul pare balansat conform ultimei figuri si majoritatea codului este rulat in paralel.

#### 5. Utilizarea unei durate de timp pentru playouts si simulari si realizarea de simulari aleatoare''' 
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_2_Time_Rnd_omp_1.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_2_Time_Rnd_omp_2.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_2_Time_Rnd_omp_3.png?format=raw" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/OMP%20profiling/Profiling_2_Time_Rnd_omp_4.png?format=raw" />

Aceasta este cea mai buna abordare. Utilizarea procesoarelor este ideala, codul nu sta in bariere (foarte putin rosu in ultima figura; 68.5s raportat la 4538.3s => 1.5%) si workload-ul este bine balansat. De asemenea, jocurile sunt aleatorii si se poate beneficia de puterea algoritmului.

**Concluzie:** In teste se vor utiliza abordarile din **4.** sau **5.** intrucat ele utilizeaza procesorul cel mai eficient.


### Saptamana 4
In aceasta saptamana am implementat varianta in MPI deoarece parea ca poate sa puna probleme mai mari decat PThreads. Activitati in cadrul acestei saptamani:
  - Implementarea MPI bazata pe paralelizarea crearii de arbori ( posibilele mutari ) -- ROOT Paralelization.
  - Ne folosim de mecanismul de comunicare pentru a trimite la ROOT ( threadId = 0 ) toti arborii creati de celelalte procese MPI. Pentru a putea face posibila aceasta trimitere am avut nevoie sa serializam informatia folosind biblioteca boost dedicata. Informatia odata primita este deserializata de catre ROOT si folosita pentru a intregii arborele principal.
  - A fost nevoie sa se faca broadcast cu arborii globali si, de asemenea, cu urmatoarea cea mai buna mutare aleasa. Astfel toate procesele vor avea noile date obtinute necesare pentru urmatoarea iteratie a jocului.


### Saptamana 5
{{{
- Status proiect:
  - Implementare Pthreads si Hibrid (OpenMP + MPI)

- Activitati:
  - Implementarea Hibrida bazata pe paralelizarea crearii de arbori ( posibilele mutari ) -- ROOT Paralelization astfel:
    - Ne folosim de codul anterior realizat pentru MPI la care adaugam paralelizarea printr-o directiva OpenMP a generarii de arbori. Astfel fiecare proces va genera OMP_NUM_THREADS posibili arbori. Acestia se vor salva local fiecarui proces dupa care vor fi trimisi printr-un mesaj la procesul MPI ROOT (threadId = 0) care va agrega informatia primita intr-un singur arbore si caruia ii va face broadcast impreuna cu mutarea cea mai buna aleasa tuturor celorlalte procese.

  - Implementarea Pthreads:
    - Initial am decis crearea de thread-uri doar pentru paralelizarea alcatuirii noilor arbori de mutari. Astfel fiecare thread ar fi facut un arbore de mutari care apoi ar fi fost agregat in structura globala de thread-ul care le-a creat.
    - Am decis ca sunt prea multe thread-uri create si nu este profitabil din punct de vedere al performantei deoarece se pierdea mult prea mult timp cu crearea acestora. Astfel am ales sa creem doar NUM_THREADS dat ca parametru programului care sa obtina job-uri dintr-un pool in care thread-ul principal adauga.
      - Se vor sincroniza thread-urile care executa job-urile astfel:
        - Cu o bariera - se asteapta incheierea tuturor job-urilor pentru a putea agrega rezultatele si alege mutarea cea mai favorabila ( in thread-ul principa )
        - Cu un mutex si un broadcast la variabila conditie - se asigura accessul la pool a unui singur thread la un moment de timp pentru obtinerea unui job
}}}


Analiza scalabilitate a fost realizata pe cluster-ul facultatii, pe hp-sl si pe ibm-dp folosind sistemul de batch. Rezultatele pentru omp, pthreads, mpi, hybrid (omp + mpi) raportate la varianta seriala (pe un singur thread) sunt urmatoarele:

Pentru hp-sl:
{{{
#!html
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/Scalability/performance-hp-sl.png" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/Scalability/scalability-hp-sl.png" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/Scalability/efficiency-hp-sl.png" />
}}}

Pentru ibm-dp:
{{{
#!html
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/Scalability/performance-ibm-dp.png" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/Scalability/scalability-ibm-dp.png" />
<img src="https://raw.githubusercontent.com/razvanalex/MCTS-GO/master/Scalability/efficiency-ibm-dp.png" />
}}}

Nota: varianta hibrida utilizeaza 2 threaduri OMP si numar variabil de procese MPI. In grafice, s-a ilustrat 2 * nr_procese = numar de threaduri.

Analiza a fost facuta pentru legea lui Gustafson (care ia in calcul cantitatea de munca realizata pe numarul de fire de executie). Din acest motiv, se justifica graficele liniare pentru scalabilitate (avem weak scaling pentru aceasta problema: mai multe fire de executie realizeaza simulari intr-o perioada de timp specificata).

Se poate observa ca variantele care folosesc doar threaduri (omp si pthreads) nu scaleaza conform asteptarilor, acest lucru fiind datorat fenomenului de 'false cache sharing'. Exista date care sunt partajate intre threaduri (precum starea curenta, arborii de cautare etc) si acest lucru influenteaza dramatic performantele (intrucat cache-ul se invalideaza foarte des si avem un bottleneck de memorie). Acest lucru este accentuat prin faptul ca in varianta de MPI aceasta problema nu apare (fiecare proces are zona sa de memorie, iar transmiterea datelor se face prin mecanismele din MPI). Varianta hibrida este undeva intre MPI si omp/pthreads.
Intre pthreads si openMP este o diferenta foarte mica, fiind putin mai eficienta varianta cu pthreads.
Scalarea este una liniara, iar dupa un punct, odata cu cresterea numarului de threaduri (in cazul de fata, dupa 16 pentru hybrid si 8 pt MPI) speedup-ul se plafoneaza. Acest lucru se datoreaza si capabilitatilor hardware disponibile pe cele doua platforme.



### Resurse:
[1] https://storage.googleapis.com/deepmind-media/alphago/AlphaGoNaturePaper.pdf  
[2] https://pdfs.semanticscholar.org/a8fc/4fe88d984348723e282653697970935ccbac.pdf  
[3] https://github.com/razvanalex/MCTS-GO/blob/master/Parallel%20Monte-Carlo%20Tree%20Search.pdf

