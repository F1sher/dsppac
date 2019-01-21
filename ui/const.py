import sys
from os import path


class _const:
    def __init__(self):
        self.PYTHON_EXEC = "python3"
        self.TXT_EDITOR = "gedit"

        self.STUFF_FOLDER = "./"
        self.MAIN_PROG_FOLDER = path.split(path.split(path.realpath(__file__))[0])[0] + "/" #should be smth like "/home/das/job/dsp/"
        print(self.MAIN_PROG_FOLDER)
        self.UI_FOLDER = "ui/"
        self.FPGA_PROG = "send_comm"
        
        self.HDRAINER_EXE = "hdrainer"
        self.ONLINE_EXE = "oconsumer"
        
        self.HISTO_FOLDER = path.join(self.MAIN_PROG_FOLDER, "test/spectra/")
        self.DEFAULT_CURR_HISTO_FOLDER = path.join(self.HISTO_FOLDER, "apr14_4det_porog=90_delay=100_coinc_10h_night/")

        self.SOCKET_COMMUNICATION_FILE = "./hidden"
        self.CFG_FILE = "cfg.json"
        self.CONSTANTS_FILE = "constants.json"

        self.PYPAC_PROG_FOLDER = "/home/das/job/pypac/"
        self.PYPAC_PROG_NAME = "rw.py"
        
        self.SLEEP_S_SOCK_READ = 1
        self.SLEEP_S_FIG_UPDATE = 110
        self.SLEEP_S_COUNTS_UPDATE = 1

        self.HISTO_FILE_NAMES = ["BUFKA1.SPK", "BUFKA2.SPK", "BUFKA3.SPK", "BUFKA4.SPK", "TIME1.SPK", "TIME2.SPK", "TIME3.SPK", "TIME4.SPK", "TIME5.SPK", "TIME6.SPK", "TIME7.SPK", "TIME8.SPK", "TIME9.SPK", "TIME10.SPK", "TIME11.SPK", "TIME12.SPK"]
        
        self.HIST_SIZE = 4096
        self.DET_NUM = 4

        self.EPS = 1e-10


    class ConstError(TypeError): pass

    def __setattr__(self, name, value):
        if name in self.__dict__:
            raise self.ConstError
        self.__dict__[name] = value


sys.modules[__name__] = _const()
