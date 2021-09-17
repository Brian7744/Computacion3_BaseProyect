#include "mbed.h"
#include <stdlib.h>


/**
 * @brief Enumeración que contiene los estados de la máquina de estados(MEF) que se implementa para 
 * el "debounce" 
 * El botón está configurado como PullUp, establece un valor lógico 1 cuando no se presiona
 * y cambia a valor lógico 0 cuando es presionado.
 */
typedef enum{
    BUTTON_DOWN,        //0
    BUTTON_UP,          //1
    BUTTON_FALLING,     //2
    BUTTON_RISING       //3
}_eButtonState;

/**
 * @brief Estructura para los pulsadores
 * 
 */
typedef struct{
    int pos;
    uint8_t estado;
    int32_t timeDown;
    int32_t timeDiff;
}_sTeclas;

#define TRUE    1
#define FALSE    0
#define VALINICIAL    0

/**
 * @brief Valor decimal para encender ese led especifico
 * 
 */
#define TOPOUNO 4

/**
 * @brief Valor decimal para encender ese led especifico
 * 
 */
#define TOPODOS 2

/**
 * @brief Valor decimal para encender ese led especifico
 * 
 */
#define TOPOTRES 8

/**
 * @brief Valor decimal para encender ese led especifico
 * 
 */
#define TOPOCUATRO 1 

/**
 * @brief Para que el/los leds destellen 3 veces estos deben pasar por 6 estados
 * 
 */
#define LEDSTATES    6
/**
 * @brief Valor de tiempo maximo para los tiempos random
 * 
 */
#define TIMEMAX    1501
/**
 * @brief Valor de tiempo minimo para los tiempos random
 * 
 */
#define BASETIME   100

/**
 * @brief Condicion para que inicie el juego
 * 
 */
#define TIMETOSTART    1000

/**
 * @brief Defino el intervalo entre lecturas para "filtrar" el ruido del Botón
 * 
 */
#define INTERVAL    40

/**
 * @brief PARPADEO es el intervalo donde el led de usuario cambia de estado
 * 
 */
#define PARPADEO    1000

/**
 * @brief Intervalo donde cambian de estado los led
 *   
 */
#define MEDIOSEG    500

/**
 * @brief numeros de botones como entrada
 * 
 */
#define NROBOTONES    4

/**
 * @brief Cantidad de Leds en la salida
 * 
 */
#define MAXLED    4  

/**
 * @brief Primer estado de la Maquina. En espera de una accion
 *  En espera de que el usuario precione un pulsador por 1000 ms para iniciar el juego
 */
#define STANDBY    0

/**
 * @brief Segundo estado de la Maquina.
 *  Controla que los botones no esten precionados cuando este listo indicara que comienza el juego 
 */
#define TECLAS    1

/**
 * @brief Tercer estado de la Maquina. Comienza el juego
 * Enciende un led cualquiera en un momento cualquiera de un intervalo definido
 */
#define TOPO    2

/**
 * @brief Cuarto estado de la Maquina. Evalua la entrada.
 * Si es correcto el pulsador con repecto al led encendido pasa al estado GANO de lo contrario a cambia TOPOF
 */
#define EVA    3

/**
 * @brief Quinto estado de la Maquina. Gano el juego
 * Los cuatro led se deben encender en simultáneo y destellar 3 veces y pasa a ENDGAME
 */
#define GANO    4

/**
 * @brief Sexto estado de la Maquina. El usuario perdio
 * El topo(LED) hace su festejo y la Maquina pasa al estado ENDGAME
 */
#define TOPOF    5

/**
 * @brief Septimo estado de la maquina. Fin del juego
 * Se reinician algunas variables, se pasa al estado STANDBY 
 */
#define ENDGAME    6

/**
 * @brief Inicializa la MEF
 * Le dá un estado inicial a myButton
 */
void startMef(uint8_t indice);

/**
 * @brief Máquina de Estados Finitos(MEF)
 * Actualiza el estado del botón cada vez que se invoca.
 * 
 * @param buttonState Este parámetro le pasa a la MEF el estado del botón.
 */
