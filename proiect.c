#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
 
int main(int argc, char *argv[]) {
    // Verificare dacă programul a primit un singur argument (numele fișierului BMP)
    if (argc != 2) {
        printf("Usage: %s <fisier_intrare>\n", argv[0]);
        return 1;
    }
 
    // Deschiderea fișierului BMP în mod de citire binară
    int bmp_file = open(argv[1], O_RDONLY);
    if (bmp_file == -1) {
        perror("Eroare deschidere fisier");
        return 1;
    }

    // Setăm poziția curentă la începutul header-ului BMP
    if (lseek(bmp_file, 14, SEEK_SET) == -1) {
        perror("Eroare la setarea poziției curente");
        close(bmp_file);
        return 1;
    }
 
    // Citim inalțimea și lățimea imaginii BMP
    struct bmp_info_header {
      int size;
      int width;
      int height;
    } info_header;
 
    if (read(bmp_file, &info_header, sizeof(struct bmp_info_header)) != sizeof(struct bmp_info_header)) {
        perror("Eroare la citirea inaltimei si latimii BMP");
        close(bmp_file);
        return 1;
    }
 
    // Obținerea informațiilor despre fișier folosind structura stat
    struct stat file_stat;
    if (fstat(bmp_file, &file_stat) == -1) {
        perror("Eroare obtinere informatii fisier");
        close(bmp_file);
        return 1;
    }
 
 
    // Obținerea username-ului
    char *username = getlogin();
    if (username == NULL) {
        perror("Eroare obtinere nume utilizator");
        close(bmp_file);
        return 1;
    }
 
    // Ultima modificare
    char modification_time_str[20]; // Buffer pentru șirul de caractere
    struct tm *tm_info = localtime(&file_stat.st_mtime);
    if (tm_info == NULL) {
        perror("Eroare obtinere timp modificare");
        close(bmp_file);
        return 1;
    }
 
    strftime(modification_time_str, sizeof(modification_time_str), "%d.%m.%Y", tm_info);
 
    // Drepturile fiecărui utilizator
    char permissions_user[4];
    char permissions_group[4];
    char permissions_other[4];
    sprintf(permissions_user, "%c%c%c",
            (file_stat.st_mode & S_IRUSR) ? 'R' : '-',
            (file_stat.st_mode & S_IWUSR) ? 'W' : '-',
            (file_stat.st_mode & S_IXUSR) ? 'X' : '-');
 
    sprintf(permissions_group, "%c%c%c",
            (file_stat.st_mode & S_IRGRP) ? 'R' : '-',
            (file_stat.st_mode & S_IWGRP) ? 'W' : '-',
            (file_stat.st_mode & S_IXGRP) ? 'X' : '-');
 
    sprintf(permissions_other, "%c%c%c",
            (file_stat.st_mode & S_IROTH) ? 'R' : '-',
            (file_stat.st_mode & S_IWOTH) ? 'W' : '-',
            (file_stat.st_mode & S_IXOTH) ? 'X' : '-');
 
    // Crearea fisierului si scrierea in fisier
    int stat_file = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stat_file == -1) {
        perror("Eroare creare fisier de statistici");
        close(bmp_file);
        return 1;
    }

 
    char stats[1024];
    sprintf(stats, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", argv[1], info_header.height, info_header.width, info_header.size, username, modification_time_str, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
 
    if (write(stat_file, stats, strlen(stats)) == -1) {
        perror("Eroare scriere in fisier de statistici");
        close(bmp_file);
        close(stat_file);
        return 1;
    }
 
    // Închiderea fișierelor
    close(bmp_file);
    close(stat_file);
 
    return 0;
}
