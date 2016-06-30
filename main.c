#include <main.h>
#include <stdlib.h>
#include "cola.c"

#define EEPROM_SDA  PIN_B5
#define EEPROM_SCL  PIN_B4
#include "24256.C"

#define RTC_SDA  PIN_B5
#define RTC_SCL  PIN_B4
#define USE_INTERRUPTS 1
#include <_ds1307.c>

#define LED PIN_C2

#priority RDA, TIMER1
/*-----Types Definitions------*/
typedef struct {
    int id;
    char value[8];
}
medicion_t;
typedef struct {
    int16 numero;
    char fecha_hora[20];
    medicion_t datos[10];
}
paquete_t;
/*-----End Types Definitions------*/

/*-----Var Definitions------*/
int16 count=0,passes=36,paquete_n=0,minutes=5;
int count_p = 0, count_f = 0, ncfg_f = 0, flag = 0,index = 0, stringCompleta = 0, flag2 = 0,index2 = 0, stringCompleta2 = 0,stringCompleta3 = 0,flag3 = 0,index3 = 0,stringCompleta4 = 0,flag4 = 0;
char ResponseBuff[100]="", ResponseBuff2[100]="",ResponseBuff3[100]="", IMEI[16]="358072040190107";
int dbg_a = 0, sin_rta=0, ft = 0, desencole = 0;
int16 w_faddr = 20, w_nextaddr = 20, r_faddr = 20, r_nextaddr = 20, pP=65535;
BYTE sec, min, hrs, day, month, yr, dow;
cola_t* cola_p, *cola_o;
paquete_t* paquete_r = NULL;
/*-----End Var Definitions------*/

/*-----Interrupts------*/
#INT_RDA HIGH //For uart1 --> Arduino
void RDA_isr(VOID){
    char d =getc(uart1);
    if(d=='$'){
        if(flag==0){
            //Comienza la string
            index=0;
            flag=1;
        }
        else{
            //Fin de la string
            flag=0;
            stringCompleta = 1;
            ResponseBuff[index+1] = "";
        }
    }
    if(d=='&'){
        if(flag4==0){
            //Comienza la string
            index=0;
            flag4=1;
        }
        else{
            //Fin de la string
            flag4=0;
            stringCompleta4 = 1;
            ResponseBuff[index+1]="";
        }
    }
    ResponseBuff[index]=d;
    index++;
    if(index>=100){
        //Buffer spill
        index=0;
    }
}

#INT_RDA2 //For uart2 --> Nodos
void rda_uart2(VOID){
    char d =getc(uart2);
    if(d=='$'){
        if(flag2==0){
            //Comienza la string
            index2=0;
            flag2=1;
        }
        else{
            //Fin de la string
            flag2=0;
            stringCompleta2 = 1;
            ResponseBuff2[index2+1]="";
        }
    }
    ResponseBuff2[index2]=d;
    index2++;
    if(index2>=100){
        //Buffer spill
        index2=0;
    }
}

#INT_TIMER0
void RDA_TIMER0(VOID){
    count++;
    count_p++;
    count_f++;
}
/*-----End Interrupts------*/

/*-----Functions------*/
void clear_buff(){
    int x;
    for(x=0;x<100;x++){
        ResponseBuff[x]="";
        ResponseBuff2[x]="";
        ResponseBuff3[x]="";
    }
}
void set_laps(){
    passes = (float)minutes * 60 / 8.3;
}
void save_minutes(){
    write_ext_eeprom(0,make8(minutes,0));
    write_ext_eeprom(1,make8(minutes,1));
}
void read_minutes(){
    int16 min = make16(read_ext_eeprom(1), read_ext_eeprom(0));
    if(min > 0 && min != pP){
      minutes = min;
    }
    else{
      save_minutes();
    }
}
void write_imei(){
    int a;
    for(a=0;a<15;a++){
        write_ext_eeprom(a+2,IMEI[a]);
    }
}
void read_imei(){
    int a, len = 15;
    int r = read_ext_eeprom(2);
    if(r != 255){ // Va a ser 255 cuando no haya sido escrito en memoria
       for(a=0;a<len;a++){
           IMEI[a] = read_ext_eeprom(a+2);
       }
    }
    else{
      write_imei();
    }
}
void setup(){
    setup_adc(ADC_OFF);//disable ADC
    setup_spi(SPI_DISABLED);
    setup_timer_0(RTCC_INTERNAL|RTCC_DIV_256);      //8.3 s overflow
    enable_interrupts(INT_TIMER0);
    enable_interrupts(INT_RDA);
    enable_interrupts(INT_RDA2);
    enable_interrupts(GLOBAL);
    ds1307_init(DS1307_OUT_ON_DISABLED_HIHG | DS1307_OUT_ENABLED | DS1307_OUT_32_KHZ);
    init_ext_eeprom();

    read_minutes();
    set_laps();
    read_imei();
    cola_p = cola_crear();
    cola_o = cola_crear();
    clear_buff();
}

