#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
 
int endsWithBmp(const char *filename) {
   // Obține lungimea numelui de fișier
   size_t len = strlen(filename);

   // Verifică dacă extensia ".bmp" este la sfârșitul numelui de fișier
   if (len >= 4 && strcmp(filename + len - 4, ".bmp") == 0) {
         return 1; // Fișierul se termină cu ".bmp"
  } else {
         return 0; // Fișierul nu se termină cu ".bmp"
  }
}

void process_file(const char *filename, const char *output_file) {
    struct stat file_stat;
    if (lstat(filename, &file_stat) == -1) {
        perror("Eroare obtinere informatii fisier");
        return;
    }
 
    char *username = getlogin();
    if (username == NULL) {
        perror("Eroare obtinere nume utilizator");
        return;
    }
 
    char modification_time_str[20];
    struct tm *tm_info = localtime(&file_stat.st_mtime);
    if (tm_info == NULL) {
        perror("Eroare obtinere timp modificare");
        return;
    }
 
    strftime(modification_time_str, sizeof(modification_time_str), "%d.%m.%Y", tm_info);
 
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
 
    int stat_file = open(output_file, O_WRONLY | O_APPEND);
    if (stat_file == -1) {
        perror("Eroare deschidere fisier de statistici");
        return;
    }
 
    char stats[1024];
    if (S_ISLNK(file_stat.st_mode)) {
        char target[1024];
        ssize_t len = readlink(filename, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            sprintf(stats, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier target: %ld\ndrepturi de acces user legatura: %s\ndrepturi de acces grup legatura: %s\ndrepturi de acces altii legatura: %s\n", filename, file_stat.st_size, file_stat.st_blocks, permissions_user, permissions_group, permissions_other);
        }
    } else if (S_ISREG(file_stat.st_mode)) {
      //Verificam daca e un fisier BMP
      if(endsWithBmp(filename)){
      	//Deschiderea fișierului BMP în mod de citire binară
      	int bmp_file = open(filename, O_RDONLY);
      	if (bmp_file == -1) {
          perror("Eroare deschidere fisier");
       }	
      	// Setăm poziția curentă la începutul header-ului BMP
      	if (lseek(bmp_file, 14, SEEK_SET) == -1) {
          perror("Eroare la setarea poziției curente");
          close(bmp_file);
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
       }

       sprintf(stats, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, info_header.height, info_header.width, info_header.size, username, modification_time_str, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
      }
      else{
      	 sprintf(stats, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_size, username, modification_time_str, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
      }
     
    } else if (S_ISDIR(file_stat.st_mode)) {
        sprintf(stats, "nume director: %s\nidentificatorul utilizatorului: %s\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, username, permissions_user, permissions_group, permissions_other);
    } else {
        // Do nothing for other types
    }
 
    if (write(stat_file, stats, strlen(stats)) == -1) {
        perror("Eroare scriere in fisier de statistici");
    }
 
    close(stat_file);
}
 
void process_directory(const char *dirname, const char *output_file) {
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        perror("Eroare deschidere director");
        return;
    }
 
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char path[1024];
            sprintf(path, "%s/%s", dirname, entry->d_name);
            process_file(path, output_file);
        }
    }
 
    closedir(dir);
}

int main(int argc, char *argv[]) {
    // Verificare dacă programul a primit un singur argument (numele directorului)
    if (argc != 2) {
        printf("Usage: %s <director_intrare>\n", argv[0]);
        return 1;
    }
 
    // Crearea fisierului de statistici
    int stat_file = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stat_file == -1) {
        perror("Eroare creare fisier de statistici");
        return 1;
    }
    close(stat_file);
 
    // Procesarea directorului
    process_directory(argv[1], "statistica.txt");
 
    return 0;
}
 
