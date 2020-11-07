#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "so_stdio.h"
#define BUF_LEN 4096

int contorRead = -1, contorWrite = -1, lastWrite = 0, flagFirstRead = 0;
int numReadBuflen = 0, cursor = 0, flagError = 0, flagAppend = 0, flagfeof = 0;
int numReadBuflenSum = 0, contorReadTotalSum = 0, Minus_1_instead_So_eof = 0;

typedef struct _so_file {
	char *buffer;
	int fd;
} SO_FILE;

/* Functia care deschide fisierul */
SO_FILE *so_fopen(const char *pathname, const char *mode)
{
	if (strcmp(mode, "r") == 0) {
		if (open(pathname, O_RDONLY) != -1) {
			SO_FILE *file = malloc(sizeof(SO_FILE));

			file->fd = open(pathname, O_RDONLY);
			file->buffer = malloc(BUF_LEN * sizeof(char));
			return file;
		} else
			return NULL;
	} else if (strcmp(mode, "r+") == 0) {
		if (open(pathname, O_RDWR) != -1) {
			SO_FILE *file = malloc(sizeof(SO_FILE));

			file->fd = open(pathname, O_RDWR);
			file->buffer = malloc(BUF_LEN * sizeof(char));
			return file;
		} else
			return NULL;
	} else if (strcmp(mode, "w") == 0) {
		SO_FILE *file = malloc(sizeof(SO_FILE));

		file->fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		file->buffer = malloc(BUF_LEN * sizeof(char));
		return file;
	} else if (strcmp(mode, "w+") == 0) {
		SO_FILE *file = malloc(sizeof(SO_FILE));

		file->fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
		file->buffer = malloc(BUF_LEN * sizeof(char));
		return file;
	} else if (strcmp(mode, "a") == 0) {
		SO_FILE *file = malloc(sizeof(SO_FILE));

		file->fd = open(pathname, O_APPEND | O_CREAT, 0644);
		file->buffer = malloc(BUF_LEN * sizeof(char));
		flagAppend = 1;
		return file;
	} else if (strcmp(mode, "a+") == 0) {
		SO_FILE *file = malloc(sizeof(SO_FILE));

		file->fd = open(pathname, O_APPEND | O_CREAT, 0644);
		file->buffer = malloc(BUF_LEN * sizeof(char));
		return file;
	} else
		return NULL;
}

/* Functia care inchide fisierul */
int so_fclose(SO_FILE *stream)
{
	if (so_fflush(stream) == SO_EOF) {
		free(stream);
		return SO_EOF;
	}
	if (close(stream->fd) == -1 && so_ferror(stream) != 1) {
		free(stream->buffer);
		free(stream);
		return SO_EOF;
	}
	free(stream->buffer);
	free(stream);
	return 0;
}

/* Functia care returneaza descriptorul de fisier */
int so_fileno(SO_FILE *stream)
{
	return stream->fd;
}


int so_fflush(SO_FILE *stream)
{
	int aux;

	/* Daca mai sunt date in buffer care inca nu au fost scrise */
	/* in fisier le scriem */
	if ((flagFirstRead == 0 || contorWrite % BUF_LEN != 0)
		&& lastWrite == 1) {
		aux = write(stream->fd, stream->buffer,
			contorWrite % BUF_LEN + 1);
		if (aux == -1) {
			free(stream->buffer);
			return SO_EOF;
		}
	}
	return 0;
}

/* Se pozitioneaza cursorul in functie de offset si whence */
int so_fseek(SO_FILE *stream, long offset, int whence)
{
	so_fflush(stream);
	if (whence == SEEK_SET) {
		cursor = (int) offset;
	} else if (whence == SEEK_CUR) {
		cursor = cursor + (int) offset;
	} else if (whence == SEEK_END) {
		int dejainfisier = 0;
		char *aux = malloc(sizeof(char));

		/* Pozitionam cursorul in functie de cate caractere */
		/* se aflau deja in fisier si offset */
		while (read(stream->fd, aux, 1) == 1)
			dejainfisier++;
		cursor = dejainfisier + (int) offset;
		free(aux);
	} else
		return -1;
	if (cursor < 0)
		return -1;
	return 0;
}