void get_time(){
    ds1307_get_date(day,month,yr,dow);        /// se obtiene la fecha
    ds1307_get_time(hrs,min,sec);             /// se obtiene la hora
}
void set_time(dia,mes,anio,dsem,hora,min,seg){
    ds1307_set_date_time(dia,mes,anio,dsem,hora,min,seg);
}
void free_paquete(paquete_t* paquete_a){
    int a;
    for(a=0;a<10;a++){
        free(paquete_a->datos[a]); //Dato[a] contiene una medicion. Libero mem alocada para mediciones
    }
    free(paquete_a);
}
void free_paquete_r(){
    int a;
    for(a=0;a<10;a++){
        free(paquete_r->datos[a]);//Free Medicion
    }
    free(paquete_r);//Free paquete
}
void get_med(int id, medicion_t *med){
    int32 timeout=0;
    char mens[10]="";
    sprintf(mens,"$%i,GET,MED$",id);
    med->id=id;
    strcpy(med->value,"XX.XX");
    fprintf(uart2,mens);
    //Vamos a tener que esperar a la interrupcion stringCompleta2 o timeout
    while(StringCompleta2!=1 && timeout<20000){timeout++;delay_us(10);} //0.20 segundos timeout
    if(StringCompleta2==1){ //Salio por respuesta
        char sep[3]="$,";
        char *ptr;
        ptr = strtok(ResponseBuff2, sep);//ID
        int idi = atoi(ptr);
        ptr = strtok(NULL, sep);//"MED"
        ptr = strtok(NULL, sep);//Valor
        delay_ms(100);
        if(id==idi){ //Just in case
            strcpy(med->value,ptr);
        }
    }
    StringCompleta2=0;
    ResponseBuff2="";
    index2 = 0;
}

void encolar_paquete(){
    paquete_t* paquete =  malloc(sizeof(paquete_t));
    paquete->numero = paquete_n;
    get_time();
    sprintf(paquete->fecha_hora,"20%02i-%02i-%02i %02i:%02i:%02i",yr,month,day,hrs,min,sec);
    /*Pulleo por mediciones. Por cada medicion agrego una medicion_t a datos*/
    int a;
    for(a=0;a<10;a++){
        get_med(a,&paquete->datos[a]);
    }
    encolar(cola_p,paquete);
    paquete_n++;
    if(paquete_n>10000){
        paquete_n = 0;
    }
    if(dbg_a == 1){
      fprintf(uart3,"Nuevo paquete %li \r\n",paquete->numero);
    }
    while(cola_p->largo > 1){ //Tengo paquetes viejos en la cola
      paquete_t* pq = desencolar(cola_p);
      encolar(cola_o,pq);
    }
}
char* get_fecha(int16 add){
    int a = 0;
    int16 ads=add;
    char das[20] = "";
    for(a=0;a<19;a++){
           das[a] = read_ext_eeprom(add);
           add++;
   }
   das[19] = '\0';
   return das;
}

