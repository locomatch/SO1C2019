
#include <stdio.h>
#include <stdlib.h>
#include "pharser.h"
#include "server.h"
#include "actions.h"
#include<string.h>




char sep[2] = {' '};//separator for null terminations
char* exec_instr(char* instr_buff){

  

    //printf("\ninstruccion:%s\n",instruction); //La dejo para debug

    char buff[strlen(instr_buff)+2];
    memcpy(buff, instr_buff, strlen(instr_buff)+1);


    int i = 0;
    int offset = 0;
    bool last_character_was_null = false;
    bool openText = false;
    while(buff[i+offset] !='\0'){
        if(!openText){
            if(buff[i]== '\n' || buff[i]==' '){

            buff[i]= '|';
            buff[i] = '|';

            if(last_character_was_null){
                int len = strlen(buff)+1;
                char* tmp = string_substring_from(buff,i+1);
                memcpy(&buff[i], tmp, strlen(tmp)+1);
                free(tmp);
                i--;
            }

            last_character_was_null = true;

            }else{

                buff[i] = buff[i];   
                last_character_was_null = false;
                if(buff[i]=='"') openText = true;
            }
        }else{
            if(buff[i] == '"') openText = false;
        }

    i++;        
    //quito las nuevas lineas 
    //quito los espacios 
    }
    buff[i-offset+1] = '\0';

    char** parameters = string_split(buff, "|");
    


    if(parameters[0]==NULL){
        free(parameters);
        return strdup("");
    } 

    int instruction_size = strlen( parameters[0])+1;
    string_to_upper(parameters[0]);

    int parameters_length = 0;

    void kill_args(){
        for(int i =0; i < parameters_length; i++){
            free(parameters[i]);
        }
        free(parameters);
    }

    while (parameters[parameters_length] != NULL){
         
         if(parameters[parameters_length][0] == '"'){
            int value_length = strlen(parameters[parameters_length]);
            if(parameters[parameters_length][value_length-1]=='"'){
                char* value = malloc(strlen(parameters[parameters_length])-1);
                memcpy(value, parameters[parameters_length]+1, value_length-2);
                memcpy(value+value_length-2, "\0", 1);
                free(parameters[parameters_length]);
                parameters[parameters_length] = value;
            }else{
   
                return "Parametro malformado.\n";
            }
        }
        parameters_length++;
    }

    if(!strcmp(parameters[0],"SELECT")){
        
        if(parameters_length != 3){
            kill_args();
            return strdup("Numero de parametros incorrectos\n");
        } 

        package_select* package = malloc(sizeof(package_select));
        package->instruction = parameters[0];

        //TABLE_NAME
        string_to_upper(parameters[1]);
        package->table_name = parameters[1];

        //KEY
        package->key = atoi(parameters[2]);
        free(parameters[2]);

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n", package->instruction, package->table_name,package->key);
        char* responce = action_select(package);
        free(parameters);
        return responce;
    }

    if(!strcmp(parameters[0],"RUN")){
        if(parameters_length != 2){
            kill_args();
            return "Numero de parametros incorrectos\n";
        }

        package_run* package = malloc(sizeof(package_run));
        package->instruction = parameters[0];

        //PATH
        package->path = parameters[1];
        //printf("\n Datos de paquete:\n instruction: %s\n path: %s\n \n", package->instruction, package->path);
        char* responce = action_run(package);
        free(parameters);
        return responce;
    }

    if(!strcmp(parameters[0],"INSERT")){
        if(parameters_length != 4 && parameters_length != 5){
            kill_args();
            return "Numero de parametros incorrectos\n";
        }

        package_insert* package = malloc(sizeof(package_insert));
        package->instruction = parameters[0];
        
        //TABLE_NAME
        string_to_upper(parameters[1]);
        package->table_name = parameters[1];        
        //KEY
        package->key = atoi(parameters[2]);

        //VALUE
        package->value = parameters[3];

        char* value = malloc(strlen(parameters[3]));
        if(parameters[3][0] == '"'){
            int value_length = strlen(parameters[3]);
            if(parameters[3][value_length-1]=='"'){
                memcpy(value, parameters[3]+1, value_length-2);
                package->value = value;
            }else{
                free(value);
                return "Parametro value malformado.\n";
            }
        }
        
    
        //TIMESTAMP
        //si hay 4 parmateros me fijo si es un timestamp
        if(parameters_length == 5){
            unsigned long timestamp = atoi(parameters[4]);
            if(timestamp>0) package->timestamp = timestamp;
        }else{
            package->timestamp = time(NULL);
        }

       //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n Key: %d\n Value: %s\n Timestamp: %lu\n", package->instruction, package->table_name, package->key, package->value, package->timestamp);
        char* responce = action_insert(package);
        free(parameters);
        return responce;
    }

    if(!strcmp(parameters[0],"CREATE")){
        if(parameters_length != 5 ){
            kill_args();
            return "Numero de parametros incorrectos\n";
        }

        package_create* package = malloc(sizeof(package_insert));
        package->instruction = parameters[0];
        
        //TABLE_NAME
        string_to_upper(parameters[1]);
        package->table_name = parameters[1];        
        //CONSISTENCIA
        char* c =  strdup(parameters[2]); 
        string_to_upper(c);
        if(!strcmp(c,"SC")){
            package->consistency = S_CONSISTENCY;
        }else if(!strcmp(c,"EC")){
            package->consistency = ANY_CONSISTENCY;
        }else if(!strcmp(c,"HC")){
            package->consistency = H_CONSISTENCY;
        }else{
            return "Criterio no valido";
        }

        free(c);
        free(parameters[2]);

        //VALUE
        package->partition_number = atoi(parameters[3]);
        
        package->compactation_time = atol(parameters[4]);

        //printf("\n Datos de paquete:\n instruction: %s\n Table name: %s\n cons: %d\n particiones: %d\n comp: %lu\n", package->instruction, package->table_name, package->consistency, package->partition_number, package->compactation_time);
        
        char* responce =action_create(package);
        free(parameters);
        return responce;
    }

    if(!strcmp(parameters[0],"DESCRIBE")){
        if(parameters_length != 1 && parameters_length != 2){
            kill_args();
            return "Numero de parametros incorrecto"; 
        }

        package_describe* package = malloc(sizeof(package_describe));
        package->instruction = parameters[0];
        

        //TABLE_NAME
        if(parameters_length>1) {
            package->table_name = NULL;
        } else {
            string_to_upper(parameters[1]);
            package->table_name = parameters[1];
        }

        //printf("\n Datos de paquete:\n instruction: %s\n table name: %s\n \n", package->instruction, package->table_name);
        char* responce = action_describe(package);
        free(parameters);
        return responce;
    }

    if(!strcmp(parameters[0],"ADD")){
        if(parameters_length != 5){
            kill_args();
            return "Numero de parametros incorrecto";
        }

        package_add* package = malloc(sizeof(package_add));
        package->instruction = parameters[0];

        string_to_upper(parameters[1]);
        string_to_upper(parameters[3]);
        if(strcmp(parameters[1], "MEMORY")) return "Instrucion ADD mal formulada.\n";
        if(strcmp(parameters[3], "TO")) return "Instruccion ADD mal formulada.\n";
        
        package->id = atoi(strdup(parameters[2]));

        char* c =  strdup(parameters[4]); //+2 por TO
        string_to_upper(c);
        if(!strcmp(c,"SC")){
            package->consistency = S_CONSISTENCY;
        }else if(!strcmp(c,"EC")){
            package->consistency = ANY_CONSISTENCY;
        }else if(!strcmp(c,"HC")){
            package->consistency = H_CONSISTENCY;
        }else{
            return "Criterio no valido\n";
        }

        free(c);
        free(parameters[4]);
        //printf("\n Datos de paquete:\n instruction: %s\n id: %d\ncriterio:%d \n", package->instruction , package->id, package->consistency);
        char* responce = action_add(package);
        free(parameters);
        return responce;
    }

    if(!strcmp(parameters[0],"MEMORY")){
        char* r =  action_intern__status();
        kill_args();
        return r;
    }

    if(!strcmp(parameters[0], "DROP")){
        if(parameters_length != 2) {
            kill_args();
            return "Numero de parametros incorrecto";
        }

        package_drop* package = malloc(sizeof(package_drop));
        package->instruction = parameters[0];
        package->table_name = parameters[1];

        free(parameters);
        return action_drop(package);
    }

    if(!strcmp(parameters[0], "JOURNAL")){
        if(parameters_length != 1) {
            kill_args();
            return "Numero de parametros incorrecto";
        }

        package_journal* pakage = malloc(sizeof(package_journal));
        pakage->instruction = parameters[0];
        free(parameters);
        return action_journal(pakage);
    }

    if(!strcmp(parameters[0], "METRICS")){
        if(parameters_length != 1){
            kill_args();
            return "Numero de parametros incorrecto";
        }
        package_metrics* pk = malloc(sizeof(package_metrics));
        pk->instruction = strdup("METRICS");
        
        free(parameters);
        return action_metrics(pk);
    }

    kill_args();

    char* error_message = strdup("no es una instruccion valida\n");
    return error_message;
    
}

