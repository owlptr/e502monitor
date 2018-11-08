// For work with Lcard E-502 device
#include "e502api.h"
// --------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TCP_CONNECTION_TOUT 5000


/*
    Get all Lcard E-502 devices connected via USB 
    or Ethernet interfaces

    Return number of found debices 
*/
uint32_t get_all_devrec(t_x502_devrec **pdevrec_list,
                        uint32_t *ip_addr_list,
                        unsigned ip_cnt)
{
    // number of found devices
    int32_t fnd_devcnt = 0;
    // number of found devices connected via USB interface
    uint32_t usb_devcnt = 0;

    t_x502_devrec *rec_list = NULL;

    // getting count of connected devices via USB interface
    E502_UsbGetDevRecordsList(NULL, 0, 0, &usb_devcnt);

    if(usb_devcnt + ip_cnt == 0){ return 0; }

    // allocate memory for the array to save the number of records found
    rec_list = malloc((usb_devcnt + ip_cnt) * sizeof(t_x502_devrec));
    
    // if memory wasn't allocated
    if(rec_list == NULL){ return 0; }

    if (usb_devcnt!=0) 
    {
        int32_t res = E502_UsbGetDevRecordsList(&rec_list[fnd_devcnt],
                                                usb_devcnt, 
                                                0, 
                                                NULL);
        if (res >= 0) { fnd_devcnt += res; }
    }
    
    for (int i=0; i < ip_cnt; i++) 
    {
        if (E502_MakeDevRecordByIpAddr(&rec_list[fnd_devcnt],
                                       ip_addr_list[i],
                                       0,
                                       TCP_CONNECTION_TOUT)
            == X502_ERR_OK // E502_MakeDevRecordByIpAddr(...) == X502_ERR_OK 
            ) 
        {
            fnd_devcnt++;
        }
    }

    // if were some mistake and no one modele
    // was getting
    if(fnd_devcnt == 0)
    {
        free(rec_list);
        return 0;
    }
    // --------------------------------------

    *pdevrec_list = rec_list;

    return fnd_devcnt;
}

/*
    Create connecting for selecting device.

    Return special handler (t_x502_hnd)
*/
t_x502_hnd open_device(t_x502_devrec *devrec_list,
                        uint32_t device_id)
{
    t_x502_hnd hnd = X502_Create();

    if (hnd==NULL) 
    {
        fprintf(stderr, "Error creating module handle!");
        return hnd;
    }
    
    /* устанавливаем связь с модулем по записи */
    int32_t err = X502_OpenByDevRecord(hnd, &devrec_list[device_id]);
    if (err != X502_ERR_OK) 
    {
        fprintf(stderr,
                "Error establishing communication with the module: %s!",
                X502_GetErrorString(err));

        X502_Free(hnd);
        hnd = NULL;
        return hnd;
    }
}

int main(int argc, char** argv) 
{

    uint32_t *ip_addr_list = NULL;
    uint32_t ip_cnt = 0;
    uint32_t fnd_devcnt = 0;

    t_x502_devrec *devrec_list = NULL;

    fnd_devcnt = get_all_devrec(&devrec_list, ip_addr_list, ip_cnt);

    if (fnd_devcnt == 0) {
        printf("No one module was found. Exit.\n");
        
        //exit from program
        return 0;
    }
    
    printf("Available modules:\n");
    for (int i=0; i < fnd_devcnt; i++) {
        printf("Module № %d: %s, %-9s", i, devrec_list[i].devname,
               devrec_list[i].iface == X502_IFACE_PCI ? "PCI/PCIe" :
               devrec_list[i].iface == X502_IFACE_USB ? "USB" :
               devrec_list[i].iface == X502_IFACE_ETH ? "Ethernet" : "?");
        if (devrec_list[i].iface != X502_IFACE_ETH) {
            printf("Serial number: %s\n", devrec_list[i].serial);
        } else {
            printf("Adress: %s\n", devrec_list[i].location);
        }
    }

    printf("Select number of module (from 0 to %d)\n", fnd_devcnt-1);
    fflush(stdout);

    uint32_t device_id;

    scanf("%d", &device_id);

    if( device_id < 0 || device_id >= fnd_devcnt)
    {
        printf("\nInvalid module number! Exit.");

        // Free memory
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);
        // --------------------------------------------

        return 0; // exit from program
    }

    t_x502_hnd hnd = open_device(devrec_list, device_id);

    if(hnd == NULL)
    {
        // Free memory
        X502_FreeDevRecordList(devrec_list, fnd_devcnt);
        free(devrec_list);
        // --------------------------------------------

        return 0;
    }

    return 0;
}


