import numpy as np
from matplotlib.ticker import FuncFormatter

import const

X_SCALE_LIM = 1000
X_SCALE_LIM_DIVIDER = 4

class Zoom():
    def __init__(self, en_fig, t_fig, en_axes, t_axes, zoom_args):
        self.en_fig = en_fig
        self.t_fig = t_fig

        self.en_axes = en_axes
        self.t_axes = t_axes

        self.en_magn = zoom_args["en_magnification"]
        self.en_v_step = zoom_args["en_v_step"]
        self.en_h_step = zoom_args["en_h_step"]
        
        self.t_magn = zoom_args["t_magnification"]
        self.t_v_step = zoom_args["t_v_step"]
        self.t_h_step = zoom_args["t_h_step"]

        self.log_flag = False
        self.t_log_flag = False

    def check_in_range(self):
        None

    def set_default_scale(self):
        self.set_default_scale_en_x()
        self.set_default_scale_en_y()
        self.set_default_scale_t_x()
        self.set_default_scale_t_y()

    def set_default_scale_en_x(self):
        #set X lim en
        for i in range(0, const.DET_NUM):
            self.en_axes[i].set_xlim(0, const.HIST_SIZE)
        None

    def set_default_scale_en_y(self):
        #set Y lim en
        None

    def set_default_scale_t_x(self):
        #set X lim t
        for i in range(0, 12):
            self.t_axes[i].set_xlim(0, const.HIST_SIZE)

    def set_default_scale_t_y(self):
        #set Y lim t
        #get ydata from axes.lines!!!
        #for i in range(0, 12):
        #    self.t_axes[i].set_ylim(-1, something)
        None

    def en_x_shrink(self):
        #shrink en x with self.en_magn
        #get x_l, x_r
        #x_l -= self.en_magn//2
        #x_r += self.en_magn//2
        for i in range(0, const.DET_NUM):
            x_l, x_r = self.en_axes[i].get_xlim()
            x_l += self.en_magn//2
            x_r -= self.en_magn//2
            
            self.en_axes[i].set_xlim(x_l, x_r)
            
            self.en_fig[i].canvas.draw()

    def en_x_squeeze(self):
        #squeeze en x with self.en_magn (reverse to shrink)
        for i in range(0, const.DET_NUM):
            x_l, x_r = self.en_axes[i].get_xlim()
            x_l -= self.en_magn//2
            x_r += self.en_magn//2
            
            self.en_axes[i].set_xlim(x_l, x_r)
            self.en_fig[i].canvas.draw()

    def en_x_move_l(self):
        #get x_l, x_r
        #x_l -= self.en_h_step
        #x_r -= self.en_h_step
        for i in range(0, const.DET_NUM):
            x_l, x_r = self.en_axes[i].get_xlim()

            h_step = self.__calc_h_step(x_l, x_r)
            x_l += h_step
            x_r += h_step

            self.en_axes[i].set_xlim(x_l, x_r)
            self.en_fig[i].canvas.draw()

    def en_x_move_r(self):
        #x_l += self.en_h_step
        #x_r += self.en_h_step
        for i in range(0, const.DET_NUM):
            x_l, x_r = self.en_axes[i].get_xlim()

            h_step = self.__calc_h_step(x_l, x_r)
            x_l -= h_step
            x_r -= h_step

            self.en_axes[i].set_xlim(x_l, x_r)
            self.en_fig[i].canvas.draw()

    def en_y_up(self):
        #get x_top, x_bot
        #x_top += self.en_v_step
        #x_bot += self.en_v_step
        for i in range(0, const.DET_NUM):
            y_bot, y_top = self.en_axes[i].get_ylim()
            y_top -= self.en_v_step
            y_bot = -1

            self.en_axes[i].set_ylim(y_bot, y_top)
            self.en_fig[i].canvas.draw()

    def en_y_down(self):
        #x_top -= self.en_v_step
        #x_bot -= self.en_v_step
        for i in range(0, const.DET_NUM):
            y_bot, y_top = self.en_axes[0].get_ylim()
            y_top += self.en_v_step
            y_bot = -1

            self.en_axes[i].set_ylim(y_bot, y_top)
            self.en_fig[i].canvas.draw()

    def en_log(self):
        line = [0]*4
        data_y = [0]*4

        for i in range(0, const.DET_NUM):
            line[i], = self.en_axes[i].lines

        for i in range(0, const.DET_NUM):
            data_y[i] = line[i].get_ydata()

        data_y = np.array(data_y)
        max_data_y = data_y.max() or 1

        if not self.log_flag:
            self.log_flag = True

            y_bot, y_top = -1, np.log(max_data_y)
            for i in range(0, const.DET_NUM):
                self.en_axes[i].set_ylim(y_bot, y_top)
                self.en_axes[i].lines[0].set_ydata(np.log(data_y[i] + 1))
                
                self.en_fig[i].canvas.draw()
        else:
            self.log_flag = False

            y_bot, y_top = -1, np.exp(max_data_y)
            for i in range(0, const.DET_NUM):
                self.en_axes[i].set_ylim(y_bot, y_top)
                self.en_axes[i].lines[0].set_ydata(np.exp(data_y[i]))
                
                self.en_fig[i].canvas.draw()

    def en_zero(self):
        line = [0]*4
        data_y = [0]*4

        for i in range(0, const.DET_NUM):
            line[i], = self.en_axes[i].lines

        for i in range(0, const.DET_NUM):
            data_y[i] = line[i].get_ydata()

        data_y = np.array(data_y)
        max_data_y = data_y.max() or 1

        x_l, x_r = 0, const.HIST_SIZE
        y_bot, y_top = -1, max_data_y

        for i in range(0, const.DET_NUM):
            self.en_axes[i].set_xlim(x_l, x_r)
            self.en_axes[i].set_ylim(y_bot, y_top)

            self.en_axes[i].set_xticks([0, 1000, 2000, 3000, 4000])

            self.en_fig[i].canvas.draw()


    def __calc_h_step(self, x_l, x_r):
        if (x_r - x_l) > X_SCALE_LIM:
            h_step = self.en_h_step
        else:
            h_step = self.en_h_step//X_SCALE_LIM_DIVIDER

        return h_step

    
    def t_x_shrink(self):
        for i in range(0, 12):
            x_l, x_r = self.t_axes[i].get_xlim()
            x_l += self.t_magn//2
            x_r -= self.t_magn//2
            
            self.t_axes[i].set_xlim(x_l, x_r)
            self.t_fig[i].canvas.draw()

    def t_x_squeeze(self):
        for i in range(0, 12):
            x_l, x_r = self.t_axes[i].get_xlim()
            x_l -= self.t_magn//2
            x_r += self.t_magn//2
            
            self.t_axes[i].set_xlim(x_l, x_r)
            self.t_fig[i].canvas.draw()

    def t_log(self):
        line = [0] * 12
        data_y = [0] * 12

        for i in range(0, 12):
            line[i], = self.t_axes[i].lines

        for i in range(0, 12):
            data_y[i] = line[i].get_ydata()

        data_y = np.array(data_y)
        max_data_y = data_y.max() or 1

        if not self.t_log_flag:
            self.t_log_flag = True

            y_bot, y_top = -1, np.log(max_data_y)
            for i in range(0, 12):
                self.t_axes[i].set_ylim(y_bot, y_top)
                self.t_axes[i].lines[0].set_ydata(np.log(data_y[i] + 1))
                
                self.t_fig[i].canvas.draw()
        else:
            self.t_log_flag = False

            y_bot, y_top = -1, np.exp(max_data_y)
            for i in range(0, 12):
                self.t_axes[i].set_ylim(y_bot, y_top)
                self.t_axes[i].lines[0].set_ydata(np.exp(data_y[i]))

                self.t_fig[i].canvas.draw()


    '''
    def __x_ticks_if_need(self, axes, x_l, x_r):
        if (x_r - x_l) < X_SCALE_LIM:
            dx = axes.get_xticks()
            dx = np.append(dx, np.arange(x_l, x_r, 200.0))
            
            return dx
        else:
            return None

    def __set_k_tick_format(self, axes):
        axes.xaxis.set_major_formatter( FuncFormatter(lambda x, pos: "{:.0f}k".format(x/1e3)) )

    def __set_normal_tick_format(self, axes):
        axes.xaxis.set_major_formatter( FuncFormatter(lambda x, pos: "{:.0f}".format(x)) )
    '''