int16 findPlace(){
//EEPROM_SIZE --  r_faddr
   int16 pos=r_faddr, oldsPack = make16( read_ext_eeprom(pos + 1), read_ext_eeprom(pos)), oldsPackPos = pos;
   char date[20]="";
   strcpy(date,get_fecha(pos+2));
   for(pos=r_faddr;pos<EEPROM_SIZE;pos+=sizeof(paquete_t)){
      int16 numpaquete = make16( read_ext_eeprom(pos + 1), read_ext_eeprom(pos));
      char da[20];
      strcpy(da,get_fecha(pos+2));
      if(numpaquete == pP && (pos+sizeof(paquete_t))<EEPROM_SIZE ){
         return pos;
      }
      if(numpaquete < oldsPack && strcmp(date,da) == 1 && (pos+sizeof(paquete_t)) < EEPROM_SIZE){ //NumPaq menor y fecha menos actual
         oldsPack = numpaquete; //Guardo para poder seguir comparando si hay uno mas viejo
         strcpy(date,da);
         oldsPackPos = pos; //Guargo posicion que es el valor de retorno
      }
   }
   return oldsPackPos;
}
void dump_to_eeprom(paquete_t* paquete){
   int a = 0, x = 0;
   int16 add = w_nextaddr;
   //Numero de paquete es un int16, lo guardo como tal en la memoria
   write_ext_eeprom(add,make8(paquete->numero,0));add++;
   write_ext_eeprom(add,make8(paquete->numero,1));add++;
   //Guardo fecha
   for(a=0;a<19;a++){
      char c = paquete->fecha_hora[a];
      write_ext_eeprom(add, c);
      add++;
   }

   for(a=0;a<10;a++){
      write_ext_eeprom(add,paquete->datos[a].id);add++;
      write_ext_eeprom(add,paquete->datos[a].value[0]);add++;
      write_ext_eeprom(add,paquete->datos[a].value[1]);add++;
      write_ext_eeprom(add,paquete->datos[a].value[2]);add++;
      write_ext_eeprom(add,paquete->datos[a].value[3]);add++;
      write_ext_eeprom(add,paquete->datos[a].value[4]);add++;
      write_ext_eeprom(add,paquete->datos[a].value[5]);add++;
      write_ext_eeprom(add,paquete->datos[a].value[6]);add++;
      write_ext_eeprom(add,paquete->datos[a].value[7]);add++;
   }
   w_nextaddr = findPlace();
   free_paquete(paquete);

}
void guardar_paquetes(){
   int16 a=1;
   for(a=1;a<cola_p->largo;a++){
      paquete_t * p = desencolar(cola_p);
      dump_to_eeprom(p);
   }
   a=0;
   for(a=0;a<cola_o->largo;a++){
      paquete_t * p = desencolar(cola_o);
      dump_to_eeprom(p);
   }
}
void restore_from_eeprom(){
   int k=0;
   for(k=0;k<=5;k++){//Trato de levantar 5 paquetes.
      int16 add = r_nextaddr;
      int16 numpaquete = make16(read_ext_eeprom(add + 1), read_ext_eeprom(add));
      write_ext_eeprom(add,make8(pP,0));add++;
      write_ext_eeprom(add,make8(pP,1));add++;
      if(numpaquete != pP ){ // Si es 32535 ya ha sido enviado y es remanente de otro momento
         paquete_t* paquete =  malloc(sizeof(paquete_t));
         if(dbg_a == 1){
            fprintf(uart3,"Recuperando paquete %li \r\n",numpaquete);
         }
         paquete->numero = numpaquete;
         int a = 0;
         //Leo la fecha
         for(a=0;a<19;a++){
            paquete->fecha_hora[a] = read_ext_eeprom(add);
            add++;
         }
         paquete->fecha_hora[19]='\0';
         for(a=0;a<10;a++){

            paquete->datos[a].id =  read_ext_eeprom(add);add++;;
            paquete->datos[a].value[0] = read_ext_eeprom(add);add++;
            paquete->datos[a].value[1] = read_ext_eeprom(add);add++;
            paquete->datos[a].value[2] = read_ext_eeprom(add);add++;
            paquete->datos[a].value[3] = read_ext_eeprom(add);add++;
            paquete->datos[a].value[4] = read_ext_eeprom(add);add++;
            paquete->datos[a].value[5] = read_ext_eeprom(add);add++;
            paquete->datos[a].value[6] = read_ext_eeprom(add);add++;
            paquete->datos[a].value[7] = read_ext_eeprom(add);add++;
         }

         encolar(cola_o,paquete);
     }

     r_nextaddr += sizeof(paquete_t);
     if((r_nextaddr + sizeof(paquete_t)) > EEPROM_SIZE){
      r_nextaddr = r_faddr;
     }
   }
}

