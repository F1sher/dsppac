#!/usr/bin/env python3

import json
import gi
import os
import signal
import subprocess
import socket
import sys
import time
from gi.repository import Gtk
from signal import SIGUSR1
from struct import unpack
from struct import error as struct_error_unpack

sys.path.append("./ui")
import const

PYTHON_EXEC = "python3"
STUFF_FOLDER = "./"
MAIN_PROG_FOLDER = "/home/das/job/dsp/"
FPGA_PROG = "send_comm"
HDRAINER_EXE = "hdrainer"
ONLINE_EXE = "oconsumer"
HISTO_FOLDER = "/home/das/job/dsp/test/histos"
SOCKET_COMMUNICATION_FILE = "./hidden"
CFG_FILE = "cfg.json"
CONSTANTS_FILE = "constants.json"
SLEEP_S_SOCK_READ = 1


class DSPPAC(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="DspPAC", resizable=False)
        self.connect("delete-event", Gtk.main_quit)

        vbox = Gtk.Box()
        self.add(vbox)

        menubar = self.__create_menubar()

        grid = Gtk.Grid()
        grid.set_column_homogeneous(False)

        vbox.pack_start(menubar, False, False, 0)
        vbox.pack_start(grid, True, True, 0)

        self.child_pid = -1
        self.prog = HDRAINER_EXE
        self.porog = [90, 90, 90, 90]
        self.delay = 100
        self.coinc = 1
        self.time = 10
        self.histo_folder = HISTO_FOLDER
        self.en_range = []
        for i in range(0, 4):
            self.en_range.append([0, 0, 0, 0])
        
        #comment it. For test purposes only
        self.parse_cfg(MAIN_PROG_FOLDER + CFG_FILE)

        combobox_prog = Gtk.ComboBoxText()
        combobox_prog.set_entry_text_column(0)
        for prog in [HDRAINER_EXE, ONLINE_EXE]:
                combobox_prog.append_text(prog)
                combobox_prog.append_text(prog + "(with signals)")
        combobox_prog.set_active(1)
        combobox_prog.connect("changed", self.on_combobox_prog_changed)
        combobox_prog.emit("changed")

        btn_start = Gtk.Button(label="Start")
        btn_start.connect("clicked", self.on_btn_start_clicked)
        btn_stop = Gtk.Button(label="Stop")
        btn_stop.connect("clicked", self.on_btn_stop_clicked)
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

        self.filechooser_histo = Gtk.FileChooserButton(title="Select a histo folder", action=Gtk.FileChooserAction.SELECT_FOLDER)
        self.filechooser_histo.set_filename(self.histo_folder)

        self.entry_en_range = []
        for i in range(0, 4):
            self.entry_en_range.append( Gtk.Entry() )
            self.entry_en_range[i].set_text( str(self.en_range[i])[1:-1] )

        self.statusbar = Gtk.Statusbar()
        self.statusbar.push(self.statusbar.get_context_id("greatings"), "Hello! It's DspPAC. It helps you to start acqusition.")

        line = 0
        grid.attach(btn_start, 0, line, 1, 1)
        grid.attach(combobox_prog, 1, line, 1, 1)
        line += 1
        grid.attach(btn_stop, 0, line, 1, 1)
        line += 1
        for i in range(4):
            grid.attach(self.entry_porog[i], 0, i + line, 1, 1)
        #    grid.attach(btn_set_porog[i], 1, i + 1, 1, 1)
        grid.attach(btn_set_porog[0], 1, line, 1, 1)
        line += i + 1
        grid.attach( Gtk.Separator.new(Gtk.Orientation.HORIZONTAL), 0, line, 2, 1 )
        line += 1
        grid.attach(self.entry_delay, 0, line, 1, 1)
        grid.attach(btn_set_delay, 1, line, 1, 1)
        line += 1
        grid.attach(btn_reset, 0, line, 1, 1)
        grid.attach(btn_coinc_on, 1, line, 1, 1)
        line += 1
        grid.attach(self.lbl_intens, 0, line, 1, 1)
        grid.attach(btn_coinc_off, 1, line, 1, 1)
        line += 1
        
        grid.attach( Gtk.Separator.new(Gtk.Orientation.HORIZONTAL), 0, line, 2, 1)
        line += 1
        grid.attach(Gtk.Label("ACQ Time [s]:"), 0, line, 1, 1)
        grid.attach(self.entry_time, 1, line, 1, 1)
        line += 1

        #grid.attach(self.entry_output_file_histo, 0, 12, 2, 1)
        grid.attach(self.filechooser_histo, 0, line, 2, 1)
        line += 1

        for i in range(4):
            grid.attach(Gtk.Label("EN range #" + str(i + 1) + ":"), 0, 13 + i, 1, 1)
            grid.attach(self.entry_en_range[i], 1, 13 + i, 1, 1)
        line += i + 1

        grid.attach(self.statusbar, 0, 17, 2, 1)
        line += 1

        self.win_counts = self.__win_intens(100)

    def __create_menubar(self):
        menubar = Gtk.MenuBar()
        menu_file = Gtk.Menu()
        menu_tools = Gtk.Menu()

        menu_it_file = Gtk.MenuItem.new_with_label("File")
        menu_it_quit = Gtk.MenuItem.new_with_label("Quit")
        menu_it_tools = Gtk.MenuItem.new_with_label("Tools")
        menu_it_edit_cfg = Gtk.MenuItem.new_with_label("Edit cfg")
        menu_it_edit_constants = Gtk.MenuItem.new_with_label("Edit constants")

        menu_it_file.set_submenu(menu_file)
        menu_file.append(menu_it_quit)
        menu_it_quit.connect("activate", Gtk.main_quit)

        menu_it_tools.set_submenu(menu_tools)
        menu_tools.append(menu_it_edit_cfg)
        menu_it_edit_cfg.connect("activate", self.on_edit_file_activate)
        menu_tools.append(menu_it_edit_constants)
        menu_it_edit_constants.connect("activate", self.on_edit_file_activate)

        menubar.append(menu_it_file)
        menubar.append(menu_it_tools)

        return menubar

    def on_edit_file_activate(self, menu_it):
        lbl = menu_it.get_label()
        if lbl == "Edit cfg":
            os.system("gedit {}".format(MAIN_PROG_FOLDER + CFG_FILE))
        elif lbl == "Edit constants":
            os.system("gedit {}".format(MAIN_PROG_FOLDER + CONSTANTS_FILE))
        else:
            print("Warning: Unknown file to edit")

    def __info_dialog(self, primary_text, secondary_text):
        dialog = Gtk.MessageDialog(self, 0, Gtk.MessageType.INFO, Gtk.ButtonsType.OK, primary_text)
        dialog.format_secondary_text(secondary_text)
        dialog.run()
        dialog.destroy()


    def on_combobox_prog_changed(self, combobox):
        self.prog = combobox.get_active_text()


    def on_btn_start_clicked(self, btn):
        try:
            self.time = int( self.entry_time.get_text() )
        except ValueError:
            self.__info_dialog("Time problem", "Time should be a number")
            return -1

        if self.time <= 0:
            self.__info_dialog("Time problem", "Time should be a positive number")
            return -1

        #output_histo_file = self.entry_output_file_histo.get_text()
        output_histo_file = self.filechooser_histo.get_filename()
        if os.path.exists(output_histo_file) is False:
            os.makedirs(output_histo_file)
        else:
            if os.path.isdir(output_histo_file) is False:
                self.__info_dialog("Output histo file problem", "In the entry the existing folder name or new name for new folder should be entered")
                return -1

        self.save_cfg(MAIN_PROG_FOLDER + CFG_FILE)

        print("self.prog = {}".format(self.prog))
        if self.prog == HDRAINER_EXE:
            self.start_C_prog(self.prog, "{}build/{}".format(MAIN_PROG_FOLDER, HDRAINER_EXE), [ "-t", str(self.time), "-e", str(self.en_range)[1:-1] ])
        elif self.prog == HDRAINER_EXE + "(with signals)":
            self.start_C_prog(self.prog, "{}build/{}".format(MAIN_PROG_FOLDER, HDRAINER_EXE), [ "-s", "-t", str(self.time), "-e", str(self.en_range)[1:-1], "-o", output_histo_file ])
        elif self.prog == ONLINE_EXE:
            self.start_C_prog(self.prog, "{}build/{}".format(MAIN_PROG_FOLDER, ONLINE_EXE), [ "-t", str(self.time), "-e", str(self.en_range)[1:-1] ])
        elif self.prog == ONLINE_EXE + "(with signals)":
            self.start_C_prog(self.prog, "{}build/{}".format(MAIN_PROG_FOLDER, ONLINE_EXE), [ "-s", "-t", str(self.time), "-e", str(self.en_range)[1:-1] ])

    def on_btn_stop_clicked(self, btn):
        ########### send STOP signal (really SIGUSR1)
        if self.child_pid > 0:
            os.kill(self.child_pid, signal.SIGUSR1)
        ##########


    def start_C_prog(self, prog_name, prog_path, prog_params):
        pid = os.fork()
        if pid == 0:
            #Child
            time.sleep(1)
            try:
                os.execl(prog_path, prog_path, *prog_params)
            except OSError:
                print("OSerror exception")
                raise
        else:
            #Parent
            self.child_pid = pid

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
                try:
                    cycles, exe_time, *diff_counts, exe_m_time = unpack("=7l", out_str)
                except struct_error_unpack:
                    #End of *DRAINER should be
                    print("Struct unpack error | out_str = {}".format(out_str))
                    cycles, exe_time = 0, 0
                    diff_counts = [0, 0, 0, 0]
                    exe_m_time = 1
                    None
                        
                print("cycles = {}, execution_time = {} | type = {}".format(cycles, exe_time, type(cycles)))
                    
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
                        cycles_per_time *= self.coinc + 1
                        self.lbl_intens.set_text("{:d} cnts/s | {:d} s".format(int(cycles_per_time), exe_time_curr))
                        time.sleep(SLEEP_S_SOCK_READ)
                        while Gtk.events_pending():
                            Gtk.main_iteration_do(False)
                        print("cycle_per_time = {:f}".format(cycles_per_time))
                        
                        for i in range(4):
                            diff_per_s = round( diff_counts[i]/(exe_m_time/1000.0) )
                            self.win_counts.lbl[i].set_text( "\t Det #{:d} = {:>6d} / s\t".format(i + 1, diff_per_s) )
                        print("exe_m_time = {:d}".format(exe_m_time))

            server_fd.close()
            os.remove(SOCKET_COMMUNICATION_FILE)

            if (ret != 0 or ret is None):
                err_msg = prog_name + " error! Return code {}.".format(ret)
                self.statusbar.push(self.statusbar.get_context_id("error in " + prog_name), err_msg)
                print("Error on {} | ret = {}".format(prog_name, ret))
            else:
                self.lbl_intens.set_text( "{:d} cnts/s | {:d} s".format(0, exe_time)) #self.time should be replaced with exe_time 
                self.statusbar.push(self.statusbar.get_context_id(prog_name + " Ok end"), prog_name + " exits normally")
                print("Success execution of {} | execution_time = {:d}".format(prog_name, exe_time))

    def __win_intens(self, intens):
        win = Gtk.Window(title="Intensities")
        win.resize(160, 80)

        vbox = Gtk.Box()
        win.add(vbox)

        grid = Gtk.Grid()
        vbox.pack_start(grid, True, True, 0)
        
        win.lbl = []
        win.lbl.append( Gtk.Label("0") )
        win.lbl.append( Gtk.Label("0") )
        win.lbl.append( Gtk.Label("0") )
        win.lbl.append( Gtk.Label("0") )

        for i in range(4):
            grid.attach(win.lbl[i], 0, i, 1, 1)

        win.show_all()
        return win



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
            self.statusbar.push(self.statusbar.get_context_id("ok reset"), "Reset OK.")
        #Next line for test only!!!
        #self.save_cfg(CFG_FILE)


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
            self.statusbar.push(self.statusbar.get_context_id("ok coinc on/off"), "Coinc ON/OFF OK.")

    def parse_cfg(self, path_cfg_file):
        with open(path_cfg_file, 'r') as cfg_file:
            #if the size of cfg.json will be larger than 2048 bytes, please change it
            config = cfg_file.read(2048)
            config_vals = json.loads(config)
            print("config_vals = {}".format(config_vals))
            
            self.porog = config_vals["porog for detectors"]#[90, 90, 90, 90]
            self.delay = config_vals["time of coincidence [ch]"]#100
            self.coinc = 1 if config_vals["coinc mode?"] == "True" else 0#1
            self.acq_time = config_vals["time of exposition [s]"]#10
            self.histo_folder = config_vals["histo folder"]
            self.en_range = []
            for i in range(0, const.DET_NUM):
                self.en_range.append( config_vals["en_range D" + str(i + 1)] )

    def save_cfg(self, path_cfg_file):
        with open(path_cfg_file, 'w') as cfg_file:
            config = {}
            config["porog for detectors"] = self.porog
            config["time of coincidence [ch]"] = self.delay
            config["coinc mode?"] = "True" if self.coinc == 1 else "False"
            config["time of exposition [s]"] = self.acq_time
            config["histo folder"] = self.histo_folder
            
            for i in range(0, const.DET_NUM):
                energies = map(int, self.entry_en_range[i].get_text().split(', '))
            for j in range(0, const.DET_NUM):
                self.en_range[i][j] = next(energies)
            for i in range(0, const.DET_NUM):
                config["en_range D" + str(i + 1)] = self.en_range[i]
                    
            def dict_to_true_cfg_str(d):
                res = "{\n"
                res += "\t\"porog for detectors\": " + str(d["porog for detectors"]) + ",\n"
                res += "\t\"time of coincidence [ch]\": " + str(d["time of coincidence [ch]"]) + ",\n"
                res += "\t\"coinc mode?\": " + "\"" + str(d["coinc mode?"]) + "\"" + ",\n"
                res += "\t\"time of exposition [s]\": " + str(d["time of exposition [s]"]) + ",\n"
                res += "\t\"histo folder\": " + "\"" + str(d["histo folder"]) + "\"" + ",\n"
                for i in range(0, 3):
                    res += "\t\"en_range D" + str(i + 1) + "\": " + str(d["en_range D" + str(i + 1)])  + ",\n"
                i = 3
                res += "\t\"en_range D" + str(i + 1) + "\": " + str(d["en_range D" + str(i + 1)])  + "\n"
                res +="}"
                return res

            cfg_file.write(dict_to_true_cfg_str(config))



if __name__ == "__main__":
    win = DSPPAC()

    win.show_all()
    Gtk.main()
