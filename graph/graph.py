from tkinter import *
from tkinter import filedialog
import matplotlib
import matplotlib.pyplot as plt
# from matplotlib.ticker import FuncFormatter
from pandas import read_csv
import sys
import os


# Matplotlib backends: ['Qt5Agg', 'TkAgg']
matplotlib.use("Qt5Agg")

# Get resource path
def resource_path(relative_path):    
    try:       
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")

    return os.path.join(base_path, relative_path)


filename = ""
window = Tk()
window.title('CSV graf')
window.iconbitmap(default=resource_path('icon.ico'))
window.geometry("400x230")


# Function for opening the
# file explorer window
def browseFiles():
    global filename
    filename = filedialog.askopenfilename(initialdir = ".",
                                          title = "Select a File",
                                          filetypes = (("CSV files",
                                                        "*.csv*"),
                                                       ("all files",
                                                        "*.*")))
    # Change label contents
    if type(filename) is not tuple:
        label_file_explorer.configure(text=filename)
    else:
        filename = ""

def form4(y, pos):
    """ This function returns a string with 3 decimal places, given the input x"""
    return '%.4f' % y

def drawGraph():
    try:
        data = read_csv(filename)
        # Plot
        x = range(len(data['vzdialenost']))
        plt.plot(x, data['priemer'])
        plt.xlabel('Vzdialenosť (mm)')
        plt.ylabel('Priemer (mm)')
        # formatter = FuncFormatter(form4)
        # plt.gca().yaxis.set_major_formatter(FuncFormatter(formatter))
        plt.gca().get_yaxis().get_major_formatter().set_useOffset(False)
        plt.show()
    except:
        pass


label_file_explorer = Label(window,
                            text = "Vyberte CSV súbor s dátami",
                            width = 50, height = 4,
                            fg = "blue")
button_explore = Button(window,
                        text = "Prehľadávať",
                        command = browseFiles)
button_graph = Button(window,
                        text = "Vykresli graf",
                        command = drawGraph)
button_exit = Button(window,
                     text = "Ukonči",
                     command = sys.exit)

label_file_explorer.grid(column = 1, row = 1)
button_explore.grid(column = 1, row = 2)
Label(window).grid(column=1, row = 3)
button_graph.grid(column = 1, row = 4)
Label(window).grid(column=1, row = 5)
button_exit.grid(column = 1, row = 6)

window.mainloop()
