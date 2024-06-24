
Structura și designul proiectului

Multithreading: Broker-ul folosește o arhitectură multithreading pentru a gestiona mai mulți clienți simultan. Fiecare conexiune de client TCP este gestionată de un thread separat.

Gestionarea topicurilor: Broker-ul menține o listă de topicuri și abonații corespunzători. Clienții se pot abona și dezabona de la topicuri în mod dinamic.

Gestionarea mesajelor:

TCP: Mesajele primite de la clienții TCP sunt analizate pentru comenzi (abonare, dezabonare, ieșire) sau tratate ca date care urmează să fie publicate pe un anumit topic.
UDP: Pachetele UDP primite sunt analizate pentru a extrage numele topicului, tipul de date și conținutul. Datele sunt apoi publicate către abonații topicului specificat.
Suport pentru tipuri de date: Brokerul acceptă următoarele tipuri de date:

INT (număr întreg)
SHORT_REAL (număr real scurt)
FLOAT (număr real)
STRING (șir de caractere)

Socket UDP: Codul creează un socket UDP și îl atașează (bind) la un port specificat. UDP (User Datagram Protocol) este ideal pentru situațiile în care viteza are prioritate față de fiabilitate, făcându-l potrivit pentru mesajele transmise rapid.

Multithreading (TCP): Conexiunile TCP sunt gestionate în paralel folosind thread-uri. Fiecare nouă conexiune de client TCP generează un thread separat (client_handler) pentru a se ocupa de comunicațiile aferente. Aceasta previne blocarea serverului de către un singur client.

Funcția select(): Funcția select() este mecanismul cheie pentru gestionarea eficientă a conexiunilor TCP și UDP. Monitorizează descriptori de fișiere multipli (socket-uri) și indică atunci cand oricare dintre aceștia este pregătit pentru citire sau are erori.

UDP Non-Blocant: UDP este în mod inerent non-blocant. Apelul recvfrom returnează imediat orice date sunt disponibile în bufferul socket-ului. Dacă nu sunt date disponibile, nu se blochează, ci returnează o eroare.

Cum gestionează mesajele rapide

Ascultarea Activității: Serverul ascultă continuu atât pe socket-ul TCP, cât și pe socket-ul UDP folosind funcția select(). Acest lucru face ca procesul să fie reactiv la activitatea de pe oricare tip de conexiune.

Apelul recvfrom() preia imediat datagrama din bufferul de recepție al socket-ului.
Datagrama este transmisă funcției process_message()

IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
Am gasit solutie sa inchid si server si clientii la ideea de a tasta "exit" in partea de server si manual functioneaza. Solutia era asta :
 if (FD_ISSET(STDIN_FILENO, &readfds)) 
        {
            char command[5];                        
            fgets(command, sizeof(command), stdin); 

            command[strcspn(command, "\n")] = 0; 

            if (strcmp(command, "exit") == 0) 
            {
                break;
            }
        }
Dar checkerul nu mai functiona , din nou merita mentionat ca manual functiona.



