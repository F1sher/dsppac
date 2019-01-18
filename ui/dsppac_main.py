#!/usr/bin/env python3.4

import json

import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, GLib

from matplotlib.animation import FuncAnimation
from matplotlib.figure import Figure
from matplotlib.backends.backend_gtk3agg import FigureCanvasGTK3Agg as FigureCanvas
from matplotlib.ticker import FuncFormatter

import numpy as np

from os import system
import os

from signal import SIGUSR1

from socket import socket
from socket import timeout as sock_timeout
from socket import AF_UNIX
from socket import SOCK_STREAM
from socket import SOCK_DGRAM

from struct import unpack
from struct import error as struct_error

from subprocess import call as subprocess_call

from time import sleep
from time import time
from time import localtime

from threading import Thread

import const
from information import Information
from zoom import Zoom
from logger import Logger

class UI():
    def __init__(self):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(const.MAIN_PROG_FOLDER + 
                                   const.UI_FOLDER + "main_ui.glade")
        handlers = {
            "on_open": self.on_open,
            "on_quit": self.on_quit,
            "run_fpga": self.run_fpga,
            "run_dsp": self.run_dsp,
            "run_pypac_signal": self.run_pypac_signal,
            "run_pypac_histo": self.run_pypac_histo,
            "edit_config": self.edit_config,
            "edit_constants": self.edit_constants,
            "on_start": self.on_start,
            "on_stop": self.on_stop,
            "on_save": self.on_save,
            "on_pause": self.on_pause,
            "on_clear": self.on_clear,

            "zoom_en_shrink": self.z_en_shrink,
            "zoom_en_squeeze": self.z_en_squeeze,
            "zoom_en_left": self.z_en_mov_l,
            "zoom_en_right": self.z_en_mov_r,
            "zoom_en_up": self.z_en_up,
            "zoom_en_down": self.z_en_down,
            "zoom_en_log": self.z_en_log,
            "zoom_en_zero": self.z_en_zero,

            "zoom_t_shrink": self.z_t_shrink,
            "zoom_t_squeeze": self.z_t_squeeze
        }
        self.builder.connect_signals(handlers)

        self.main_win = self.builder.get_object("main_win")
        
        self.init_Figs()
        
        main_grid = self.builder.get_object("main_grid")
        self.attach_Figs(main_grid)

        #tmp_foldername = "/home/das/job/plis/histo/apr14_4det_porog=90_delay=100_coinc_10h_night"
        self.curr_histo_folder = const.DEFAULT_CURR_HISTO_FOLDER
        self.spectra = Spectra(self.curr_histo_folder)
        self.fill_Figs(self.spectra.histo)

        zoom_args = {
            "en_magnification": 1e2,
            "en_v_step": 5e3,
            "en_h_step": 1e2,
            "t_magnification": 1e2,
            "t_v_step": 1e3,
            "t_h_step": 100.0
        }
        self.zoom = Zoom(self.en_fig, self.t_fig, self.en_axes, self.t_axes, zoom_args)

        self.logger = Logger(const.MAIN_PROG_FOLDER + "journal.log")
        self.logger.info("Program initialization is ok")

        self.update_counts([0]*4)

        self.info_textview = self.builder.get_object("info_textview")
        self.information = Information(self.info_textview)

        self.statusbar = self.builder.get_object("main_statusbar")

    def create_t_title(self, i):
        titles = ["D1-D2", "D2-D1", "D1-D3", "D3-D1",
                  "D1-D4", "D4-D1", "D2-D3", "D3-D2",
                  "D2-D4", "D4-D2", "D3-D4", "D4-D3"]
            
        return titles[i]

    def init_Figs(self):
        self.en_fig = []
        self.en_axes = []
        for i in range(0, const.DET_NUM):
            self.en_fig.append( Figure(figsize=(1, 1), dpi=80, frameon=False) )
            self.en_axes.append( self.en_fig[i].add_subplot(111) )
            
            ###adjust subplots to specific size
            if i == 0:
                self.en_fig[i].subplots_adjust(left=0.2, bottom=0.05, right=0.99, top=0.97)
            else:
                self.en_fig[i].subplots_adjust(left=0.05, bottom=0.05, right=0.99, top=0.97)
            ###
            ###xlim ylim
            self.en_axes[i].set_xlim(0, const.HIST_SIZE)
            self.en_axes[i].set_ylim(bottom = -1)
            ###
            ###x, y ticks
            self.en_axes[i].set_xticks([0, 1000, 2000, 3000, 4000])
            self.en_axes[i].minorticks_on()
            self.en_axes[i].xaxis.set_major_formatter( FuncFormatter(lambda x, pos: "{:.0f}k".format(x/1e3)) )

            if i == 0:
                self.en_axes[i].yaxis.set_major_formatter( FuncFormatter(lambda x, pos: "{:.1f}k".format(x/1e3)) )
            else:
                self.en_axes[i].yaxis.set_major_formatter( FuncFormatter(lambda x, pos: "") )
                #self.en_axes[i].set_yticks([])
            ###
        
        self.t_fig = []
        self.t_axes = []
        for i in range(0, 12):
            self.t_fig.append( Figure(figsize=(1, 1), dpi=80, frameon=False) )
            self.t_axes.append( self.t_fig[i].add_subplot(111) )
            title_text = self.create_t_title(i)

            if (i == 0) or (i == 1):
                self.t_fig[i].suptitle(title_text, x=0.45, y=0.95)
            else:
                self.t_fig[i].suptitle(title_text, x=0.3, y=0.95)
            
            ###adjust subplots to specific size
            if i == 0:
                self.t_fig[i].subplots_adjust(left=0.2, bottom=0.05, right=0.99, top=0.99)
            elif i == 1:
                self.t_fig[i].subplots_adjust(left=0.2, bottom=0.05, right=0.99, top=0.99)
            elif i in [2, 4, 6, 8, 10]:
                self.t_fig[i].subplots_adjust(left=0.05, bottom=0.05, right=0.99, top=0.99)
            else:
                self.t_fig[i].subplots_adjust(left=0.05, bottom=0.05, right=0.99, top=0.99)
            ###
            ###xlim ylim
            self.t_axes[i].set_xlim(0, const.HIST_SIZE)
            self.t_axes[i].set_ylim(bottom = -1)
            ###
            ###x, y ticks
            self.t_axes[i].set_xticks([0, 1000, 2000, 3000, 4000])
            self.t_axes[i].minorticks_on()
            self.t_axes[i].xaxis.set_major_formatter( FuncFormatter(lambda x, pos: "{:.0f}k".format(x/1e3)) )

            if (i == 0) or (i == 1):
                self.t_axes[i].yaxis.set_major_formatter( FuncFormatter(lambda x, pos: "{:.1f}k".format(x/1e3)) )
            else:
                self.t_axes[i].yaxis.set_major_formatter( FuncFormatter(lambda x, pos: "") )
            ###

    def attach_Figs(self, grid):
        for i in range(4):
            en_d_vbox_name = "en_d{:d}_vbox".format(i)
            en_d_vbox = self.builder.get_object(en_d_vbox_name)
            en_d_vbox.pack_start(FigureCanvas(self.en_fig[i]), True, True, 0)
        
        col, row = 0, 1
        for i in range(6):
            grid.attach(FigureCanvas(self.t_fig[2*i]), col, row, 1, 1)
            col += 1
        
        col, row = 0, 2
        for i in range(6):
            grid.attach(FigureCanvas(self.t_fig[2*i + 1]), col, row, 1, 1)
            col += 1

    def fill_Figs(self, histo):
        histo_np_arr = np.array(histo)

        max_histo_en = histo_np_arr[0:const.DET_NUM].max() or 1
        max_histo_t = histo_np_arr[const.DET_NUM:].max() or 1

        self.en_axes_line = [0]*const.DET_NUM
        self.t_axes_line = [0]*12

        for i in range(0, const.DET_NUM):
            x_data = np.arange(const.HIST_SIZE)
            y_data = histo_np_arr[i]
            self.en_axes_line[i], = self.en_axes[i].plot(x_data, y_data, color = "blue")
            self.en_axes[i].set_ylim(top = max_histo_en)

        for i in range(0, 12):
            x_data = np.arange(const.HIST_SIZE)
            y_data = histo_np_arr[const.DET_NUM + i]
            self.t_axes_line[i], = self.t_axes[i].plot(x_data, y_data, color = "red")
            self.t_axes[i].set_ylim(top = max_histo_t)

    def update_Figs(self, histo):
        histo_np_arr = np.array(histo)

        max_histo_en = histo_np_arr[0:const.DET_NUM].max() or 1
        max_histo_t = histo_np_arr[const.DET_NUM:,1:].max() or 1

        ###for test
        t = time()
        ###

        print("max_histo_en = {}".format(max_histo_en))
        for i in range(0, const.DET_NUM):
            #self.en_axes[i].set_xlim(left = 80, right = 100)
            self.en_axes[i].set_ylim(top = max_histo_en)
            self.en_axes_line[i].set_ydata( histo_np_arr[i] )
            self.en_fig[i].canvas.draw()

        for i in range(0, 12):
            self.t_axes[i].set_ylim(top = max_histo_t)
            self.t_axes_line[i].set_ydata( histo_np_arr[const.DET_NUM + i] )
            self.t_fig[i].canvas.draw()

            text = "{}    {}".format(self.create_t_title(i), np.sum(histo_np_arr[const.DET_NUM+i][1:]))
            if (i == 0) or (i == 1):
                self.t_fig[i].suptitle(text, x=0.55, y=0.95)
            else:
                self.t_fig[i].suptitle(text, x=0.4, y=0.95)

        print("plotting time = {}".format(time() - t))

    def update_counts(self, det_counts):
        all_zero_counts = 0
        for i in range(0, const.DET_NUM):
            if det_counts[i] == 0:
                all_zero_counts += 1
            
            text = "D{:d}    ".format(i + 1)
            if det_counts[i] < 1e3:
                text += "{:.0f}".format(det_counts[i])
            else:
                text += "{:.2f}K".format(det_counts[i]/1e3)
            #text = "D{:d}    {:.2f}K".format(i + 1, det_counts[i]/1e3)
            if i == 0:
                self.en_fig[i].suptitle(text, x=0.45, y=0.95)
            else:
                self.en_fig[i].suptitle(text, x=0.3, y=0.95)

        for i in range(0, const.DET_NUM):
            self.en_fig[i].canvas.draw()

    def on_open(self, *args):
        print("open histo")

        fcd = Gtk.FileChooserDialog("Open", None, Gtk.FileChooserAction.SELECT_FOLDER, ("Cancel", Gtk.ResponseType.CANCEL, "Open", Gtk.ResponseType.OK))
        
        fcd.set_current_folder(self.curr_histo_folder)

        response = fcd.run()
        if response == Gtk.ResponseType.OK:
            self.curr_histo_folder = fcd.get_current_folder()
            self.spectra = Spectra(self.curr_histo_folder)
            self.update_Figs(self.spectra.histo)

            fcd.destroy()
        elif response == Gtk.ResponseType.CANCEL:
            fcd.destroy()

    def on_quit(self, *args):
        Gtk.main_quit(*args)

    def run_fpga(self, *args):
        print("Run fpga app")
        self.fpga = FPGA()
        self.fpga.win.show_all()

    def run_dsp(self, *args):
        print("Run dsp app")
        print(args)

    def run_pypac_signal(self, *args):
        print("Run pypac signal app")

        file_list = os.listdir(self.curr_histo_folder)
        signal_file = None

        for name in file_list:
            try:
                if name.split('.')[1] == "sgnl":
                    if self.curr_histo_folder[-1] == '/':
                        signal_file = self.curr_histo_folder + name
                    else:
                        signal_file = self.curr_histo_folder + '/' + name
                    break
            except IndexError:
                None

        if signal_file is not None:
            system("{:s} -i {:s}".format(const.PYPAC_PROG_FOLDER + const.PYPAC_PROG_NAME, signal_file))
        else:
            text = "There are no signal files in current histo folder: {:s}", self.curr_histo_folder 
            self.statusbar_push(self, "error", err_msg=text)
            print("!Error: " + text)
        
    def run_pypac_histo(self, *args):
        print("Run pypac histo app")
        '''
        if "fpga" not in self.__dict__:
            print("Run FPGA should be started first")
            self.fpga = FPGA()
        '''

        system("{:s} -h {:s}".format(const.PYPAC_PROG_FOLDER + const.PYPAC_PROG_NAME, self.curr_histo_folder))

    def edit_config(self, *args):
        system("{:s} {:s}".format(const.TXT_EDITOR, const.MAIN_PROG_FOLDER + const.CFG_FILE))

    def edit_constants(self, *args):
        system("{:s} {:s}".format(const.TXT_EDITOR, const.MAIN_PROG_FOLDER + const.CONSTANTS_FILE))

    def on_start(self, *args):
        btn = args[0]

        if "fpga" not in self.__dict__:
            print("Run FPGA should be started first")
            self.fpga = FPGA()

        print( "On start | print acq_time = {}".format(self.fpga.acq_time) )
        self.fpga.save_consts_cfg(self.fpga.histo_folder)

        ###should be moved to separate func
        info = {
            "name": self.fpga.histo_folder,
            "start": localtime(),
            "expos": self.fpga.acq_time,
            "time": 1,
            "intens": 0,
        }
        self.information.update(info)
        ###

        self.curr_histo_folder = self.fpga.histo_folder

        print("signal flag = {}".format(self.fpga.with_signals_flag))
        if self.fpga.with_signals_flag:
            prog_name = const.HDRAINER_EXE + "(with signals)"
        else:
            prog_name = const.HDRAINER_EXE
        self.hdrainer = Hdrainer(prog_name, self.fpga.acq_time, self.fpga.coinc, self.fpga.en_range, self.fpga.histo_folder)
        
        self.thread_args = {"ret": -1}
        t1 = Thread(target = self.hdrainer.start)
        t1.start()

        self.online_flag = 1

        GLib.timeout_add_seconds(const.SLEEP_S_FIG_UPDATE, 
                                 self.read_plot_online_histo, self.fpga.histo_folder)
        GLib.timeout_add_seconds(const.SLEEP_S_COUNTS_UPDATE,
                                 self.update_counts_online_histo)

        self.glib_loop = GLib.MainLoop()
        self.glib_loop.run()
        #self.glib_loop.quit()

        t1.join()

        ret = self.hdrainer.ret
        if (ret != 0 or ret is None):
            text = prog_name + " error! Return code {}.".format(ret)
            self.statusbar_push("error", err_msg=text)
        else: 
            text = "Success execution of {} | execution_time = {:d}".format(prog_name, self.fpga.acq_time)
            self.statusbar_push("ok", normal_msg=text)
            print(text)

        info["time"] = 0
        info["intens"] = 0
        self.information.update(info)
        
        self.hdrainer.det_counts = [0]*4
        self.update_counts(self.hdrainer.det_counts)

        btn.handler_block_by_func(self.on_start)
        btn.set_active(False)
        btn.handler_unblock_by_func(self.on_start)

    def on_stop(self, *args):
        print("On stop")
        self.hdrainer.stop()

    def on_save(self, *args):
        print("On save")

    def on_pause(self, *args):
        print("On pause")

    def on_clear(self, *args):
        print("On clear")

    def z_en_shrink(self, *args):
        print("z_en_shrink")
        self.zoom.en_x_shrink()

    def z_en_squeeze(self, *args):
        print("z_en_squeeze")
        self.zoom.en_x_squeeze()

    def z_en_mov_l(self, *args):
        print("z_en_mov_l")
        self.zoom.en_x_move_l()

    def z_en_mov_r(self, *args):
        print("z_en_mov_r")
        self.zoom.en_x_move_r()

    def z_en_up(self, *args):
        print("z_en_up")
        self.zoom.en_y_up()

    def z_en_down(self, *args):
        print("z_en_down")
        self.zoom.en_y_down()

    def z_en_log(self, *args):
        print("z_en_log")
        self.zoom.en_log()

    def z_en_zero(self, *args):
        print("z_en_zero")
        self.zoom.en_zero()

    def z_t_shrink(self, *args):
        print("z_t_shrink")
        self.zoom.t_x_shrink()
        
    def z_t_squeeze(self, *args):
        print("z_t_squeeze")
        self.zoom.t_x_squeeze()


    def read_plot_online_histo(self, hdir):
        #now readbin_histo each 1 sec!!!
        #update_counts should be each 1 sec only

        #histo read should be 1 time ~100 sec
        histo = self.spectra.readbin_histo_files(hdir)
        self.update_Figs(histo)
    
        if self.hdrainer.online:
            res = True
        else:
            self.glib_loop.quit()
            res = False

        return res

    def update_counts_online_histo(self):
        #Update EN and T spk counts (top on the fig)
        self.update_counts(self.hdrainer.det_counts)

        info = {
            "time": self.hdrainer.exec_time,
            "intens": self.hdrainer.cycles_per_time,
        }
        self.information.update(info)

        if self.hdrainer.online:
            res = True
        else:
            self.glib_loop.quit()
            res = False

        return res
    

    def is_glib_alive(self, x):
        print("main loop is alive? {}".format( self.glib_loop.is_running() ))

    
    def statusbar_push(self, context, normal_msg="", err_msg=""):
        print("context = {}".format(context))
        context_id = self.statusbar.get_context_id(context)
        if err_msg:
            msg = "!Error!: " + err_msg
        else:
            msg = normal_msg
        
        self.statusbar.push(context_id, msg)