char* parse_package_select(package_select* package){
    char* buffer; 
    int tot_len = strlen(package->instruction)+1 + strlen(package->table_name)+1 + 16+1;
    
    buffer = malloc(tot_len);
    char* num_buff = string_itoa(package->key);
    strcpy(buffer, package->instruction);
    strcat(buffer, sep); //emular NULL terminations
    strcat(buffer, package->table_name);
    strcat(buffer, sep);
    strcat(buffer, num_buff);
    strcat(buffer, "\n");

    free(package->instruction);
    free(package->table_name);
    free(num_buff);
    free(package);

    return buffer;
}

char* parse_package_run(package_run* pk){
    char* buffer;
    int tot_len = strlen(pk->instruction) + strlen(pk->path) +2;
    buffer = malloc(2+tot_len);
    strcpy(buffer, pk->instruction);
    strcat(buffer,sep);
    strcat(buffer,pk->path);
    strcat(buffer, "\n");    

    free(pk->instruction);
    free(pk->path);
    free(pk);

    return buffer;
}

char* parse_package_insert(package_insert* package){
    
    char* buffer; 
    
    int tot_len = strlen(2+package->instruction)+1 + strlen(package->table_name)+1 + strlen(string_itoa(package->key))+1 + strlen(package->value)+1 + 40 +5;
    buffer = malloc(tot_len);
    
    strcpy(buffer, package->instruction);
    strcat(buffer, sep); //emular NULL terminations
    strcat(buffer, package->table_name);
    strcat(buffer, sep);
    strcat(buffer, string_itoa(package->key));
    strcat(buffer, sep);
    strcat(buffer, package->value);
    strcat(buffer, sep);
    char* buff = malloc(30);
    sprintf(buff, "%ld",package->timestamp);
    strcat(buffer, buff);
    free(buff);
    strcat(buffer, "\n");

    free(package->instruction);
    free(package->table_name);
    free(package->value);

    return buffer;
}

