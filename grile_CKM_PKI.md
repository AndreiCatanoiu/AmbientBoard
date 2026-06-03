# Set de grile – Managementul cheilor criptografice & PKI

> Bazate pe documentele: **CKM**, **1. KeyMgmt (EN)**, **2. PKI (EN)**.
> Unele întrebări pot avea **mai multe răspunsuri corecte**. Cheia de răspunsuri este la finalul documentului.

---

## Tema 1 – Managementul cheilor criptografice

**1. De ce este managementul cheilor un aspect critic al unui sistem criptografic?**
A. Pentru că securitatea sistemului depinde în mare măsură de modul în care sunt administrate cheile
B. Pentru că algoritmii criptografici sunt secreți
C. Pentru că erorile pot apărea atât în implementare, cât și mai ales în faza de operare
D. Pentru că factorul uman are un rol important și greu de eliminat

**2. Care sunt etapele din ciclul de viață al unei chei criptografice?**
A. Generare și distribuție
B. Instalare și utilizare
C. Salvare/restaurare și arhivare
D. Ștergere (terminare)

**3. Referitor la generarea cheilor, care afirmații sunt adevărate?**
A. Pe un sistem de calcul este imposibil să se genereze numere cu adevărat aleatoare
B. Generatoarele deterministe (pseudoaleatoare) folosesc un algoritm (ex. FIPS 186-3)
C. Generatoarele nedeterministe folosesc surse fizice (zgomot termic, contoare Geiger-Müller)
D. Numerele generate trebuie să fie predictibile pentru a putea fi verificate

**4. Care este cea mai critică operație din managementul cheilor?**
A. Generarea cheilor
B. Distribuția cheilor
C. Arhivarea cheilor
D. Ștergerea cheilor

**5. Cum se pot distribui cheile în sistemele criptografice simetrice?**
A. Prin curier de încredere (valiză diplomatică)
B. Prin Quantum Key Distribution (QKD)
C. Prin certificate digitale
D. Prin publicarea lor într-un Repository

