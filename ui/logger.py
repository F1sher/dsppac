import logging


class Logger():
    def __init__(self, filename):
        self.filename = filename

        logging.basicConfig(filename=self.filename, filemode="w", \
                            datefmt="%b %d %H:%M:%S", level=logging.DEBUG, \
                            format="%(asctime)s %(module)s: <%(levelname)s> %(message)s")
    
    def debug(self, msg):
        logging.debug(msg)

    def info(self, msg):
        logging.info(msg)

    def warning(self, msg):
        logging.info(msg)
