/*
 * filtrar: filtra por contenido los archivos del directorio indicado.
 *
 * Copyright (c) 2013,2017 Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización 
 * de este proyecto o de cualquier material derivado de este, 
 * incluyendo la solución particular que desarrolle el alumno.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <dlfcn.h>      /* defines dlopen(), etc.       */

#include <sys/signal.h> // alarmas/signal

#include "filtrar.h"

#define INPUT_END 1
#define OUTPUT_END 0



/* ---------------- PROTOTIPOS ----------------- */ 
/* Esta funcion monta el filtro indicado y busca el simbolo "tratar"
   que debe contener, y aplica dicha funcion "tratar()" para filtrar
   toda la informacion que le llega por su entrada estandar antes
   de enviarla hacia su salida estandar. */
extern void filtrar_con_filtro(char* nombre_filtro);

/* Esta funcion lanza todos los procesos necesarios para ejecutar los filtros.
   Dichos procesos tendran que tener redirigida su entrada y su salida. */
void preparar_filtros(void);

/* Esta funcion recorrera el directorio pasado como argumento y por cada entrada
   que no sea un directorio o cuyo nombre comience por un punto '.' la lee y 
   la escribe por la salida estandar (que seria redirigida al primero de los 
   filtros, si existe). */
void recorrer_directorio(char* nombre_dir);

/* Esta funcion recorre los procesos arrancados para ejecutar los filtros, 
   esperando a su terminacion y recogiendo su estado de terminacion. */ 
void esperar_terminacion(void);

/* Desarrolle una funcion que permita controlar la temporizacion de la ejecucion
   de los filtros. */ 
extern void preparar_alarma(void);

//Codigo que mata los hijos del proceso
void alarm_handler(void);



/* ---------------- IMPLEMENTACIONES ----------------- */ 
char** filtros;   /* Lista de nombres de los filtros a aplicar */
int    n_filtros; /* Tama~no de dicha lista */
pid_t* pids;      /* Lista de los PIDs de los procesos que ejecutan los filtros */



/* Funcion principal */
int main(int argc, char* argv[])
{
	
	/* Chequeo de argumentos */
	if(argc<2) 
	{
		/* Invocacion sin argumentos  o con un numero de argumentos insuficiente */
		fprintf(stderr,"Uso: %s directorio [filtro...]\n",argv[0]);
		exit(1);
	}

	filtros = &(argv[2]);                             /* Lista de filtros a aplicar */
	n_filtros = argc-2;                               /* Numero de filtros a usar */
	pids = (pid_t*)malloc(sizeof(pid_t)*n_filtros);   /* Lista de pids */

	preparar_alarma();

	//Preparamos los filtros si los hay
	if(argc>2) preparar_filtros();

	recorrer_directorio(argv[1]);

	//sleep(1);//esperamos a que todos los hijos terminen

	esperar_terminacion();

	return 0;
}

extern void filtrar_con_filtro(char* nombre_filtro)
{
	
	void* lib_handle;
	/* first define a function pointer variable to hold the function's address */
	int (*tratar)(const char* buff_in,const char* buff_out,int tam);
	/* then define a pointer to a possible error string */
	const char* error_msg;
	int res=0;
	
	char buff_in[4096]; 
	char buff_out[4096]; 
	int tam=4096;

	lib_handle = dlopen(nombre_filtro, RTLD_LAZY);
	if (!lib_handle) {
    	fprintf(stderr, "Error al abrir la biblioteca '%s'\n", nombre_filtro);
    	exit(1);
		}
			
	/* now locate the 'tratar' function in the library */
	tratar = dlsym(lib_handle, "tratar");
	// *(void **) (&tratar) = dlsym(lib_handle, "tratar");
	/* check that no error occured */
	error_msg = dlerror();
		if (error_msg) {
    	fprintf(stderr, "Error al buscar el simbolo '%s' en '%s'\n","tratar", nombre_filtro);
    	exit(1);
		}
		
	int cantidadleida;
	while((cantidadleida=read(STDIN_FILENO,buff_in,tam))>0){
	res=tratar(buff_in,buff_out,cantidadleida);
	write(STDOUT_FILENO,buff_out,res);
	}


	dlclose(lib_handle);
	
}

void recorrer_directorio(char* nombre_dir)   
{
	DIR* dir =NULL;
	struct dirent* ent;
	char fich[1024];
	char buff[4096];
	int fd;
	int status;
	struct stat st_buf;


	/* Abrir el directorio */
	dir=opendir(nombre_dir);
	/* Tratamiento del error. */
	
	if(dir==NULL){
		fprintf(stderr,"Error al abrir el directorio '%s'\n '%s'\n",nombre_dir,strerror(errno));
		exit(1);
	}

	
	
	errno=0;
	
	/* Recorremos las entradas del directorio */
	while((ent=readdir(dir))!=NULL)
	{	
		/* Nos saltamos las que comienzan por un punto "." */
		if(ent->d_name[0]=='.'){
			errno=0;
			continue;
		}
		/* fich debe contener la ruta completa al fichero */
		strcpy(fich, nombre_dir);
		strcat(fich,"/");
        strcat(fich, ent->d_name);
		

		/* Nos saltamos las rutas que sean directorios. */
		status=stat(fich,&st_buf);
		if(status!=0){
			fprintf(stderr,"AVISO: No se puede stat el fichero '%s'!\n",fich);
			errno=0;
			continue;
		}
		if(S_ISDIR (st_buf.st_mode)){
			errno=0;
		    continue;		
		}

		
		
		/* Abrir el archivo. */
		fd=open(fich,O_RDONLY);

		/* Tratamiento del error. */
		if(fd==-1){
			fprintf(stderr,"AVISO: No se puede abrir el fichero '%s'!\n",fich);
			errno=0;	
			continue;
		}

		/* Cuidado con escribir en un pipe sin lectores! */
		//fprintf(stderr,"Uso: %s directorio [filtro...]\n",nombre_dir);
		
		//fprintf (stderr,"Directorio actual: %s \n", fich);
		
		/* Emitimos el contenido del archivo por la salida estandar. */
		while(write(1,buff,read(fd,buff,4096))>0);
		close(fd);
		
		/* Cerrar. */
		errno=0;
		
			/*nos aseguramos que nuestro error al leer el directorio
			no tenga nada que ver con otra cosa que no sea readdir */
	}
	if(errno!=0&&ent==NULL){
			fprintf(stderr,"Error al leer el directorio '%s'\n",nombre_dir);
			exit(1);
		}
	/* Cerrar. */
	close(1);
	closedir(dir);

	/* IMPORTANTE:
	 * Para que los lectores del pipe puedan terminar
	 * no deben quedar escritores al otro extremo. */
	// IMPORTANTE
}


