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
#include <sys/wait.h>
 
 
struct bmp_info_header {
    int size;
    int width;
    int height;
};
 
int endsWithBmp(const char *filename) {
    size_t len = strlen(filename);
    return len >= 4 && strcmp(filename + len - 4, ".bmp") == 0;
}
 
void process_file(const char *filename, const char *output_dir, struct dirent *entry) {
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
 
    char stats[1024];
    if (S_ISLNK(file_stat.st_mode)) {
        char target[1024];
        ssize_t len = readlink(filename, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            sprintf(stats, "nume legatura: %s\ndimensiune legatura: %ld\ndimensiune fisier target: %ld\ndrepturi de acces user legatura: %s\ndrepturi de acces grup legatura: %s\ndrepturi de acces altii legatura: %s\n", filename, file_stat.st_size, file_stat.st_blocks, permissions_user, permissions_group, permissions_other);
        }
    } else if (S_ISREG(file_stat.st_mode)) {
        if (endsWithBmp(filename)) {
            pid_t pid = fork();
 
            if (pid == -1) {
                perror("Eroare la crearea procesului pentru imaginea BMP");
            } else if (pid == 0) { // Proces fiu pentru imaginea BMP
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
 
                // Citim înălțimea și lățimea imaginii BMP
		struct bmp_info_header info_header;
                if (read(bmp_file, &info_header, sizeof(struct bmp_info_header)) != sizeof(struct bmp_info_header)) {
                perror("Eroare la citirea înălțimii și lățimii BMP");
                close(bmp_file);
                return;
		}
 
		 sprintf(stats, "nume fisier: %s\ndimensiune: %u\ninaltime: %u\nlungime: %u\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, info_header.size, info_header.height, info_header.width, username, modification_time_str, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
		 
                // Calculăm numărul total de pixeli
                int num_pixels = info_header.width * info_header.height;
 
 
                // Citim și procesăm pixelii imaginii BMP
                for (int i = 0; i < num_pixels; i++) {
                    unsigned char pixel[3];
                    if (read(bmp_file, pixel, sizeof(pixel)) != sizeof(pixel)) {
                        perror("Eroare la citirea pixelilor BMP");
                        close(bmp_file);
                        exit(EXIT_FAILURE);
                    }
 
                    // Calculăm intensitatea tonului de gri
                    unsigned char P_gri = 0.299 * pixel[2] + 0.587 * pixel[1] + 0.114 * pixel[0];
 
                    // Suprascriem cele 3 valori cu valoarea P_gri
                    memset(pixel, P_gri, sizeof(pixel));
 
                    // Revenim la poziția curentă a pixelului în fișier
                    if (lseek(bmp_file, -sizeof(pixel), SEEK_CUR) == -1) {
                        perror("Eroare la revenirea la poziția curentă");
                        close(bmp_file);
                        exit(EXIT_FAILURE);
                    }
 
                    // Scriem noile valori în fișier
                    if (write(bmp_file, pixel, sizeof(pixel)) != sizeof(pixel)) {
                        perror("Eroare la scrierea pixelilor BMP");
                        close(bmp_file);
                        exit(EXIT_FAILURE);
                    }
                }
 
 
              
            } else { 
               int status;
                waitpid(pid, &status, 0);
                printf("S-a încheiat procesul pentru imaginea BMP cu pid-ul %d și codul %d\n", pid, WEXITSTATUS(status));
            }
        }else{
	  // Restul fișierelor
	  sprintf(stats, "nume fisier: %s\ndimensiune: %ld\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s\ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_size, username, modification_time_str, file_stat.st_nlink, permissions_user, permissions_group, permissions_other);
	}
 
    } else if (S_ISDIR(file_stat.st_mode)) {
        sprintf(stats, "nume director: %s\nidentificatorul utilizatorului: %s\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, username, permissions_user, permissions_group, permissions_other);
    } else {
        // Alte tipuri de fișiere
        sprintf(stats, "nume necunoscut: %s\ndimensiune: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n", filename, file_stat.st_size, permissions_user, permissions_group, permissions_other);
    }
 
    // Scriem în fișierul de statistici corespunzător
    char output_file[1024];
    sprintf(output_file, "%s/%s_statistica.txt", output_dir, entry->d_name);
    int stat_file = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stat_file == -1) {
        perror("Eroare deschidere fisier de statistici");
        return;
    }
 
    if (write(stat_file, stats, strlen(stats)) == -1) {
        perror("Eroare scriere in fisier de statistici");
    }
 
    close(stat_file);
}
 
void process_directory(const char *dirname, const char *output_dir) {
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
            process_file(path, output_dir, entry);
        }
    }
 
    closedir(dir);
}
 
int main(int argc, char *argv[]) {
    // Verificare dacă programul a primit două argumente
    if (argc != 3) {
        printf("Usage: %s <director_intrare> <director_iesire>\n", argv[0]);
        return 1;
    }
 
    // Crearea directorului de ieșire
    if (mkdir(argv[2], 0777) == -1) {
        perror("Eroare creare director de iesire");
        return 1;
    }
 
    // Procesarea directorului
    process_directory(argv[1], argv[2]);
 
    return 0;
}
 

 