void actuallizaMef(uint8_t indice );

/**
 * @brief Función para cambiar el estado del LED cada vez que sea llamada.
 * 
 */
void togleLed(uint8_t indice);




/**
 * @brief Dato del tipo Enum para asignar los estados de la MEF
 * 
 */
_eButtonState myButton;

_sTeclas ourButton[NROBOTONES];

uint16_t mask[]={0x0001,0x0002,0x0004,0x0008};

DigitalOut LED(PC_13); //!< Defino la salida del led
BusIn botones(PB_6,PB_7,PB_8,PB_9);
BusOut leds(PB_12,PB_13,PB_14,PB_15);


Timer miTimer; //!< Timer para hacer la espera de 40 milisegundos

int tiempoMs=VALINICIAL; //!< variable donde voy a almacenar el tiempo del timer una vez cumplido

uint8_t estado=STANDBY; //!< Variable que funciona para moverme entre los distintos estados de la maquina

unsigned int ultimo=VALINICIAL;  //!< variable para el control del heartbeats. guarda un valor de tiempo en ms

uint16_t ledAuxRandom=VALINICIAL;   //!< selecciona uno de los 4 led para encender


int main()
{
    miTimer.start();

    int tiempoRandom = VALINICIAL; //!< Valor de referencia para la comparacion
    int ledAuxRandomTime = VALINICIAL; //!<Determina cuanto tiempo estara encendido el led
    int ledAuxJuegoStart = VALINICIAL; 
    int ledEncenderTime = VALINICIAL;
    int timeOfIncert = VALINICIAL; //!<tiempo de incertidumbre, osea, tiempo antes que el topo salga
    int checkPull= VALINICIAL;
    int acumTime = VALINICIAL;
    int posVector;  //!<guarda el pulsador que se activo
    
    uint8_t indiceAux = VALINICIAL;
    uint8_t varControl=FALSE;   //!< esta variable funciona como una bandera solo cuando tenga un led para encender cambiara a true
    uint8_t acum=1;
    uint8_t ledOn;  //!< Topo fuera de la madrigera
 
    LED=FALSE; //!< Con esta linea el led de la placa comienza el programa apagado

    for(uint8_t indice=0; indice<NROBOTONES;indice++){
        startMef(indice);
        ourButton[indice].pos = indice;
    }

    while(TRUE)
    {
        if((miTimer.read_ms()-ultimo)>PARPADEO){
           ultimo=miTimer.read_ms();
           LED=!LED;
        }  

        switch(estado){
            case STANDBY:
                if ((miTimer.read_ms()-tiempoMs)>INTERVAL){
                    tiempoMs=miTimer.read_ms();
                    for(uint8_t indice=0; indice<NROBOTONES;indice++){
                        actuallizaMef(indice);
                        if(ourButton[indice].timeDiff >= TIMETOSTART){
                            srand(miTimer.read_us());
                            estado=TECLAS;
                               
                        }
                    }
                }
            break;
            case TECLAS:
                for( indiceAux=0; indiceAux<NROBOTONES;indiceAux++){
                    actuallizaMef(indiceAux);
                    if(ourButton[indiceAux].estado!=BUTTON_UP){
                        break;
                    }
                        
                }
                if(indiceAux==NROBOTONES){
                    estado=TOPO;
                    leds=15;
                    ledAuxJuegoStart=miTimer.read_ms();
                }
            break;
            case TOPO:

                if(leds==0){
                    
                    ledAuxRandom = rand() % (MAXLED);   
                    ledAuxRandomTime = (rand() % (TIMEMAX))+BASETIME; 
                    tiempoRandom=miTimer.read_ms();
                    timeOfIncert= (rand() % (TIMEMAX))+700;
                    
                    
                    if((miTimer.read_ms()- ledEncenderTime)>timeOfIncert){
                        ledEncenderTime=miTimer.read_ms();
                        leds=mask[ledAuxRandom];
                    }

                    varControl=TRUE;
                }else{
                    if((miTimer.read_ms()- ledAuxJuegoStart)> TIMETOSTART) {
                        if(leds==15){
                            ledAuxJuegoStart=miTimer.read_ms();
                            leds=0;
                        }
                    }
                }
               
                if (((miTimer.read_ms()-tiempoRandom)>ledAuxRandomTime)&&(varControl)){
                    leds=0;
                }
                
                if((leds!=0)&&(leds!=15)){
                    if((miTimer.read_ms()-checkPull)>INTERVAL){
                        checkPull=miTimer.read_ms();
                        for(uint8_t indice=0; indice<NROBOTONES;indice++){
                            actuallizaMef(indice);
                            if(ourButton[indice].estado==BUTTON_DOWN){
                                estado=EVA;
                                posVector=ourButton[indice].pos;
                                ledOn=ledAuxRandom;
                            }
                        }                            
                    }
                }

                

            break;
            case EVA:
                if(ledOn==(posVector)){
                    estado=GANO;  
                }else{
                    estado=TOPOF;
                }
            break;
            case TOPOF:
                if((miTimer.read_ms()-acumTime)>MEDIOSEG){
                    acumTime = miTimer.read_ms();
                    
                    if((acum%2)==0){
                        leds=0;
                    }else{
                        switch (ledOn)
                        {
                        case 1:
                                leds = TOPODOS;    
                            break;
                        case 2:
                                leds = TOPOUNO;      
                            break;
                        case 3:
                                leds = TOPOTRES;    
                            break;
                        case 0:
                                leds = TOPOCUATRO;  
                            break;
                        default:
                            break;
                        }
                        
                    }
                    if(acum==LEDSTATES){
                        estado=ENDGAME;
                        acum=0;
                    }
                    acum++;
                }   
            break;
            case GANO:
                
                if((miTimer.read_ms()-acumTime)>MEDIOSEG){
                    acumTime = miTimer.read_ms();
                    
                    if((acum%2)==0){
                        leds=0;
                    }else{
                        leds=15;
                    }
                    if(acum==LEDSTATES){
                        estado=ENDGAME;
                        acum=0;
                    }
                    acum++;
                }
                   
            break;
            
            case ENDGAME:
                
                estado=STANDBY;
                varControl=FALSE;
                tiempoMs = VALINICIAL;
                tiempoRandom = VALINICIAL; 
                ledAuxRandomTime = VALINICIAL; 
                ledAuxJuegoStart = VALINICIAL; 
                ledEncenderTime = VALINICIAL;
                indiceAux = VALINICIAL;
                checkPull = VALINICIAL;
                acumTime = VALINICIAL;
                acum=1;
                
                for(int j=0; j<NROBOTONES;j++){
                    startMef(j);
                    ourButton[j].timeDiff=1;
                    ourButton[j].timeDown=1;
                }

            break;

            default:
                estado=STANDBY;
        }

    }
    return 0;
}



