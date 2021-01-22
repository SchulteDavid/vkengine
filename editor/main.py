import tkinter as tk


def empty_callback(*args):
    pass


class Editor(tk.Tk):

    def __init__(self):
        tk.Tk.__init__(self)
        self.protocol("WM_DELETE_WINDOW", empty_callback)


if __name__ == '__main__':
    print("This should be loaded with vkengine")
    exit(-1)