class Spectra():
    def __init__(self, foldername):
        if foldername is not None:
            self.histo = self.readbin_histo_files(foldername)

    def readbin_histo_files(self, hdir):
        sizeof_int = 4
        i = 0
        res = [[0]*const.HIST_SIZE]*16

        for name in const.HISTO_FILE_NAMES:
            try:
                with open("{}/{}".format(hdir, name), mode="rb") as in_fd:
                    in_fd.seek(512, 0)
                    data_in = in_fd.read(sizeof_int*const.HIST_SIZE)

                    try:
                        res[i] = list(unpack(str(const.HIST_SIZE) + 'i', data_in))
                        i += 1
                    except struct_error:
                        filename = "{}/{}".format(hdir, name)
                        print("!Error in retriving histo data!: "
                              "size of file '" + filename + "' is " + \
                              str( os.path.getsize("{}/{}".format(hdir, name)) ) + \
                              " bytes but should be " + str(sizeof_int*const.HIST_SIZE))
            except FileNotFoundError:
                print("!Error histo File {:s} not found".format(name))
                
                return res
                #raise

        return res



class FPGA():
    def __init__(self):
        self.builder = Gtk.Builder()
        self.builder.add_from_file(const.MAIN_PROG_FOLDER + 
                                   const.UI_FOLDER + "fpga_ui.glade")

        handlers = {
            "on_btn_set_porog_clicked": self.on_btn_set_porog_clicked,
            "on_btn_set_delay_clicked": self.on_btn_set_delay_clicked,
            "on_btn_reset_clicked": self.on_btn_reset_clicked,
            "on_btn_coinc_clicked": self.on_btn_coinc_clicked,
            "on_btn_ok_clicked": self.on_btn_ok_clicked
        }
        self.builder.connect_signals(handlers)
    
        self.win = self.builder.get_object("win_fpga")
        
        self.switch_hdrainer = self.builder.get_object("switch_hdrainer")
        self.with_signals_flag = False

        self.entry_porog = []
        for i in range(0, const.DET_NUM):
            self.entry_porog.append( self.builder.get_object("entry_porog_" + str(i)) )

        self.entry_delay = self.builder.get_object("entry_delay")

        self.entry_acq_time = self.builder.get_object("entry_acq_time")

        self.filechooser_histo = self.builder.get_object("filechooser_histo")

        self.entry_en_range = []
        for i in range(0, const.DET_NUM):
            self.entry_en_range.append( self.builder.get_object("entry_en_range_" + str(i))  )

        self.statusbar = self.builder.get_object("statusbar")

        self.parse_cfg(const.MAIN_PROG_FOLDER + const.CFG_FILE)
        self.fill_entries()

        self.btn_ok = self.builder.get_object("btn_ok")

    def on_btn_set_porog_clicked(self, btn):
        #check for errors for all porogs!!!
        for num_btn in range(0, const.DET_NUM):
            self.porog[num_btn] = int( self.entry_porog[num_btn].get_text() )
            if (self.porog[num_btn] < 0 or self.porog[num_btn] >= 8192):
                self.__info_dialog("Porog #{} range problem".format(num_btn), "Porog should be in range [0:8K]")
                return -1

        #execl C prog for set porog #num_btn
        str_porog = "{:d} {:d} {:d} {:d}".format(self.porog[0], self.porog[1], self.porog[2], self.porog[3])
        print("str_porog = {}".format(str_porog))
        ret = subprocess_call( ["{}build/{}".format(const.MAIN_PROG_FOLDER, const.FPGA_PROG), "-p", str_porog] )
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

        ret = subprocess_call( ["{}build/{}".format(const.MAIN_PROG_FOLDER, const.FPGA_PROG), "-d", str(self.delay)] )
        if ret != 0:
            err_msg = "Set delay error! Return code {}.".format(ret)
            self.statusbar.push(self.statusbar.get_context_id("error in set delay"), err_msg)
        else:
            self.statusbar.push(self.statusbar.get_context_id("ok set delay"), "Set delay OK.")
        print("delay = {}".format(self.delay))

    def on_btn_reset_clicked(self, btn):
        ret = subprocess_call( ["{}build/{}".format(const.MAIN_PROG_FOLDER, const.FPGA_PROG), "-r"] )
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
            ret = subprocess_call( ["{}build/{}".format(const.MAIN_PROG_FOLDER, const.FPGA_PROG), "-c"] )
        elif txt_btn == "Coinc off":
            self.coinc = 0
            ret = subprocess_call( ["{}build/{}".format(const.MAIN_PROG_FOLDER, const.FPGA_PROG), "-n"] )

        if ret != 0:
            err_msg = "Coinc ON/OFF error! Return code {}.".format(ret)
            self.statusbar.push(self.statusbar.get_context_id("error in coinc"), err_msg)
        else:
            self.statusbar.push(self.statusbar.get_context_id("ok coinc on/off"), "Coinc ON/OFF OK.")

    def on_btn_ok_clicked(self, btn):
        print("Save cfg and close")
        self.get_entries()
        self.save_cfg(const.MAIN_PROG_FOLDER + const.CFG_FILE)
        self.win.close()

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

    def save_consts_cfg(self, path_to_save):
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
        #ret

    def fill_entries(self):
        if self.with_signals_flag:
            self.switch_hdrainer.set_active(1)
        else:
            self.switch_hdrainer.set_active(0)

        for i in range(0, const.DET_NUM):
            self.entry_porog[i].set_text( str(self.porog[i]) )

        self.entry_delay.set_text( str(self.delay) )
        self.entry_acq_time.set_text( str(self.acq_time) )

        self.filechooser_histo.set_filename(self.histo_folder)

        for i in range(0, const.DET_NUM):
            self.entry_en_range[i].set_text( str(self.en_range[i])[1:-1] )

    def get_entries(self):
        self.with_signals_flag = self.switch_hdrainer.get_active()
        print("signal flag = {}".format(self.with_signals_flag))

        for i in range(0, const.DET_NUM):
            self.porog[i] = int(self.entry_porog[i].get_text())

        self.delay = int(self.entry_delay.get_text())
        acq_time_s = self.entry_acq_time.get_text()
        if acq_time_s[-1] == "m":
            self.acq_time = 60 * int(acq_time_s[:-1])
        elif acq_time_s[-1] == "h":
            self.acq_time = 60 * 60 * int(acq_time_s[:-1])
        elif acq_time_s[-1] == "d":
            self.acq_time = 24 * 60 * 60 * int(acq_time_s[:-1])
        else:
            self.acq_time = int(acq_time_s)

        self.histo_folder = self.filechooser_histo.get_filename()
        
        for i in range(0, const.DET_NUM):
            text = self.entry_en_range[i].get_text()
            en_rang = [int(n) for n in text.split(", ")]
            self.en_range[i] = en_rang


    def __info_dialog(self, primary_text, secondary_text):
        dialog = Gtk.MessageDialog(self, 0, Gtk.MessageType.INFO, Gtk.ButtonsType.OK, primary_text)
        dialog.format_secondary_text(secondary_text)
        dialog.run()
        dialog.destroy()

    