void enviar_paquetes(){
    if(paquete_r != NULL){//Si hab?a un paquete que esperaba ACK y no lleg?, lo re-encolo
        paquete_t* paqu = paquete_r;
        if(cola_p->largo > 0){ //Si hay algo en la cola, es probable que este sea un paquete viejo
         encolar(cola_o,paqu);
        }
        else{ //Sino, es el unico paquete que hay dando vueltas
         encolar(cola_p,paqu);
        }
        paquete_r = NULL;
        sin_rta++;
        //Si se esta sin conexion, inmediatamente dumpeamos
        if(sin_rta > 5){
          guardar_paquetes();
        }
    }
    //Una vez re-encolado el paquete "colgado", se puede seguir con la cola
    paquete_r = desencolar(cola_p); //Desencolo el paquete
    desencole = 0;
    if (paquete_r != NULL){
        char mensaje[200]="";
        //sprintf(mensaje,"$B,%s,%li,0,00,%s,%s,Interfaz Ethernet,%s,$E",IMEI,paquete_r->numero,paquete_r->datos[0].value,paquete_r->datos[1].value,paquete_r->fecha_hora);
        //Armo el mensaje - con las 10 mediciones maximas
        char mediciones[200]="";
        int a = 0;
        char def[6]="XX.XX",coma[]=",",empt[]="-99";
        for(a=0;a<10;a++){
          char medi[8];
          sprintf(medi,"%s",paquete_r->datos[a].value);
          if(strcmp(medi,def)==0){//No tiene medicion
            strcat(mediciones,empt);
          }
          else{
            strcat(mediciones,medi);
          }
          strcat(mediciones,coma);
        }

        sprintf(mensaje,"$B,%s,%li,0,00,%sEthernet,%s,$E",IMEI,paquete_r->numero,mediciones,paquete_r->fecha_hora);
        if(dbg_a == 1){
            fprintf(uart3, "Paquete actual enviado: %s \r\n", mensaje);
            delay_ms(100);
        }
        puts(mensaje,uart1); //Envio a Ardunio

    }
    //else Cola vacia
}
void enviar_paquetes_o(){
    if(paquete_r != NULL){//Si hab?a un paquete que esperaba ACK y no lleg?, lo re-encolo en mis paquetes viejos
        paquete_t* paqu = paquete_r;
        encolar(cola_o,paqu);
        paquete_r = NULL;
        sin_rta++;
    }
    //Una vez re-encolado el paquete "colgado", se puede seguir con la cola
    paquete_r = desencolar(cola_o); //Desencolo el paquete
    desencole = 1;
    if (paquete_r != NULL){
        char mensaje[200]="";
        //sprintf(mensaje,"$B,%s,%li,0,00,%s,%s,Interfaz Ethernet,%s,$E",IMEI,paquete_r->numero,paquete_r->datos[0].value,paquete_r->datos[1].value,paquete_r->fecha_hora);
        //Armo el mensaje - con las 10 mediciones maximas
         char mediciones[200]="";
        int a = 0;
        char def[6]="XX.XX",coma[]=",",empt[]="-99";
        for(a=0;a<10;a++){
          char medi[8];
          sprintf(medi,"%s",paquete_r->datos[a].value);
          if(strcmp(medi,def)!=0){//0 es igual
            //Tiene una medicion
            strcat(mediciones,medi);
          }
          else{
            strcat(mediciones,empt);
          }
          strcat(mediciones,coma);
        }

        sprintf(mensaje,"$B,%s,%li,0,00,%sEthernet,%s,$E",IMEI,paquete_r->numero,mediciones,paquete_r->fecha_hora);
        if(dbg_a == 1){
            fprintf(uart3, "Paquete atrasado enviado: %s \r\n", mensaje);
            delay_ms(100);
        }
        puts(mensaje,uart1); //Envio a Ardunio

    }
    //else Cola vacia
    else{
        restore_from_eeprom();
    }
}
void inspeccionarUSB(){
   if(kbhit(uart3)){
            char d = getc(uart3);
            if(d=='$'){
                if(flag3==0){
                    //Comienza la string
                    index3=0;
                    flag3=1;
                }
                else{
                    //Fin de la string
                    flag3=0;
                    stringCompleta3 = 1;
                    ResponseBuff3[index3+1]="";
                }
            }
            ResponseBuff3[index3]=d;
            index3++;
            if(index3>=100){
                //Buffer spill
                index3=0;
            }
        }
        if(stringCompleta3==1){
            char delm[]="$,",cfg[]="CFG",ncfg[]="NCFG",getp[]="GET",setp[]="SET",time[]="TIME",minp[]="MIN",
            imeip[]="IMEI",dbug[]="DBG",ndbug[]="NDBG",gsmp[]="GSM",EPR[]="EPR"/*,betap[]="BETA",resp[]="RES",med[]="MED"*/;//Params Generales
            char *ptr, ord[5], param[5], value[20],mens[50];
            //char //Params GSM
            int16 id_n;
            strcpy(mens,ResponseBuff3);
            ptr = strtok(ResponseBuff3,delm);//Id
            id_n = atol(ptr);//Id - 255 Master, 254 Arduino, 0-253 Nodos
            ptr = strtok(NULL, delm);//Get, set, cfg, ncfg o DBG
            strcpy(ord,ptr);
            ptr = strtok(NULL, delm);//Param para get o set
            if(ptr==NULL){
                goto exec;
            }
            strcpy(param,ptr);//No fue null
            ptr = strtok(NULL, delm);//Value en caso de set
            if(ptr==NULL){
                goto exec;
            }
            strcpy(value,ptr);//No fue null
            exec:
            if(id_n==255){//Master
                if(strcmp(ord,cfg)==0){//Modo configuracion
                    disable_interrupts(INT_TIMER0); //Deshabilito el timer para que no quiera hacer nada paquetoso mientras configuro
                    count=0;count_p=0; ncfg_f = 1;
                    fprintf(uart3,"$%li,OK$",id_n);//Devuelvo para que sepa que soy el que busca
                }
                else if(strcmp(ord,ncfg)==0){//Quita modo configuracion
                    enable_interrupts(INT_TIMER0); //Empieze de nuevo
                    ncfg_f = 0;
                    fprintf(uart3,"$%li,OK$",id_n);//Devuelvo para que sepa que todo bien
                    output_high(PIN_A3);
                    delay_ms(300);
                    output_low(PIN_A3);
                }
                else if(strcmp(ord,dbug)==0){//Modo Debug
                    //Activa dbg
                    dbg_a = 1;
                    enable_interrupts(INT_TIMER0); //Empieze de nuevo
                    fprintf(uart3,"$%li,OK$",id_n);//Devuelvo para que sepa que soy el que busca
                }
                else if(strcmp(ord,ndbug)==0){//Modo No-Debug
                    //Desactiva dbg
                    dbg_a = 0;
                    fprintf(uart3,"$%li,OK$",id_n);//Devuelvo para que sepa que soy el que busca
                }
                else if(strcmp(ord,EPR)==0){
                  //Limpia eeprom
                  for(EEPROM_ADDRESS i = 0; i < EEPROM_SIZE; i++){
                   if(read_ext_eeprom(i) != 0xFF){
                     write_ext_eeprom(i,0xFF);
                   }
                  }
                  fprintf(uart3,"$%li,OK$",id_n);//Confirmo el final
                }
                else if(strcmp(ord,getp)==0){ //GET
                    if(strcmp(param,time)==0){//Get Time
                        get_time();
                        fprintf(uart3,"$%li,%s,20%02i-%02i-%02i %02i:%02i:%02i$",id_n,param,yr,month,day,hrs,min,sec);
                    }
                    else if(strcmp(param,minp)==0){
                        fprintf(uart3,"$%li,%s,%li$",id_n,param,minutes);
                    }
                    else if(strcmp(param,imeip)==0){
                        // IMEI Value
                        fprintf(uart3, "$%li,%s,%s$", id_n,param,IMEI);
                    }
                }
                else if(strcmp(ord,setp)==0){ //SET
                    if(strcmp(param,time)==0){//Set Time
                        //value: 20xx-xx-xx xx:xx:xx
                        int d,m,y,h,mi,s;
                        char dc[3],mc[3],yc[3],hc[3],mic[3],sc[3];
                        char *ptr2, delm2[]="-: ";
                        ptr2 = strtok(value,delm2);//20xx
                        strcpy(yc,ptr2+2);//Y: xx
                        ptr2=strtok(NULL,delm2);//M:xx
                        strcpy(mc,ptr2);
                        ptr2=strtok(NULL,delm2);//D:xx
                        strcpy(dc,ptr2);
                        ptr2=strtok(NULL,delm2);//H:xx
                        strcpy(hc,ptr2);
                        ptr2=strtok(NULL,delm2);//Mi:xx
                        strcpy(mic,ptr2);
                        ptr2=strtok(NULL,delm2);//S:xx
                        strcpy(sc,ptr2);
                        d=atoi(dc);
                        m=atoi(mc);
                        y=atoi(yc);
                        h=atoi(hc);
                        mi=atoi(mic);
                        s=atoi(sc);
                        set_time(d,m,y,0,h,mi,s);
                        get_time();
                        fprintf(uart3,"$%li,%s,20%02i-%02i-%02i %02i:%02i:%02i$",id_n,param,yr,month,day,hrs,min,sec);
                    }
                    else if(strcmp(param,minp)==0){
                        //Value tiene los minutos en string
                        minutes = atoi(value);
                        set_laps();
                        save_minutes();
                        fprintf(uart3,"$%li,%s,%li$",id_n,param,minutes);
                    }
                    else if(strcmp(param,imeip)==0){
                        //Value tiene el imei
                        strcpy(IMEI,value);
                        write_imei();
                        fprintf(uart3, "$%li,%s,%s$", id_n,param,IMEI);
                    }
                }
            }
            else if(id_n==254){//Arduino
                char msg[50];
                if(strcmp(ord,getp)==0){//Get
                    sprintf(msg,"&%s,%s&",ord,param);//(EX:&GET,IP&)
                    puts(msg,uart1);
                }
                else if(strcmp(ord,setp)==0){//Set
                    sprintf(msg,"&%s,%s,%s&",ord,param,value);//(EX:&SET,IP,192.168.10.0&)
                    puts(msg,uart1);
                }
            }
            else{//Nodos
                //A los nodos le mando directamente lo que recibi y espero su respuesta
                fprintf(uart2,mens);
                //puts(mens,uart2);
                int32 timeout=0;
                //Vamos a tener que esperar a la interrupcion stringCompleta2 o timeout
                while(StringCompleta2!=1 && timeout<50000){timeout++;delay_us(10);}//0.5
                if(StringCompleta2==1){ //Salio por respuesta
                    fprintf(uart3,"%s",ResponseBuff2);
                }
                else{//Salio por TO
                    fprintf(uart3,"$NO$");
                }
                StringCompleta2=0;
                ResponseBuff2="";
                index2 = 0;
            }
            stringCompleta3=0;
            index=0;
            ResponseBuff3="";
            ord="";
            param="";
            value="";
        }

        if(stringCompleta4==1){//Respuesta Arduino comando
            //$ID,PARAM,VALUE$
            char delemA[]="&,", *ptr, param[3],value[20];
            ptr = strtok(ResponseBuff,delemA);//Param
            strcpy(param,ptr);
            ptr = strtok(NULL,delemA);//Value
            strcpy(value,ptr);
            fprintf(uart3,"$254,%s,%s$",param,value);
            stringCompleta4=0;
            ResponseBuff="";
            index=0;
        }
}
/*-----End Functions------*/
void main()
{
    setup();
    output_high(LED);
    while(count_f < 15 || ncfg_f != 0){
      inspeccionarUSB();
      if(dbg_a == 1){
         break;
      }
    }
    w_nextaddr = findPlace();
    restore_from_eeprom(); // Levanto paquetes que hubiera pendientes
    encolar_paquete();

    WHILE(true)
    {
        count_f = 0;
        inspeccionarUSB();

        if(count_p >= 4){ //Cada 35 seg intenta enviar de la cola
            count_p=0;
            if(dbg_a == 1){
               fprintf(uart3,"Paquetes actuales: %li \r\nPaquetes atrasados: %li\r\n",cola_p->largo,cola_o->largo);
            }
            if(cola_p->largo > 0){
              enviar_paquetes();
            }
            else{
              enviar_paquetes_o();
            }
         }

         if(count >= passes){//Cada X minutos encola un nuevo paquete
            count=0;
            encolar_paquete();
        }
        // Chequear cuantos paquetes no se pudieron enviar
        if(sin_rta >= 5){
            if(dbg_a == 1){
               if(ft == 0){
                  fprintf(uart3, "No se detecta conexion con el servidor. Se guardan los paquetes en la memoria interna. \r\n");
                  ft = 1;
               }
            }
            //Dejar un paquete en la cola, mandar el resto a la eeprom
            guardar_paquetes();
        }
        if(stringCompleta==1){//ACK
            int16 ack_n;
            char *ptr, sep[4]="$,=",no_rta[3]="NO",ackp[4]="ACK";
            char resp[50];
            strcpy(resp,ResponseBuff);
            ptr = strtok(ResponseBuff,sep);
            if (strcmp(ptr,no_rta)==0){ //Si Arduino devolvio un NO, reencolo
                if(dbg_a == 1){
                    fprintf(uart3,"");
                    fprintf(uart3,"Paquete no enviado\r\n");
                }
                sin_rta++;
                goto reencolar;
            }
            else{ //Si la respuesta es otra
                if(dbg_a == 1){
                    fprintf(uart3,"Respuesta: %s \r\n", resp);
                }
                while(strcmp(ptr,ackp)!=0 && ptr!=NULL){//Mientras no encuentre "ACK" y no se acabe la string
                    ptr = strtok(NULL, sep);
                }
                if(strcmp(ptr,ackp)==0){  //Salio por el ACK?
                    ptr = strtok(NULL, sep); //Numero paquete ACK
                    ack_n = atol(ptr);
                    if(ack_n == paquete_r->numero){ //ACK igual al paquete, todo OK
                        free_paquete_r();
                        paquete_r = NULL;
                        if(sin_rta >= 5){ //Hubo paquetes sin enviar por falta de conexion
                           restore_from_eeprom();
                        }
                        sin_rta=0;
                        ft = 0;
                        goto clear;
                    }
                    else{ //Paquete incorrecto
                        goto reencolar;
                    }
                }
                else{//Si no encontro ACK, no era el paquete o el arduino dijo NO
                    reencolar:
                    paquete_t* paqu = paquete_r;
                    if(desencole == 1){ //Si el que falla es viejo
                     encolar(cola_o,paqu);
                    }
                    else{ //Si lo saque de la cola nueva
                      if(cola_p->largo > 0){ //Si hay algo en la cola, es probable que este sea un paquete viejo
                         encolar(cola_o,paqu);
                     }
                     else{ //Sino, es el unico paquete que hay dando vueltas
                        encolar(cola_p,paqu);
                     }
                    }
                    paquete_r = NULL;
                }
            }
            clear:
            StringCompleta=0;
            ResponseBuff="";
            index = 0;
            if(sin_rta >= 5){
               guardar_paquetes();
            }
        }

        //Si llega algo no pedido del nodo. asumo alarma: pulleo y encolo
        if(stringCompleta2==1){
          encolar_paquete();
        }

        if(cola_o->largo == 0 && sin_rta < 5 && (count_p == 2 || count_p == 4 || count_p == 6)){
            restore_from_eeprom();
        }
    }
}
