from gi.repository.Gtk import TextMark
from gi.repository import Pango

import time

FILELINE_NUM = 0
STARTLINE_NUM = 1
EXPOSLINE_NUM = 2
TIMELINE_NUM = 3
INTENSLINE_NUM = 4

class Information():
    def __init__(self, textview):
        self.textview = textview
        self.textbuff = textview.get_buffer()

        self.large_tag = self.textbuff.create_tag("bold", size_points=16, weight=Pango.Weight.BOLD)
        
        self.fileline_mark = TextMark()
        self.startline_mark = TextMark()
        self.exposline_mark = TextMark()
        self.timeline_mark = TextMark()
        self.intensline_mark = TextMark()

        iter = self.textbuff.get_iter_at_line(STARTLINE_NUM)
        text_file = "File: {:s}\n".format("xyz.file")
        self.textbuff.insert_with_tags(iter, text_file, self.large_tag)

        loc_time = time.localtime()
        iter = self.textbuff.get_iter_at_line(STARTLINE_NUM)
        text_start = "Start: {:s}\n".format(time.strftime("%d/%m/%y %H:%M", loc_time))
        self.textbuff.insert_with_tags(iter, text_start, self.large_tag)

        iter = self.textbuff.get_iter_at_line(EXPOSLINE_NUM)
        text_expos = "Exposition: 0 s\n"
        self.textbuff.insert_with_tags(iter, text_expos, self.large_tag)

        iter = self.textbuff.get_iter_at_line(TIMELINE_NUM)
        text_time = "Time: 0 s\n"
        self.textbuff.insert_with_tags(iter, text_time, self.large_tag)

        iter = self.textbuff.get_iter_at_line(INTENSLINE_NUM)
        text_intens = "Intensity: 0 cnts/s\n"
        self.textbuff.insert_with_tags(iter, text_intens, self.large_tag)

        iter = self.textbuff.get_iter_at_line_offset(FILELINE_NUM, len(text_file) - 1)
        self.textbuff.add_mark(self.fileline_mark, iter)

        iter = self.textbuff.get_iter_at_line_offset(STARTLINE_NUM, len(text_start) - 1)
        self.textbuff.add_mark(self.startline_mark, iter)

        iter = self.textbuff.get_iter_at_line_offset(EXPOSLINE_NUM, len(text_expos) - 1)
        self.textbuff.add_mark(self.exposline_mark, iter)

        iter = self.textbuff.get_iter_at_line_offset(TIMELINE_NUM, len(text_time) - 1)
        self.textbuff.add_mark(self.timeline_mark, iter)
        
        iter = self.textbuff.get_iter_at_line_offset(INTENSLINE_NUM, len(text_intens) - 1)
        self.textbuff.add_mark(self.intensline_mark, iter)
        

    def set_filename(self, name):
        iter_start = self.textbuff.get_iter_at_line(FILELINE_NUM)
        iter_end = self.textbuff.get_iter_at_mark(self.fileline_mark)
        self.textbuff.delete(iter_start, iter_end)

        name_parts = name.split('/')
        if len(name_parts) <= 4:
            text = "File: {:s}".format(name)
        else:
            text = "File: ....../{:s}/{:s}".format(name_parts[-2], name_parts[-1])
        
        iter = self.textbuff.get_iter_at_line(FILELINE_NUM)
        self.textbuff.insert_with_tags(iter, text, self.large_tag)

    def set_start(self, tim):
        iter_start = self.textbuff.get_iter_at_line(STARTLINE_NUM)
        iter_end = self.textbuff.get_iter_at_mark(self.startline_mark)
        self.textbuff.delete(iter_start, iter_end)

        text = "Start: {:s}".format(time.strftime("%d/%m/%y %H:%M", tim))
        iter = self.textbuff.get_iter_at_line(STARTLINE_NUM)
        self.textbuff.insert_with_tags(iter, text, self.large_tag)

    def set_expos(self, tim):
        iter_start = self.textbuff.get_iter_at_line(EXPOSLINE_NUM)
        iter_end = self.textbuff.get_iter_at_mark(self.exposline_mark)
        self.textbuff.delete(iter_start, iter_end)

        text = "Expos: {} s".format(tim)
        iter = self.textbuff.get_iter_at_line(EXPOSLINE_NUM)
        self.textbuff.insert_with_tags(iter, text, self.large_tag)

    def set_time(self, tim):
        iter_start = self.textbuff.get_iter_at_line(TIMELINE_NUM)
        iter_end = self.textbuff.get_iter_at_mark(self.timeline_mark)
        self.textbuff.delete(iter_start, iter_end)

        text = "Time: {:d} s".format(round(tim))
        iter = self.textbuff.get_iter_at_line(TIMELINE_NUM)
        self.textbuff.insert_with_tags(iter, text, self.large_tag)

    def set_intens(self, intens):
        iter_start = self.textbuff.get_iter_at_line(INTENSLINE_NUM)
        iter_end = self.textbuff.get_iter_at_mark(self.intensline_mark)
        self.textbuff.delete(iter_start, iter_end)

        text = "Intensity: {:d} cnts/s".format(int(intens))
        iter = self.textbuff.get_iter_at_line(INTENSLINE_NUM)
        self.textbuff.insert_with_tags(iter, text, self.large_tag)

    def update(self, args):
        try:
            self.set_filename(args["name"])
        except KeyError:
            None

        try:
            self.set_start(args["start"])
        except KeyError:
            None

        try:
            self.set_expos(args["expos"])
        except KeyError:
            None

        try:
            self.set_time(args["time"])
        except KeyError:
            None

        try:
            self.set_intens(args["intens"])
        except KeyError:
            None