class Hdrainer():
    def __init__(self, prog_name, time, coinc, en_range, out_foldername):
        self.prog_name = prog_name
        self.time = time
        self.coinc = coinc
        self.en_range = en_range
        if out_foldername[-1] == "/":
            self.out_foldername = out_foldername
        else:
            self.out_foldername = out_foldername + "/"

        self.cycles_per_time = 0
        self.exec_time = 0

        self.det_counts = [0]*4 

        self.online = False
        self.ret = 0

        print("prog_name in Hdrainer = {}".format(prog_name))
        if prog_name == const.HDRAINER_EXE:
            self.prog_path = "{}build/{}".format(const.MAIN_PROG_FOLDER, const.HDRAINER_EXE)
            self.prog_params = [ "-t", str(self.time), "-e", str(self.en_range)[1:-1], "-o", self.out_foldername]
        elif prog_name == const.HDRAINER_EXE + "(with signals)":
            self.prog_path = "{}build/{}".format(const.MAIN_PROG_FOLDER, const.HDRAINER_EXE)
            self.prog_params = [ "-s", "-t", str(self.time), "-e", str(self.en_range)[1:-1], "-o", self.out_foldername]

    def start(self):
        self.online = True
        self.ret = self.start_C_prog()
        self.online = False

        return True
        
    def start_C_prog(self):
        pid = os.fork()
        if pid == 0:
            #Child
            sleep(1)
            try:
                os.execl(self.prog_path, self.prog_path, *self.prog_params)
            except OSError:
                print("OSerror exception")
                raise
        else:
            #Parent
            self.child_pid = pid

            cycle_prev, cycle_curr = -1, -1
            exe_time_prev, exe_time_curr = -1, -1

            counts_prev = [-1, -1, -1, -1]
            counts_curr = [-1, -1, -1, -1]
            
            import zmq
            
            context = zmq.Context()
            zmq_subscriber = context.socket(zmq.SUB)
            zmq_subscriber.connect("tcp://localhost:5556")
            zmq_subscriber.setsockopt(zmq.SUBSCRIBE, b"")

            while True:
                w_pid, ret = os.waitpid(pid, os.WNOHANG)
                if w_pid == pid:
                    print("Break hdrainer cycle")
                    break

                #should be with NONBLOCK flag and try except
                try:
                    #out_str = zmq_subscriber.recv(flags=zmq.NOBLOCK)
                    out_str = zmq_subscriber.recv()
                except zmq.ZMQError:
                    print("No msg in zmq queue")
                    out_str = b"" #???

                #Count intensity and execution time
                #remove exe_m_time since exe_time in microseconds!
                try:
                    cycles, exe_time, *counts, exe_m_time = unpack("=7l", out_str)
                except struct_error:
                    #End of *DRAINER should be OR syncro problems
                    print("STRUCT UNPACK ERROR | out_str = {} (len={})".format(out_str, len(out_str)))
                    cycles = 0
                    exe_time = 0
                    counts = [0, 0, 0, 0]
                    exe_m_time = 1
                    None
                
                #Now exe_time in miliseconds. It should be in seconds!
                if abs(exe_time) >= const.EPS:
                    exe_time /= 1000.0
                    self.exec_time = exe_time

                print("cycles = {}, execution_time = {} | type = {}".format(cycles, self.exec_time, type(cycles)))
                    
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
                        self.cycles_per_time = (cycle_curr - cycle_prev)/(exe_time_curr - exe_time_prev)
                        self.cycles_per_time *= self.coinc + 1

                        while Gtk.events_pending():
                            Gtk.main_iteration_do(False)
                        
                        print("cycle_per_time = {:f}".format(self.cycles_per_time))

                        '''
                        for i in range(4):
                            diff_per_s = round( diff_counts[i]/(exe_m_time/1000.0) )
                            self.det_counts[i] = diff_per_s
                            
                            print("det_counts[{}] = {}".format(i, self.det_counts[i]))
                        
                        print("exe_m_time = {:d}".format(exe_m_time))
                        '''
                        
                        if counts != [0, 0, 0, 0]:
                            for i in range(const.DET_NUM):
                                self.det_counts[i] = 0
                                
                                if counts_prev[i] == -1:
                                    counts_prev[i] = counts[i]
                                else:
                                    if counts_curr[i] == -1:
                                        counts_curr[i] = counts[i]
                                    else:
                                        counts_prev[i] = counts_curr[i]
                                        counts_curr[i] = counts[i]
                                        
                                        #diff_per_s = round( (counts_curr[i] - counts_prev[i])/(exe_m_time/1000) )
                                        diff_per_s = round( (counts_curr[i] - counts_prev[i])/(exe_time_curr - exe_time_prev) )
                                        self.det_counts[i] = diff_per_s
                                
                        print("exe_m_time = {:d}".format(exe_m_time))



                sleep(const.SLEEP_S_SOCK_READ)

        return 0
    
    def stop(self):
        print("Stop function here")
        if self.child_pid > 0:
            os.kill(self.child_pid, SIGUSR1)


def main():
    ui = UI()
    ui.main_win.show_all()
    Gtk.main()

if __name__ == "__main__":
    main()