void startMef(uint8_t indice){
   ourButton[indice].estado=BUTTON_UP;   
}

void actuallizaMef(uint8_t indice){

    switch (ourButton[indice].estado)
    {
    case BUTTON_DOWN:
        if(botones.read() & mask[indice] )
           ourButton[indice].estado=BUTTON_RISING;
    
    break;
    case BUTTON_UP:
        if(!(botones.read() & mask[indice]))
            ourButton[indice].estado=BUTTON_FALLING;
    
    break;
    case BUTTON_FALLING:
        if(!(botones.read() & mask[indice]))
        {
            ourButton[indice].timeDown=miTimer.read_ms();
            ourButton[indice].estado=BUTTON_DOWN;
            //Flanco de bajada
        }
        else
            ourButton[indice].estado=BUTTON_UP;    

    break;
    case BUTTON_RISING:
        if(botones.read() & mask[indice]){
            ourButton[indice].estado=BUTTON_UP;
            //Flanco de Subida
            ourButton[indice].timeDiff=miTimer.read_ms()-ourButton[indice].timeDown;

        }

        else
            ourButton[indice].estado=BUTTON_DOWN;
    
    break;
    
    default:
    startMef(indice);
        break;
    }
}

void togleLed(uint8_t indice){
   leds=mask[indice];
}
