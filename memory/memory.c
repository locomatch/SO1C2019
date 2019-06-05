#include <stdlib.h>
#include <commons/log.h>
#include <pthread.h>
#include <string.h>
#include "../server.h"
#include <commons/config.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "../pharser.h"
#include "../actions.h"
#include "../console.h"

//logger global para que lo accedan los threads
t_log* logger;
int fs_socket;

//declaro memoria principal
char* main_memory;
int main_memory_size;

int PAGE_SIZE = 512;
int SEGMENT_SIZE = 1024; //TODO obtener el numero posta del handshake con fs

typedef struct{
  char* name;
  char* base;
  int limit;
  int pages[2];
}segment_info;

typedef struct segment{
  segment_info data;
  struct segment *next;
}segment;

segment* create_segment(){
  segment* temp;
  temp = (segment*)malloc(sizeof(segment));
  temp->next = NULL;
  return temp;
}

segment* SEGMENT_TABLE;


segment* get_segment(int index){
    segment* temp = SEGMENT_TABLE;
    int i = 1;
    while(i < index){ //itero sobre los segments hasta llegar al index
      temp = temp->next;
      i++;
    }
    return temp;
}

void add_segment_to_table(int index, segment* new_segment){ //TODO guardar el value en memoria principal, hasta ahora solo guardo lo demas en la table
  if(index == 0){
    segment* temp = SEGMENT_TABLE;
    SEGMENT_TABLE = new_segment;
    new_segment->next = temp;
  } 
  else{
    segment* temp = get_segment(index);
    new_segment->next = temp->next;
    temp->next = new_segment;
  }
}

//devuelve la ultima direccion de memoria del segment, habria que agregarle -1 
//
char* get_end_memory_address(int index){
  if(index == 0){
    return &main_memory[0];
  }
  segment* temp = get_segment(index);
  return &temp->data.base[temp->data.limit - 1]; //base de memoria del segment con desplazamiento limit
}

// devuelve la direccion que el nuevo segmento deberia usar de base
char* get_first_memory_address_after(int index){
  if(index == 0){
    return &main_memory[0];
  }
  return get_end_memory_address(index) + 0x1;
}

//retorna el index del segmento que tiene suficiente despues del mismo y el proximo segmento. -1 si no encuentra nada. Si tira -1 hay que hacer lugar.
int find_memory_space(int memory_needed){
  char* base_memory = &main_memory[0];  
  
  if(memory_needed > main_memory_size){
    return -1; //TODO hacer que tire error en vez de devolver -1 porque no hay manera de encontrar espacio.
  }

  //si SEGMENT_TABLE es null, entonces no hay segmentos por lo que memoria esta vacia. Asigno en el primer byte de memoria
  if(SEGMENT_TABLE == NULL){
    return 0;
  }

  segment* temp = SEGMENT_TABLE;
  int i = 1;
  while(temp != NULL){
    if(temp->data.base - base_memory >= memory_needed){
      return i - 1;
    }
    base_memory = get_first_memory_address_after(i); //asigno la base de memoria como la base del segmento mas el desplazamiento para comparar el proximo segmento con esta base
    temp = temp->next;
    i++;
  }

  //estoy en el ultimo segmento, la diferencia entre el utlimo byte de memoria y la base_memory es el espacio de memoria que queda.
  //a[10] = [0..9]
  //a[size(a)-1] = 9
  //a[0] = 0
  //si se restan, dan 9, cuando el size es 10, hay que sumar 1
  return &main_memory[main_memory_size - 1] - base_memory + 1 >= memory_needed ? i - 1 : -1;
}
// TODO: corroborar que el [temp->data.limit - 1] y el get_end_memory_address(index) + 1 este ok

void save_segment_to_memory(segment_info segment_info){
  int index = find_memory_space(segment_info.limit);
  printf("Index to save segment info: %d\n", index);
  // busco espacio en memoria y me devuelve un index
  
  char* base_memory = get_first_memory_address_after(index);
  printf("Base del nuevo segmento %p\n", base_memory);
  
  // traduzco el index al address de memoria

  segment_info.base = base_memory;
  segment* new_segment = create_segment();
  new_segment->data = segment_info;
  add_segment_to_table(index, new_segment);
}

void print_segment_info(segment* temp){
  printf("Nombre de tabla: %s\n", temp->data.name);
  printf("Base de memoria: %p\n", temp->data.base);
  printf("Tamanio segmento: %d\n", &temp->data.base[temp->data.limit - 1] - temp->data.base + 1);
  printf("Ultima posicion de memoria: %p\n\n", &temp->data.base[temp->data.limit - 1]);
}

// devuelve el indice del segmento que contiene el nombre de tabla o -1 si no encuentra
int find_table(char* table_name){
  segment* temp = SEGMENT_TABLE;
  int index = 1;
  while(temp != NULL){
    if(strcmp(temp->data.name, table_name) == 0){
      return index;
    }
    index++;
    temp = temp->next;
  }
  return -1;
}

int find_page(int pages[], int size, int key){
  for (int i = 0; i < size; i++){
    if(pages[i] == key){
      return i;
    }
  }
  return -1;
}

// int get_value(char table_name, int key){
//   int segment_index = find_table(table_name);
//   if (segment_index == -1){
//     return -1;
//   }
//   segment* segment = get_segment(segment_index);
//   int page_index = find_page(segment->data.pages, key);
//   if(page_index == -1){
//     return -1;
//   }
// }

int get_memory_offset(char* base){
  return (int) base - (int) &main_memory[0];
}

