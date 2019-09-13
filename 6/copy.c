#include<stdio.h>
#include<pthread.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<stdlib.h>

int is_root = 1;
pthread_mutex_t mutex;

struct Param {
	char* path_src;
	char* path_dst;
};

mode_t permissions(struct stat status) {
	/*int chmod_decimal = status.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
	int octal = 0, tmp = 1;
	while(chmod_decimal != 0) {
		octal = octal + (chmod_decimal%8)*tmp;
		chmod_decimal = chmod_decimal / 8;
		tmp*= 10;
	}
	*/

//	printf("file permision is : %d\n", octal);
	return (status.st_mode&(S_IRWXU | S_IRWXG | S_IRWXO));
}

void* copy_file(void* args) {

	char* file_path;
	struct Param param = *((struct Param*)args);
	printf("try to copy file : %s to %s\n", param.path_src, param.path_dst);
	int src, dst;
	struct stat statbuf;

        if(stat(param.path_src, &statbuf) == -1) {
                	 perror("error statbuf\n");
			 return 1;
	}

	src = open(param.path_src, O_RDONLY);
	dst = open(param.path_dst, O_WRONLY | O_APPEND | O_CREAT | O_EXCL, permissions(statbuf));
	if(src == -1 || dst == -1)
		return 1;
	int res;
	char data[128];

	while((res = read(src, &data, 127)) > 0) 
		write(dst, &data, res);

	close(src);
	close(dst);
	return 0;
}


void* copy_dir(void *args) {
        pthread_t* threads;
	threads = (pthread_t*)malloc(sizeof(pthread_t));
	int threads_count = 0, i, status, status_addr;
	struct Param param = *((struct Param*)args);
	printf("try to copy directory %s to %s\n", param.path_src, param.path_dst);
	DIR* dir = opendir(param.path_src);
	struct dirent* buf;
	struct dirent* de;
	long buf_size = sizeof(struct dirent) + pathconf(param.path_src, _PC_NAME_MAX) + 1;
	buf = (struct dirent*)malloc(buf_size);
	long result = 0;
	int iter = 0;
	printf("dir opened\n");

//	struct Param* recurs_param;
//	int recurs_param_size = 0;
//	recurs_param = (struct Param*)malloc(sizeof(struct Param));

	if(dir != NULL && buf_size > 0) {
		printf("try readdir_r\n");
		while(result == 0) {
			result = readdir_r(dir, buf, &de);
			if(!de)
				break;
		//	if(result!= 0 && errno == EMFILE) {
		//		sleep(2);
		//		continue;
		//	}

		//	if(result == 0) {
				printf("readdir_r succeded ---path_src(%s)%d--d_name(%s)-%d\n", param.path_src, strlen(param.path_src), de->d_name, strlen(de->d_name));
				int elem_size = strlen(param.path_src) + 2 + strlen(de->d_name);
				char elem_path[elem_size];

				if(iter < 2) {
					iter++;
					continue;
				}

				sprintf(elem_path, "%s/%s", param.path_src, de->d_name);
				printf("got elem path: %s\n", elem_path);
				struct stat statbuf;

				if(stat(elem_path, &statbuf) == -1) {
					perror("error statbuf\n");
					continue;
				}



	//			recurs_param_size++;
	//			recurs_param = (struct Param*)realloc(recurs_param, recurs_param_size);


				printf("new path is: %s\n", elem_path);
				if(statbuf.st_mode & S_IFDIR) {
					printf("create thread for /dir %s\n", elem_path);
					threads_count++;
                                        threads = (pthread_t*)realloc(threads, threads_count);
					int directory_dst_count = strlen(param.path_dst) + 2 + strlen(de->d_name);

				//	recurs_param[recurs_param_size - 1].path_src = (char*)malloc(sizeof(char)*(strlen(elem_path)+1));
                                  //      recurs_param[recurs_param_size - 1].path_dst = (char*)malloc(sizeof(char)*(directory_dst_count+1));
					char pass[2][257];
					struct Param recurs_param;
					recurs_param.path_src = pass[0];
					recurs_param.path_dst = pass[1];
					sprintf(recurs_param.path_src, "%s", elem_path);
					sprintf(recurs_param.path_dst, "%s/%s", param.path_dst, de->d_name);

					mkdir(recurs_param.path_dst, permissions(statbuf));

					printf("chectk dir : %s %s \n", recurs_param.path_src, recurs_param.path_dst);
					status = pthread_create(&(threads[threads_count-1]), NULL, copy_dir, (void*)(&recurs_param));
                                        if(status != 0)
                                                fprintf(stderr, "Error creating file copy thread\n");
				}
				if(statbuf.st_mode & S_IFREG) {
					printf("create thread for /file %s and threads_count=%d\n", elem_path, threads_count);
					threads_count++;
					threads = (pthread_t*)realloc(threads, threads_count);

					printf("thread realloced\n");
					int file_dst_count = strlen(param.path_dst) + 2 + strlen(de->d_name); 

				//	recurs_param[recurs_param_size - 1].path_src = (char*)malloc(sizeof(char)*(strlen(elem_path)+1));
				//	recurs_param[recurs_param_size - 1].path_dst = (char*)malloc(sizeof(char)*(file_dst_count+1));
                                        char pass[2][257];
					struct Param recurs_param;
                                        recurs_param.path_src = pass[0];
                                        recurs_param.path_dst = pass[1];

					sprintf(recurs_param.path_src, "%s", elem_path);
                                        sprintf(recurs_param.path_dst, "%s/%s", param.path_dst, de->d_name);

					status = pthread_create(&(threads[threads_count-1]), NULL, copy_file, (void*)(&recurs_param));
				        if(status != 0)
                				fprintf(stderr, "Error creating file copy thread\n");
				}

		//	}

		}
		for(i = 0; i<threads_count; i++) {
			 status = pthread_join(threads[i], (void**)&status_addr);
		         if(status != 0)
                		fprintf(stderr, "error joining thread\n");
        	}
 
		printf("clear threads in %s\n", param.path_src);
		free(threads);
/*
		for(i = 0; i<recurs_param_size; i++) {
			printf("clear src and dst in %s ---%d\n", recurs_param[i].path_src, recurs_param_size);
			free(recurs_param[i].path_src);
			recurs_param[i].path_src = NULL;
			printf("clear src and dst in %s\n", recurs_param[i].path_dst);
			free(recurs_param[i].path_dst);
			recurs_param[i].path_dst = NULL;
		}
*/

//		printf("clear recurs_param in %s\n", param.path_src);
//		free(recurs_param);
//		recurs_param = NULL;
	}
	return 0;
}


int main(int argc, char* argv[]) {
	if(argc < 3) {
		perror("not enougt args\n");
		pthread_exit((void*)0);
	}

	pthread_mutex_init(&mutex, NULL);
	struct Param param;
//	param.path_src = (char*)malloc(sizeof(char)*(strlen(argv[1])+1));
//	param.path_dst = (char*)malloc(sizeof(char)*(strlen(argv[2])+1));
	char pass[2][257];
	param.path_src = pass[0];
	param.path_dst = pass[1];
	sprintf(param.path_src, "%s", argv[1]);
	sprintf(param.path_dst, "%s", argv[2]);
	copy_dir(&param);
        return 0;
}

