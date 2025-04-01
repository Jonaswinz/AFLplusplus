/*
 * Copyright (C) 2025 ICE RWTH-Aachen
 *
 * This file is part of AFL++ VP-Mode.
 *
 * AFL++ VP-Mode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * AFL++ VP-Mode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AFL++ VP-Mode. If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdint.h>

#define IO_BASE 0x10009000  // Base address for ARM PL011 UART

#define U_RX_START   0x00 
#define U_TX_START   0x08 
#define U_TX_SEND    0x51c
#define U_RX_RECEIVE 0x518
#define U_RXDRDY     0x108

#define UART_RX_START()      (*((volatile unsigned char *)(IO_BASE+U_RX_START)))
#define UART_TX_START()      (*((volatile unsigned char *)(IO_BASE+U_TX_START)))

#define UART_SEND(symb)      {*((volatile unsigned char *)(IO_BASE+U_TX_START)) = (1); *((volatile unsigned char *)(IO_BASE+U_TX_SEND)) = (symb); *((volatile unsigned char *)(IO_BASE+U_TX_START)) = (0);}
#define UART_RECEIVE()   (*((volatile unsigned char *)(IO_BASE+U_RX_RECEIVE)))
#define UART_RXDRDY()      (*((volatile uint32_t *)(IO_BASE+U_RXDRDY)))

#define MAX 10

extern "C" void exit(int exit_code){
    *((int*) 0x10008000) = 1; // stop the simulation using the simdev
    while(1){}
}

int my_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char *)str1 - *(const unsigned char *)str2;
}

void main()
{
    int i = 0;
    char str[] = "pass";
    char read_str[MAX];
    char read_c;

    do{
        read_c = UART_RECEIVE();
        read_str[i]=read_c;
        i++;

    } while(read_c!='\n' && read_c!='\0' && i<MAX);

    read_str[i-1] = 0;
    
    if(!my_strcmp(str, read_str)) {
        exit(1);
    }else{
        exit(0);
    }
}