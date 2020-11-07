# Stdio-Library
Abordare generala:
-----------------
In functia so_fflush am verificat daca in buffer au mai ramas date care
nu au fost scrise in fisier, caz in care le vom scrie.
In functia so_fseek am pozitionat cursorul in functie de offset si whence,
iar in cazul in care trebuia pozitionat fisierul in functie de finalul
fisierului am numarat cate caractere se gaseau deja in fisier.
In functia so_fread se apeleaza functia so_fgetc de size * nmemb ori si
la fiecare iteratie se verifica daca so_fgetc returneaza SO_EOF (nu -1
corect), caz in care se iese din for.
In functia so_fwrite, asemanator cu so_read se apeleaza functia so_fputc de
size * nmemb ori si la fiecare iteratie se verifica daca so_fputc returneaza
SO_EOF (nu -1 corect), caz in care se iese din for.
In functia so_fgetc se citesc folosind read cate buf_len caractere si se
seteaza pe 1 flag-ul de feof daca numarul de apeluri fgetc - numarul de
apeluri fputc este mai mare decat buf_len curent.
In functia so_fread se efectueaza write cand in buffer sunt un
numar de elemente divizibil cu buf_len si mai mare sau egal decat buf_len.

Dupa parerea mea, tema este utila intrucat arata modul efectiv de
implementare al functilor din biblioteca stdio.

Cred ca implementarea este medie, se putea mai bine.

Am intampinat dificultati deoarece la un moment dat faceam niste salturi
conditionate care se bazau pe variabile neinitializate si aveam memcheck
failed si initial nu mi-am dat seama de posibilitatea ca fread sau fwrite
sa trateze -1 corect ca SO_EOF (returnate de so_fgetc sau fo_putc).

Pentru build folosim gcc si compilam biblioteca dinamica libso_stdio.so.

Am folosit laboratorul 1 de SO.
