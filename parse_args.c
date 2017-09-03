#include "parse_args.h"

static const char *out_foldername_default = "/home/das/job/dsp/test/histos";

const char *parse_and_give_comm(int argc, char **argv, cyusb_handle *usb_h, int *with_signal_flag, unsigned int *time, int en_range[][4])
{
	int i, x = 0;
    int res = 0;
	int option = 0;
	const char *out_foldername = out_foldername_default;
	unsigned int p[4] = {100, 100, 100, 100};
	char porogs[512];
	char energy_range[512];
	char *range = NULL;
	strncpy(porogs, "100 100 100 100", 512);
	char *token = NULL;
	int delay = 100;

	if (argc == 1) {
		*time = 100;

		return out_foldername;
	}

	while ( (option = getopt (argc, argv, "t:o:p:d:e:rncs") ) != -1) {
		switch (option) {
		case 't':
			//set time of measurements
			*time = atoi(optarg);

			break;
		case 'o':
			//select foldername for output
			out_foldername = optarg;
		
			break;
		case 'p':
			//set porogs
			strncpy(porogs, optarg, 512);
            
            i = 0;
            token = strtok(porogs, " ");
            while (token != NULL) {
                p[i++] = atoi(token);
                token = strtok(NULL, " ");
            }
			
            usleep(100);
			res = control_send_comm(usb_h, "s0", p[0]);
            
            if (res != 0) {
                return NULL;
            }
            
            usleep(100);
			res = control_send_comm(usb_h, "s1", p[1]);
            
            if (res != 0) {
                return NULL;
            }
          
			usleep(100);
            res = control_send_comm(usb_h, "s2", p[2]);
          
            if (res != 0) {
                return NULL;
            }
            
			usleep(100);
            res = control_send_comm(usb_h, "s3", p[3]);
            
            if (res != 0) {
                return NULL;
            }
            
			break;
		case 'd':
			//set delay
			delay = atoi(optarg);
            
            usleep(100);
			res = control_send_comm(usb_h, "w", delay);
            
            if (res != 0) {
                return NULL;
            }
            
            /*
            res = control_send_comm(usb_h, "r", 0);
            if (res != 0) {
                return NULL;
            }
            */
			break;
		case 'e':
			//set energy_range
			strncpy(energy_range, optarg, 512);

            range = strtok(energy_range, " ");
			i = 0;
			while (range != NULL) {
				if (i == 0) {
					sscanf(range, "[%d,", &x);
				}
				else if (i % 4 == 0) {
					sscanf(range, "[%d", &x);
				}
				else {
					x = atoi(range);
				}
				
				en_range[(int)(i/4)][i-4*(int)(i/4)] = x;

				i++;
				range = strtok(NULL, "], ");
			}

		case 'r':
			//reset
			usleep(100);
            res = control_send_comm(usb_h, "r", 0);
            
			if (res != 0) {
                return NULL;
            }
            
            break;
		case 'n':
			//set no-coinc mode
            usleep(100);
			res = control_send_comm(usb_h, "n", 0);
            
			if (res != 0) {
                return NULL;
            }
            
			break;
        case 'c':
            //set coinc mode
            usleep(100);
			res = control_send_comm(usb_h, "c", 0);
            
			if (res != 0) {
                return NULL;
            }
            
            break;
		case 's':
			//set signal mode acqusition
			*with_signal_flag = 1;

			break;
		default:
			return NULL;
		}
	}

	return out_foldername;
}


#define SET_PARAM_INT(param) do { strncpy(temp_str, const_data + t[i+1].start, t[i+1].end - t[i+1].start); const_params->param = atoi(temp_str); strncpy(temp_str, "", 512); } while(0)
#define SET_PARAM_FLOAT(param) do { strncpy(temp_str, const_data + t[i+1].start, t[i+1].end - t[i+1].start); const_params->param = atof(temp_str); strncpy(temp_str, "", 512); } while(0)

static int json_equal(const char *json, jsmntok_t *tok, const char *s) 
{
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start && \
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		
		return 0;
	}

	return -1;
}

