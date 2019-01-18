import const

def save_consts_cfg(path_to_save):
    #read const file
    with open(const.MAIN_PROG_FOLDER + const.CONSTANTS_FILE, 'r') as const_file:
        constants = const_file.read()
        
    #read cfg file
    with open(const.MAIN_PROG_FOLDER + const.CFG_FILE, 'r') as cfg_file:
        config = cfg_file.read()
        
    #save const file in histo folder
    with open(path_to_save + '/' + const.CONSTANTS_FILE, 'w+') as histo_folder_const_file:
        histo_folder_const_file.write(constants)

    #save cfg file in histo folder
    with open(path_to_save + '/' + const.CFG_FILE, 'w+') as histo_folder_cfg_file:
        histo_folder_cfg_file.write(config)
    #rte


if __name__ == "__main__":
    path_to_save = "/home/das/job/test"
    save_consts_cfg(path_to_save)
