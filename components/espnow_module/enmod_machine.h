#pragma once

/*

    Máquina de estados finitos.

    Existen estados predefinidos posibles de la máquina:
        ESPNOW_STATE_IDLE,
        ESPNOW_STATE_DISCOVERING,
        ESPNOW_STATE_CONNECTING,
        ESPNOW_STATE_RECONNECTING,
        ESPNOW_STATE_CONNECTED,
        ESPNOW_STATE_LINK_LOST,
    y a cada uno le corresponde un comportamiento y un tiempo de duración:

    ESPNOW_STATE_IDLE
        descripción: no está emparejado
        tiempo: indefinido. estado estable (sólo existe porque no hay un peer_mac_address configurado)
        comportamiento: idle
    
    ESPNOW_STATE_DISCOVERING
        descripción: está intentando emparejarse con un nuevo peer
        tiempo: definido al ejecutarse. al finalizar volverá a ESPNOW_STATE_IDLE
        comportamiento: intenta constantemente emparejarse con un nuevo peer
    
    ESPNOW_STATE_CONNECTING
        descripción: está intentando emparejarse con un nuevo peer
        tiempo: definido en configuración. puede finalizar exitosa o erronamente (pasará a CONNECTED o IDLE respectivamente)
        comportamiento: comunicación mutua para establecer, por primera vez, conexión encriptada con peer designado

    ESPNOW_STATE_RECONNECTING
        descripción: está volviendo a conectar con un peer preasignado
        tiempo: indefinido. intentará reconectarse permanentemente hasta que haya éxito o sea cancelado externamente
        comportamiento: intenta reconectar constantemente con peer preasignado
    
    ESPNOW_STATE_CONNECTED
        descripción: está concetado con un peer designado y corroborando constantemente la conexión
        tiempo: indefinido. estado estable
        comportamiento: ping-pong constante con peer designado para corroborar estado de la conexión

    ESPNOW_STATE_LINK_LOST
        descripción: perdió la conexión (previamente CONNECTED), intenta resolver constantemente la pérdida
        tiempo: indefinido. debe permanentemente intentar reconectar
        comportamiento: ídem RECONNECTING

*/

#include "enmod_basics.h"
#include "esp_now.h"

// static void enmod_machine_main_task(void);

// frames (private)
// static void enmod_machine_frame_IDLE(void);
// static void enmod_machine_frame_DISCOVERING(void);
// static void enmod_machine_frame_CONNECTING(void);
// static void enmod_machine_frame_RECONNECTING(void);
// static void enmod_machine_frame_CONNECTED(void);
// static void enmod_machine_frame_LINK_LOST(void);

// etc
void enmod_machine_recv_cb();
void enmod_machine_init();