char* parse_package_describe(package_describe* pk){

    if(pk->table_name == NULL){

        free(pk->instruction);
        free(pk->table_name);
        free(pk);

        return "DESCRIBE\n";
    }
    char* buffer = malloc(2+strlen(pk->instruction) + strlen(pk->table_name) +4);
    strcpy(buffer, pk->instruction);
    strcat(buffer,sep);
    strcat(buffer,pk->table_name);
    strcat(buffer, "\n");

    free(pk->instruction);
    free(pk->table_name);
    free(pk);

    return buffer;
}

char* parse_package_create(package_create* pk){
    char* buffer = malloc(2+ strlen(pk->instruction) + strlen(pk->table_name) + 3 /*consistencia */ + strlen(string_itoa(pk->partition_number) + 22 + 9));
     
    strcpy(buffer, pk->instruction);
    strcat(buffer, sep);
    strcat(buffer, pk->table_name);
    strcat(buffer, sep);
    char* consistency = malloc(4);
    switch (pk->consistency)
    {
    case S_CONSISTENCY:
        consistency = strdup("SC");
        break;
    case H_CONSISTENCY:
        consistency = strdup("HC");
    case ANY_CONSISTENCY:
        consistency = strdup("EC");
    
    default:
        consistency = strdup("ERR");
        break;
    }


    strcat(buffer, consistency );
    free(consistency);
 
    strcat(buffer, sep);
    strcat(buffer, string_itoa(pk->partition_number));
    strcat(buffer, sep);

    char* buff = malloc(30);
    
    sprintf(buff, "%ld", pk->compactation_time);
    strcat(buffer, buff);
    strcat(buffer, "\n");
    free(buff);

    free(pk->table_name);
    free(pk->instruction);
    free(pk);

    return buffer;
}

char* parse_package_drop(package_drop* pk){
    char* bff = malloc( strlen(pk->instruction) + strlen(pk->table_name) + 4);
    strcpy(bff, pk->instruction);
    strcat(bff, sep);
    strcat(bff, pk->table_name);

    free(pk->instruction);
    free(pk->table_name);
    free(pk);

    return bff;
}

char* parse_package_journal(package_journal* pk){
    char* r = strdup(pk->instruction);
    free(pk->instruction);
    free(pk);
    return r;
}