**6. Unde/cum pot fi instalate cheile criptografice?**
A. Pe hard disk, în fișiere criptate cu parolă (PKCS #12)
B. Pe dispozitive criptografice (HSM, smart card, token USB)
C. În registrul de sistem, în clar
D. Cu acces restricționat de sistemul de operare

**7. Referitor la salvarea (backup) și restaurarea cheilor, ce este corect?**
A. Se salvează doar cheile de decriptare a datelor
B. Cheile de semnătură NU trebuie salvate
C. Salvarea cheilor de semnătură ar duce la pierderea proprietății de non-repudiere
D. Se salvează toate cheile, inclusiv cele de semnătură

**8. La ce se referă termenul „key escrow”?**
A. Un protocol care negociază o cheie de sesiune între două entități
B. Un aranjament prin care cheile criptografice sunt păstrate la o terță parte de încredere și utilizate când este necesar
C. Un test de primalitate folosit la generarea cheilor RSA
D. Accesul la date atunci când este nevoie, prin recuperarea cheii

**9. Ce sunt schemele „m din n” în contextul cheilor criptografice?**
A. Scheme prin care sunt necesari cel puțin m din n deținători pentru a accesa/recupera o cheie
B. Un mod de operare al cifrului AES
C. Sisteme speciale ce asigură integritatea și controlul accesului la chei
D. Un protocol de distribuție a cheilor publice

**10. Referitor la utilizarea și arhivarea cheilor, ce este adevărat?**
A. Memoria trebuie ștearsă după dealocarea cheilor
B. Fiecare acces la cheie trebuie autentificat
C. Cheile arhivate pot fi reutilizate liber în producție
D. Cheilor arhivate li se asociază marcaje de timp (timestamps)

**11. De ce ștergerea cheilor de pe suporturile de stocare nu este simplă?**
A. Comanda de ștergere doar marchează spațiul ca liber
B. Alinierea capului de citire/scriere pe hard disk nu este perfectă
C. Sunt necesari algoritmi speciali de „wipe” (suprascriere repetată cu valori aleatoare)
D. Cheile pot fi recuperate ușor după ștergerea standard

**12. Ce ar trebui făcut pentru a asigura protecția cheilor criptografice?**
A. Păstrarea secretă a algoritmilor de criptare folosiți
B. Alegerea unor lungimi de chei cât mai mari
C. Folosirea unor algoritmi și module criptografice validate (FIPS 140-2)
D. Conștientizarea de către utilizatori a importanței managementului corect al cheilor

**13. Care sunt câteva „best practices” în managementul cheilor?**
A. O cheie ar trebui folosită pentru un singur scop (criptare SAU semnare)
B. Cheile ar trebui schimbate cât mai des posibil
C. Cheile ar trebui stocate pe dispozitive criptografice (HSM, smart card)
D. Trebuie să existe un plan pentru tratarea cheilor compromise înainte de operare

**14. Ce algoritmi sunt recomandați pentru funcțiile hash?**
A. SHA-2 (224, 256, 384, 512 biți)
B. SHA-3 (Keccak)
C. MD5
D. DES

**15. Ce algoritmi sunt recomandați pentru semnături digitale?**
A. RSA
B. ECDSA / EdDSA
C. AES
D. Dilithium, Falcon

**16. Care sunt lungimile minime recomandate pentru a asigura protecția datelor pentru ~10 ani?**
A. 128 biți pentru algoritmi simetrici și 2048 biți pentru algoritmi cu cheie publică
B. 256 biți simetric și 4096 biți cheie publică
C. 64 biți simetric și 1024 biți cheie publică
D. 192 biți simetric și 3072 biți cheie publică

**17. Ce demonstrează spargerea DES de către EFF (1998) și distributed.net (1999)?**
A. Dificultatea unui atac de tip brute-force crește exponențial cu lungimea cheii
B. O cheie DES de 56 de biți a fost spartă în 4,5 zile
C. S-au atins viteze de ~250 miliarde de chei/secundă
D. DES este sigur pentru protecția datelor pe termen lung

---

## Tema 2 – Certificate digitale și PKI

**18. Ce servicii de securitate sunt fundamentate de criptografia cu cheie publică?**
A. Autentificare
B. Confidențialitate
C. Integritate
D. Non-repudiere

**19. Ce reprezintă un certificat digital?**
A. O legătură rezistentă la falsificare între o cheie publică și un atribut al deținătorului ei
B. O structură de date ce conține valoarea cheii publice și informații de identificare
C. Un document emis și semnat digital de o Autoritate de Certificare (CA)
D. O listă cu certificatele revocate

**20. Cine emite certificatele digitale și ce rol are?**
A. Registration Authority (RA), care semnează certificatele
B. Certification Authority (CA), o terță parte de încredere
C. CA certifică autenticitatea datelor incluse în certificat
D. CA semnează digital certificatele, asigurând integritate și autenticitate

**21. Care dintre următoarele câmpuri fac parte dintr-un certificat X.509 v3?**
A. Version și Serial Number
B. Signature Algorithm (identificat prin OID)
C. Issuer DN și Subject DN
D. Subject Public Key Info și Validity Period

**22. Referitor la numărul de serie (Serial Number) al unui certificat, ce este adevărat?**
A. Este un număr de identificare a certificatului
B. Trebuie să fie unic pentru fiecare certificat emis de un anumit CA
C. Identifică algoritmul de semnare
D. Reprezintă DN-ul subiectului

**23. Ce este un Distinguished Name (DN)?**
A. Numele distinctiv al unei entități (ex. „CN=Ion Bica, O=ABC, C=RO”)
B. Numărul de serie al certificatului
C. Identificatorul folosit pentru Issuer și Subject
D. Cheia publică a subiectului

**24. Ce sunt extensiile (Certificate Extensions) introduse în X.509 v3?**
A. Câmpuri obligatorii prezente în toate versiunile X.509
B. Câmpuri opționale folosite pentru a adăuga atribute suplimentare
C. Au formatul: tip, critic/non-critic, valoare
D. Au fost introduse în versiunea 3 a formatului de certificat

**25. Care este extensia ce definește categoriile de aplicații care pot folosi un certificat digital?**
A. Key Usage
B. Subject Public Key Info
C. Extended Key Usage
D. Policy Constraints

**26. Care dintre următoarele sunt extensii standard X.509?**
A. Authority Key Identifier / Subject Key Identifier
B. Key Usage / Extended Key Usage
C. CRL Distribution Point
D. Basic Constraints / Name Constraints / Policy Constraints

**27. Care sunt alte tipuri de certificate, în afară de X.509?**
A. Certificate SPKI – pentru autorizare
B. Certificate PGP – Web of Trust
C. Certificate de atribute – management de privilegii (PMI)
D. Certificate CRL – pentru revocare

**28. Din ce este compus un PKI?**
A. Politici și proceduri
B. Software și hardware
C. Personal
D. Doar algoritmi criptografici

**29. Care sunt componentele principale ale unui PKI?**
A. Certification Authority (CA)
B. Registration Authority (RA)
C. Repository
D. End Entities (utilizatori finali)

**30. Care este rolul unei Registration Authority (RA)?**
A. Verifică cererile de emitere a certificatelor și identitatea entităților finale
B. Arhivează cheile private de decriptare ale utilizatorilor
C. Înregistrează cheile private folosite pentru semnare
D. Emite și revocă certificate digitale

**31. Care este rolul unei Certification Authority (CA)?**
A. Emite și revocă certificate digitale
B. Stabilește relații cu alte CA pentru cross-certificare
C. Verifică identitatea entităților finale în locul RA
D. Semnează certificatele digitale

**32. La ce servește un Repository (depozit de certificate)?**
A. Distribuția certificatelor digitale
B. Distribuția CRL-urilor
C. Logarea evenimentelor legate de managementul certificatelor
D. Interfața prin care utilizatorii trimit cereri de emitere

---

## Tema 3 – Arhitecturi PKI, politici și validare

**33. Care sunt avantajele arhitecturilor PKI ierarhice?**
A. Există un singur punct de încredere – Root CA
B. Căi de certificare unidirecționale și ușor de determinat
C. Căi de certificare scurte și scalabilitate ridicată
D. Compromiterea cheii private a unei autorități nu le afectează pe celelalte

**34. Care este principalul dezavantaj al arhitecturii PKI ierarhice?**
A. Compromiterea Root CA compromite întreaga infrastructură
B. Căile de certificare sunt prea lungi
C. Validarea depinde de utilizator
D. Nu este suportată de aplicațiile PKI actuale

**35. Care sunt caracteristicile arhitecturii PKI de tip rețea (mesh)?**
A. Cross-certificare directă între autorități
B. Flexibilitate ridicată și model natural de încredere
C. Compromiterea unei CA nu le afectează pe celelalte
D. Construirea căilor de certificare este complexă și dependentă de utilizator

**36. Cum se poate determina nivelul de încredere într-un certificat emis de o CA?**
A. Pe baza extensiei Subject Key Identifier
B. Prin analiza CP și CPS
C. Prin operarea CA în conformitate cu politici și proceduri clar stabilite și audituri externe
D. Citind atributul „Trust Level” din Repository

**37. Ce diferență există între CP și CPS?**
A. CP (Certificate Policy) = ce reguli trebuie respectate
B. CPS (Certification Practice Statement) = cum sunt aplicate aceste reguli
C. CP și CPS sunt termeni identici
D. CP și CPS sunt create de Policy Management Authorities (PMA)

**38. Care standarde sunt asociate politicilor și practicilor de certificare?**
A. RFC 2527
B. RFC 3647
C. ETSI TS 101 456
D. PKCS #12

**39. Care sunt motivele pentru care un certificat trebuie revocat?**
A. Cheia privată asociată este compromisă sau pierdută
B. Utilizatorul părăsește organizația
C. Schimbarea numelui subiectului
D. Certificatul a intrat în posesia unei terțe părți

**40. Care sunt mecanismele de determinare a stării de revocare a unui certificat?**
A. CRL (Certificate Revocation List)
B. OCSP (Online Certificate Status Protocol)
C. HSM
D. PKCS #12

**41. Ce este o CRL (Certificate Revocation List)?**
A. Fișiere publicate periodic cu certificatele revocate
B. Odată descărcată, este folosită din cache-ul local până expiră
C. Acuratețea schemei depinde de perioada de publicare a CRL
D. Trebuie găsit un optim între perioada de publicare și încărcarea rețelei

**42. Ce este o Delta CRL?**
A. Conține doar certificatele revocate după publicarea CRL-ului de bază
B. Oferă o acuratețe mai mare
C. Este publicată la intervale scurte
D. Înlocuiește complet CRL-ul de bază, care nu mai este necesar

**43. Care sunt caracteristicile principale ale protocolului OCSP?**
A. Este un protocol simplu de tip cerere/răspuns
B. Degrevează clienții de procesarea complexă specifică CRL-urilor
C. Răspunsurile posibile sunt: good, revoked, unknown
D. Permite clienților să revoce certificate în caz de compromitere a cheii private

**44. Cum poate un OCSP responder să determine starea unui certificat?**
A. Interogând direct baza de date a CA
B. Procesând CRL-urile emise de CA
C. Redirecționând cererea către alt OCSP responder
D. Generând o nouă cheie privată pentru certificat

---

## Tema 4 – Interoperabilitate și implementare PKI

**45. Care sunt soluțiile pentru asigurarea interoperabilității între domenii PKI?**
A. Certificate Trust List (CTL) – ex. Windows
B. Cross-certificare între autorități
C. Bridge CA (BCA)
D. Mărirea lungimii cheilor

**46. Care sunt avantajele utilizării unui Bridge CA pentru interoperabilitatea PKI?**
A. Reducerea numărului de cross-certificări bilaterale
B. Simplificarea procesului de echivalare a politicilor de certificare (policy mapping)
C. Bridge CA devine singurul punct de încredere pentru toți utilizatorii
D. Este o metodă standardizată suportată de majoritatea aplicațiilor PKI

**47. Referitor la cross-certificare, ce este adevărat?**
A. Stabilește relații de încredere reciproce
B. Are scalabilitate limitată (necesită n(n−1)/2 relații)
C. Încrederea poate fi limitată prin name/policy/path length constraints
D. Bridge CA este ancora de încredere pentru utilizatorii finali

**48. Care sunt aspectele de luat în considerare la implementarea unui PKI organizațional?**
A. Suportul managementului și instruirea utilizatorilor
B. Managementul cheilor (HSM-uri, smart carduri)
C. Politici și proceduri (CP & CPS)
D. Interoperabilitate cu alte organizații și conformitate legală

**49. Care este ordinea corectă (orientativă) a etapelor de implementare a unui PKI?**
A. Analiză cerințe → Definire proiect → Proiectare arhitectură
B. Alegere produse → Pilot/testare → Implementare
C. Integrare & testare → Instruire personal
D. Implementare → Analiză cerințe → Proiectare arhitectură

**50. Într-un exemplu de topologie PKI, ce caracteristici are Root CA-ul?**
A. Este offline
B. Are o durată de viață de 20+ ani
C. Folosește o cheie de 4096 biți
D. Are durată de viață de 1 an și cheie de 2048 biți

**51. Care dintre următorii sunt furnizori de soluții PKI?**
A. Entrust
B. PrimeKey EJBCA
C. Microsoft Certificate Services
D. GlobalSign (Managed PKI Services)

---

## Tema 5 – Microsoft Certificate Services 2003 & HSM

**52. Cine poate recupera o cheie privată într-un CA enterprise implementat cu Microsoft Certificate Services 2003?**
A. Key Recovery Agent
B. CA Administrator
C. Backup Operator
D. Certificate Manager

**53. Cum sunt stocate cheile private ale utilizatorilor în baza de date a unui CA enterprise (Microsoft Certificate Services 2003)?**
A. Necriptate, protejate doar prin drepturi de acces și credențiale la nivel de bază de date
B. Criptate cu cheia publică a Key Recovery Agent
C. Criptate cu cheia privată a Backup Operators
D. Criptate cu cheia privată a CA

**54. Care afirmații despre Microsoft Certificate Services 2003 versiunea Enterprise sunt adevărate?**
A. Permite implementarea schemelor „m din n” pentru restaurarea cheilor private
B. Poate fi integrat cu orice server de directoare, nu doar Active Directory
C. Folosește IIS Web Server ca interfață cu utilizatorul
D. Permite definirea de noi template-uri pentru certificate

**55. Care este rolul unui HSM (Hardware Security Module)?**
A. Accelerează operațiile criptografice
B. Asigură protecția cheilor criptografice
C. Permite păstrarea secretă a algoritmilor criptografici folosiți
D. Asigură protecția calculatorului pe care rulează software-ul CA

---

## Cheia de răspunsuri

| Nr. | Răspuns(uri) corecte |
|-----|----------------------|
| 1 | A, C, D |
| 2 | A, B, C, D |
| 3 | A, B, C |
| 4 | B |
| 5 | A, B |
| 6 | A, B, D |
| 7 | A, B, C |
| 8 | B, D |
| 9 | A, C |
| 10 | A, B, D |
| 11 | A, B, C |
| 12 | B, C, D |
| 13 | A, B, C, D |
| 14 | A, B |
| 15 | A, B, D |
| 16 | A |
| 17 | A, B, C |
| 18 | A, B, C, D |
| 19 | A, B, C |
| 20 | B, C, D |
| 21 | A, B, C, D |
| 22 | A, B |
| 23 | A, C |
| 24 | B, C, D |
| 25 | C |
| 26 | A, B, C, D |
| 27 | A, B, C |
| 28 | A, B, C |
| 29 | A, B, C, D |
| 30 | A |
| 31 | A, B, D |
| 32 | A, B |
| 33 | A, B, C |
| 34 | A |
| 35 | A, B, C, D |
| 36 | B, C |
| 37 | A, B, D |
| 38 | A, B, C |
| 39 | A, B, C, D |
| 40 | A, B |
| 41 | A, B, C, D |
| 42 | A, B, C |
| 43 | A, B, C |
| 44 | A, B, C |
| 45 | A, B, C |
| 46 | A, B |
| 47 | A, B, C |
| 48 | A, B, C, D |
| 49 | A, B, C |
| 50 | A, B, C |
| 51 | A, B, C, D |
| 52 | A |
| 53 | B |
| 54 | A, C, D |
| 55 | A, B |

---

### Note / observații

- **Întrebarea 16:** conform slide-ului de Key Management (slide 15) și grilei din document, varianta corectă este 128 biți simetric / 2048 biți cheie publică pentru ~10 ani.
- **Întrebarea 46:** afirmația „Bridge CA devine singurul punct de încredere” este **falsă** – în slide-uri se precizează explicit că „Bridge CA is NOT the trust anchor for end users”.
- **Întrebarea 54:** afirmația B este falsă – Microsoft Certificate Services 2003 Enterprise se integrează cu Active Directory.
- Întrebările cu un singur răspuns corect: 4, 16, 25, 30(*), 34, 52, 53. Restul pot avea răspunsuri multiple.
