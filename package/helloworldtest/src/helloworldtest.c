#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

 /* Sequence using the gpiodlib library 
     1. OPEN CHIP
     2. LINE SETTINGS: Configure lines as input, output with settings
     3. LINE CONFIG: Configure the Line and add one or more line settings
     4. REQUEST CONFIGURATION: Request configuration object from kernel
     5. REQUEST TO KERNEL: Request a set of lines for exclusive usage
     6. Read or set the value(s)
   https://libgpiod.readthedocs.io/en/latest/core_api.html  
*/


/* Buttons connected on nabby-linux board:
    BUTTON  PIN 	gpiochip0 number
    RIGHT   PE3     131
    UP      PE4     132
    DOWN    PE5     133
    LEFT    PE6     134
    A       PE12    140
    B       PE11    139
    C       PD15    111
*/

#define CHIP "/dev/gpiochip0"
#define PE3_RIGHT  131
#define PE4_UP     132
#define PE5_DOWN   133
#define PE6_LEFT   134
#define PE12_A     140
#define PE11_B     139
#define PD15_C     111


int main(void)
{
    printf("Hello [v11Apr26],  Reading digital inputs...\n");

    /* 1. OPEN CHIP */
    struct gpiod_chip *chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }
    printf("chip initilized\n");
        const char *version = gpiod_api_version();
        printf("API version: %s\n", version);
        struct gpiod_chip_info *info = gpiod_chip_get_info(chip);
        const char *name = gpiod_chip_info_get_name(info);
        printf("Chip name = %s\n", name);
        const char *label = gpiod_chip_info_get_label(info);
        size_t nrlines = gpiod_chip_info_get_num_lines(info);  
        printf("Chip label = %s   nr_lines=%d\n", label, nrlines);

    /* Line offset array */
    unsigned int offsets[] = { PE3_RIGHT, PE4_UP, PE5_DOWN, PE6_LEFT, PE12_A, PE11_B, PD15_C};

    /* LINE SETTINGS: Configure lines as input, output with settings */
    struct gpiod_line_settings *line_settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(line_settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(line_settings, GPIOD_LINE_EDGE_RISING); // added -2-

    /* 3. LINE CONFIG: Configure the Line and add one or more line settings*/
    struct gpiod_line_config *lineconfig = gpiod_line_config_new();
    int rslt = gpiod_line_config_add_line_settings(lineconfig,offsets,7,line_settings);
    if (rslt < 0) {
        perror("gpiod_line_add_line_settings");
        gpiod_chip_close(chip);
        return 1;
    }

    /* 4. REQUEST CONFIGURATION: Request configuration object from kernel */
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "input-reader");


    /* 5. REQUEST TO KERNEL: Request a set of lines for exclusive usage */
    struct gpiod_line_request *linerequest =
        gpiod_chip_request_lines(chip, req_cfg, lineconfig);
    if (!linerequest) {
        perror("gpiod_chip_request_lines");
        gpiod_chip_close(chip);
        return 1;
    }

    /* 6. Read or set the value(s) */
    enum gpiod_line_value DIvalues[7]; /* array to store input values read */

//    int value = gpiod_line_request_get_value(linerequest, PE3_RIGHT); /* read one line */
    int value = gpiod_line_request_get_values(linerequest, DIvalues);
    if (value < 0) {
        perror("gpiod_line_request_get_value");
        gpiod_chip_close(chip);
        return 1;
    }
 
    for (int i=0; i<7 ; i++) {
       printf("offsets[%d] = %d\n", i, DIvalues[i]); /* print all read lines */
    }

   /* Event loop */
    while (1) {
        struct gpiod_edge_event_buffer *buf =
            gpiod_edge_event_buffer_new(8);

        int ret = gpiod_line_request_read_edge_events(linerequest, buf, 8);
        if (ret < 0) {
            perror("gpiod_line_request_read_edge_events");
            break;
        }

        for (int i = 0; i < ret; i++) {
            const struct gpiod_edge_event *ev =
                gpiod_edge_event_buffer_get_event(buf, i);

            unsigned int line = gpiod_edge_event_get_line_offset(ev);
            enum gpiod_edge_event_type type =
                gpiod_edge_event_get_event_type(ev);

            const char *etype =
                (type == GPIOD_EDGE_EVENT_RISING_EDGE) ? "RISING" : "FALLING";

            printf("Line %u: %s edge\n", line, etype);
        }

        gpiod_edge_event_buffer_free(buf);
    }


    gpiod_line_request_release(linerequest);
    gpiod_chip_close(chip); // will release all resources.
    return 0;
}