int const_parser(const char *const_filename, const_t *const_params)
{
	int i, j;
	int res = 0;
	int num_items = 0;
	FILE *in = NULL;
	long int file_size = 0;
	char temp_str[512];

	if (const_filename == NULL) {
		fprintf(stderr, "Error with const_filename\n");
		 	
		return -1;
	}

	in = fopen(const_filename, "r");
	if (in == NULL) {
		perror("Error with open const file");

		return -1;
	}

	fseek(in, 0L, SEEK_END);
	file_size = ftell(in);
	rewind(in);

	if (file_size > 2048) {
		fprintf(stderr, "The const file seems too large\n");
		fclose(in);

		return -1;
	}
	char const_data[2048] = {0};
	
	res = fread(const_data, sizeof(char), file_size, in);
	if (res != (int) sizeof(char)*file_size) {
		fprintf(stderr, "Error while reading const file | read = %d bytes but should be %ld bytes\n", res, file_size);
		fclose(in);

		return -1;
	}

	jsmn_parser p;
	jsmntok_t t[2048]; /* We expect no more than 2048 tokens */
	jsmntok_t *g;

	jsmn_init(&p);
	res = jsmn_parse(&p, const_data, file_size, t, sizeof(t)/sizeof(t[0]));
	if (res < 0) {
		fprintf(stderr, "Failed to parse JSON const file: %d\n", res);

		return -1;
	}

	/* Assume the top-level element is an object */
	if (res < 1 || t[0].type != JSMN_OBJECT) {
		fprintf(stderr, "Object expected in const file as first item\n");
		
		return -1;
	}

#ifdef DEBUG
	printf("const_data = %s\n", const_data);
#endif

	num_items = res;
	for (i = 1; i < num_items; i++) {
		//printf("i = %d | cfg_data = %s\n", i, cfg_data);
        if (json_equal(const_data, &t[i], "EN_normal") == 0) {
            SET_PARAM_FLOAT(EN_normal);
            i++;
        }
        else if (json_equal(const_data, &t[i], "K_trap") == 0) {
            SET_PARAM_INT(K_trap);
            i++;
        }
        else if (json_equal(const_data, &t[i], "L_trap") == 0) {
            SET_PARAM_INT(L_trap);
            i++;
        }
        else if (json_equal(const_data, &t[i], "Tau_trap") == 0) {
            SET_PARAM_FLOAT(Tau_trap);
            i++;
        }
		else if (json_equal(const_data, &t[i], "AVERAGE_trap") == 0) {
            SET_PARAM_INT(AVERAGE_trap);
            i++;
        }
		else if (json_equal(const_data, &t[i], "I_MIN_S_SHIFT_trap") == 0) {
            SET_PARAM_INT(I_MIN_S_SHIFT_trap);
            i++;
        }
        else if (json_equal(const_data, &t[i], "INTEGRAL_steps_back") == 0) {
            SET_PARAM_INT(INTEGRAL_steps_back);
            i++;
        }
        else if (json_equal(const_data, &t[i], "INTEGRAL_steps_forw") == 0) {
            SET_PARAM_INT(INTEGRAL_steps_forw);
            i++;
        }
        else if (json_equal(const_data, &t[i], "CFT_fraction") == 0) {
            SET_PARAM_FLOAT(CFT_fraction);
            i++;
        }
        else if (json_equal(const_data, &t[i], "T_SCALE") == 0) {
            if (t[i+1].type != JSMN_ARRAY) {
                fprintf(stderr, "T_SCALE expected to be an array of 2 real numbers\n");
                
                continue;
            }
        
            for (j = 0; j < t[i+1].size; j++) {
                g = &t[i+j+2];
                strncpy(temp_str, const_data + g->start, g->end - g->start);
                (const_params->T_SCALE)[j] = atof(temp_str);
                strncpy(temp_str, "", 512);
            }
            i += t[i+1].size + 1;
        }
        else {
			fprintf(stderr, "Unexpected parameter in const: %.*s\n", t[i].end - t[i].start, \
					const_data + t[i].start);
		}
	}

	fclose(in);

	return 0;
}


void set_const_params(const_t const_params)
{
	EN_normal = HIST_SIZE/const_params.EN_normal;
	K_trap = const_params.K_trap;
	L_trap = const_params.L_trap;
	Tau_trap = const_params.Tau_trap;

	AVERAGE_trap = const_params.AVERAGE_trap;
	I_MIN_S_SHIFT_trap = const_params.I_MIN_S_SHIFT_trap;

	INTEGRAL_steps_back = const_params.INTEGRAL_steps_back;
	INTEGRAL_steps_forw = const_params.INTEGRAL_steps_forw;

	CFT_fraction = const_params.CFT_fraction;
	T_SCALE[0] = const_params.T_SCALE[0];
	T_SCALE[1] = const_params.T_SCALE[1];
}