void save_value_to_memory(segment* segment, int page_index){
  int offset = page_index * PAGE_SIZE;
  
  
  printf("%s: %d\n", "Base memoria", get_memory_offset(segment->data.base));
}

char* get_value(segment* segment, int page_index){
  int offset =  page_index * PAGE_SIZE;

}


//punto de entrada para el programa y el kernel
int main(int argc, char const *argv[])
{
  //set up config  
  t_config* config = config_create("config");
  char* LOGPATH = config_get_string_value(config, "LOG_PATH");
  int PORT = config_get_int_value(config, "PORT");

  //set up log
  logger = log_create(LOGPATH, "Memory", 1, LOG_LEVEL_INFO);
  log_info(logger, "El log fue creado con exito\n");

  //set up server
  pthread_t tid;
  server_info* serverInfo = malloc(sizeof(server_info));
  memset(serverInfo, 0, sizeof(server_info));    
  serverInfo->logger = logger;
  serverInfo->portNumber = PORT; 
  int reslt = pthread_create(&tid, NULL, create_server, (void*) serverInfo);

  //set up client 
  fs_socket = socket(AF_INET, SOCK_STREAM, 0);
  char* FS_IP = config_get_string_value(config, "FS_IP");
  int FS_PORT = config_get_int_value(config, "FS_PORT");
  printf("%s %d\n", FS_IP, FS_PORT);
  struct sockaddr_in sock_client;
  
  sock_client.sin_family = AF_INET; 
  sock_client.sin_addr.s_addr = inet_addr(FS_IP); 
  sock_client.sin_port = htons(FS_PORT);

  int connection_result =  connect(fs_socket, (struct sockaddr*)&sock_client, sizeof(sock_client));
  
  if(connection_result < 0){
    log_error(logger, "No se logro establecer la conexion con el File System");   
  }
  else{
  }

  //reservo memoria contigua para la memoria principal
  main_memory_size = config_get_int_value(config, "TAM_MEM");
  main_memory = malloc(main_memory_size);
  memset(main_memory, 0, main_memory_size);
  printf("%s: %p\n", "Puntero a memoria[0]", &main_memory[0]);
  printf("%s: %p\n", "Puntero a memoria[4096]", &main_memory[main_memory_size -1]);
  printf("%s: %d\n\n", "Tamanio memoria", &main_memory[main_memory_size -1] - &main_memory[0] + 1);

  SEGMENT_TABLE = NULL;

  segment* segment1 = create_segment();
  segment_info seg_info1;
  seg_info1.limit = SEGMENT_SIZE;
  seg_info1.name = "tabla1";
  seg_info1.pages[0] = 20;
  seg_info1.pages[1] = 23;
  segment1->data = seg_info1;

  segment* segment2 = create_segment();
  segment_info seg_info2;
  seg_info2.limit = SEGMENT_SIZE;
  seg_info2.name = "tabla2";
  segment2->data = seg_info2;

  segment* segment3 = create_segment();
  segment_info seg_info3;
  seg_info3.limit = SEGMENT_SIZE;
  seg_info3.name = "tabla3";
  segment3->data = seg_info3;

  save_segment_to_memory(seg_info1);
  save_segment_to_memory(seg_info2);
  save_segment_to_memory(seg_info3);

  print_segment_info(get_segment(1));
  print_segment_info(get_segment(2));
  print_segment_info(get_segment(3));

  save_value_to_memory(get_segment(2), 1);

  char* tabla_a_buscar = "tabla1";
  printf("Se encuentra %s en memoria?: %d\n", tabla_a_buscar, find_table(tabla_a_buscar));
  int index_tabla_a_buscar = find_table(tabla_a_buscar);
  if(index_tabla_a_buscar != -1){
    segment* test_segment = get_segment(index_tabla_a_buscar);
    // sizeof(test_segment->data.pages) / PAGE_SIZE;
    int index_key = find_page(test_segment->data.pages, 2, 23);
    if(index_key != -1){
      
    }
  }

  //inicio lectura por consola
  pthread_t tid_console;
  pthread_create(&tid_console, NULL, console_input, "Memory");
    
  
  //Espera a que terminen las threads antes de seguir
  pthread_join(tid,NULL);
  
  //FREE MEMORY
  free(LOGPATH);
  free(logger);
  free(serverInfo);
  config_destroy(config);

  return 0;
}

//IMPLEMENTACION DE FUNCIONES (Devolver errror fuera del subconjunto)
char* action_select(package_select* select_info){
  log_info(logger, "Memory: Se recibio una accion select");
  int asd = find_memory_space(400);
  // printf("Segment index: %d\n", asd);
  char* response = parse_package_select(select_info);
  send(fs_socket, response, strlen(response)+1, 0);


  return string_new("holis"); //tienen que devolver algo si no se rompe
}

void action_insert(package_insert* insert_info){
  log_info(logger, "Se recibio una accion insert");
}

void action_create(package_create* create_info){
  log_info(logger, "Se recibio una accion create");
}

void action_describe(package_describe* describe_info){
  log_info(logger, "Se recibio una accion describe");
}

void action_drop(package_drop* drop_info){
  log_info(logger, "Se recibio una accion drop");
}

void action_journal(package_journal* journal_info){
  log_info(logger, "Se recibio una accion select");
}

void action_add(package_add* add_info){
  log_info(logger, "Se recibio una accion select");
}

char* action_run(package_run* run_info){
  log_info(logger, "Se recibio una accion run");
}

void action_metrics(package_metrics* metrics_info){
  log_info(logger, "Se recibio una accion metrics");
}

char* parse_input(char* input){
  return exec_instr(input);
}
