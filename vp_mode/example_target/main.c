/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original Author: Shay Gal-on
*/

/* File: core_main.c
        This file contains the framework to acquire a block of memory, seed
   initial parameters, tun t he benchmark and report the results.
*/


#include <string.h>
#include <stdint.h>

#define MAX 10
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

extern void _start();

void exit(int exit_code){
    //*((int*) 0x10008000) = 1; // stop the simulation using the simdev
    _start();
}

void main()
{
    int i = 0;
    unsigned char str[] = "pass";
    unsigned char read_str[MAX];
    unsigned char read_c;

    UART_RX_START() = 1;
    do{
            //while(!UART_RXDRDY());
            //read the string
            read_c = UART_RECEIVE();
            
            read_str[i]=read_c;
            i++;

    } while(read_c!='\n' && read_c!='\0' && i<MAX);

    read_str[i-1] = 0;

   // UART_RX_START() = 0; TODO !=!?=!=!=!

    /*
    if(!strcmp(read_str, str)) 
        ee_printf("right password\n");
    else
        ee_printf("wrong password\n");
    */
    
    if(!strcmp(read_str, str)) {
        exit(1);
    }else{
        exit(0);
    }
}