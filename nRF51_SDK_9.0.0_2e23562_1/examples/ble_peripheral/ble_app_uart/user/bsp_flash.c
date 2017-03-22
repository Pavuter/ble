#include "bsp_flash.h"

#include <stdbool.h>
#include <stdio.h>

#include "nrf.h"
#include "nordic_common.h"
#include "nrf_delay.h"


static void flash_page_erase(uint32_t * page_address);
static void flash_word_write(uint32_t * address, uint32_t value);

/** @brief Function for erasing a page in flash.
 *
 * @param page_address Address of the first word in the page to be erased.
 */
static void flash_page_erase(uint32_t * page_address)
{
    // Turn on flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Erase page:
    NRF_NVMC->ERASEPAGE = (uint32_t)page_address;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Turn off flash erase enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}


/** @brief Function for filling a page in flash with a value.
 *
 * @param[in] address Address of the first word in the page to be filled.
 * @param[in] value Value to be written to flash.
 */
static void flash_word_write(uint32_t * address, uint32_t value)
{
    // Turn on flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    *address = value;

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }

    // Turn off flash write enable and wait until the NVMC is ready:
    NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);

    while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
    {
        // Do nothing.
    }
}

void nrf_flash_test(void)
{
    uint32_t * addr;
    uint32_t    patwr;
    uint32_t    patrd;
    //uint8_t    patold;
    uint32_t   i;
    uint32_t   pg_size;
    uint32_t   pg_num;
    
    printf("Flashwrite example\r\n\r");
    //patold  = 0;
    pg_size = NRF_FICR->CODEPAGESIZE;
    pg_num  = NRF_FICR->CODESIZE - 1;  // Use last page in flash

    //while (true)
    {
        // Start address:
        addr = (uint32_t *)(pg_size * pg_num);
        // Erase page:
        flash_page_erase(addr);
        i = 0;
        printf("Flashwrite erase finished.\r\n");
        do
        {
            //printf("Enter char to write to flash.\r\n");
            // Read char from uart, and write it to flash:
            patwr = 0x11223344;
            //if (patold != patwr)
            {
                //patold = patwr;
                flash_word_write(addr++, (uint32_t)patwr);
                i += 4;
                //printf("'%x' - ", patwr);
            }
        }
        while (i < pg_size);
        printf("Flashwrite write finished.\r\n");
        i = 0;
        addr = (uint32_t *)(pg_size * pg_num);
        do
        {
            // Read pattern from flash and send it back:
            patrd = (uint32_t)*(addr + i);
            i += 1;
            printf("%x\r\n", patrd);
            nrf_delay_ms(5);
            
        }while( i < pg_size/4);
        
        printf("Flashwrite read finish.\r\n");
    }
}