/* Functia care returneaza pozitia cursorului */
long so_ftell(SO_FILE *stream)
{
	if (cursor < 0 || flagError == 1)
		return -1;
	return cursor;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	int aux[1] = {0}, i = 0;
	char auxchar[1] = "0";

	/* La fiecare iteratie se verifica daca so_fgetc returneaza */
	/* SO_EOF (nu -1 corect), caz in care se iese din for */
	for (i = 0; i < nmemb * size; i++) {
		aux[0] = so_fgetc(stream);
		if (aux[0] == SO_EOF && Minus_1_instead_So_eof == 0)
			break;
		if (aux[0] == SO_EOF && Minus_1_instead_So_eof == 1)
			Minus_1_instead_So_eof = 0;
		auxchar[0] = (char) aux[0];
		memcpy((char *) (ptr + i), auxchar, 1);
	}
	lastWrite = 0;
	return i;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
	char auxchar[1];
	int i = 0, auxiliar;

	/* La fiecare iteratie se verifica daca so_fputc returneaza */
	/* SO_EOF (nu -1 corect), caz in care se iese din for */
	for (i = 0; i < nmemb * size; i++) {
		memcpy(auxchar, (char *) (ptr + i), 1);
		auxiliar = so_fputc(auxchar[0], stream);
		if (auxiliar == SO_EOF && Minus_1_instead_So_eof == 0)
			break;
		if (auxiliar == SO_EOF && Minus_1_instead_So_eof == 1)
			Minus_1_instead_So_eof = 0;
	}
	lastWrite = 1;
	if (flagAppend == 1)
		so_fseek(stream, 0, SEEK_CUR);
	return i;
}

int so_fgetc(SO_FILE *stream)
{
	/* Se citesc folosind read cate buf_len caractere; */
	/* contorRead -> cate caractere au fost citite; se reseteaza cand */
	/* ajunge la buf_len; */
	/* contoReadTotalSum -> suma contorRead-urilor efectuata la resetarea */
	/* fiecaruia dintre acestea */
	contorRead++;
	cursor++;
	contorReadTotalSum++;
	if (flagfeof == 1)
		return SO_EOF;
	if (contorRead % BUF_LEN == 0) {
		contorRead = 0;
		numReadBuflen = read(stream->fd, stream->buffer, BUF_LEN);
		flagFirstRead = 1;
		if (numReadBuflen == -1) {
			flagError = 1;
			return SO_EOF;
		}
		numReadBuflenSum = numReadBuflenSum + numReadBuflen;
	}

	/* Daca numarul de apeluri fgetc - numarul de apeluri fputc */
	/* este mai mare decat buf_len curent inseamna ca am ajuns  */
	/* la finalul fisierului */
	if (numReadBuflenSum % BUF_LEN != 0 &&
		contorReadTotalSum > numReadBuflenSum) {
		read(stream->fd, stream->buffer, BUF_LEN);
		flagfeof = -1;
		return SO_EOF;
	}

	if (contorRead >= numReadBuflenSum) {
		flagfeof = -1;
		return SO_EOF;
	}

	lastWrite = 0;
	if ((int) stream->buffer[contorRead % BUF_LEN] == -1)
		Minus_1_instead_So_eof = 1;
	return (int) stream->buffer[contorRead % BUF_LEN];
}


int so_fputc(int c, SO_FILE *stream)
{
	contorWrite++;
	cursor++;
	int aux;

	/* Se efectueaza write cand in buffer sunt un numar de elemente */
	/* divizibil cu buf_len si mai mare sau egal decat buf_len */
	stream->buffer[contorWrite % BUF_LEN] = c;
	if (((contorWrite + 1) % BUF_LEN == 0) && contorWrite > 0) {
		aux = write(stream->fd, stream->buffer, BUF_LEN);
		if (aux == -1) {
			flagError = 1;
			return SO_EOF;
		}
	}
	lastWrite = 1;
	if ((int) stream->buffer[contorWrite % BUF_LEN] == -1)
		Minus_1_instead_So_eof = 1;
	return stream->buffer[contorWrite % BUF_LEN];
}

/* Functia care verifica daca s-a ajuns la finalul fisierului */
int so_feof(SO_FILE *stream)
{
	if (flagfeof == -1) {
		flagfeof = 1;
		return 1;
	}
	return 0;
}

/* Functia care verifica daca s-a petrecut vreo eroare pe parcurs */
int so_ferror(SO_FILE *stream)
{
	if (flagError == 1)
		return 1;
	return 0;
}

SO_FILE *so_popen(const char *command, const char *type)
{
	return NULL;
}

int so_pclose(SO_FILE *stream)
{
	return 0;
}
