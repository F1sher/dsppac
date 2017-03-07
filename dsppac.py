#!/usr/bin/env python3.4

import json
import gi
import os
import subprocess
import socket
import time
from gi.repository import Gtk
from signal import SIGUSR1
from struct import unpack
from struct import error as struct_error_unpack


PYTHON_EXEC = "python3.4"
STUFF_FOLDER = "./"
MAIN_PROG_FOLDER = "/home/das/job/dsp/"
FPGA_PROG = "send_comm"
HDRAINER_EXE = "hdrainer"
ONLINE_EXE = "oconsumer"
HISTO_FOLDER = "/home/das/job/dsp/test/histos"
SOCKET_COMMUNICATION_FILE = "./hidden"
CFG_FILE = "cfg.json"


class StartWin(Gtk.Window):
    
    def __init__(self):
        Gtk.Window.__init__(self, title="DspPAC", resizable=False)
        
        grid = Gtk.Grid()
        grid.set_column_homogeneous(False)
        self.add(grid)

        self.prog = HDRAINER_EXE
        self.porog = [90, 90, 90, 90]
        self.delay = 100
        self.coinc = 1
        self.time = 10
        self.histo_folder = HISTO_FOLDER
        self.en_range = []
        for i in range(0, 4):
            self.en_range.append([])
        
        self.parse_cfg(MAIN_PROG_FOLDER + CFG_FILE)

        combobox_prog = Gtk.ComboBoxText()
        combobox_prog.set_entry_text_column(0)
        for prog in [HDRAINER_EXE, ONLINE_EXE]:
                combobox_prog.append_text(prog)
                combobox_prog.append_text(prog + "(with signals)")
        combobox_prog.set_active(0)
        combobox_prog.connect("changed", self.on_combobox_prog_changed)

        btn_read_data = Gtk.Button(label="Read data")
        btn_read_data.connect("clicked", self.on_btn_read_data_clicked)
        btn_set_porog = []
        btn_set_porog.append( Gtk.Button(label="Set porog #{:d} - #{:d}".format(1, 4)) )
        btn_set_porog[0].connect("clicked", self.on_btn_set_porog_clicked)
        
        btn_set_delay = Gtk.Button(label="Set delay")
        btn_set_delay.connect("clicked", self.on_btn_set_delay_clicked)
        btn_coinc_on = Gtk.Button(label="Coinc on")
        btn_coinc_on.connect("clicked", self.on_btn_coinc_clicked)
        btn_coinc_off = Gtk.Button(label="Coinc off")
        btn_coinc_off.connect("clicked", self.on_btn_coinc_clicked)
        btn_reset = Gtk.Button(label="Reset")
        btn_reset.connect("clicked", self.on_btn_reset_clicked)

        self.entry_time = Gtk.Entry()
        self.entry_time.set_text( str(self.time) )
        self.lbl_intens = Gtk.Label(label="0 cnts/s | 0 s")

        self.entry_porog = []
        for i in range(0, 4):
            self.entry_porog.append( Gtk.Entry() )
            self.entry_porog[i].set_text( str(self.porog[i]) )
        self.entry_delay = Gtk.Entry()
        self.entry_delay.set_text( str(self.delay) )

        self.entry_output_file_histo = Gtk.Entry()
        self.entry_output_file_histo.set_text(self.histo_folder)

        self.statusbar = Gtk.Statusbar()
        self.statusbar.push(self.statusbar.get_context_id("greatings"), "Hello! It's DspPAC. It helps you to start acqusition.")

        grid.attach(btn_read_data, 0, 0, 1, 1)
        grid.attach(combobox_prog, 1, 0, 1, 1)
        for i in range(0, 4):
            grid.attach(self.entry_porog[i], 0, i + 1, 1, 1)
        #    grid.attach(btn_set_porog[i], 1, i + 1, 1, 1)
        grid.attach(btn_set_porog[0], 1, 1, 1, 1)
        grid.attach( Gtk.Separator.new(Gtk.Orientation.HORIZONTAL), 0, 6, 2, 1 )
        grid.attach(self.entry_delay, 0, 7, 1, 1)
        grid.attach(btn_set_delay, 1, 7, 1, 1)
        grid.attach(btn_reset, 0, 8, 1, 1)
        grid.attach(btn_coinc_on, 1, 8, 1, 1)
        grid.attach(self.lbl_intens, 0, 9, 1, 1)
        grid.attach(btn_coinc_off, 1, 9, 1, 1)
        grid.attach( Gtk.Separator.new(Gtk.Orientation.HORIZONTAL), 0, 10, 2, 1)
        grid.attach(Gtk.Label("Time [s]:"), 0, 11, 1, 1)
        grid.attach(self.entry_time, 1, 11, 1, 1)
        grid.attach(self.entry_output_file_histo, 0, 12, 2, 1)
        grid.attach(self.statusbar, 0, 13, 2, 1)


    def __info_dialog(self, primary_text, secondary_text):
        dialog = Gtk.MessageDialog(self, 0, Gtk.MessageType.INFO, Gtk.ButtonsType.OK, primary_text)
        dialog.format_secondary_text(secondary_text)
        dialog.run()
        dialog.destroy()

    def on_combobox_prog_changed(self, combobox):
        self.prog = combobox.get_active_text()
        print("self.prog = {}".format(self.prog))

    def on_btn_read_data_clicked(self, btn):
        try:
            self.time = int( self.entry_time.get_text() )
        except ValueError:
            self.__info_dialog("Time problem", "Time should be a number")
            return -1

        if self.time <= 0:
            self.__info_dialog("Time problem", "Time should be a positive number")
            return -1

        output_histo_file = self.entry_output_file_histo.get_text()
        if os.path.isdir(output_histo_file) is False:
            self.__info_dialog("Output histo file problem", "In the entry the existing folder name should be entered")
            return -1

        if self.prog == HDRAINER_EXE:
            #os.execl("{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "-t", str(self.time))
            pid = os.fork()
            if pid == 0:
                time.sleep(1)
                os.execl("{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "-t", str(self.time))
                #os.execl("{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "-t", str(self.time), "-o", output_histo_file)
            else:
                if os.path.exists(SOCKET_COMMUNICATION_FILE):
                    os.remove(SOCKET_COMMUNICATION_FILE)

                server_fd = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                server_fd.bind(SOCKET_COMMUNICATION_FILE)
                server_fd.listen(1)

                cycle_prev, cycle_curr = -1, -1
                exe_time_prev, exe_time_curr = -1, -1
                conn, address = server_fd.accept()
                while True:
                    w_pid, ret = os.waitpid(pid, os.WNOHANG)
                    if w_pid == pid:
                        break
                    out_str = conn.recv(128)
                    
                    #Count intensity
                    print( "out_str = {} ".format(out_str) )
                    try:
                        cycles, exe_time = unpack("=2l", out_str)
                    except struct_error_unpack:
                        #End of *DRAINER should be
                        None
                        
                    print("cycles = {}, exe_time = {} | type = {}".format(cycles, exe_time, type(cycles)))
                    
                    if cycle_prev == -1:
                        cycle_prev = cycles
                        exe_time_prev = exe_time
                    else:
                        if cycle_curr == -1:
                            cycle_curr = cycles
                            exe_time_curr = exe_time

                        else:
                            cycle_prev, cycle_curr = cycle_curr, cycles
                            exe_time_prev, exe_time_curr = exe_time_curr, exe_time
                        if (exe_time_curr - exe_time_prev == 0):
                            None
                        else:
                            cycles_per_time = (cycle_curr - cycle_prev)/(exe_time_curr - exe_time_prev)
                            print("cycle_per_time = {:f}".format(cycles_per_time))
                            
                    '''
                    bin_s_num_cycles = unpack("10c", out_str[0:10])
                    #cycles = int(bin_s_num_cycles[0])*10**3 + int(bin_s_num_cycles[1])*10**2 + int(bin_s_num_cycles[2])*10 + int(bin_s_num_cycles) 
                    cycles, i = 0, 
                    for num in bin_s_num_cycles[::-1]:
                        cycles += num*10**i
                        i += 1 ## should be check for large numbers! 
                    print("out_str = {} | num_cycles[3] = {}, cycles = {}".format(out_str, bin_s_num_cycles, cycles))
                    '''
                    time.sleep(0.5)

                server_fd.close()
                os.remove(SOCKET_COMMUNICATION_FILE)
            '''
            ret_proc = subprocess.Popen( ["{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "-t", str(self.time)] )
            ret_proc.wait()
            ret = ret_proc.returncode
            '''
            if (ret != 0 or ret is None):
                print("Error on {} | ret = {}".format(HDRAINER_EXE, ret))
            else:
                print("Success execution of {}".format(HDRAINER_EXE))
        elif self.prog == HDRAINER_EXE + "(with signals)":
            ret_proc = subprocess.Popen( ["{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "-t", str(self.time), "-s"] )
            ret_proc.wait()

            if ret_proc.returncode != 0:
                print("Error on {}".format(HDRAINER_EXE))
            else:
                print("Success execution of {}".format(HDRAINER_EXE))
        elif self.prog == ONLINE_EXE:
            os.execl("{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "{}build/{}".format(MAIN_PROG_FOLDER, self.prog), "-t", str(self.time))


    def on_btn_set_porog_clicked(self, btn):
        ''''
        txt_btn = btn.get_label()
        try:
            num_btn = int(txt_btn[-1])
        except ValueError:
            self.__info_dialog("Porog #{} range problem".format(num_btn), "Porog should be a number")
            return -1
        '''
        #check for errors for all porogs!!!
        for num_btn in range(0, 4):
            self.porog[num_btn] = int( self.entry_porog[num_btn].get_text() )
            if (self.porog[num_btn] < 0 or self.porog[num_btn] >= 8192):
                self.__info_dialog("Porog #{} range problem".format(num_btn), "Porog should be in range [0:8K]")
                return -1

        #execl C prog for set porog #num_btn
        str_porog = "{:d} {:d} {:d} {:d}".format(self.porog[0], self.porog[1], self.porog[2], self.porog[3])
        print("str_porog = {}".format(str_porog))
        ret = subprocess.call( ["{}build/{}".format(MAIN_PROG_FOLDER, FPGA_PROG), "-p", str_porog] )
        if ret != 0:
            err_msg = "Set porog error! Return code {}.".format(ret)
            self.statusbar.push(self.statusbar.get_context_id("error in set porog"), err_msg)
        else:
            self.statusbar.push(self.statusbar.get_context_id("ok set porog"), "Set porog OK.")

        
    def on_btn_set_delay_clicked(self, btn):
        try:
            self.delay = int( self.entry_delay.get_text() )
        except ValueError:
            self.__info_dialog("Delay range problem", "Delay should be a number")
            return -1

        if (self.delay < 0 or self.delay > 255):
            self.__info_dialog("Delay range problem", "Delay should be in range [0, 255]")
            return -1

        ret = subprocess.call( ["{}build/{}".format(MAIN_PROG_FOLDER, FPGA_PROG), "-d", str(self.delay)] )
        if ret != 0:
            err_msg = "Set delay error! Return code {}.".format(ret)
            self.statusbar.push(self.statusbar.get_context_id("error in set delay"), err_msg)
        else:
            self.statusbar.push(self.statusbar.get_context_id("ok set delay"), "Set delay OK.")
        print("delay = {}".format(self.delay))


    def on_btn_reset_clicked(self, btn):
        ret = subprocess.call( ["{}build/{}".format(MAIN_PROG_FOLDER, FPGA_PROG), "-r"] )
        if ret != 0:
            err_msg = "Reset error! Return code {}.".format(ret)
            self.statusbar.push(self.statusbar.get_context_id("error in reset"), err_msg)
        else:
            self.statusbar.push(self.statusbar.get_context_id("ok reser"), "Reset OK.")


    def on_btn_coinc_clicked(self, btn):
        txt_btn = btn.get_label()
        if txt_btn == "Coinc on":
            self.coinc = 1
            ret = subprocess.call( ["{}build/{}".format(MAIN_PROG_FOLDER, FPGA_PROG), "-c"] )
        elif txt_btn == "Coinc off":
            self.coinc = 0
            ret = subprocess.call( ["{}build/{}".format(MAIN_PROG_FOLDER, FPGA_PROG), "-n"] )

        if ret != 0:
            err_msg = "Coinc ON/OFF error! Return code {}.".format(ret)
            self.statusbar.push(self.statusbar.get_context_id("error in coinc"), err_msg)
        else:
            self.statusbar.push(self.statusbar.get_context_id("ok reser"), "Coinc ON/OFF OK.")
    

    def parse_cfg(self, path_cfg_file):
        with open(path_cfg_file, 'r') as cfg_file:
            #if the size of cfg.json will be larger than 2048 bytes, please change it
            config = cfg_file.read(2048)
            config_vals = json.loads(config)
            print("config_vals = {}".format(config_vals))
            
            self.porog = config_vals["porog"]#[90, 90, 90, 90]
            self.delay = config_vals["delay"]#100
            self.coinc = 1 if config_vals["coinc?"] is "True" else 0#1
            self.time = config_vals["time"]#10
            self.histo_folder = config_vals["histo folder"]
            for i in range(0, 4):
                self.en_range[i] = config_vals["en range " + str(i)]


if __name__ == "__main__":
    win = StartWin()
    win.connect("delete-event", Gtk.main_quit)

    win.show_all()
    Gtk.main()