void preparar_filtros(void)
{
	pid_t childpid;
	int contchild=n_filtros;
	char *point;
	char *nombreFiltro;
	int numFiltros=n_filtros;
	while(numFiltros>0){
		
		nombreFiltro=filtros[--numFiltros];
	
	
		int fd[2];

	
		/* Tuberia hacia el hijo (que es el proceso que filtra). */
		
		if(pipe(fd)==-1){	//0-->input 1-->output
			fprintf(stderr,"Error al crear el pipe\n");
			exit(1);
		}
	
		/* Lanzar nuevo proceso */
		switch(childpid=fork())
		{
		case -1:
			/* Error. Mostrar y terminar. */
			fprintf(stderr,"Error al crear proceso %d\n",childpid);
            exit(1);

		case  0:
			/* Hijo: Redireccion y Ejecuta el filtro. */
				//sleep(1);
				//close(STDIN_FILENO);
				//dup(fd[OUTPUT_END]);
				dup2(fd[OUTPUT_END], STDIN_FILENO); // cerramos nuestra entrada estandar y abrimos en su lugar la salida del pipe
				close(fd[INPUT_END]); 
				close(fd[OUTPUT_END]);
					//ESTE ESCRIBE EN STDIN
				/* El nombre termina en ".so" ? */
				if((point = strrchr(nombreFiltro,'.')) != NULL && strcmp(point,".so")==0) {
    			
         		//ends with .so

					//FILTROS DINAMICOS
				/* SI. Montar biblioteca y utilizar filtro. */

				filtrar_con_filtro(nombreFiltro);
            	exit(EXIT_SUCCESS);
				
			}
				else
			{	
				 
				/* NO. Ejecutar como mandato estandar. */
				
				execlp(nombreFiltro,nombreFiltro,(char *)NULL);
				fprintf(stderr,"Error al ejecutar el mandato '%s'\n",nombreFiltro);
				exit(EXIT_FAILURE);
			}
		//
		default:
			/* Padre: Redireccion */
			//ESTE ESCRIBE EN FD[1]
			//close(STDOUT_FILENO);
			//dup(fd[INPUT_END]);
			dup2(fd[INPUT_END], STDOUT_FILENO); //cerramos salida estandar y la abrimos en su lugar la zona de INPUT (entrada) del pipe
			close (fd[OUTPUT_END]); //cerramos entrada pipe
			close (fd[INPUT_END]);

			break;
		}
		
		pids[--contchild]=childpid;
		//fprintf(stderr,"%d\n",pids[contchild]);
		//contchild--;
	}
}


void imprimir_estado(char* filtro, int status)
{
	/* Imprimimos el nombre del filtro y su estado de terminacion */
	if(WIFEXITED(status))
		fprintf(stderr,"%s: %d\n",filtro,WEXITSTATUS(status));
	else
		fprintf(stderr,"%s: senyal %d\n",filtro,WTERMSIG(status));
}


void esperar_terminacion(void)
{	
	
	int p;
    int st=0;
    for(p=0;p<n_filtros;p++)
    {	
		
		if(waitpid(pids[p],&st,0)==-1){
			fprintf(stderr,"Error al esperar proceso\n");
			exit(1);
		}
		
	 //Espera al proceso pids[p]
		imprimir_estado(filtros[p],st);
	 // Muestra su estado. 
		}
}


extern void preparar_alarma(void)
{
	char* puntenv; //MIRAR SI SE PUEDE CAMBIAR PARA QUE PUTENV SOLO SEAN NUMEROS
	if((puntenv=getenv("FILTRAR_TIMEOUT"))!=NULL){
		int valor_env=atoi(puntenv);
		if(valor_env<1) { //||!(isdigit(puntenv))
			fprintf(stderr,"Error FILTRAR_TIMEOUT no es entero positivo: '%s'\n",puntenv);
			exit(1);
		}
		fprintf(stderr,"AVISO: La alarma vencera tras %d segundos!\n",valor_env);
		alarm(valor_env);
		signal(SIGALRM, (void (*)(int))alarm_handler);
		
		
		
	}
}

//Codigo que mata los hijos del proceso
void alarm_handler(void){
	int i;
	fprintf(stderr,"AVISO: La alarma ha saltado!\n");
	for(i=0;i<n_filtros;i++){
		if((kill(pids[i],0)==0)&&(kill(pids[i],SIGKILL)==-1)){
			fprintf(stderr,"Error al intentar matar proceso %d\n",pids[i]);
			exit(1);
		}
	}
}


